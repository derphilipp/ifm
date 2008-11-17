## Automake template for Vars library.

SRCS = vars-array.c vars-buffer.c vars-compat.c vars-config.c vars-copy.c \
vars-database.c vars-debug.c vars-destroy.c vars-freeze.c vars-func.c	  \
vars-getopt.c vars-graph.c vars-hash.c vars-image.c vars-iterate.c	  \
vars-list.c vars-matrix.c vars-memory.c vars-parser.c vars-print.c	  \
vars-queue.c vars-random.c vars-regex.c vars-scalar.c vars-system.c	  \
vars-traverse.c vars-type.c vars-utils.c vars-vector.c vars-write.c	  \
vars-xml.c vars-yaml.c

NOINST_SRCS = vars-freeze-lex.l vars-parser-lex.l vars-parser-yacc.y

HDRS = vars-array.h vars-buffer.h vars-compat.h vars-config.h vars-copy.h \
vars-database.h vars-debug.h vars-destroy.h vars-freeze.h vars-func.h	  \
vars-getopt.h vars-graph.h vars-hash.h vars-image.h vars-iterate.h	  \
vars-list.h vars-macros.h vars-matrix.h vars-memory.h vars-parser.h	  \
vars-print.h vars-queue.h vars-random.h vars-regex.h vars-scalar.h	  \
vars-system.h vars-traverse.h vars-type.h vars-utils.h vars-vector.h	  \
vars-win32.h vars-write.h vars-xml.h vars-yaml.h vars.h

NOINST_HDRS = vars-freeze-lex.h vars-parser-lex.h vars-parser-yacc.h

INCLUDES = -DHAVE_MKSTEMP

noinst_HEADERS = $(HDRS)
noinst_LIBRARIES = libvars.a

LEX_PREFIX = `echo $< | sed -e 's/vars-//' -e 's/-lex.l$$//'`

AM_LFLAGS = -CFa -8 -P$(LEX_PREFIX)
AM_YFLAGS = -v -d

LEX_OUTPUT_ROOT = lex.$(LEX_PREFIX)

VARS_SRCS = $(SRCS) $(NOINST_SRCS) $(HDRS) $(NOINST_HDRS) vars-config.h

libvars_a_SOURCES  = $(VARS_SRCS)
