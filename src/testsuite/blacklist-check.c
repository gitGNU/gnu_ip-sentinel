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
#include "arguments.h"
#include "compat.h"

#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <netinet/ether.h>
#include <net/ethernet.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

volatile sig_atomic_t		child_count;


int main(int argc, char *argv[])
{
  BlackList		lst;
  FILE *		ip_file;
  FILE *		result_file;
  struct Arguments	args = {
    .mac_type  = mcRANDOM,
    .ipfile    = argv[2]
  };

  if (argc!=4) {
    write(2, "Wrong argument-count; aborting...\n", 33);
    return EXIT_FAILURE;
  }

  ip_file = fopen(argv[1], "r");
  if (ip_file==0) {
    perror("fopen()");
    return EXIT_FAILURE;
  }

  result_file = fopen(argv[3], "r");
  if (result_file==0) {
    perror("fopen()");
    return EXIT_FAILURE;
  }

  
  
  BlackList_init(&lst, &args);
  BlackList_softUpdate(&lst);
  BlackList_print(&lst, 3);
  write(1, "\n", 1);

  while (!ferror(ip_file) && !feof(ip_file)) {
    char		ip_str[128], mac_str[128];
    int			res_i = fscanf(ip_file,     "%s\n", ip_str);
    int			res_r = fscanf(result_file, "%s\n", mac_str);
    struct ether_addr	result, exp_result;
    struct in_addr	inp;
    bool		is_ok = 1;

    if (ip_str[0]=='#' || ip_str[0]=='\n' || ip_str[0]=='\0') continue;
    
    if (res_i==0 || res_r==0) {
      write(2, "Invalid format; aborting...\n", 28);
      return EXIT_FAILURE;
    }

    if (inet_aton(ip_str, &inp)==0) {
      write(2, "Invalid IP; aborting...\n", 24);
      return EXIT_FAILURE;
    }

    if (mac_str[0]!='-' && mac_str[0]!='R' &&
	ether_aton_r(mac_str, &exp_result)==0)
    {
      write(2, "Invalid MAC; aborting...\n", 25);
      return EXIT_FAILURE;
    }

    printf("%-15s\t", ip_str);
    if (BlackList_getMac(&lst, inp, &result)) {
      char		buffer[128];
      sprintf(buffer, "%s", ether_ntoa(&result));
      if (mac_str[0]=='-') is_ok = 0;
      else if (mac_str[0]!='R') {
	sprintf(buffer+strlen(buffer), "/%s", ether_ntoa(&exp_result));
	is_ok = is_ok && (memcmp(&result, &exp_result, sizeof result)==0);
      }
      printf("%-35s\t", buffer);
    }
    else {
      printf("%-35s\t", "not found");
      is_ok = is_ok && (mac_str[0]=='-');
    }

    if (is_ok) printf("OK\n");
    else       printf("FAIL\n");
  }

  BlackList_free(&lst);
  fclose(result_file);
  fclose(ip_file);
  
  return EXIT_SUCCESS;  
}


  /// Local Variables:
  /// compile-command: "make -C .. check"
  /// End:
