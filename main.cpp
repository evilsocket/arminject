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
#include "traced.hpp"

int usage( char *argvz ){
    printf( "Usage: %s <pid> <library>\n", argvz );
    return 1;
}

int main( int argc, char **argv )
{
    if( argc < 3 ){
        return usage(argv[0]);
    }

    pid_t pid = atoi(argv[1]);
    if( pid == 0 ){
        fprintf( stderr, "Invaid PID %s\n", argv[1] );
        return 1;
    }

    Traced proc(pid);

    char *library = strdup(argv[2]);

    printf( "@ Injecting library %s into process %d.\n\n", library, pid );

    printf( "@ Calling dlopen in target process ...\n" );

    unsigned long dlret = proc.dlopen( library );

    printf( "@ dlopen returned %p\n", dlret );

/*
    If you want to remotely call a function inside the injected library,
    uncomment the following code.

    unsigned long rsym = proc.dlsym( dlret, "you_symbol_name" );

    printf( "@ dlsym returned %p\n", rsym );

    proc.call( (void *)rsym, 1, some_argument );
*/

    free( library );
    
    return 0;
}
