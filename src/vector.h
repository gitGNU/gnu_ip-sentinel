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

#ifndef H_IPSENTINEL_VECTOR_H
#define H_IPSENTINEL_VECTOR_H

#include "compat.h"
#include <stdlib.h>

struct Vector
{
    void	*data;
    size_t	count;
    size_t	allocated;

    size_t	elem_size;
};

void	Vector_init(struct Vector *, size_t elem_size);
void *	Vector_search(struct Vector *, void const *key, int (*compar)(const void *, const void *));
void	Vector_sort(struct Vector *, int (*compar)(const void *, const void *));
void *	Vector_pushback(struct Vector *);
void *	Vector_insert(struct Vector *, void const *key, int (*compar)(const void *, const void *));
void *	Vector_begin(struct Vector *);
void *	Vector_end(struct Vector *);
void	Vector_popback(struct Vector *);
void	Vector_resize(struct Vector *vec);
void	Vector_clear(struct Vector *vec);


#endif	//  H_IPSENTINEL_VECTOR_H
