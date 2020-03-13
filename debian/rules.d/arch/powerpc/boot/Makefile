PROGS = \
	addnote \
	hack-coff \
	mktree

SCRIPTS = \
	wrapper

include $(top_rulesdir)/Makefile.inc

CPPFLAGS += -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

# gcc for ppc64 currently seems to be treating -I like -isystem, and
# since arch/powerpc/boot has substitute versions of some standard
# headers for use at boot time, they are also included in these tools.
# In this case there is actually no need to use the -I option, so
# filter it out.
CPPFLAGS := $(filter-out -I%,$(CPPFLAGS))
