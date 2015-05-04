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

typedef int (* open_t)(const char *, int);

open_t __open = NULL;

int hook_open(const char *pathname, int flags) {
    HOOKLOG( "[%d] open('%s', %d)", getpid(), pathname, flags );

    return __open( pathname, flags );
}

void __attribute__ ((constructor)) libhook_main()
{
    HOOKLOG( "LIBRARY LOADED FROM PID %d.", getpid() );

    const char *modules[] = {
        "libc.so",
        "libjavacore.so",
        "libandroid_runtime.so",
        "libandroid.so",
        "libart.so",
        "libbcc.so",
        "libc++.so",
        "libcutils.so"
    };

    for( int i = 0, n = sizeof(modules) / sizeof(modules[0]); i < n; ++i ){
        HOOKLOG( "Hooking module %s ...", modules[i] );

        __open = (open_t)libhook_addhook( modules[i], "open", hook_open );

        HOOKLOG( "  Original open @ %p", __open );
    }
}
