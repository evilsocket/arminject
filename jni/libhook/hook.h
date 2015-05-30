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
#ifndef HOOK_H
#define HOOK_H

#include <android/log.h>
#include <string>
#include <sys/types.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "linker.h"

#define HOOKLOG(F,...) \
    __android_log_print( ANDROID_LOG_INFO, "LIBHOOK", F, __VA_ARGS__ )

#define ORIGINAL( TYPENAME, ... ) \
    ((TYPENAME ## _t)find_original( #TYPENAME ))( __VA_ARGS__ )

#define DEFINEHOOK( RET_TYPE, NAME, ARGS ) \
    typedef RET_TYPE (* NAME ## _t)ARGS; \
    RET_TYPE hook_ ## NAME ARGS

#define ADDHOOK( NAME ) \
    { #NAME, 0, (uintptr_t)&hook_ ## NAME }

typedef struct ld_module
{
    uintptr_t   address;
    std::string name;

    ld_module( uintptr_t a, const std::string& n ) : address(a), name(n) {

    }
}
ld_module_t;

typedef std::vector<ld_module_t> ld_modules_t;

ld_modules_t libhook_get_modules();
unsigned     libhook_addhook( const char *soname, const char *symbol, unsigned newval );

#endif
