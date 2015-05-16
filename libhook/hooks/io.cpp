/*
 * Copyright (c) 2015, Simone Margaritelli <evilsocket at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of ARM Inject nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "hook.h"
#include "io.h"
#include "report.h"
#include <map>
#include <sstream>
#include <pthread.h>

typedef std::map< int, std::string > fds_map_t;

static pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
static fds_map_t __descriptors;

#define LOCK() pthread_mutex_lock(&__lock)
#define UNLOCK() pthread_mutex_unlock(&__lock)

extern uintptr_t find_original( const char *name );

void io_add_descriptor( int fd, const char *name ) {
    LOCK();

    __descriptors[fd] = name;

    UNLOCK();
}

void io_del_descriptor( int fd ) {
    LOCK();

    fds_map_t::iterator i = __descriptors.find(fd);
    if( i != __descriptors.end() ){
        __descriptors.erase(i);
    }

    UNLOCK();
}

std::string io_resolve_descriptor( int fd ) {
    std::string name;

    LOCK();

    fds_map_t::iterator i = __descriptors.find(fd);
    if( i == __descriptors.end() ){
        // attempt to read descriptor from /proc/self/fd
        char descpath[0xFF] = {0},
             descbuff[0xFF] = {0};

        sprintf( descpath, "/proc/self/fd/%d", fd );
        if( readlink( descpath, descbuff, 0xFF ) != -1 ){
            name = descbuff;
        }
        else {
            std::ostringstream s;
            s << "(" << fd << ")";
            name = s.str();
        }
    }
    else {
        name = i->second;
    }

    UNLOCK();

    return name;
}

DEFINEHOOK( int, open, (const char *pathname, int flags) ) {
    int fd = ORIGINAL( open, pathname, flags );

    if( fd != -1 ){
        io_add_descriptor( fd, pathname );
    }

    report_add( "open", "si.i",
        "pathname", pathname,
        "flags", flags,
        fd );

    return fd;
}

DEFINEHOOK( ssize_t, read, (int fd, void *buf, size_t count) ) {
    ssize_t r = ORIGINAL( read, fd, buf, count );

    report_add( "read", "spu.i",
        "fd", io_resolve_descriptor(fd).c_str(),
        "buf", buf,
        "count", count,
        r );

    return r;
}

DEFINEHOOK( ssize_t, write, (int fd, const void *buf, size_t len, int flags) ) {
    ssize_t wrote = ORIGINAL( write, fd, buf, len, flags );

    report_add( "write", "spui.i",
        "fd", io_resolve_descriptor(fd).c_str(),
        "buf", buf,
        "len", len,
        "flags", flags,
        wrote );

    return wrote;
}

DEFINEHOOK( int, close, (int fd) ) {
    int c = ORIGINAL( close, fd );

    report_add( "close", "s.i",
        "fd", io_resolve_descriptor(fd).c_str(),
        c );

    io_del_descriptor( fd );

    return c;
}

DEFINEHOOK( int, connect, (int sockfd, const struct sockaddr *addr, socklen_t addrlen) ) {
    int ret = ORIGINAL( connect, sockfd, addr, addrlen );

    struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
    std::ostringstream s;

    if( addr_in->sin_family == 1 ){
        struct sockaddr_un *sun = (struct sockaddr_un *)addr;

        s << "unix://" << sun->sun_path;
    }
    else {
        s << "ip://" << inet_ntoa(addr_in->sin_addr);
    }

    if( ret == 0 ){
        io_add_descriptor( sockfd, s.str().c_str() );
    }

    report_add( "connect", "spd.i",
        "sockfd", io_resolve_descriptor(sockfd).c_str(),
        "addr", addr,
        "addrlen", addrlen,
        ret );

    return ret;
}

DEFINEHOOK( ssize_t, send, (int sockfd, const void *buf, size_t len, int flags) ) {
    ssize_t sent = ORIGINAL( send, sockfd, buf, len, flags );

    report_add( "send", "spui.i",
        "sockfd", io_resolve_descriptor(sockfd).c_str(),
        "buf", buf,
        "len", len,
        "flags", flags,
        sent );

    return sent;
}

DEFINEHOOK( ssize_t, sendto, (int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) ) {
    ssize_t sent = ORIGINAL( sendto, sockfd, buf, len, flags, dest_addr, addrlen );

    report_add( "sendto", "spuibu.i",
        "sockfd", io_resolve_descriptor(sockfd).c_str(),
        "buf", buf,
        "len", len,
        "flags", flags,
        "dest_addr", dest_addr,
        "addrlen", addrlen,
        sent );

    return sent;
}

DEFINEHOOK( ssize_t, sendmsg, (int sockfd, const struct msghdr *msg, int flags) ) {
    ssize_t sent = ORIGINAL( sendmsg, sockfd, msg, flags );

    report_add( "sendmsg", "spi.i",
        "sockfd", io_resolve_descriptor(sockfd).c_str(),
        "msg", msg,
        "flags", flags,
        sent );

    return sent;
}

DEFINEHOOK( ssize_t, recv, (int sockfd, const void *buf, size_t len, int flags) ) {
    ssize_t recvd = ORIGINAL( recv, sockfd, buf, len, flags );

    report_add( "recv", "spui.i",
        "sockfd", io_resolve_descriptor(sockfd).c_str(),
        "buf", buf,
        "len", len,
        "flags", flags,
        recvd );

    return recvd;
}

DEFINEHOOK( ssize_t, recvfrom, (int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) ) {
    ssize_t recvd = ORIGINAL( recvfrom, sockfd, buf, len, flags, dest_addr, addrlen );

    report_add( "recvfrom", "spuipu.i",
        "sockfd", io_resolve_descriptor(sockfd).c_str(),
        "buf", buf,
        "len", len,
        "flags", flags,
        "dest_addr", dest_addr,
        "addrlen", addrlen,
        recvd );

    return recvd;
}

DEFINEHOOK( ssize_t, recvmsg, (int sockfd, const struct msghdr *msg, int flags) ) {
    ssize_t recvd = ORIGINAL( recvmsg, sockfd, msg, flags );

    report_add( "recvmsg", "spi.i",
        "sockfd", io_resolve_descriptor(sockfd).c_str(),
        "msg", msg,
        "flags", flags,
        recvd );

    return recvd;
}

DEFINEHOOK( int, shutdown, (int sockfd, int how) ) {
    int ret = ORIGINAL( shutdown, sockfd, how );

    report_add( "shutdown", "si.i",
        "sockfd", io_resolve_descriptor(sockfd).c_str(),
        "how", how,
        ret );

    return ret;
}
