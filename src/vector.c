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

#include "vector.h"
#include "util.h"
#include "wrappers.h"
#include "parameters.h"

#include <assert.h>


void
Vector_init(struct Vector *vec, size_t elem_size)
{
  assert(vec!=0);
  assert(elem_size!=0);

  vec->elem_size = elem_size;
  vec->data      = 0;
  vec->count     = 0;
  vec->allocated = 0;
}

void *
Vector_search(struct Vector *vec, void const *key,
	      int (*compare)(const void *, const void *))
{
  if (vec->count==0) return 0;
  assert(vec->data!=0);
  
  return bsearch(key, vec->data, vec->count, vec->elem_size, compare);
}

void
Vector_sort(struct Vector *vec, int (*compare)(const void *, const void *))
{
  if (vec->count==0) return;

  qsort(vec->data, vec->count, vec->elem_size, compare);
}

static void
Vector_resizeInternal(struct Vector *vec)
{
  vec->allocated = vec->count * VECTOR_SET_THRESHOLD;
  ++vec->allocated;

  assert(vec->allocated >= vec->count);
    
  vec->data = Erealloc(vec->data, vec->allocated * vec->elem_size);
  assert(vec->data!=0);
}

void *
Vector_insert(struct Vector *vec, void const *key,
	      int (*compare)(const void *, const void *))
{
  char *	data;
  char *	end_ptr = Vector_pushback(vec);

  for (data=vec->data; data<end_ptr; data += vec->elem_size) {
    if (compare(key, data)>0) {
      memmove(data, data+vec->elem_size,
	      static_cast(char *)(end_ptr) - static_cast(char *)(data));
      return data;
    }
  }

  return end_ptr;
}

void *
Vector_pushback(struct Vector *vec)
{
  ++vec->count;
  if (vec->allocated<vec->count)
    Vector_resizeInternal(vec);

  return static_cast(char *)(vec->data) + ((vec->count-1) * vec->elem_size);
}

void
Vector_popback(struct Vector *vec)
{
  assert(vec->count>0);

  if (vec->count>0) --vec->count;
}

void
Vector_resize(struct Vector *vec)
{
  if (vec->allocated * VECTOR_DEC_THRESHOLD > vec->count+1)
    Vector_resizeInternal(vec);
}

void
Vector_clear(struct Vector *vec)
{
  assert(vec!=0);
  vec->count = 0;
}
