dnl Copyright (C) 2021  Trevor Woerner <twoerner@gmail.com>

dnl AC_PREREQ(2.57)
AC_INIT([mcp3001], 1.0.0, twoerner@gmail.com, mpc3001)
AC_CONFIG_SRCDIR(src/mcp3001.c)
AC_CONFIG_AUX_DIR(cfg)
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 1.9])
AM_CONFIG_HEADER(config.h)

SUBDIRS="src"

dnl **********************************
dnl checks for programs
dnl **********************************
AC_PROG_CC
AC_PROG_CPP

AC_PROG_MAKE_SET

AC_PROG_INSTALL
AC_PROG_LN_S

dnl **********************************
dnl checks for header files
dnl **********************************
AC_HEADER_STDC
AC_CHECK_HEADERS(stdio.h stdint.h stdlib.h string.h)
AC_CHECK_HEADERS(getopt.h fcntl.h time.h)
AC_CHECK_HEADERS(sys/ioctl.h sys/stat.h)
AC_CHECK_HEADERS(linux/ioctl.h linux/types.h linux/spi/spidev.h)

dnl **********************************
dnl other stuff
dnl **********************************
AC_SUBST(SUBDIRS)

dnl **********************************
dnl output
dnl **********************************
AC_OUTPUT(Makefile
src/Makefile)
