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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "util.h"

#include <unistd.h>
#include <assert.h>
#include <sys/time.h>
#include <arpa/inet.h>

void
writeMsgTimestamp(int fd)
{
  struct timeval                tv;
  time_t                        aux;
  size_t                        fill_cnt = 1;

  if (fd==-1) return;
  
  (void)gettimeofday(&tv, 0);
  
  {
#ifdef ENABLE_LOGGING
    char                        buffer[16];
    struct tm                   tmval;
    
    (void)localtime_r(&tv.tv_sec, &tmval);
    if (strftime(buffer, sizeof buffer, "%T", &tmval)>0) {
      (void)write(fd, buffer, strlen(buffer));
    }
    else
#endif
    {
      writeUInt(fd, static_cast(unsigned int)(tv.tv_sec));
      (void)write(fd, ".", 1);
    }
  }

  aux  = tv.tv_usec;
  aux |= 1; // Prevent 'aux==0' which will cause an endless-loop below
  assert(aux>0);
  
  while (aux<100000) { ++fill_cnt; aux *= 10; }
  (void)write(fd, "000000", fill_cnt-1);
  writeUInt(fd, static_cast(unsigned int)(tv.tv_usec));
}

void
writeUInt(int fd, unsigned int val)
{
  char                  buffer[32];
  register char         *ptr = &buffer[sizeof(buffer) - 1];

  do {
      /*@-strictops@*/
    *ptr-- = '0' + static_cast(char)(val%10);
      /*@=strictops@*/
    val   /= 10;
  } while (val!=0);

  ++ptr;
  assertDefined(ptr);
  assert(ptr<=&buffer[sizeof(buffer)]);

    /*@-strictops@*/
  (void)write(fd, ptr, &buffer[sizeof(buffer)] - ptr);
    /*@=strictops@*/
}

void
writeIP(int fd, struct in_addr ip)
{
  char const	*ip_str = inet_ntoa(ip);
  write(fd, ip_str, strlen(ip_str));
}
