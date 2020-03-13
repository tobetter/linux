include $(top_rulesdir)/Makefile.inc

all:
# For now, only supported target architecture is x86.  Later we'll need to build
# for multiple targets and add a wrapper, same as for modpost.
# objtool explicitly sets CC and LD to be native tools; we need to override
# this on the command line to make cross-builds work.
	$(MAKE) -C $(top_srcdir)/tools/objtool O=$(CURDIR) HOSTARCH=$(KERNEL_ARCH) ARCH=x86 CC=$(CC) LD=$(CROSS_COMPILE)ld V=$(KBUILD_VERBOSE)

install:
	install -D -m755 objtool $(DESTDIR)/$(installdir)/objtool
