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

#include "blacklist.h"
#include "ip-sentinel.h"
#include "parameters.h"
#include "util.h"
#include "compat.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <netinet/ether.h>
#include <net/ethernet.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>


struct ether_addr const		DEFAULT_MAC = {{  0xde, 0xad, 0xbe, 0xef, 0, 0 } };
typedef enum {blUNDECIDED, blIGNORE, blSET, blRAND}	BlackListStatus;

struct IPData
{
    struct in_addr	ip;
    BlackListStatus	status;
    struct ether_addr	mac;
};

struct NetData
{
    struct in_addr	ip;
    struct in_addr	mask;
    BlackListStatus	status;
    struct ether_addr	mac;
};

static unsigned int
getBitCount(unsigned int val)
{
  int	result = 0;
  
  while (val!=0) {
    result += val & 1;
    val   >>= 1;
  }

  return result;
}


static int
IPData_searchCompare(void const *lhs_v, void const *rhs_v)
{
  struct in_addr const *	lhs = lhs_v;
  struct IPData const *		rhs = rhs_v;
  assert(lhs!=0 && rhs!=0);

  if      (lhs->s_addr < rhs->ip.s_addr) return -1;
  else if (lhs->s_addr > rhs->ip.s_addr) return +1;
  else                                   return  0;
}

static int
IPData_sortCompare(void const *lhs_v, void const *rhs_v)
{
  struct IPData const *		lhs = lhs_v;
  assert(lhs!=0);
  
  return IPData_searchCompare(&lhs->ip, rhs_v);
}

static int
NetData_sortCompare(void const *lhs_v, void const *rhs_v)
{
  struct NetData const *	lhs = lhs_v;
  struct NetData const *	rhs = rhs_v;
  int				result;
  assert(lhs!=0 && rhs!=0);
  
  result = -getBitCount(lhs->mask.s_addr) + getBitCount(rhs->mask.s_addr);

#if ENSC_TESTSUITE
    // When being in testsuite-mode compare the network-data in a more deterministic way. Else,
    // different bsort() implementations can sort elements in a different order when they compare
    // equally and the expected output will differ from the actual one.
  if      (result!=0) {}
  else if (ntohl(lhs->mask.s_addr) < ntohl(rhs->mask.s_addr)) result = -1;
  else if (ntohl(lhs->mask.s_addr) > ntohl(rhs->mask.s_addr)) result = +1;
  else if (ntohl(lhs->ip.s_addr)   < ntohl(rhs->ip.s_addr))   result = -1;
  else if (ntohl(lhs->ip.s_addr)   > ntohl(rhs->ip.s_addr))   result = +1;
  else result = lhs->status - rhs->status;
#endif  

  return result;
}

void
BlackList_init(BlackList *lst, char const *filename)
{
  assert(lst!=0);
  
  Vector_init(&lst->ip_list,  sizeof(struct IPData));
  Vector_init(&lst->net_list, sizeof(struct NetData));

  lst->filename   = strdup(filename);
  lst->last_mtime = 0;

  BlackList_update(lst);
}

void
BlackList_free(BlackList *lst)
{
  assert(lst!=0);

  free(const_cast(char *)(lst->filename));
  Vector_free(&lst->ip_list);
  Vector_free(&lst->net_list);

#ifndef NDEBUG
  lst->filename = (void *)(0xdeadbeef);
#endif
}


static bool
BlackList_parseLine(BlackList *lst, char *start, char const *end, size_t line_nr)
{
  BlackListStatus	parse_status = blUNDECIDED;
  char			*pos;
  bool			has_mask = false;

  struct in_addr	parse_ip;
  struct in_addr	parse_mask;
  struct ether_addr	parse_mac;

    // Ignore leading whitespaces
  while (*start==' ' || *start=='\t') ++start;
  if (start==end) return false;

    // The '!' prefix
  switch (*start) {
    case '#'	:  return false;		// comment
    case '!'	:  parse_status = blIGNORE; ++start; break;
    default	:  break;
  }

    // The host-part
  for (pos=start; pos<end; ++pos) {
    switch (*pos) {
      case '/'		:  has_mask = true;
      case ' '		:
      case '\t'		:  *pos = '\0'; break;
    }

    if (*pos=='\0') break;
  }

  if (inet_aton(start, &parse_ip)==0) {
    writeUInt(2, line_nr);
    WRITE_MSGSTR(2, ": invalid ip '");
    WRITE_MSG   (2, start);
    WRITE_MSGSTR(2, "'\n");
    return false;
  }

    // The netmask-part
  if (pos<end) start = pos+1;
  else         start = pos;
  
  if (!has_mask) parse_mask.s_addr = 0xffffffff;
  else {
    char	*endptr;
    long	val;
    
    for (pos=start; pos<end; ++pos) {
      switch (*pos) {
	case ' '	:
	case '\t'	:  *pos = '\0'; break;
	default		:  break;
      }

      if (*pos=='\0') break;
    }

    val = strtol(start, &endptr, 0);
    if (*start=='\0' || *endptr!='\0') {
      if (inet_aton(start, &parse_mask)==0) {
	writeUInt(2, line_nr);
	WRITE_MSGSTR(2, ": invalid netmask '");
	WRITE_MSG   (2, start);
	WRITE_MSGSTR(2, "'\n");
	return false;
      }
    }
    else if (val<0 || val>32) {
      writeUInt(2, line_nr);
      WRITE_MSGSTR(2, ": invalid netmask (too small or large)\n");
      return false;
    }
    else {
	// Avoid (~0u << 32) because this gives ~0u, but not 0.
      parse_mask.s_addr   = ~0u;
      if (val<32) {
	parse_mask.s_addr <<= (32-val-1);
	parse_mask.s_addr <<= 1;
      }
      parse_mask.s_addr   = htonl(parse_mask.s_addr);
    }

    if (pos<end) start = pos+1;
    else         start = pos;
  }

    // Skip whitespaces
  for (; start<end; ++start)
    if (*start!=' ' && *start!='\t') break;

    // The MAC
  if (start==end || *start=='#') {
    parse_mac    = DEFAULT_MAC;
    if (parse_status!=blIGNORE) parse_status = blRAND;
  }
  else {
    for (pos=start; pos<end; ++pos) {
      switch (*pos) {
	case '#'	:
	case ' '	:
	case '\t'	:  *pos = '\0'; break;
      }

      if (*pos=='\0') break;
    }

    if (parse_status==blIGNORE) {
      writeUInt(2, line_nr);
      WRITE_MSGSTR(2, ": can not both ignore an IP and assign a MAC\n");
      return false;
    }

    if (ether_aton_r(start, &parse_mac)==0) {
      writeUInt(2, line_nr);
      WRITE_MSGSTR(2, ": invalid MAC '");
      WRITE_MSG   (2, start);
      WRITE_MSGSTR(2, "'\n");
      return false;
    }

    parse_status = blSET;
  }

  if (has_mask) {
    struct NetData *	data = Vector_pushback(&lst->net_list);

    data->ip     = parse_ip;
    data->mask   = parse_mask;
    data->status = parse_status;
    data->mac	 = parse_mac;

    data->ip.s_addr &= data->mask.s_addr;
  }
  else {
    struct IPData *	data = Vector_pushback(&lst->ip_list);

    data->ip     = parse_ip;
    data->status = parse_status;
    data->mac    = parse_mac;
  }

  return true;
}

static bool
BlackList_updateInternal(BlackList *lst, int fd)
{
  size_t	line_nr     = 1;
  char		buffer[512];
  char *	read_ptr   = buffer;

  Vector_clear(&lst->ip_list);
  Vector_clear(&lst->net_list);
  
  assert(fd!=-1);
  while(true) {
    char const *	end_ptr;
    char *		line_start;
    size_t		size = TEMP_FAILURE_RETRY(read(fd, read_ptr,
						       buffer+sizeof(buffer) - read_ptr));

    if      (size==0) break;
    else if (static_cast(ssize_t)(size)==-1) {
      perror("read()");
      return false;
    }

    line_start = buffer;
    end_ptr    = read_ptr + size;
    read_ptr   = buffer;
    
    while (line_start<end_ptr) {
      char	*pos = memchr(line_start, '\n', end_ptr-line_start);
      if (pos==0) {
	if (line_start==buffer) {
	  writeUInt(2, line_nr);
	  WRITE_MSGSTR(2, ": line too long\n");
	  return false;
	}

	memmove(buffer, line_start, end_ptr - line_start);
	read_ptr = buffer + (end_ptr - line_start);
	break;
      }
      *pos = '\0';
      BlackList_parseLine(lst, line_start, pos, line_nr);
      line_start = pos+1;
      ++line_nr;
    }
  }

  Vector_sort(&lst->ip_list,  IPData_sortCompare);
  Vector_sort(&lst->net_list, NetData_sortCompare);

  return true;
}

void
BlackList_softUpdate(BlackList *lst)
{
  static int	error_count = 0;
  struct stat	status;
  int		fd = -1;
  
  if (stat(lst->filename, &status)==-1) {
    ++error_count;
    perror("stat()");
  }
  else if (lst->last_mtime != status.st_mtime) {
    
    writeMsgTimestamp(1);
    WRITE_MSGSTR(1, ": (Re)reading blacklist. Active children: ");
    writeUInt(1, child_count);
    WRITE_MSGSTR(1, "\n");

    fd = open(lst->filename, O_RDONLY);
    if (fd==-1) {
      ++error_count;
      perror("open()");
    }
  }

  if (fd!=-1) {
    if (!BlackList_updateInternal(lst, fd)) {
      ++error_count;
      WRITE_MSGSTR(2, "Failed to read blacklist\n");
      lst->last_mtime = 0;
    }
    else {
      lst->last_mtime = status.st_mtime;
      error_count     = 0;
    }
  }

  close(fd);
  
  if (error_count>0) {
    if (error_count>MAX_ERRORS) {
      WRITE_MSGSTR(2, "Too much errors; aborting...");
      exit(1);
    }
    sleep(1 + error_count/2);
  }
}

void
BlackList_update(BlackList *lst)
{
  lst->last_mtime = 0;
  BlackList_softUpdate(lst);
}

struct ether_addr const *
BlackList_getMac(BlackList const *lst_const, struct in_addr const ip, struct ether_addr *res)
{
  struct ether_addr const *	result = 0;
    // 'status' is tied to 'result'; therefore compiler warnings about possible uninitialized usage
    // can be ignored.
  BlackListStatus		status;
    // C does not allow Vector_begin()/end() functions accepting both const and non-const
    // parameters. To prevent const_cast'ing in every call to these functions, const_cast the
    // lst-object here...
  BlackList *			lst = const_cast(BlackList *)(lst_const);
  
  assert(lst!=0);

  {
    struct IPData const	*data = Vector_search(&lst->ip_list, &ip, IPData_searchCompare);
    if (data!=0) {
      status = data->status;
      *res   = data->mac;
      result = res;
    }
  }

  if (result==0) {
    struct NetData const *	data    = Vector_begin(&lst->net_list);
    struct NetData const *	end_ptr = Vector_end(&lst->net_list);
    for (; data!=end_ptr; ++data) {
      if ((ip.s_addr & data->mask.s_addr) == data->ip.s_addr) {
	status = data->status;
	*res   = data->mac;
	result = res;
	break;
      }
    }
  }

  if (result!=0) {
    switch (status) {
      case blIGNORE	:  result = 0;
      case blSET	:  break;
      case blRAND	:
      {
	time_t			t = time(0);

	res->ether_addr_octet[5]  = (rand()%BLACKLIST_RAND_COUNT +
				     t/BLACKLIST_RAND_PERIOD)%256;
	break;
      }
      default		:  assert(false); result = 0; break;
    }
  }
  
  return result;
}

#if !defined(NDEBUG) || defined(ENSC_TESTSUITE)
void
BlackList_print(BlackList *lst, int fd)
{
  assert(lst!=0);
  
  {
    struct IPData const		*i;

    for (i =Vector_begin(&lst->ip_list);
	 i!=Vector_end(&lst->ip_list); ++i) {
      char *		aux = ether_ntoa(&i->mac);
      
      switch (i->status) {
	case blUNDECIDED	:  write(fd, "?", 1); break;
	case blIGNORE		:  write(fd, "!", 1); break;
	case blRAND		:
	case blSET		:  break;
	default			:  assert(false);
      }

      writeIP(fd, i->ip);
      WRITE_MSGSTR(fd, "\t\t");
      WRITE_MSG(fd, aux);
      WRITE_MSGSTR(fd, "\n");
    }
  }

  {
    struct NetData const	*i;

    for (i =Vector_begin(&lst->net_list);
	 i!=Vector_end(&lst->net_list); ++i) {
      char *		aux = ether_ntoa(&i->mac);

      switch (i->status) {
	case blUNDECIDED	:  write(fd, "?", 1); break;
	case blIGNORE		:  write(fd, "!", 1); break;
	case blRAND		:
	case blSET		:  break;
	default			:  assert(false);
      }

      writeIP(fd, i->ip);
      WRITE_MSGSTR(fd, "/");
      writeIP(fd, i->mask);
      WRITE_MSGSTR(fd, "\t\t");
      WRITE_MSG(fd, aux);
      WRITE_MSGSTR(fd, "\n");
    }
  }
}
#endif
