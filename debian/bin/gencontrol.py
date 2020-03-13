#!/usr/bin/python3

import sys
import locale
import io
import os
import os.path
import subprocess
import re

from debian_linux import config
from debian_linux.debian import PackageDescription, PackageRelation, \
    PackageRelationEntry, PackageRelationGroup, VersionLinux
from debian_linux.gencontrol import Gencontrol as Base, merge_packages, \
    iter_featuresets
from debian_linux.utils import Templates, read_control

locale.setlocale(locale.LC_CTYPE, "C.UTF-8")


class Gencontrol(Base):
    config_schema = {
        'abi': {
            'ignore-changes': config.SchemaItemList(),
        },
        'build': {
            'debug-info': config.SchemaItemBoolean(),
            'signed-code': config.SchemaItemBoolean(),
            'vdso': config.SchemaItemBoolean(),
        },
        'description': {
            'parts': config.SchemaItemList(),
        },
        'image': {
            'bootloaders': config.SchemaItemList(),
            'configs': config.SchemaItemList(),
            'initramfs-generators': config.SchemaItemList(),
            'check-size': config.SchemaItemInteger(),
            'check-size-with-dtb': config.SchemaItemBoolean(),
            'check-uncompressed-size': config.SchemaItemInteger(),
            'depends': config.SchemaItemList(','),
            'provides': config.SchemaItemList(','),
            'suggests': config.SchemaItemList(','),
            'recommends': config.SchemaItemList(','),
            'conflicts': config.SchemaItemList(','),
            'breaks': config.SchemaItemList(','),
        },
        'relations': {
        },
        'packages': {
            'docs': config.SchemaItemBoolean(),
            'headers-all': config.SchemaItemBoolean(),
            'installer': config.SchemaItemBoolean(),
            'libc-dev': config.SchemaItemBoolean(),
            'meta': config.SchemaItemBoolean(),
            'tools-unversioned': config.SchemaItemBoolean(),
            'tools-versioned': config.SchemaItemBoolean(),
            'source': config.SchemaItemBoolean(),
        }
    }

    def __init__(self, config_dirs=["debian/config"],
                 template_dirs=["debian/templates"]):
        super(Gencontrol, self).__init__(
            config.ConfigCoreHierarchy(self.config_schema, config_dirs),
            Templates(template_dirs),
            VersionLinux)
        self.process_changelog()
        self.config_dirs = config_dirs

    def _setup_makeflags(self, names, makeflags, data):
        for src, dst, optional in names:
            if src in data or not optional:
                makeflags[dst] = data[src]

    def do_main_setup(self, vars, makeflags, extra):
        super(Gencontrol, self).do_main_setup(vars, makeflags, extra)
        makeflags.update({
            'VERSION': self.version.linux_version,
            'UPSTREAMVERSION': self.version.linux_upstream,
            'ABINAME': self.abiname_version + self.abiname_part,
            'SOURCEVERSION': self.version.complete,
        })
        makeflags['SOURCE_BASENAME'] = vars['source_basename']
        makeflags['SOURCE_SUFFIX'] = vars['source_suffix']

        # Prepare to generate debian/tests/control
        self.tests_control = self.process_packages(
            self.templates['tests-control.main'], vars)
        self.tests_control_image = None

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

            # All architectures that have some installer udebs
            arches = set()
            for package in udeb_packages:
                arches.update(package['Architecture'])

            # Code-signing status for those architectures
            # If we're going to build signed udebs later, don't actually
            # generate udebs.  Just test that we *can* build, so we find
            # configuration errors before building linux-signed.
            build_signed = {}
            for arch in arches:
                build_signed[arch] = self.config.merge('build', arch) \
                                                .get('signed-code', False)

            for package in udeb_packages:
                # kernel-wedge currently chokes on Build-Profiles so add it now
                if any(build_signed[arch] for arch in package['Architecture']):
                    assert all(build_signed[arch]
                               for arch in package['Architecture'])
                    # XXX This is a hack to exclude the udebs from
                    # the package list while still being able to
                    # convince debhelper and kernel-wedge to go
                    # part way to building them.
                    package['Build-Profiles'] = (
                        '<pkg.linux.udeb-unsigned-test-build>')
                else:
                    package['Build-Profiles'] = '<!stage1 !pkg.linux.nokernel>'

                for arch in package['Architecture']:
                    self.installer_packages.setdefault(arch, []) \
                                           .append(package)

    def do_main_makefile(self, makefile, makeflags, extra):
        for featureset in iter_featuresets(self.config):
            makeflags_featureset = makeflags.copy()
            makeflags_featureset['FEATURESET'] = featureset
            cmds_source = ["$(MAKE) -f debian/rules.real source-featureset %s"
                           % makeflags_featureset]
            makefile.add('source_%s_real' % featureset, cmds=cmds_source)
            makefile.add('source_%s' % featureset,
                         ['source_%s_real' % featureset])
            makefile.add('source', ['source_%s' % featureset])

        makeflags = makeflags.copy()
        makeflags['ALL_FEATURESETS'] = ' '.join(iter_featuresets(self.config))
        super(Gencontrol, self).do_main_makefile(makefile, makeflags, extra)

    def do_main_packages(self, packages, vars, makeflags, extra):
        packages.extend(self.process_packages(
            self.templates["control.main"], vars))

        # Only build the metapackages if their names won't exactly match
        # the packages they depend on
        do_meta = self.config.merge('packages').get('meta', True) \
            and vars['source_suffix'] != '-' + vars['version']

        if self.config.merge('packages').get('docs', True):
            packages.extend(self.process_packages(
                self.templates["control.docs"], vars))
            if do_meta:
                packages.extend(self.process_packages(
                    self.templates["control.docs.meta"], vars))
                self.substitute_debhelper_config(
                    'docs.meta', vars,
                    '%(source_basename)s-doc%(source_suffix)s' % vars)
        if self.config.merge('packages').get('tools-unversioned', True):
            packages.extend(self.process_packages(
                self.templates["control.tools-unversioned"], vars))
        if self.config.merge('packages').get('tools-versioned', True):
            packages.extend(self.process_packages(
                self.templates["control.tools-versioned"], vars))
            self.substitute_debhelper_config('perf', vars,
                                              'linux-perf-%(version)s' % vars)
            if do_meta:
                packages.extend(self.process_packages(
                    self.templates["control.tools-versioned.meta"], vars))
                self.substitute_debhelper_config('perf.meta', vars,
                                                  'linux-perf')
        if self.config.merge('packages').get('source', True):
            packages.extend(self.process_packages(
                self.templates["control.sourcebin"], vars))
            if do_meta:
                packages.extend(self.process_packages(
                    self.templates["control.sourcebin.meta"], vars))
                self.substitute_debhelper_config(
                    'sourcebin.meta', vars,
                    '%(source_basename)s-source%(source_suffix)s' % vars)

    def do_indep_featureset_setup(self, vars, makeflags, featureset, extra):
        makeflags['LOCALVERSION'] = vars['localversion']
        kernel_arches = set()
        for arch in iter(self.config['base', ]['arches']):
            if self.config.get_merge('base', arch, featureset, None,
                                     'flavours'):
                kernel_arches.add(self.config['base', arch]['kernel-arch'])
        makeflags['ALL_KERNEL_ARCHES'] = ' '.join(sorted(list(kernel_arches)))

        vars['featureset_desc'] = ''
        if featureset != 'none':
            desc = self.config[('description', None, featureset)]
            desc_parts = desc['parts']
            vars['featureset_desc'] = (' with the %s featureset' %
                                       desc['part-short-%s' % desc_parts[0]])

    def do_indep_featureset_packages(self, packages, makefile, featureset,
                                     vars, makeflags, extra):
        headers_featureset = self.templates["control.headers.featureset"]
        packages.extend(self.process_packages(headers_featureset, vars))

        cmds_binary_arch = ["$(MAKE) -f debian/rules.real "
                            "binary-indep-featureset %s" %
                            makeflags]
        makefile.add('binary-indep_%s_real' % featureset,
                     cmds=cmds_binary_arch)

    arch_makeflags = (
        ('kernel-arch', 'KERNEL_ARCH', False),
    )

    def do_arch_setup(self, vars, makeflags, arch, extra):
        config_base = self.config.merge('base', arch)

        self._setup_makeflags(self.arch_makeflags, makeflags, config_base)

        try:
            gnu_type_bytes = subprocess.check_output(
                ['dpkg-architecture', '-f', '-a', arch,
                 '-q', 'DEB_HOST_GNU_TYPE'],
                stderr=subprocess.DEVNULL)
        except subprocess.CalledProcessError:
            # This sometimes happens for the newest ports :-/
            print('W: Unable to get GNU type for %s' % arch, file=sys.stderr)
        else:
            vars['gnu-type-package'] = (
                gnu_type_bytes.decode('utf-8').strip().replace('_', '-'))

    def do_arch_packages(self, packages, makefile, arch, vars, makeflags,
                         extra):
        if self.version.linux_modifier is None:
            try:
                abiname_part = '-%s' % self.config['abi', arch]['abiname']
            except KeyError:
                abiname_part = self.abiname_part
            makeflags['ABINAME'] = vars['abiname'] = \
                self.abiname_version + abiname_part

        build_signed = self.config.merge('build', arch) \
                                  .get('signed-code', False)

        # Some userland architectures require kernels from another
        # (Debian) architecture, e.g. x32/amd64.
        # And some derivatives don't need the headers-all packages
        # for other reasons.
        if self.config['base', arch].get('featuresets') and \
           self.config.merge('packages').get('headers-all', True):
            headers_arch = self.templates["control.headers.arch"]
            packages_headers_arch = self.process_packages(headers_arch, vars)
            packages_headers_arch[-1]['Depends'].extend(PackageRelation())
            extra['headers_arch_depends'] = (
                packages_headers_arch[-1]['Depends'])
        else:
            packages_headers_arch = []

        if self.config.merge('packages').get('libc-dev', True):
            libc_dev = self.templates["control.libc-dev"]
            packages_headers_arch[0:0] = self.process_packages(libc_dev, {})

        merge_packages(packages, packages_headers_arch, arch)

        if self.config['base', arch].get('featuresets') and \
           self.config.merge('packages').get('source', True):
            merge_packages(packages,
                           self.process_packages(
                               self.templates["control.config"], vars),
                           arch)

        cmds_build_arch = ["$(MAKE) -f debian/rules.real build-arch-arch %s" %
                           makeflags]
        makefile.add('build-arch_%s_real' % arch, cmds=cmds_build_arch)

        cmds_binary_arch = ["$(MAKE) -f debian/rules.real binary-arch-arch %s"
                            % makeflags]
        makefile.add('binary-arch_%s_real' % arch, cmds=cmds_binary_arch,
                     deps=['setup_%s' % arch])

        udeb_packages = self.installer_packages.get(arch, [])
        if udeb_packages:
            merge_packages(packages, udeb_packages, arch)

            # These packages must be built after the per-flavour/
            # per-featureset packages.  Also, this won't work
            # correctly with an empty package list.
            makefile.add(
                'binary-arch_%s' % arch,
                cmds=["$(MAKE) -f debian/rules.real install-udeb_%s %s "
                      "PACKAGE_NAMES='%s' UDEB_UNSIGNED_TEST_BUILD=%s" %
                      (arch, makeflags,
                       ' '.join(p['Package'] for p in udeb_packages),
                       build_signed)])

        # This also needs to be built after the per-flavour/per-featureset
        # packages.
        if build_signed:
            merge_packages(packages,
                           self.process_packages(
                               self.templates['control.signed-template'],
                               vars),
                           arch)
            makefile.add(
                'binary-arch_%s' % arch,
                cmds=["$(MAKE) -f debian/rules.real "
                      "install-signed-template_%s %s" %
                      (arch, makeflags)])

    def do_featureset_setup(self, vars, makeflags, arch, featureset, extra):
        vars['localversion_headers'] = vars['localversion']
        makeflags['LOCALVERSION_HEADERS'] = vars['localversion_headers']

    flavour_makeflags_base = (
        ('compiler', 'COMPILER', False),
        ('compiler-filename', 'COMPILER', True),
        ('kernel-arch', 'KERNEL_ARCH', False),
        ('cflags', 'KCFLAGS', True),
        ('override-host-type', 'OVERRIDE_HOST_TYPE', True),
        ('cross-compile-compat', 'CROSS_COMPILE_COMPAT', True),
    )

    flavour_makeflags_build = (
        ('image-file', 'IMAGE_FILE', True),
    )

    flavour_makeflags_image = (
        ('install-stem', 'IMAGE_INSTALL_STEM', True),
    )

    flavour_makeflags_other = (
        ('localversion', 'LOCALVERSION', False),
        ('localversion-image', 'LOCALVERSION_IMAGE', True),
    )

    def do_flavour_setup(self, vars, makeflags, arch, featureset, flavour,
                         extra):
        config_base = self.config.merge('base', arch, featureset, flavour)
        config_build = self.config.merge('build', arch, featureset, flavour)
        config_description = self.config.merge('description', arch, featureset,
                                               flavour)
        config_image = self.config.merge('image', arch, featureset, flavour)

        vars['flavour'] = vars['localversion'][1:]
        vars['class'] = config_description['hardware']
        vars['longclass'] = (config_description.get('hardware-long')
                             or vars['class'])

        vars['localversion-image'] = vars['localversion']
        override_localversion = config_image.get('override-localversion', None)
        if override_localversion is not None:
            vars['localversion-image'] = (vars['localversion_headers'] + '-'
                                          + override_localversion)
        vars['image-stem'] = config_image.get('install-stem')

        self._setup_makeflags(self.flavour_makeflags_base, makeflags,
                              config_base)
        self._setup_makeflags(self.flavour_makeflags_build, makeflags,
                              config_build)
        self._setup_makeflags(self.flavour_makeflags_image, makeflags,
                              config_image)
        self._setup_makeflags(self.flavour_makeflags_other, makeflags, vars)

    def do_flavour_packages(self, packages, makefile, arch, featureset,
                            flavour, vars, makeflags, extra):
        headers = self.templates["control.headers"]
        assert len(headers) == 1

        do_meta = self.config.merge('packages').get('meta', True)
        config_entry_base = self.config.merge('base', arch, featureset,
                                              flavour)
        config_entry_build = self.config.merge('build', arch, featureset,
                                               flavour)
        config_entry_description = self.config.merge('description', arch,
                                                     featureset, flavour)
        config_entry_relations = self.config.merge('relations', arch,
                                                   featureset, flavour)

        def config_entry_image(key, *args, **kwargs):
            return self.config.get_merge(
                'image', arch, featureset, flavour, key, *args, **kwargs)

        compiler = config_entry_base.get('compiler', 'gcc')

        # Work out dependency from linux-headers to compiler.  Drop
        # dependencies for cross-builds.  Strip any remaining
        # restrictions, as they don't apply to binary Depends.
        relations_compiler_headers = PackageRelation(
            self.substitute(config_entry_relations.get('headers%' + compiler)
                            or config_entry_relations.get(compiler), vars))
        relations_compiler_headers = PackageRelation(
            PackageRelationGroup(entry for entry in group
                                 if 'cross' not in entry.restrictions)
            for group in relations_compiler_headers)
        for group in relations_compiler_headers:
            for entry in group:
                entry.restrictions = []

        relations_compiler_build_dep = PackageRelation(
            self.substitute(config_entry_relations[compiler], vars))
        for group in relations_compiler_build_dep:
            for item in group:
                item.arches = [arch]
        packages['source']['Build-Depends-Arch'].extend(
            relations_compiler_build_dep)

        image_fields = {'Description': PackageDescription()}
        for field in ('Depends', 'Provides', 'Suggests', 'Recommends',
                      'Conflicts', 'Breaks'):
            image_fields[field] = PackageRelation(
                config_entry_image(field.lower(), None),
                override_arches=(arch,))

        generators = config_entry_image('initramfs-generators')
        group = PackageRelationGroup()
        for i in generators:
            i = config_entry_relations.get(i, i)
            group.append(i)
            a = PackageRelationEntry(i)
            if a.operator is not None:
                a.operator = -a.operator
                image_fields['Breaks'].append(PackageRelationGroup([a]))
        for item in group:
            item.arches = [arch]
        image_fields['Depends'].append(group)

        bootloaders = config_entry_image('bootloaders', None)
        if bootloaders:
            group = PackageRelationGroup()
            for i in bootloaders:
                i = config_entry_relations.get(i, i)
                group.append(i)
                a = PackageRelationEntry(i)
                if a.operator is not None:
                    a.operator = -a.operator
                    image_fields['Breaks'].append(PackageRelationGroup([a]))
            for item in group:
                item.arches = [arch]
            image_fields['Suggests'].append(group)

        desc_parts = self.config.get_merge('description', arch, featureset,
                                           flavour, 'parts')
        if desc_parts:
            # XXX: Workaround, we need to support multiple entries of the same
            # name
            parts = list(set(desc_parts))
            parts.sort()
            desc = image_fields['Description']
            for part in parts:
                desc.append(config_entry_description['part-long-' + part])
                desc.append_short(config_entry_description
                                  .get('part-short-' + part, ''))

        packages_own = []

        build_signed = config_entry_build.get('signed-code')

        image = self.templates[build_signed and "control.image-unsigned"
                               or "control.image"]
        assert len(image) == 1

        vars.setdefault('desc', None)

        image_main = self.process_real_image(image[0], image_fields, vars)
        packages_own.append(image_main)
        makeflags['IMAGE_PACKAGE_NAME'] = image_main['Package']

        # The image meta-packages will depend on signed linux-image
        # packages where applicable, so should be built from the
        # signed source packages
        if do_meta and not build_signed:
            packages_own.extend(self.process_packages(
                self.templates["control.image.meta"], vars))
            self.substitute_debhelper_config(
                "image.meta", vars,
                "linux-image%(localversion)s" % vars)

        package_headers = self.process_package(headers[0], vars)
        package_headers['Depends'].extend(relations_compiler_headers)
        packages_own.append(package_headers)
        if extra.get('headers_arch_depends'):
            extra['headers_arch_depends'].append('%s (= ${binary:Version})' %
                                                 packages_own[-1]['Package'])
        if do_meta:
            packages_own.extend(self.process_packages(
                self.templates["control.headers.meta"], vars))
            self.substitute_debhelper_config(
                'headers.meta', vars,
                'linux-headers%(localversion)s' % vars)

        if config_entry_build.get('vdso', False):
            makeflags['VDSO'] = True

        build_debug = config_entry_build.get('debug-info')

        if os.getenv('DEBIAN_KERNEL_DISABLE_DEBUG'):
            if self.changelog[0].distribution == 'UNRELEASED':
                import warnings
                warnings.warn('Disable debug infos on request '
                              '(DEBIAN_KERNEL_DISABLE_DEBUG set)')
                build_debug = False
            else:
                raise RuntimeError(
                    'Unable to disable debug infos in release build '
                    '(DEBIAN_KERNEL_DISABLE_DEBUG set)')

        if build_debug:
            makeflags['DEBUG'] = True
            packages_own.extend(self.process_packages(
                self.templates['control.image-dbg'], vars))
            if do_meta:
                packages_own.extend(self.process_packages(
                    self.templates["control.image-dbg.meta"], vars))
                self.substitute_debhelper_config(
                    'image-dbg.meta', vars,
                    'linux-image%(localversion)s-dbg' % vars)

        merge_packages(packages, packages_own, arch)

        tests_control = self.process_package(
            self.templates['tests-control.image'][0], vars)
        tests_control['Depends'].append(
            PackageRelationGroup(image_main['Package'],
                                 override_arches=(arch,)))
        if self.tests_control_image:
            self.tests_control_image['Depends'].extend(
                tests_control['Depends'])
        else:
            self.tests_control_image = tests_control
            self.tests_control.append(tests_control)

        def get_config(*entry_name):
            entry_real = ('image',) + entry_name
            entry = self.config.get(entry_real, None)
            if entry is None:
                return None
            return entry.get('configs', None)

        def check_config_default(fail, f):
            for d in self.config_dirs[::-1]:
                f1 = d + '/' + f
                if os.path.exists(f1):
                    return [f1]
            if fail:
                raise RuntimeError("%s unavailable" % f)
            return []

        def check_config_files(files):
            ret = []
            for f in files:
                for d in self.config_dirs[::-1]:
                    f1 = d + '/' + f
                    if os.path.exists(f1):
                        ret.append(f1)
                        break
                else:
                    raise RuntimeError("%s unavailable" % f)
            return ret

        def check_config(default, fail, *entry_name):
            configs = get_config(*entry_name)
            if configs is None:
                return check_config_default(fail, default)
            return check_config_files(configs)

        kconfig = check_config('config', True)
        kconfig.extend(check_config("kernelarch-%s/config" %
                                    config_entry_base['kernel-arch'],
                                    False))
        kconfig.extend(check_config("%s/config" % arch, True, arch))
        kconfig.extend(check_config("%s/config.%s" % (arch, flavour), False,
                                    arch, None, flavour))
        kconfig.extend(check_config("featureset-%s/config" % featureset, False,
                                    None, featureset))
        kconfig.extend(check_config("%s/%s/config" % (arch, featureset), False,
                                    arch, featureset))
        kconfig.extend(check_config("%s/%s/config.%s" %
                                    (arch, featureset, flavour), False,
                                    arch, featureset, flavour))
        makeflags['KCONFIG'] = ' '.join(kconfig)
        makeflags['KCONFIG_OPTIONS'] = ''
        if build_debug:
            makeflags['KCONFIG_OPTIONS'] += ' -o DEBUG_INFO=y'
        if build_signed:
            makeflags['KCONFIG_OPTIONS'] += ' -o MODULE_SIG=y'
        # Add "salt" to fix #872263
        makeflags['KCONFIG_OPTIONS'] += \
            ' -o "BUILD_SALT=\\"%(abiname)s%(localversion)s\\""' % vars

        cmds_binary_arch = ["$(MAKE) -f debian/rules.real binary-arch-flavour "
                            "%s" %
                            makeflags]
        cmds_build = ["$(MAKE) -f debian/rules.real build-arch-flavour %s" %
                      makeflags]
        cmds_setup = ["$(MAKE) -f debian/rules.real setup-arch-flavour %s" %
                      makeflags]
        makefile.add('binary-arch_%s_%s_%s_real' % (arch, featureset, flavour),
                     cmds=cmds_binary_arch)
        makefile.add('build-arch_%s_%s_%s_real' % (arch, featureset, flavour),
                     cmds=cmds_build)
        makefile.add('setup_%s_%s_%s_real' % (arch, featureset, flavour),
                     cmds=cmds_setup)

        merged_config = ('debian/build/config.%s_%s_%s' %
                         (arch, featureset, flavour))
        makefile.add(merged_config,
                     cmds=["$(MAKE) -f debian/rules.real %s %s" %
                           (merged_config, makeflags)])

        self.substitute_debhelper_config(
            'headers', vars,
            'linux-headers-%(abiname)s%(localversion)s' % vars)
        self.substitute_debhelper_config('image', vars, image_main['Package'])
        if build_debug:
            self.substitute_debhelper_config(
                'image-dbg', vars,
                'linux-image-%(abiname)s%(localversion)s-dbg' % vars)

    def process_changelog(self):
        version = self.version = self.changelog[0].version
        if self.version.linux_modifier is not None:
            self.abiname_part = ''
        else:
            self.abiname_part = '-%s' % self.config['abi', ]['abiname']
        # We need to keep at least three version components to avoid
        # userland breakage (e.g. #742226, #745984).
        self.abiname_version = re.sub(r'^(\d+\.\d+)(?=-|$)', r'\1.0',
                                      self.version.linux_upstream)
        self.vars = {
            'upstreamversion': self.version.linux_upstream,
            'version': self.version.linux_version,
            'source_basename': re.sub(r'-[\d.]+$', '',
                                      self.changelog[0].source),
            'source_upstream': self.version.upstream,
            'source_package': self.changelog[0].source,
            'abiname': self.abiname_version + self.abiname_part,
        }
        self.vars['source_suffix'] = \
            self.changelog[0].source[len(self.vars['source_basename']):]
        self.config['version', ] = {'source': self.version.complete,
                                    'upstream': self.version.linux_upstream,
                                    'abiname_base': self.abiname_version,
                                    'abiname': (self.abiname_version
                                                + self.abiname_part)}

        distribution = self.changelog[0].distribution
        if distribution in ('unstable', ):
            if version.linux_revision_experimental or \
               version.linux_revision_backports or \
               version.linux_revision_other:
                raise RuntimeError("Can't upload to %s with a version of %s" %
                                   (distribution, version))
        if distribution in ('experimental', ):
            if not version.linux_revision_experimental:
                raise RuntimeError("Can't upload to %s with a version of %s" %
                                   (distribution, version))
        if distribution.endswith('-security') or distribution.endswith('-lts'):
            if version.linux_revision_backports or \
               version.linux_revision_other:
                raise RuntimeError("Can't upload to %s with a version of %s" %
                                   (distribution, version))
        if distribution.endswith('-backports'):
            if not version.linux_revision_backports:
                raise RuntimeError("Can't upload to %s with a version of %s" %
                                   (distribution, version))

    def process_real_image(self, entry, fields, vars):
        entry = self.process_package(entry, vars)
        for key, value in fields.items():
            if key in entry:
                real = entry[key]
                real.extend(value)
            elif value:
                entry[key] = value
        return entry

    def write(self, packages, makefile):
        self.write_config()
        super(Gencontrol, self).write(packages, makefile)
        self.write_tests_control()

    def write_config(self):
        f = open("debian/config.defines.dump", 'wb')
        self.config.dump(f)
        f.close()

    def write_tests_control(self):
        self.write_rfc822(open("debian/tests/control", 'w'),
                          self.tests_control)


if __name__ == '__main__':
    Gencontrol()()
