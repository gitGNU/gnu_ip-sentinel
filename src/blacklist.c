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
#include "parameters.h"
#include "util.h"
#include "arguments.h"
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

static int UNUSED
IPData_uniqueCompare(void const *lhs_v, void const *rhs_v)
{
  struct IPData const *		lhs = lhs_v;
  struct IPData const *		rhs = rhs_v;
  assert(lhs!=0 && rhs!=0);

  if (lhs->status!=rhs->status) return 1;
  else return IPData_sortCompare(lhs_v, rhs_v);
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

static int UNUSED
NetData_uniqueCompare(void const *lhs_v, void const *rhs_v)
{
  struct NetData const *	lhs = lhs_v;
  struct NetData const *	rhs = rhs_v;
  assert(lhs!=0 && rhs!=0);

  if      (lhs->status!=rhs->status)       return 1;
  else if (lhs->ip.s_addr!=rhs->ip.s_addr) return 1;
  else return NetData_sortCompare(lhs_v, rhs_v);
}

void
BlackList_init(BlackList *lst, struct Arguments const *args)
{
  assert(lst!=0);
  
  Vector_init(&lst->ip_list,  sizeof(struct IPData));
  Vector_init(&lst->net_list, sizeof(struct NetData));

  lst->args_      = args;
  lst->last_mtime = 0;

  BlackList_update(lst);
}

#if ENSC_TESTSUITE
void
BlackList_free(BlackList *lst)
{
  assert(lst!=0);

  Vector_free(&lst->ip_list);
  Vector_free(&lst->net_list);
}
#endif

#define PARSE_ERRROR(MSG)			\
do {						\
  writeUInt(2, line_nr);			\
  WRITE_MSGSTR(2, ": " MSG "\n");		\
  return false;					\
} while (false)


static BlackListStatus ALWAYSINLINE
BlackList_setDefaultMac(BlackList const *lst, struct ether_addr *addr)
{
  BlackListStatus	res;
  
  switch (lst->args_->mac.type) {
    case mcRANDOM	:  res = blRAND; break;
    case mcFIXED	:
      *addr = lst->args_->mac.addr.ether;
      res   = blSET;
      break;
    default		:  assert(false); res = blUNDECIDED; break;
  }

  return res;
}


static bool ALWAYSINLINE
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
    else if (val<0 || val>32) PARSE_ERRROR("invalid netmask (too small or large)");
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
    if (parse_status!=blIGNORE)
      parse_status = BlackList_setDefaultMac(lst, &parse_mac);
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

    if (parse_status==blIGNORE) PARSE_ERRROR("can not both ignore an IP and assign a MAC");

    if (strcmp(start, "RANDOM")==0) parse_status = blRAND;
    else {
      if (xether_aton_r(start, &parse_mac)==0) {
	writeUInt(2, line_nr);
	WRITE_MSGSTR(2, ": invalid MAC '");
	WRITE_MSG   (2, start);
	WRITE_MSGSTR(2, "'\n");
	return false;
      }

      parse_status = blSET;
    }
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
BlackList_expandLine(BlackList *lst, char *start, char const *end, size_t line_nr);


static bool ALWAYSINLINE
BlackList_iterateList(BlackList *lst,
		      char *list_start,    char const *list_end,
		      char *prefix_start,  char *prefix_end,
		      char *postfix_start, size_t postfix_len,
		      size_t line_nr)
{
  char		 *ptr;

  for (ptr=list_start; ptr<=list_end; ++ptr) {
    if (*ptr==',' || ptr==list_end) {
      size_t	len = ptr-list_start;
      
      memcpy(prefix_end,     list_start,    len);
      memcpy(prefix_end+len, postfix_start, postfix_len);

      len            += postfix_len;
      prefix_end[len] = '\0';

      if (!BlackList_expandLine(lst, prefix_start, prefix_end+len, line_nr))
	return false;

      list_start = ptr+1;
    }
  }

  return true;
}

static bool ALWAYSINLINE
BlackList_iterateRange(BlackList *lst,
		       char *list_start,    char const *list_end,
		       char *prefix_start,  char *prefix_end,
		       char *postfix_start, size_t postfix_len,
		       size_t line_nr)
{
  int		start, end;
  char		*err_ptr;

    // strtol() stays in valid memory since it returns at least at *list_end=='}'
  start = strtol(list_start, &err_ptr, 10);

  if (err_ptr==0 || *err_ptr!='-') PARSE_ERRROR("unexpected error while parsing a range");
    
  end = strtol(err_ptr+1, &err_ptr, 10);

  if (err_ptr==0 || *err_ptr!='}') PARSE_ERRROR("range not terminated correctly");
  if (start<0 || end<0)            PARSE_ERRROR("negative values are not allowed in ranges");
  if (start>end)                   PARSE_ERRROR("descending ranges are not allowed");
  if (err_ptr>list_end)            PARSE_ERRROR("unexpected error while parsing range");

  for (; start<=end; ++start) {
    size_t	len = fillUInt(prefix_end, start);
    
    memcpy(prefix_end+len, postfix_start, postfix_len);

    len            += postfix_len;
    prefix_end[len] = '\0';

    if (!BlackList_expandLine(lst, prefix_start, prefix_end+len, line_nr))
      return false;
  }

  return true;
}

static bool
BlackList_expandLine(BlackList *lst, char *start, char const *end, size_t line_nr)
{
  enum {rgUNDECIDED, rgLIST, rgRANGE}	range_type = rgUNDECIDED;
  char					*buf, *ptr, *i;
  char					*range_start=0, *range_end=0;
  bool					parsed_line = false;

#if 0  
  WRITE_MSGSTR(1, "Expanding line '");
  write(1, start, end-start);
  WRITE_MSGSTR(1, "'\n");
#endif  
  
    // This works since the expanded lines are shorter than the original line
  buf = alloca(end-start);
  ptr = buf;

    // HACK-ALERT!! abort on comments
  for (i=start; i<end && *i!='#' && !parsed_line; ++i) {
    switch (*i) {
      case '{'	:
	if (range_start!=0) PARSE_ERRROR("nested ranges are not allowed");

	range_start = i;
	range_type  = rgUNDECIDED;
	break;

      case '}'	:
	if (range_start==0) PARSE_ERRROR("unexpected end of range");

	range_end  =i;
	break;

      case '-'	:
      case ','	:
      {
	bool	is_mismatch = false;

	switch (*i) {
	  case '-'	:
	    is_mismatch=(range_type!=rgUNDECIDED && range_type!=rgRANGE);
	    range_type = rgRANGE;
	    break;
	
	  case ','	:
	    is_mismatch=(range_type!=rgUNDECIDED && range_type!=rgLIST);
	    range_type = rgLIST;
	    break;
	    
	  default	:
	    assert(false);
	}

	if (is_mismatch) PARSE_ERRROR("mixed range-types not allowed");
	
	break;
      }

      default	:
	if (range_start!=0 && (*i<'0' || *i>'9'))
	  PARSE_ERRROR("non-digits not allowed as range-marks");

	break;
    }

    if (range_end!=0) {
      assert(range_start!=0 && range_start<range_end);
      
      if (range_type==rgUNDECIDED) {
      }

	// copy string from start till the '{' (exclusive)
      memcpy(buf, start, range_start-start);

      switch (range_type) {
	case rgUNDECIDED	:  PARSE_ERRROR("can not parse range");

	case rgLIST		:
	    // 'end-range_end-1 >= 0' holds since 'range_end' points to the '}' and 'end' to the
	    // final '\0'
	  if (!BlackList_iterateList(lst, range_start+1, range_end,
				     buf, buf + (range_start-start),
				     range_end+1, end-range_end-1, line_nr))
	    return false;
	  break;

	case rgRANGE		:
	  if (!BlackList_iterateRange(lst, range_start+1, range_end,
				      buf, buf + (range_start-start),
				      range_end+1, end-range_end-1, line_nr))
	    return false;
	  break;

	default			:
	  assert(false);
	  break;
      }

      parsed_line = true;
      range_start = 0;
      range_end   = 0;
    }
  }

  if (range_start!=0) PARSE_ERRROR("range not closed");

  if (!parsed_line)
    BlackList_parseLine(lst, start, end, line_nr);

  return true;
}

static bool ALWAYSINLINE
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
      BlackList_expandLine(lst, line_start, pos, line_nr);
      line_start = pos+1;
      ++line_nr;
    }
  }

  Vector_sort(&lst->ip_list,  IPData_sortCompare);
  Vector_sort(&lst->net_list, NetData_sortCompare);
#if !defined(ENSC_TESTSUITE)
  Vector_unique(&lst->ip_list,  IPData_uniqueCompare);
  Vector_unique(&lst->net_list, NetData_uniqueCompare);
#endif

  return true;
}

void
BlackList_softUpdate(BlackList *lst)
{
  static unsigned int	error_count = 0;
  struct stat		status;
  int			fd = -1;
  
  if (stat(lst->args_->ipfile, &status)==-1) {
    ++error_count;
    perror("stat()");
  }
  else if (lst->last_mtime != status.st_mtime) {
    
    writeMsgTimestamp(1);
    WRITE_MSGSTR(1, ": (Re)reading blacklist.\n");

    fd = open(lst->args_->ipfile, O_RDONLY);
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
    close(fd);
  }
  
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
      case blRAND	:  Util_setRandomMac(res); break;
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
      char *		aux = 0;
      
      switch (i->status) {
	case blUNDECIDED	:  write(fd, "?", 1); aux = "FAIL"; break;
	case blIGNORE		:  write(fd, "!", 1); aux = "";     break;
	case blRAND		:  aux = "RANDOM";    break;
	case blSET		:  break;
	default			:  assert(false);
      }

      if (aux==0) aux = ether_ntoa(&i->mac);

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
      char *		aux = 0;

      switch (i->status) {
	case blUNDECIDED	:  write(fd, "?", 1); aux = "FAIL"; break;
	case blIGNORE		:  write(fd, "!", 1); aux = "";     break;
	case blRAND		:  aux = "RANDOM";    break;
	case blSET		:  break;
	default			:  assert(false);
      }

      if (aux==0) aux = ether_ntoa(&i->mac);

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
