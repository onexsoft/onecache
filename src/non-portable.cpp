/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*/

#include "util/logger.h"
#include "non-portable.h"

#ifndef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>

#include <sys/wait.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

struct ARP_PACKET
{
    unsigned char dest_mac[6];
    unsigned char src_mac[6];
    unsigned short type;
    unsigned short hw_type;
    unsigned short pro_type;
    unsigned char hw_len;
    unsigned char pro_len;
    unsigned short op;
    unsigned char from_mac[6];
    unsigned char from_ip[4];
    unsigned char to_mac[6];
    unsigned char to_ip[4];
};

int send_arp_pack(int ifndx, char *address, char *mac)
{
    int err=0;
    int sfd = 0;
    char buf[256];
    struct ARP_PACKET *ah = (struct ARP_PACKET *)buf;
    struct sockaddr_ll dest;
    int dlen = sizeof(dest);

    memset(buf,0,256);
    sfd = socket(PF_PACKET,SOCK_RAW, htons(ETH_P_RARP));
    if (sfd >= 0)
    {
        memset(&dest,0, sizeof(dest));
        dest.sll_family = AF_PACKET;
        dest.sll_halen =  ETH_ALEN;
        dest.sll_ifindex = ifndx;
        memcpy(dest.sll_addr, mac, ETH_ALEN);

        ah->type     = htons(ETHERTYPE_ARP);
        memset(ah->dest_mac, 0xff, 6);
        memcpy(ah->src_mac, mac, 6);
        ah->hw_type  = htons(ARPHRD_ETHER);
        ah->pro_type = htons(ETH_P_IP);
        ah->hw_len   = ETH_ALEN;
        ah->pro_len  = 4;
        ah->op       = htons(ARPOP_REPLY);
        memcpy(ah->from_mac, mac, 6);
        memcpy(ah->from_ip, address, 4);
        memset(ah->to_ip, 0x00, 4);
        memset(ah->to_mac, 0xff, 6);

        err = sendto(sfd, buf, sizeof(*ah), 0, (struct sockaddr *)&dest, dlen);
        close(sfd);
        if (err == sizeof(*ah)) return 1;
    }
    return 0;
}

int NonPortable::setVipAddress(const char *ifname, const char *address, int isdel)
{
    int i = 0;
    int err = 0;
    int sfd = 0;
    struct ifreq ifr;
    char arpcmd[512];

    int    addr_mask = 0;
    struct sockaddr_in brdaddr;
    struct sockaddr_in netmask;

    int    ifindex = 0;
    struct sockaddr_in ipaddr;
    struct sockaddr macaddr;

    struct sockaddr_in *sin = NULL;

    sfd = socket (AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) return 0;

    i=0;
    while(ifname[i] && ifname[i] != ':') i++;
    if (ifname[i] == ':')
    {
        memset(&ifr, 0, sizeof (ifr));
        memcpy(ifr.ifr_name, ifname, i);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';
        addr_mask = 0;
        memset(&ifr.ifr_ifru, 0, sizeof(ifr.ifr_ifru));
        if (ioctl (sfd, SIOCGIFBRDADDR, &ifr) >= 0)
        {
            memcpy(&brdaddr, &ifr.ifr_broadaddr, sizeof(struct sockaddr_in));
            addr_mask = addr_mask | 0x01;
        }
        memset(&ifr.ifr_ifru, 0, sizeof(ifr.ifr_ifru));
        if (ioctl (sfd, SIOCGIFNETMASK, &ifr) >= 0)
        {
            memcpy(&netmask, &ifr.ifr_netmask, sizeof(struct sockaddr_in));
            addr_mask = addr_mask | 0x02;
        }
        memset(&ifr.ifr_ifru, 0, sizeof(ifr.ifr_ifru));
        if (ioctl (sfd, SIOCGIFHWADDR, &ifr) >= 0)
        {
            memcpy(&macaddr, &ifr.ifr_hwaddr, sizeof(struct sockaddr_in));
            addr_mask = addr_mask | 0x04;
        }
        memset(&ifr.ifr_ifru, 0, sizeof(ifr.ifr_ifru));
        if (ioctl (sfd, SIOCGIFINDEX, &ifr) >= 0)
        {
            ifindex = ifr.ifr_ifindex;
        }
        memset(&ifr.ifr_ifru, 0, sizeof(ifr.ifr_ifru));
        if (ioctl (sfd, SIOCGIFADDR, &ifr) >= 0)
        {
            memcpy(&ipaddr, &ifr.ifr_addr, sizeof(struct sockaddr_in));
            addr_mask = addr_mask | 0x08;
        }
    }

    memset(arpcmd, 0, 512);
    sprintf(arpcmd, "/sbin/arping -A -c 1 -I %s %s > /dev/null 2>&1",ifr.ifr_name, address);

    memset(&ifr, 0, sizeof (ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    ioctl (sfd, SIOCSIFFLAGS, &ifr);
    memset(&ifr.ifr_ifru, 0, sizeof(ifr.ifr_ifru));

    if (isdel)
    {
        close(sfd);
        return 0;
    }

    sin = (struct sockaddr_in *) &ifr.ifr_addr;
    sin->sin_family = AF_INET;
    err = inet_aton(address, &sin->sin_addr);
    netmask.sin_addr.s_addr = 0xffffffff;
    if (err)
    {
        err = ioctl (sfd, SIOCSIFADDR, &ifr);
        if (err < 0)
        {
            close(sfd);
            return 0;
        }

        if (addr_mask & 0x1)
        {
            memset(&ifr.ifr_ifru, 0, sizeof(ifr.ifr_ifru));
            memcpy(&ifr.ifr_broadaddr, &brdaddr, sizeof(struct sockaddr_in));
            ioctl (sfd, SIOCSIFBRDADDR, &ifr);
        }
        if (addr_mask & 0x2)
        {
            memset(&ifr.ifr_ifru, 0, sizeof(ifr.ifr_ifru));
            memcpy(&ifr.ifr_netmask, &netmask, sizeof(struct sockaddr_in));
            ioctl (sfd, SIOCSIFNETMASK, &ifr);
        }
        close(sfd);
        sin->sin_family = AF_INET;
        err = inet_aton(address, &sin->sin_addr);
        if (!send_arp_pack(ifindex, (char*)&sin->sin_addr.s_addr, macaddr.sa_data)) {
            err = system(arpcmd);
        }
        return 1;
    }
    close(sfd);
    return 0;
}
#else
int NonPortable::setVipAddress(const char*, const char*, int)
{
    return 0;
}
#endif



socket_t NonPortable::createUnixSocketFile(const char* file)
{
#ifndef WIN32
    socket_t server_sockfd = -1;
    server_sockfd = ::socket(AF_UNIX, SOCK_STREAM, 0);

    sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, file);

    if (::bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        TcpSocket::close(server_sockfd);
        return -1;
    }

    if (::listen(server_sockfd, 128) != 0) {
        TcpSocket::close(server_sockfd);
        return -1;
    }
    return server_sockfd;
#else
    (void)file;
    return -1;
#endif
}



void NonPortable::guard(void (*driverFunc)(), int exit_key)
{
#ifdef WIN32
    (void)exit_key;
    driverFunc();
#else
    do {
        pid_t pid = fork();
        if (0 == pid) {
            driverFunc();
            exit(0);
        } else if (pid > 0) {
            int status;
            pid_t pidWait = waitpid(pid, &status, 0);
            if (pidWait < 0) {
                Logger::log(Logger::Error, "waitpid() failed");
                exit(0);
            }

            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == exit_key) {
                    exit(0);
                }
            }
            Logger::log(Logger::Message, "Restart the program...", status);
        } else {
            Logger::log(Logger::Error, "fork() failed");
            exit(1);
        }
    } while (true);
#endif
}

int NonPortable::daemonize(void)
{
#ifndef WIN32
    int status;
    pid_t pid, sid;
    int fd;

    char appdir[1024];
    char* appdirptr = getcwd(appdir, 1024);

    pid = fork();
    switch (pid) {
    case -1:
        Logger::log(Logger::Error, "fork() failed: %s", strerror(errno));
        return 1;

    case 0:
        break;

    default:
        exit(0);
    }

    sid = setsid();
    if (sid < 0) {
        Logger::log(Logger::Error, "setsid() failed: %s", strerror(errno));
        return 1;
    }

    if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
        Logger::log(Logger::Error, "signal(SIGHUP, SIG_IGN) failed: %s", strerror(errno));
        return 1;
    }

    pid = fork();
    switch (pid) {
    case -1:
        Logger::log(Logger::Error, "fork() failed: %s", strerror(errno));
        return 1;

    case 0:
        break;

    default:
        exit(0);
    }


    umask(0);

    fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        Logger::log(Logger::Error, "open(\"/dev/null\") failed: %s", strerror(errno));
        return 1;
    }

    status = dup2(fd, STDIN_FILENO);
    if (status < 0) {
        Logger::log(Logger::Error, "dup2(%d, STDIN) failed: %s", fd, strerror(errno));
        close(fd);
        return 1;
    }

    status = dup2(fd, STDOUT_FILENO);
    if (status < 0) {
        Logger::log(Logger::Error, "dup2(%d, STDOUT) failed: %s", fd, strerror(errno));
        close(fd);
        return 1;
    }

    status = dup2(fd, STDERR_FILENO);
    if (status < 0) {
        Logger::log(Logger::Error, "dup2(%d, STDERR) failed: %s", fd, strerror(errno));
        close(fd);
        return 1;
    }

    if (fd > STDERR_FILENO) {
        status = close(fd);
        if (status < 0) {
            Logger::log(Logger::Error, "close(%d) failed: %s", fd, strerror(errno));
            return 1;
        }
    }

    if (chdir(appdirptr) != -1) {
        Logger::log(Logger::Error, "chdir() failed");
    }
#endif
    return 0;
}
