// $Id$    --*- c++ -*--

// Copyright (C) 2002 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//  

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "blacklist.h"
#include <signal.h>

volatile sig_atomic_t		child_count;


int main(int argc, char *argv[])
{
  BlackList		lst;

  if (argc!=2) return EXIT_FAILURE;
  
  BlackList_init(&lst, argv[1]);
  BlackList_softUpdate(&lst);
  BlackList_print(&lst,1);

  return EXIT_SUCCESS;
}


  /// Local Variables:
  /// compile-command: "make -C .. check"
  /// End:
