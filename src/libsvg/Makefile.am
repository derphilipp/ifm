## Automake template for SVG library.

noinst_LIBRARIES = libsvg.a

SRCS = svg-attr.c svg-file.c svg-object.c svg-util.c

HDRS = svg-attr.h svg-file.h svg-object.h svg-util.h svg.h

libsvg_a_SOURCES = $(SRCS) $(HDRS)

EXTRA_DIST = svg-format.txt

EXTRA_PROGRAMS = testsvg

AM_CFLAGS = $(LOCAL_CFLAGS)
AM_CPPFLAGS = -I$(srcdir)/../libvars

testsvg_SOURCES = testsvg.c
testsvg_LDFLAGS = -L$(srcdir)/../libvars
testsvg_LDADD = libsvg.a -lvars -lm

test: testsvg
	./testsvg | tee test.svg

CLEANFILES = test.svg* testsvg

include $(top_srcdir)/etc/Makefile.common
