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
#include "wrappers.h"
#include "util.h"
#include "parameters.h"
#include "blacklist.h"
#include "antidos.h"
#include "arpmessage.h"
#include "compat.h"
#include "ip-sentinel.h"

#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <grp.h>

static struct ether_addr const	BCAST_MAC   = { { 255, 255, 255, 255, 255, 255 } };

volatile sig_atomic_t		child_count;
static volatile sig_atomic_t	do_reload;

static void sigChild(int);
static void sigHup(int);

void
sigChild(int sig UNUSED)
{
  while (waitpid(-1, 0, WNOHANG)>0) { if (child_count>0) --child_count; }
  signal(SIGCHLD, sigChild);
}

void
sigHup(int sig UNUSED)
{
  do_reload = 1;
  signal(SIGHUP, sigHup);
}

inline static void
adjustUserGroup(Arguments *arguments, uid_t *uid, gid_t *gid)
{
  struct passwd const *	pw_user;
  char *		err_ptr;

  assert(uid!=0 && gid!=0);
  
  *gid = static_cast(gid_t)(-1);
  *uid = strtol(arguments->user, &err_ptr, 0);
  if (arguments->user[0]!='\0' && *err_ptr=='\0') pw_user = getpwuid(*uid);
  else                                            pw_user = Egetpwnam(arguments->user);
  
  if (pw_user!=0) {
    *uid = pw_user->pw_uid;
    *gid = pw_user->pw_gid;
    if (arguments->chroot==0) arguments->chroot = pw_user->pw_dir;
  }

  if (arguments->group!=0) {
    *gid = strtol(arguments->group, &err_ptr, 0);
    if (arguments->group[0]=='\0' || *err_ptr!='\0')
      *gid = Egetgrnam(arguments->group)->gr_gid;
  }

  if (*gid==static_cast(gid_t)(-1)) {
    WRITE_MSGSTR(2, "Failed to determine gid; perhaps you should use the '-g' option. Aborting...\n");
    exit(1);
  }
}

static void
daemonize(Arguments *arguments)
{
  int			err_fd, out_fd, pid_fd;
  int			aux;
  uid_t			uid;
  gid_t			gid;
  pid_t			daemon_pid;

  err_fd = Eopen(arguments->errfile, O_WRONLY|O_CREAT|O_APPEND|O_NONBLOCK, 0700);
  out_fd = Eopen(arguments->logfile, O_WRONLY|O_CREAT|O_APPEND|O_NONBLOCK, 0700);
  pid_fd = (!arguments->do_fork ? -1 :
	    Eopen(arguments->pidfile, O_WRONLY|O_CREAT|O_TRUNC, 0755));

  adjustUserGroup(arguments, &uid, &gid);
  
  if (arguments->chroot != 0) {
    Echdir(arguments->chroot);
    Echroot(arguments->chroot);
  }

  Esetgroups(1, &gid);
  Esetgid(gid);
  Esetuid(uid);

  Eclose(0);

  aux = Eopen(arguments->ipfile, O_RDONLY, 0);
  Eclose(aux);

  if (arguments->do_fork) daemon_pid = Efork();
  else                    daemon_pid = 0;

  switch (daemon_pid) {
    case 0	:  break;
    default	:
      writeUInt(pid_fd, daemon_pid);
      Ewrite  (pid_fd, "\n", 1);
      assert(daemon_pid!=-1);
      exit(0);
      break;
  }

  if (arguments->do_fork) Esetsid();

  Edup2(out_fd, 1);
  Edup2(err_fd, 2);

  if (pid_fd!=-1) Eclose(pid_fd);
  Eclose(out_fd);
  Eclose(err_fd);
}

static int
getIfIndex(int fd, char const *iface_name)
{
  struct ifreq		iface;
  
  assert(iface_name!=0);
  memcpy(iface.ifr_name, iface_name, IFNAMSIZ);
  Eioctl(fd, SIOCGIFINDEX, &iface);
  if (iface.ifr_ifindex<0) {
    WRITE_MSGSTR(2, "No such interface '");
    WRITE_MSG   (2, iface_name);
    WRITE_MSGSTR(2, "'\n");
    exit(1);
  }

  return iface.ifr_ifindex;
}

static void
sendPacket(int s, int if_idx, struct ether_addr const *mac, in_addr_t const ip,
	   int count, long sleep_dur)
{
  ArpMessage d;
  struct sockaddr_ll	addr;

  memset(&d,    0, sizeof d);
  memset(&addr, 0, sizeof addr);

  d.padding[0] = 0x66;
  d.padding[1] = 0x60;
  d.padding[sizeof(d.padding)-2] = 0x0B;
  d.padding[sizeof(d.padding)-1] = 0x5E;
  
  d.data.ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
  d.data.ea_hdr.ar_pro = htons(ETHERTYPE_IP);
  d.data.ea_hdr.ar_hln = ETHER_ADDR_LEN;
  d.data.ea_hdr.ar_pln = 4;
  d.data.ea_hdr.ar_op  = htons(ARPOP_REPLY);

  memcpy(d.data.arp_sha, mac->ether_addr_octet, sizeof(d.data.arp_sha));
  memcpy(d.data.arp_spa, &ip, sizeof(d.data.arp_spa));

  memcpy(d.data.arp_tha, BCAST_MAC.ether_addr_octet, sizeof(d.data.arp_tha));
  *(in_addr_t*)(&d.data.arp_tpa) = INADDR_ANY;

  memcpy(d.header.ether_dhost, BCAST_MAC.ether_addr_octet, sizeof(d.header.ether_dhost));
  memcpy(d.header.ether_shost, d.data.arp_sha,             sizeof(d.header.ether_shost));
  d.header.ether_type  = htons(ETH_P_ARP);

  addr.sll_family   = AF_PACKET;
  addr.sll_protocol = 0;
  addr.sll_ifindex  = if_idx;
  addr.sll_hatype   = 0;
  addr.sll_pkttype  = 0;
  addr.sll_halen    = ETHER_ADDR_LEN;
  memcpy(addr.sll_addr, d.data.arp_tha, ETHER_ADDR_LEN);

  while (count!=0) {
    int ret = TEMP_FAILURE_RETRY(sendto(s, &d, sizeof(d), 0,
					reinterpret_cast(struct sockaddr const *)(&addr), sizeof(addr)));

    FatalErrnoError(ret==-1, 1, "sendto()");
    
    if (count!=-1) --count;
    if (count!=0) sleep(sleep_dur);
  }
}

static void
handlePacket(int sock,  int if_idx, struct ether_addr const *mac, struct ether_arp const *data)
{
  char			buffer[1024];
  char *		buf_ptr = buffer;
  size_t		len     = sizeof(buffer);

  struct in_addr const	*source = reinterpret_cast(struct in_addr const *)(&data->arp_spa);
  struct in_addr const	*target = reinterpret_cast(struct in_addr const *)(&data->arp_tpa);

  assert(target!=0 && source!=0);
  
  XSTRCAT(&buf_ptr, &len, "Handle IP '");
  xstrcat(&buf_ptr, &len, inet_ntoa(*target));
  XSTRCAT(&buf_ptr, &len, "' requested by '");
  xstrcat(&buf_ptr, &len, inet_ntoa(*source));
  XSTRCAT(&buf_ptr, &len, "'\n");

  write(1, buffer, sizeof(buffer)-len);

  sendPacket(sock, if_idx, mac, target->s_addr, 1, 0);
  usleep(1500000);
  sendPacket(sock, if_idx, mac, target->s_addr, 3, 2);
}

static void
handleMessage(int sock, int if_idx, struct ether_addr const *mac, struct ether_arp const *data)
{
  static int		error_count =0;
  pid_t			pid;
  sigset_t		block_set, old_set;

  while (child_count=>MAX_CHILDS) {
    writeMsgTimestamp(2);
    WRITE_MSGSTR(2, ": Too much children (");
    writeUInt(2, child_count);
    WRITE_MSGSTR(2, ") forked; sleeping a while...\n");
    sleep(1);
  }

  sigfillset(&block_set);
  sigprocmask(SIG_BLOCK, &block_set, &old_set);

  pid = fork();
  switch (pid) {
    case -1	:
      perror("fork()");
      writeMsgTimestamp(2);
      WRITE_MSGSTR(2, ": fork() failed");
      ++error_count;
      if (error_count>MAX_ERRORS) {
	WRITE_MSGSTR(2, "aborting...");
	exit(1);
      }
      break;
      
    case 0	:
      handlePacket(sock, if_idx, mac, data);
      exit(0);

    default	:
      ++child_count;
      error_count = 0;
      break;
  }

  sigprocmask(SIG_SETMASK, &old_set, 0);
}

static void NORETURN
run(int sock, int if_idx, char const *filename) 
{
  BlackList			cfg;
  AntiDOS			anti_dos;
  int				error_count = 0;
  struct sockaddr_ll		addr;
  socklen_t			from_len = sizeof(addr);
  char				buffer[4096];
  ArpMessage const * const	msg    = reinterpret_cast(ArpMessage const *)(buffer);
  struct in_addr const	*	src_ip = reinterpret_cast(struct in_addr const *)(msg->data.arp_spa);
  unsigned int			oversize_sleep = 1;

  memset(&addr, 0, sizeof(addr));
  addr.sll_family  = AF_PACKET;
  addr.sll_ifindex = if_idx;
  
  BlackList_init(&cfg, filename);
  AntiDOS_init(&anti_dos);

  while (true) {
    size_t			size;
    int				arp_count;
    struct ether_addr const	*mac;
    struct ether_addr		mac_buffer;
    
    AntiDOS_update(&anti_dos);
    size = TEMP_FAILURE_RETRY(recvfrom(sock, buffer, sizeof buffer, 0,
				       (struct sockaddr *)(&addr), &from_len));

    if (static_cast(ssize_t)(size)==-1) {
      ++error_count;
      if (error_count>MAX_ERRORS) {
	perror("recvfrom()");
	exit(1);
      }

      continue;
    }

    error_count = 0;

    if (ntohs(addr.sll_protocol)!=ETHERTYPE_ARP)      continue;
    if (ntohs(msg->data.ea_hdr.ar_op)!=ARPOP_REQUEST) continue;

    if (!do_reload) BlackList_softUpdate(&cfg);
    else {
      BlackList_update(&cfg);
      do_reload = false;
    }

    mac = BlackList_getMac(&cfg, *reinterpret_cast(struct in_addr *)(msg->data.arp_tpa), &mac_buffer);
    if (mac==0) continue;

    assert(src_ip!=0);
    if (!AntiDOS_isOversized(&anti_dos)) oversize_sleep = 1;
    else {
      writeMsgTimestamp(2);
      WRITE_MSGSTR(2, ": Too much requests from too much IPs; last IP was ");
      writeIP     (2, *src_ip);
      WRITE_MSGSTR(2, "\n");

      sleep(oversize_sleep);
      oversize_sleep = MIN(oversize_sleep+1, 10);
      continue;
    }

    arp_count = AntiDOS_registerIP(&anti_dos, *src_ip);

    if (isDOS(arp_count)) {
      writeMsgTimestamp(2);
      WRITE_MSGSTR(2, ": Too much requests from ");
      writeIP     (2, *src_ip);
      WRITE_MSGSTR(2, "; DOS-measurement was ");
      writeUInt   (2, arp_count);
      WRITE_MSGSTR(2, "\n");

      continue;
    }


    handleMessage(sock, if_idx, mac, &msg->data);
  }
}

int
main(int argc, char *argv[])
{
  Arguments		arguments;
  int			sock;
  int			if_idx;
  
  parseOptions(argc, argv, &arguments);

  sock   = Esocket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
  if_idx = getIfIndex(sock, arguments.iface);
  daemonize(&arguments);

  signal(SIGCHLD, sigChild);
  signal(SIGHUP,  sigHup);

  run(sock, if_idx, arguments.ipfile);
}
