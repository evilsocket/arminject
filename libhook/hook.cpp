/*
 * Copyright (c) 2015, Simone Margaritelli <evilsocket at gmail dot com>
 * All rights reserved.
 *
 * Most of the ELF manipulation code was taken from the Andrey Petrov's
 * blog post "Android hacking: hooking system functions used by Dalvik"
 * http://shadowwhowalks.blogspot.it/2013/01/android-hacking-hooking-system.html
 * and fixed by me.
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
#include <sys/mman.h>


static unsigned elfhash(const char *_name) {
   const unsigned char *name = (const unsigned char *) _name;
   unsigned h = 0, g;
   while(*name) {
       h = (h << 4) + *name++;
       g = h & 0xf0000000;
       h ^= g;
       h ^= g >> 24;
   }
   return h;
}

static Elf32_Sym *soinfo_elf_lookup(struct soinfo *si, unsigned hash, const char *name) {
    Elf32_Sym *symtab = si->symtab;
    const char *strtab = si->strtab;
    unsigned n;

    for( n = si->bucket[hash % si->nbucket]; n != 0; n = si->chain[n] ) {
        Elf32_Sym *s = symtab + n;
        if( strcmp(strtab + s->st_name, name) == 0 ) {
            return s;
        }
    }

    return NULL;
}

ld_modules_t libhook_get_modules() {
    ld_modules_t modules;
    char buffer[1024] = {0};
    uintptr_t address;
    std::string name;

    FILE *fp = fopen( "/proc/self/maps", "rt" );
    if( fp == NULL ){
        perror("fopen");
        goto done;
    }

    while( fgets( buffer, sizeof(buffer), fp ) ) {
        if( strstr( buffer, "r-xp" ) && strstr( buffer, ".so" ) ){
            address = (uintptr_t)strtoul( buffer, NULL, 16 );
            name    = strrchr( buffer, ' ' ) + 1;
            name.resize( name.size() - 1 );

            modules.push_back( ld_module_t( address, name ) );
        }
    }

    done:

    if(fp){
        fclose(fp);
    }

    return modules;
}

unsigned libhook_addhook( const char *soname, const char *symbol, unsigned newval ) {
    struct soinfo *si = NULL;
    Elf32_Rel *rel = NULL;
    Elf32_Sym *s = NULL;
    unsigned int sym_offset = 0;
    size_t i;

    si = (struct soinfo *)dlopen(soname,0);
    if( !si ){
        HOOKLOG( "dlopen error: %s.", dlerror() );
        return 0;
    }

    s = soinfo_elf_lookup( si, elfhash(symbol), symbol );
    if( !s ){
        return 0;
    }

    sym_offset = s - si->symtab;
    rel = si->plt_rel;

    // loop reloc table to find the symbol by index
    for( i = 0; i < si->plt_rel_count; ++i, ++rel ) {
        unsigned type   = ELF32_R_TYPE(rel->r_info);
        unsigned sym    = ELF32_R_SYM(rel->r_info);
        unsigned reloc  = (unsigned)(rel->r_offset + si->base);
        unsigned original = 0;
        size_t pagesize;
        const void* aligned_pointer;

        if( sym_offset == sym ) {
            switch(type) {
                case R_ARM_JUMP_SLOT:
                    pagesize = sysconf(_SC_PAGESIZE);
                    aligned_pointer = (const void*)(reloc & ~(pagesize - 1));

                    mprotect(aligned_pointer, pagesize, PROT_WRITE | PROT_READ);

                    original = *(unsigned *)reloc;
                    *((unsigned*)reloc) = newval;

                    mprotect(aligned_pointer, pagesize, PROT_READ);

                    return original;

                default:

                    HOOKLOG( "Expected R_ARM_JUMP_SLOT, found 0x%X", type );
                    return 0;
            }
        }
    }

    HOOKLOG( "Unable to find symbol in the reloc table ( plt_rel_count=%u ).", si->plt_rel_count );

    return 0;
}
