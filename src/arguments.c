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

#include "arguments.h"
#include "util.h"

#include <getopt.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#ifndef DEFAULT_IPFILE
#  define DEFAULT_IPFILE		"ips.cfg"
#endif

#ifndef DEFAULT_PIDFILE
#  define DEFAULT_PIDFILE		"/var/run/ip-sentinel.run"
#endif

#ifndef DEFAULT_LOGFILE
#  define DEFAULT_LOGFILE		"/var/log/ip-sentinel.out"
#endif

#ifndef DEFAULT_ERRFILE
#  define DEFAULT_ERRFILE		"/var/log/ip-sentinel.err"
#endif

#ifndef DEFAULT_USER
#  define DEFAULT_USER			SENTINEL_USER
#endif

#ifndef DEFAULT_CHROOT
#  define DEFAULT_CHROOT		0
#endif


static struct option
cmdline_options[] = {
  { "ipfile",  required_argument, 0, 'i' },
  { "pidfile", required_argument, 0, 'p' },
  { "logfile", required_argument, 0, 'l' },
  { "errfile", required_argument, 0, 'e' },
  { "user",    required_argument, 0, 'u' },
  { "nofork",  no_argument,       0, 'n' },
  { "chroot",  required_argument, 0, 'r' },
  { "help",    no_argument,       0, 'h' },
  { "version", no_argument,       0, 'v' },
  { 0, 0, 0, 0 }
};

static void
printHelp(char const *cmd, int fd)
{
  WRITE_MSGSTR(fd, "ip-sentinel " PACKAGE_VERSION " -- keeps your ip-space clean\n\nUsage: \n   ");
  WRITE_MSG   (fd, cmd);
  WRITE_MSGSTR(fd,
	       " [--ipfile|-i <FILE>] [--pidfile|-p <FILE>]\n"
	       "        [--logfile|-l <FILE>] [--errfile|-e <FILE>]"
	       " [--user|-u <USER>]\n"
   	       "        [--chroot|-r <DIR>] [--nofork|-n] [--help|-h]\n"
	       " [--version] <interface>\n\n");
  WRITE_MSGSTR(fd,
	       "      --ipfile|-i <FILE>      read blocked IPs from FILE [" DEFAULT_IPFILE "]\n"
	       "                              within CHROOT\n"
	       "      --pidfile|-p <FILE>     write daemon-pid into FILE [" DEFAULT_PIDFILE "]\n"
	       "      --logfile|-l <FILE>     log activities into FILE [" DEFAULT_LOGFILE "]\n"
	       "      --errfile|-e <FILE>     log errors into FILE [" DEFAULT_ERRFILE "]\n"
	       "      --user|-u <USER>        run as user USER [" DEFAULT_USER "]\n"
	       "      --chroot|-r <DIR>       go into chroot-jail at DIR [<HOME>]\n"
	       "      --nofork|-n             do not fork a daemon-process\n"
	       "      --help|-h               display this text and exit\n"
	       "      --version               print version and exit\n"
	       "      interface               ethernet-interface where to listen"
	       "                              on ARP-requests\n"
	       "\nPlease report errors to <" PACKAGE_BUGREPORT ">\n");
}

static void
printVersion(int fd)
{
  WRITE_MSGSTR(fd,
	       "ip-sentinel " PACKAGE_VERSION " -- keeps your ip-space clean\n"
	       "Copyright 2002 Enrico Scholz\n"
	       "This program is free software; you may redistribute it under the terms of\n"
	       "the GNU General Public License.  This program has absolutely no warranty.\n");
}

void
parseOptions(int argc, char *argv[], Arguments *options)
{
  assert(options!=0);
  
  options->ipfile   = DEFAULT_IPFILE;
  options->pidfile  = DEFAULT_PIDFILE;
  options->logfile  = DEFAULT_LOGFILE;
  options->errfile  = DEFAULT_ERRFILE;
  options->user     = DEFAULT_USER;
  options->do_fork  = true;
  options->chroot   = 0;

  while (1) {
    int	c = getopt_long(argc, argv, "hi:p:l:e:u:nr:", cmdline_options, 0);
    if (c==-1) break;

    switch (c) {
      case 'h'  :  printHelp(argv[0],1); exit(0); break;
      case 'i'	:  options->ipfile   = optarg; break;
      case 'p'	:  options->pidfile  = optarg; break;
      case 'l'	:  options->logfile  = optarg; break;
      case 'e'	:  options->errfile  = optarg; break;
      case 'u'	:  options->user     = optarg; break;
      case 'n'	:  options->do_fork  = false;  break;
      case 'r'	:  options->chroot   = optarg; break;
      case 'v'	:  printVersion(1); exit(0);    break;
      default	:
	WRITE_MSGSTR(2, "Try \"");
	WRITE_MSG   (2, argv[0]);
	WRITE_MSGSTR(2, " --help\" for more information.\n");
	exit(1);
	break;
    }
  }

  if (optind>=argc)        WRITE_MSGSTR(2, "No interface specified; ");
  else if (optind+1!=argc) WRITE_MSGSTR(2, "Too much interfaces specified; ");

  if (optind+1!=argc) {
    WRITE_MSGSTR(2, "try \"");
    WRITE_MSG   (2, argv[0]);
    WRITE_MSGSTR(2, " --help\" for more information.\n");
    exit(1);
  }

  options->iface = argv[optind];
}
