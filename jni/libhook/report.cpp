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
#include "report.h"
#include "hook.h"
#include <sstream>
#include <iomanip>
#include <time.h>

#define LOCK() pthread_mutex_lock(&__lock)
#define UNLOCK() pthread_mutex_unlock(&__lock)

static report_options_t __opts = { LOGCAT, "", 0 };
static pthread_mutex_t  __lock = PTHREAD_MUTEX_INITIALIZER;

long int timestamp() {
    struct timeval tp = {0};
    gettimeofday(&tp, NULL);

    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

static long int __started = timestamp();

void report_set_options( report_options_t *opts ) {
    LOCK();

    __opts.mode = opts->mode;
    __opts.dest = opts->dest;
    __opts.port = opts->port;

    UNLOCK();
}

std::ostringstream& parse( char fmt, std::ostringstream& s, va_list& va ) {
    switch( fmt )
    {
        case 'i':
            s << va_arg( va, int ) << " ";
        break;

        case 'u':
            s << va_arg( va, unsigned int ) << " ";
        break;

        case 'p':
            s << std::hex << std::setfill('0') << "0x" << va_arg( va, uintptr_t ) << " ";
        break;

        case 's':
            s << '"' << va_arg( va, const char * ) << '"' << " ";
        break;

        default:

            s << std::hex << std::setfill('0') << "0x" << va_arg( va, uintptr_t ) << " ";
    }

    return s;
}

void report_add( const char *fnname, const char *argsfmt, ... ) {
	va_list va;
    size_t i, argc = strlen(argsfmt);

    LOCK();

    if( __opts.mode == LOGCAT ) {
        std::ostringstream s;

        s << "[ ts=" << ( timestamp() - __started ) << " pid=" << getpid() << ", tid=" << gettid() << " ] " << fnname << "( ";

        va_start( va, argsfmt );

        for( i = 0; i < argc; ++i ) {
            char fmt = argsfmt[i];

            // next will be the return value, break
            if( fmt == '.' ){
                break;
            }

            s << va_arg( va, char * ) << "=";
            parse( fmt, s, va );
        }

        s << ")";

        // get return value
        if( i != argc ){
            char fmt = argsfmt[i + 1];
            s << " -> ";
            parse( fmt, s, va );
        }

        va_end( va );

        HOOKLOG( "%s", s.str().c_str() );
    }

    UNLOCK();
}
