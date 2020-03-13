#!/usr/bin/python3

import codecs
import hashlib
import io
import json
import os.path
import re
import ssl
import subprocess
import sys

from debian_linux.config import ConfigCoreDump
from debian_linux.debian import VersionLinux
from debian_linux.gencontrol import Gencontrol as Base, merge_packages
from debian_linux.utils import Templates, read_control


class Gencontrol(Base):
    def __init__(self, arch):
        super(Gencontrol, self).__init__(
            ConfigCoreDump(fp=open('debian/config.defines.dump', 'rb')),
            Templates(['debian/signing_templates', 'debian/templates']))

        image_binary_version = self.changelog[0].version.complete

        config_entry = self.config[('version',)]
        self.version = VersionLinux(config_entry['source'])

        # Check config version matches changelog version
        assert self.version.complete == re.sub(r'\+b\d+$', r'',
                                               image_binary_version)

        self.abiname = config_entry['abiname']
        self.vars = {
            'template': 'linux-image-%s-signed-template' % arch,
            'upstreamversion': self.version.linux_upstream,
            'version': self.version.linux_version,
            'source_suffix': '',
            'source_upstream': self.version.upstream,
            'abiname': self.abiname,
            'imagebinaryversion': image_binary_version,
            'imagesourceversion': self.version.complete,
            'arch': arch,
        }

        self.package_dir = 'debian/%(template)s' % self.vars
        self.template_top_dir = (self.package_dir
                                 + '/usr/share/code-signing/%(template)s'
                                 % self.vars)
        self.template_debian_dir = (self.template_top_dir
                                    + '/source-template/debian')
        os.makedirs(self.template_debian_dir, exist_ok=True)

        self.image_packages = []

    def do_main_setup(self, vars, makeflags, extra):
        makeflags['VERSION'] = self.version.linux_version
        makeflags['GENCONTROL_ARGS'] = (
            '-v%(imagebinaryversion)s '
            '-DBuilt-Using="linux (= %(imagesourceversion)s)"' %
            vars)
        makeflags['PACKAGE_VERSION'] = vars['imagebinaryversion']

        self.installer_packages = {}

        if os.getenv('DEBIAN_KERNEL_DISABLE_INSTALLER'):
            if self.changelog[0].distribution == 'UNRELEASED':
                import warnings
                warnings.warn('Disable installer modules on request '
                              '(DEBIAN_KERNEL_DISABLE_INSTALLER set)')
            else:
                raise RuntimeError(
                    'Unable to disable installer modules in release build '
                    '(DEBIAN_KERNEL_DISABLE_INSTALLER set)')
        elif self.config.merge('packages').get('installer', True):
            # Add udebs using kernel-wedge
            kw_env = os.environ.copy()
            kw_env['KW_DEFCONFIG_DIR'] = 'debian/installer'
            kw_env['KW_CONFIG_DIR'] = 'debian/installer'
            kw_proc = subprocess.Popen(
                ['kernel-wedge', 'gen-control', vars['abiname']],
                stdout=subprocess.PIPE,
                env=kw_env)
            if not isinstance(kw_proc.stdout, io.IOBase):
                udeb_packages = read_control(io.open(kw_proc.stdout.fileno(),
                                                     closefd=False))
            else:
                udeb_packages = read_control(io.TextIOWrapper(kw_proc.stdout))
            kw_proc.wait()
            if kw_proc.returncode != 0:
                raise RuntimeError('kernel-wedge exited with code %d' %
                                   kw_proc.returncode)

            for package in udeb_packages:
                for arch in package['Architecture']:
                    if self.config.merge('build', arch) \
                                  .get('signed-code', False):
                        self.installer_packages.setdefault(arch, []) \
                                               .append(package)

    def do_main_packages(self, packages, vars, makeflags, extra):
        # Assume that arch:all packages do not get binNMU'd
        packages['source']['Build-Depends'].append(
            'linux-support-%(abiname)s (= %(imagesourceversion)s)' % vars)

    def do_main_recurse(self, packages, makefile, vars, makeflags, extra):
        # Each signed source package only covers a single architecture
        self.do_arch(packages, makefile, vars['arch'], vars.copy(),
                     makeflags.copy(), extra)

    def do_extra(self, packages, makefile):
        pass

    def do_arch_setup(self, vars, makeflags, arch, extra):
        super(Gencontrol, self).do_main_setup(vars, makeflags, extra)

        if self.version.linux_modifier is None:
            abiname_part = '-%s' % self.config.merge('abi', arch)['abiname']
        else:
            abiname_part = ''
        makeflags['ABINAME'] = vars['abiname'] = \
            self.config['version', ]['abiname_base'] + abiname_part

    def do_arch_packages(self, packages, makefile, arch, vars, makeflags,
                         extra):
        udeb_packages = self.installer_packages.get(arch, [])
        if udeb_packages:
            merge_packages(packages, udeb_packages, arch)

            # These packages must be built after the per-flavour/
            # per-featureset packages.  Also, this won't work
            # correctly with an empty package list.
            if udeb_packages:
                makefile.add(
                    'binary-arch_%s' % arch,
                    cmds=["$(MAKE) -f debian/rules.real install-udeb_%s %s "
                          "PACKAGE_NAMES='%s'" %
                          (arch, makeflags,
                           ' '.join(p['Package'] for p in udeb_packages))])

    def do_flavour_setup(self, vars, makeflags, arch, featureset, flavour,
                         extra):
        super(Gencontrol, self).do_flavour_setup(vars, makeflags, arch,
                                                 featureset, flavour, extra)

        config_description = self.config.merge('description', arch, featureset,
                                               flavour)
        config_image = self.config.merge('image', arch, featureset, flavour)

        vars['flavour'] = vars['localversion'][1:]
        vars['class'] = config_description['hardware']
        vars['longclass'] = (config_description.get('hardware-long')
                             or vars['class'])

        vars['image-stem'] = config_image.get('install-stem')
        makeflags['IMAGE_INSTALL_STEM'] = vars['image-stem']

    def do_flavour_packages(self, packages, makefile, arch, featureset,
                            flavour, vars, makeflags, extra):
        if not (self.config.merge('build', arch, featureset, flavour)
                .get('signed-code', False)):
            return

        image_suffix = '%(abiname)s%(localversion)s' % vars
        image_package_name = 'linux-image-%s-unsigned' % image_suffix

        # Verify that this flavour is configured to support Secure Boot,
        # and get the trusted certificates filename.
        with open('debian/%s/boot/config-%s' %
                  (image_package_name, image_suffix)) as f:
            kconfig = f.readlines()
        assert 'CONFIG_EFI_STUB=y\n' in kconfig
        assert 'CONFIG_LOCK_DOWN_IN_EFI_SECURE_BOOT=y\n' in kconfig
        cert_re = re.compile(r'CONFIG_SYSTEM_TRUSTED_KEYS="(.*)"$')
        cert_file_name = None
        for line in kconfig:
            match = cert_re.match(line)
            if match:
                cert_file_name = match.group(1)
                break
        assert cert_file_name
        if featureset != "none":
            cert_file_name = os.path.join('debian/build/source_%s' %
                                          featureset,
                                          cert_file_name)

        self.image_packages.append((image_suffix, image_package_name,
                                    cert_file_name))

        packages['source']['Build-Depends'].append(
            image_package_name
            + ' (= %(imagebinaryversion)s) [%(arch)s]' % vars)

        packages_own = self.process_packages(
            self.templates['control.image'], vars)
        assert len(packages_own) == 1
        cmds_binary_arch = ["$(MAKE) -f debian/rules.real install-signed "
                            "PACKAGE_NAME='%s' %s" %
                            (packages_own[0]['Package'], makeflags)]

        if self.config.merge('packages').get('meta', True):
            packages_meta = self.process_packages(
                self.templates['control.image.meta'], vars)
            assert len(packages_meta) == 1

            # Don't pretend to support build-profiles
            del packages_meta[0]['Build-Profiles']

            packages_own.extend(packages_meta)
            cmds_binary_arch += ["$(MAKE) -f debian/rules.real install-meta "
                                 "PACKAGE_NAME='%s' %s" %
                                 (packages_meta[0]['Package'], makeflags)]

            self.substitute_debhelper_config(
                'image.meta', vars,
                'linux-image%(localversion)s' % vars,
                output_dir=self.template_debian_dir)

        merge_packages(packages, packages_own, arch)
        makefile.add('binary-arch_%s_%s_%s_real' % (arch, featureset, flavour),
                     cmds=cmds_binary_arch)

        self.substitute_debhelper_config(
            'image', vars,
            'linux-image-%(abiname)s%(localversion)s' % vars,
            output_dir=self.template_debian_dir)

    def write(self, packages, makefile):
        self.write_changelog()
        self.write_control(packages.values(),
                           name=(self.template_debian_dir + '/control'))
        self.write_makefile(makefile,
                            name=(self.template_debian_dir + '/rules.gen'))
        self.write_files_json()

    def write_changelog(self):
        # We need to insert a new version entry.
        # Take the distribution and urgency from the linux changelog, and
        # the base version from the changelog template.
        vars = self.vars.copy()
        vars['source'] = self.changelog[0].source
        vars['distribution'] = self.changelog[0].distribution
        vars['urgency'] = self.changelog[0].urgency
        vars['signedsourceversion'] = (re.sub(r'-', r'+',
                                              vars['imagebinaryversion']))

        with codecs.open(self.template_debian_dir + '/changelog', 'w',
                         'utf-8') as f:
            f.write(self.substitute('''\
linux-signed-@arch@ (@signedsourceversion@) @distribution@; urgency=@urgency@

  * Sign kernel from @source@ @imagebinaryversion@

''',
                                    vars))

            with codecs.open('debian/changelog', 'r', 'utf-8') as changelog_in:
                # Ignore first two header lines
                changelog_in.readline()
                changelog_in.readline()

                for d in changelog_in.read():
                    f.write(d)

    def write_files_json(self):
        # Can't raise from a lambda function :-(
        def raise_func(e):
            raise e

        # Some functions in openssl work with multiple concatenated
        # PEM-format certificates, but others do not.
        def get_certs(file_name):
            certs = []
            BEGIN, MIDDLE = 0, 1
            state = BEGIN
            with open(file_name) as f:
                for line in f:
                    if line == '-----BEGIN CERTIFICATE-----\n':
                        assert state == BEGIN
                        certs.append([])
                        state = MIDDLE
                    elif line == '-----END CERTIFICATE-----\n':
                        assert state == MIDDLE
                        state = BEGIN
                    else:
                        assert line[0] != '-'
                        assert state == MIDDLE
                    certs[-1].append(line)
            assert state == BEGIN
            return [''.join(cert_lines) for cert_lines in certs]

        def get_cert_fingerprint(cert, algo):
            hasher = hashlib.new(algo)
            hasher.update(ssl.PEM_cert_to_DER_cert(cert))
            return hasher.hexdigest()

        all_files = {'packages': {}}

        for image_suffix, image_package_name, cert_file_name in \
                self.image_packages:
            package_dir = 'debian/%s' % image_package_name
            package_files = []
            package_modules = []
            package_files.append({'sig_type': 'efi',
                                  'file': 'boot/vmlinuz-%s' % image_suffix})
            for root, dirs, files in os.walk('%s/lib/modules' % package_dir,
                                             onerror=raise_func):
                for name in files:
                    if name.endswith('.ko'):
                        package_modules.append(
                            '%s/%s' %
                            (root[(len(package_dir) + 1):], name))
            package_modules.sort()
            for module in package_modules:
                package_files.append(
                    {'sig_type': 'linux-module',
                     'file': module})
            package_certs = [get_cert_fingerprint(cert, 'sha256')
                             for cert in get_certs(cert_file_name)]
            assert len(package_certs) >= 1
            all_files['packages'][image_package_name] = {
                'trusted_certs': package_certs,
                'files': package_files
            }

        with codecs.open(self.template_top_dir + '/files.json', 'w') as f:
            json.dump(all_files, f)


if __name__ == '__main__':
    Gencontrol(sys.argv[1])()
