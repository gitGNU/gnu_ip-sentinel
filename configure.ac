# Process this file with autoconf to produce a configure script.
AC_INIT(ip-sentinel, 0.6.90, enrico.scholz@informatik.tu-chemnitz.de, [ip-sentinel])
AC_CONFIG_SRCDIR([src/antidos.c])
AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([gnits dist-bzip2 subdir-objects])
AM_MAINTAINER_MODE

username=nobody
configfile=ips.cfg

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_LN_S
AC_PROG_RANLIB

ENSC_STANDARD_TOOLS
ENSC_RELEASE(IS_RELEASE)
ENSC_DEVELOPING(IS_DEVELOPING)

ENSC_DIET
ENSC_DIET_FIX
AC_DEFINE(_GNU_SOURCE, 1, [Uses GNU features])

ENSC_CHECK_CC_FLAG([-Wall -W -Wno-unused-function])
ENSC_CHECK_CC_FLAG([-std=c99 ${RELEASE_CFLAGS} ${DEVELOPING_CFLAGS} ${DEVELOPING_STRICT_CFLAGS} -Wno-unreachable-code])


AC_MSG_CHECKING([for configfile])
AC_ARG_WITH(configfile,
            AC_HELP_STRING([--with-configfile=DIR],
                           [Path of configfile]),
            [ case x"$withval" in
		x|xyes|xno)	AC_MSG_ERROR("Invalid value for --with-configfile");;
		*)              configfile=${withval};;
	      esac ], [ : ])
AC_MSG_RESULT($configfile)
AC_SUBST(CONFIGFILE, $configfile)

AC_MSG_CHECKING([for username])
AC_ARG_WITH(username,
            AC_HELP_STRING([--with-username=NAME],
                           [Name of user running the sentinel]),
            [ case x"$withval" in
		x|xyes|xno)	AC_MSG_ERROR("Invalid value for --with-username");;
		*)              username=${withval};;
	      esac ], [ : ])
AC_MSG_RESULT($username)
AC_SUBST(USERNAME, $username)

AC_DEFINE_UNQUOTED([SENTINEL_USER],    ["$username"],    [Sentineluser])
AC_DEFINE_UNQUOTED([PATH_CONFIGFILE],  ["$configfile"],  [Configfilename])



ENSC_MODERN_COMPILER_CHECK


# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
ENSC_DIET_CHECK_IN_ADDR_T

# Checks for library functions.
ENSC_DIET_CHECK_ETHER

AC_CONFIG_FILES([Makefile m4/Makefile
                 src/Makefile ip-sentinel.spec])
AC_OUTPUT
