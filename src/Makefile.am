## $Id$

# Copyright (C) 2002 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
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
#  

sbin_PROGRAMS		=  ip-sentinel

if ENSC_NEED_ETHER
ether_SRCS			=  ether.c
else
ether_SRCS			=
endif

lib_SRCS			=  arguments.c util.c blacklist.c 	\
				   antidos.c vector.c $(ether_SRCS)

EXTRA_ip_sentinel_SOURCES	=  ether.c
ip_sentinel_SOURCES		=  ip-sentinel.c $(lib_SRCS)

AM_CPPFLAGS			=  -D_GNU_SOURCE

noinst_HEADERS			=  antidos.h arguments.h arpmessage.h	\
				   blacklist.h compat.h ip-sentinel.h	\
				   parameters.h util.h vector.h		\
				   wrappers.h vector.ic

EXTRA_DIST			=  $(testsuite_FILES)

include $(top_srcdir)/m4/diet.am
include $(srcdir)/testsuite/Makefile-files
