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

#ifndef H_IPSENTINEL_PARAMETERS_H
#define H_IPSENTINEL_PARAMETERS_H

#define MAX_CHILDS		20
#define MAX_ERRORS		6

  // When resizing of a vector is *necessarily*, the new allocated size will be
  // set to VECTOR_SET_THRESHOLD * count. Omitting of parentheses is intended
  // to allow rational numbers but to avoid floating point operations
#define VECTOR_SET_THRESHOLD	20/16
  // When resizing of a vector is *requested*, resizing will be done if
  // allocated size is more than VECTOR_DEC_THRESHOLD * count. This value must
  // be higher than VECTOR_SET_THRESHOLD.
#define VECTOR_DEC_THRESHOLD	24/16


  // DOS-measurement is given in a unit of connections per ANTIDOS_TIME_BASE
  // seconds
#define ANTIDOS_TIME_BASE	10

  // ARP-requests with DOS-values below LOW will be answered; between LOW and
  // HIGH the probability of an answer is decreasing linearly and requests over
  // HIGH will not be answered.
#define ANTIDOS_COUNT_LOW	10u
#define ANTIDOS_COUNT_HIGH	50u

  // Maximum DOS-values; this value is used to prevent overflows
#define ANTIDOS_COUNT_MAX	1000u

#define ANTIDOS_ENTRIES_MAX	0x8000

#endif	//  H_IPSENTINEL_PARAMETERS_H


  /// Local Variables:
  /// fill-column: 79
  /// End:
