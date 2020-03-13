ifeq ($(filter i386 amd64 x32,$(DEB_HOST_ARCH)),)

# Build nothing
include $(top_rulesdir)/Makefile.inc

else

PROGS = \
	hv_fcopy_daemon \
	hv_kvp_daemon \
	hv_vss_daemon

installdir = /usr/sbin

include $(top_rulesdir)/Makefile.inc

endif
