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

ACLOCAL_AMFLAGS	=  -I m4

WWW_HOST	=  janus.hrz
WWW_DIR		=  public_html/ip-sentinel
WWW_FILES_ROOT	=  $(srcdir)/doc/index.html
WWW_FILES_FILES	=  $(distdir).tar.bz2 $(distdir).tar.bz2.asc $(srcdir)/README $(srcdir)/NEWS

SUBDIRS		=  . src
DIST_SUBDIRS	=  $(SUBDIRS) m4

EXTRA_DIST	=  ip-sentinel.spec ${contrib_FILES} $(doc_FILES) \
		   disttest disttest-data
CLEANFILES	=  $(doc_CLEANFILES)

if MAINTAINER_MODE
TESTS		=  disttest
endif

include $(srcdir)/contrib/Makefile-files
include $(srcdir)/doc/Makefile-files

include $(top_srcdir)/m4/validate.am
include $(top_srcdir)/m4/changelog.am
include $(top_srcdir)/m4/diet.am
include $(top_srcdir)/m4/gpgsig.am
include $(top_srcdir)/m4/substrule.am
include $(top_srcdir)/m4/wwwpublish.am
