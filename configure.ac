## $Id$

# Copyright (C) 2003 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
#  
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#  
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#  
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

AC_INIT(ip-sentinel, 0.9.1, enrico.scholz@informatik.tu-chemnitz.de, [ip-sentinel])
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

ENSC_CHANGELOG()

ENSC_STANDARD_TOOLS
ENSC_RELEASE(IS_RELEASE)
ENSC_DEVELOPING(IS_DEVELOPING)

ENSC_DIET
ENSC_DIET_FIX
AC_DEFINE(_GNU_SOURCE, 1, [Uses GNU features])

ENSC_CHECK_CC_FLAG([-Wall -W -Wno-unused-function])
ENSC_CHECK_CC_FLAG([-std=c99 ${RELEASE_CFLAGS} ${DEVELOPING_CFLAGS} ${DEVELOPING_STRICT_CFLAGS} -Wno-unreachable-code])

ENSC_INITRDDIR(initrddir)

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

AH_BOTTOM([#include "compat.h"])
AC_CONFIG_FILES([Makefile
                 src/Makefile ip-sentinel.spec])
AC_OUTPUT
