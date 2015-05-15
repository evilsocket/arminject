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
#include "file.h"

extern uintptr_t find_original( const char *name );

DEFINEHOOK( int, open, (const char *pathname, int flags) ) {
    int fd = ORIGINAL( open, pathname, flags );

    HOOKLOG( "[%d] open('%s', %d) -> %d", getpid(), pathname, flags, fd );

    return fd;
}

DEFINEHOOK( ssize_t, read, (int fd, void *buf, size_t count) ) {
    ssize_t r = ORIGINAL( read, fd, buf, count );

    HOOKLOG( "[%d] read( %d, %p, %u ) -> %d", getpid(), fd, buf, count, r );

    return r;
}

DEFINEHOOK( ssize_t, write, (int fd, const void *buf, size_t len, int flags) ) {
    ssize_t wrote = ORIGINAL( write, fd, buf, len, flags );

    HOOKLOG( "[%d] write( %d, %s, %u, %d ) -> %d", getpid(), fd, (const char *)buf, len, flags, wrote );

    return wrote;
}

DEFINEHOOK( int, close, (int fd) ) {
    int c = ORIGINAL( close, fd );

    HOOKLOG( "[%d] close( %d ) -> %d", getpid(), fd, c );

    return c;
}
