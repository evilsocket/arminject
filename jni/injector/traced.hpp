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
#ifndef TRACED_H__
#define TRACED_H__

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <dlfcn.h>
#include <stdarg.h>

#define CPSR_T_MASK ( 1u << 5 )

class Traced
{
private:

    pid_t _pid;

    void *_dlopen;
    void *_dlsym;
    void *_dlerror;
    void *_calloc;
    void *_free;

    // ptrace wrapper with some error checking.
    long trace( int request, void *addr = 0, size_t data = 0 ) {
        long ret = ptrace( request, _pid, (caddr_t)addr, (void *)data );
        if( ret == -1 && (errno == EBUSY || errno == EFAULT || errno == ESRCH) ){
            perror("ptrace");
            return -1;
        }
        return ret;
    }

public:

    /*
     * This method will open /proc/<pid>/maps and search for the specified
     * library base address.
     */
    uintptr_t findLibrary( const char *library, pid_t pid = -1 ) {
        char filename[0xFF] = {0},
             buffer[1024] = {0};
        FILE *fp = NULL;
        uintptr_t address = 0;

        sprintf( filename, "/proc/%d/maps", pid == -1 ? _pid : pid );

        fp = fopen( filename, "rt" );
        if( fp == NULL ){
            perror("fopen");
            goto done;
        }

        while( fgets( buffer, sizeof(buffer), fp ) ) {
            if( strstr( buffer, library ) ){
                address = (uintptr_t)strtoul( buffer, NULL, 16 );
                goto done;
            }
        }

        done:

        if(fp){
            fclose(fp);
        }

        return address;
    }

    /*
     * Compute the delta of the local and the remote modules and apply it to
     * the local address of the symbol ... BOOM, remote symbol address!
     */
    void *findFunction( const char* library, void* local_addr ){
        uintptr_t local_handle, remote_handle;

        local_handle = findLibrary( library, getpid() );
        remote_handle = findLibrary( library );

        return (void *)( (uintptr_t)local_addr + (uintptr_t)remote_handle - (uintptr_t)local_handle );
    }

    /*
     * Read 'blen' bytes from the remote process at 'addr' address.
     */
    bool read( size_t addr, unsigned char *buf, size_t blen ){
        size_t i = 0;
        long ret = 0;

        for( i = 0; i < blen; i += sizeof(size_t) ){
            ret = trace( PTRACE_PEEKTEXT, (void *)(addr + i) );
            if( ret == -1 ) {
                return false;
            }

            memcpy( &buf[i], &ret, sizeof(ret) );
        }

        return true;
    }

    /*
     * Write 'blen' bytes to the remote process at 'addr' address.
     */
    bool write( size_t addr, unsigned char *buf, size_t blen){
        size_t i = 0;
        long ret;

        // make sure the buffer is word aligned
        char *ptr = (char *)calloc(blen + blen % sizeof(size_t), 1);
        memcpy(ptr, buf, blen);

        for( i = 0; i < blen; i += sizeof(size_t) ){
            ret = trace( PTRACE_POKETEXT, (void *)(addr + i), *(size_t *)&ptr[i] );
            if( ret == -1 ) {
                ::free(ptr);
                return false;
            }
        }

        ::free(ptr);

        return true;
    }

    /*
     * Remotely call the remote function given its address, the number of
     * arguments and the arguments themselves.
     */
    unsigned long call( void *function, int nargs, ... ) {
        int i = 0;
        struct pt_regs regs = {{0}}, rbackup = {{0}};

        // get registers and backup them
        trace( PTRACE_GETREGS, 0, (size_t)&regs );
        memcpy( &rbackup, &regs, sizeof(struct pt_regs) );

        va_list vl;
        va_start(vl,nargs);

        for( i = 0; i < nargs; ++i ){
            unsigned long arg = va_arg( vl, long );

            // fill R0-R3 with the first 4 arguments
            if( i < 4 ){
                regs.uregs[i] = arg;
            }
            // push remaining params onto stack
            else {
                regs.ARM_sp -= sizeof(long) ;
                write( (size_t)regs.ARM_sp, (uint8_t *)&arg, sizeof(long) );
            }
        }

        va_end(vl);

        regs.ARM_lr = 0;
        regs.ARM_pc = (long int)function;
        // setup the current processor status register
        if ( regs.ARM_pc & 1 ){
            /* thumb */
            regs.ARM_pc   &= (~1u);
            regs.ARM_cpsr |= CPSR_T_MASK;
        }
        else{
            /* arm */
            regs.ARM_cpsr &= ~CPSR_T_MASK;
        }

        // do the call
        trace( PTRACE_SETREGS, 0, (size_t)&regs );
        trace( PTRACE_CONT );
        waitpid( _pid, NULL, WUNTRACED );

        // get registers again, R0 holds the return value
        trace( PTRACE_GETREGS, 0, (size_t)&regs );

        // restore original registers state
        trace( PTRACE_SETREGS, 0, (size_t)&rbackup );

        return regs.ARM_r0;
    }

    // Copy a given string into the remote process memory.
    unsigned long copyString( const char *s ) {
        unsigned long mem = call( _calloc, 2, strlen(s) + 1, 1 );

        write( mem, (unsigned char *)s, strlen(s) + 1 );

        return mem;
    }

    // Remotely force the target process to dlopen a library.
    unsigned long dlopen( const char *libname ) {
        unsigned long pmem = copyString(libname);
        unsigned long plib = call( _dlopen, 2, pmem, 0 );

        free(pmem);

        return plib;
    }

    // Remotely call dlsym on the target process.
    unsigned long dlsym( unsigned long dl, const char *symname ) {
        unsigned long pmem = copyString(symname);
        unsigned long psym = call( _dlsym, 2, dl, pmem );

        free(pmem);

        return psym;
    }

    // Free remotely allocated memory.
    void free( unsigned long p ) {
        call( _free, 1, p );
    }

    Traced( pid_t pid ) : _pid(pid) {
        if( trace( PTRACE_ATTACH ) != -1 ){
            int status;
            waitpid( _pid, &status, 0 );

            /*
             * First thing first, we need to search these functions into the target
             * process address space.
             */
            _dlopen  = findFunction( "/system/bin/linker", (void *)::dlopen );
            _dlsym   = findFunction( "/system/bin/linker", (void *)::dlsym );
            _dlerror = findFunction( "/system/bin/linker", (void *)::dlerror );
            _calloc  = findFunction( "/system/lib/libc.so", (void *)::calloc );
            _free    = findFunction( "/system/lib/libc.so", (void *)::free );

            if( !_calloc ){
                fprintf( stderr, "Could not find calloc symbol.\n" );
            }
            else if( !_free ){
                fprintf( stderr, "Could not find dlopen symbol.\n" );
            }
            else if( !_dlopen ){
                fprintf( stderr, "Could not find dlopen symbol.\n" );
            }
            else if( !_dlsym ){
                fprintf( stderr, "Could not find dlsym symbol.\n" );
            }
            else if( !_dlerror ){
                fprintf( stderr, "Could not find dlerror symbol.\n" );
            }
        }
        else {
            fprintf( stderr, "Failed to attach to process %d.", _pid );
        }
    }

    virtual ~Traced() {
        trace( PTRACE_DETACH );
    }
};

#endif
