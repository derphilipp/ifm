dnl Process this file with autoconf to produce a configure script.

dnl Initialize autoconf.
AC_INIT(ifm, [5.4], [zondo42@gmail.com])
AC_CONFIG_SRCDIR(src/ifm-main.c)
AC_CONFIG_AUX_DIR(etc)
AC_CONFIG_HEADERS([etc/config.h])

dnl Initialize automake.
AM_INIT_AUTOMAKE(foreign)

dnl Check for compiler and flags.
AC_PROG_CC

AM_CHECK_CC_OPTION(-pipe)

AM_CHECK_CC_OPTION(-W)
AM_CHECK_CC_OPTION(-Wall)

AM_CHECK_CC_OPTION(-Werror-implicit-function-declaration)

AM_CHECK_CC_OPTION(-Wno-unused-but-set-variable)
AM_CHECK_CC_OPTION(-Wno-unused-parameter)
AM_CHECK_CC_OPTION(-Wno-unused-value)

AC_SUBST(LOCAL_CFLAGS)

dnl Checks for programs.
AC_CANONICAL_HOST

AM_PROG_LEX
AC_PROG_YACC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

dnl Check for building on MinGW.
MINGW32=no
if test $build_os == mingw32; then
   MINGW32=yes
   AC_DEFINE(MINGW32, [], [Whether building using MinGW])
fi

AM_CONDITIONAL(MINGW32, test $MINGW32 = yes)

dnl Checks for header files.
AC_HEADER_STDC

AC_CHECK_HEADERS(malloc.h alloca.h unistd.h process.h signal.h)

dnl Checks for functions.
AC_FUNC_ALLOCA
AC_CHECK_FUNCS(mkstemp)

dnl Check for wish.
AC_PATH_PROGS(WISH, wish8.0 wish80 wish, no)

tkifm=tkifm
AC_SUBST(tkifm)

if test $WISH = no; then
   AC_MSG_WARN(wish not found -- not building wish scripts)
   tkifm=
fi

dnl Check for perl.
AC_PATH_PROGS(PERL, perl5.002 perl5 perl, no)

ifm2dev=ifm2dev
AC_SUBST(ifm2dev)

ifm2tex=ifm2tex
AC_SUBST(ifm2tex)

ifm2web=ifm2web
AC_SUBST(ifm2web)

scr2ifm=scr2ifm
AC_SUBST(scr2ifm)

if test $PERL = no; then
   AC_MSG_WARN(perl not found -- not building perl scripts)
   ifm2dev=
   ifm2tex=
   ifm2web=
   scr2ifm=
fi

dnl Write Makefiles.
AC_OUTPUT(
Makefile
doc/Makefile
doc/conf.py
lib/Makefile
man/Makefile
progs/Makefile
src/Makefile
src/libfig/Makefile
src/libvars/Makefile
tests/Makefile
win32/Makefile
)
