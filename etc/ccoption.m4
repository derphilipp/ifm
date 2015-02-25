dnl AM_CHECK_CC_OPTION(option)
dnl 
dnl Check for support C compiler option.
dnl 
AC_DEFUN([AM_CHECK_CC_OPTION],[
AC_MSG_CHECKING(whether C compiler accepts $1)

OLDCFLAGS=$CFLAGS
CFLAGS=$1
AC_TRY_COMPILE(,, result=yes, result=no)
CFLAGS=$OLDCFLAGS

AC_MSG_RESULT($result)

if test $result = yes; then
   LOCAL_CFLAGS="$LOCAL_CFLAGS $1"
fi
])
