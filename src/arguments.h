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

#ifndef H_IPSENTINEL_ARGUMENTS_H
#define H_IPSENTINEL_ARGUMENTS_H

#include <stdbool.h>

typedef struct
{
    char const *	ipfile;
    char const *	pidfile;
    char const *	logfile;
    char const *	errfile;
    char const *	user;
    char const *	group;
    bool		do_fork;
    char const *	chroot;
    char const *	iface;
} Arguments;

void
parseOptions(int argc, char *argv[], Arguments *options);

#endif	//  H_IPSENTINEL_ARGUMENTS_H
