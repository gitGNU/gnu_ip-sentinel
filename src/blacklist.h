// $Id$    --*- c++ -*--

// Copyright (C) 2002,2003 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
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

#ifndef H_IPSENTINEL_BLACKLIST_H
#define H_IPSENTINEL_BLACKLIST_H

#include "vector.h"

#include <netinet/in.h>
#include <time.h>

typedef struct {
    struct Vector		ip_list;
    struct Vector		net_list;
    char const *		filename;
    time_t			last_mtime;
} BlackList;

struct ether_addr const *
BlackList_getMac(BlackList const *lst, struct in_addr const ip, struct ether_addr *res);

void		BlackList_init(BlackList *lst, char const *filename);
void		BlackList_free(BlackList *);
void		BlackList_softUpdate(BlackList *lst);
void		BlackList_update(BlackList *lst);

#if !defined(NDEBUG) || defined(ENSC_TESTSUITE)
void		BlackList_print(BlackList *lst, int fd);
#endif


#endif	//  H_IPSENTINEL_BLACKLIST_H
