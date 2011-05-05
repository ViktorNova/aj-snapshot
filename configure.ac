AC_PREREQ(2.59)
AC_INIT([aj-snapshot], [0.9.3], [lievenmoors@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/aj-snapshot.c])
AC_PREFIX_DEFAULT(/usr/local)
AC_PROG_CC
AC_PROG_INSTALL

AC_CHECK_HEADER(getopt.h, [])
AC_CHECK_HEADER(alsa/asoundlib.h, [])
AC_CHECK_HEADER(jack/jack.h, [])
AC_CHECK_HEADER(mxml.h, [])

AC_CHECK_LIB(asound, main, [], [AC_MSG_ERROR(ALSA library (asound) is required)])
AC_CHECK_LIB(jack, main, [], [AC_MSG_ERROR(JACK library (jack) is required)])
AC_CHECK_LIB(mxml, mxmlLoadFile, [], [AC_MSG_ERROR(mxml library (mxml) is required)])
PKG_CHECK_MODULES([ALSA], [alsa])
PKG_CHECK_MODULES([JACK], [jack])
PKG_CHECK_MODULES([MXML], [mxml])


AC_OUTPUT(Makefile src/Makefile)

