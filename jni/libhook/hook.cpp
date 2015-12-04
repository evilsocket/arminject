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

static uint32_t  gnuhash(const char *_name) {
    uint32_t h = 5381;
    const uint8_t* name = reinterpret_cast<const uint8_t*>(_name);
    while (*name != 0) {
        h += (h << 5) + *name++; // h*33 + c = h + h * 32 + c = h + h << 5 + c
    }
    return h;
}

static Elf32_Sym *soinfo_gnu_lookup(struct soinfo *si, uint32_t hash, const char *name) 
{
    uint32_t h2 = hash >> si->gnu_shift2_;

    uint32_t bloom_mask_bits = sizeof(uintptr_t) * 8;
    uint32_t word_num = (hash / bloom_mask_bits) & si->gnu_maskwords_;
    uintptr_t bloom_word = si->gnu_bloom_filter_[word_num];

    if ((1 & (bloom_word >> (hash % bloom_mask_bits)) & (bloom_word >> (h2 % bloom_mask_bits))) == 0) {
        return NULL;
    }

    uint32_t n = si->gnu_bucket_[hash % si->gnu_nbucket_];
    if (n == 0) {
        return NULL;
    }

    do {
        Elf32_Sym* s = si->symtab + n;

        if (((si->gnu_chain_[n] ^ hash) >> 1) == 0 &&
            strcmp(si->strtab + s->st_name, name) == 0) {
            return s;
        }
    } while ((si->gnu_chain_[n++] & 1) == 0);

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
        if( strstr( buffer, "r-xp" ) ){
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

unsigned libhook_patch_address( unsigned addr, unsigned newval ) {
    unsigned original = -1;
    size_t pagesize = sysconf(_SC_PAGESIZE);
    const void *aligned_pointer = (const void*)(addr & ~(pagesize - 1));

    mprotect(aligned_pointer, pagesize, PROT_WRITE | PROT_READ);

    original = *(unsigned *)addr;
    *((unsigned*)addr) = newval;

    mprotect(aligned_pointer, pagesize, PROT_READ);

    return original;
}

unsigned libhook_addhook( const char *soname, const char *symbol, unsigned newval ) {
    struct soinfo *si = NULL;
    Elf32_Rel *rel = NULL;
    Elf32_Sym *s = NULL;
    unsigned int sym_offset = 0;
    size_t i;

    // since we know the module is already loaded and mostly
    // we DO NOT want its constructors to be called again,
    // ise RTLD_NOLOAD to just get its soinfo address.
    si = (struct soinfo *)dlopen( soname, 4 /* RTLD_NOLOAD */ );
    if( !si ){
        HOOKLOG( "dlopen error: %s.", dlerror() );
        return 0;
    }

    if ((si->flags & FLAG_GNU_HASH) != 0)
    {
        s = soinfo_gnu_lookup(si, gnuhash(symbol), symbol);
    }
    else
    {
        s = soinfo_elf_lookup(si, elfhash(symbol), symbol);
    }

    if(!s &&
       // GNU hash table doesn't contain relocation symbols.
       // We still need to search the .rel.plt section for the symbol
       (si->flags & FLAG_GNU_HASH) == 0)
    {
        return 0;
    }

    sym_offset = s - si->symtab;

    // loop reloc table to find the symbol by index
    for( i = 0, rel = si->plt_rel; i < si->plt_rel_count; ++i, ++rel ) {
        unsigned type  = ELF32_R_TYPE(rel->r_info);
        unsigned sym   = ELF32_R_SYM(rel->r_info);
        unsigned reloc = (unsigned)(rel->r_offset + si->load_bias);

        if( sym_offset == sym ||
            strcmp(si->strtab + ((Elf32_Sym*)(si->symtab + sym))->st_name, symbol) == 0) {
            switch(type) {
                case R_ARM_JUMP_SLOT:

                    return libhook_patch_address( reloc, newval );

                default:

                    HOOKLOG( "Expected R_ARM_JUMP_SLOT, found 0x%X", type );
            }
        }
    }

    unsigned original = 0;

    // loop dyn reloc table
    for( i = 0, rel = si->rel; i < si->rel_count; ++i, ++rel ) {
        unsigned type  = ELF32_R_TYPE(rel->r_info);
        unsigned sym   = ELF32_R_SYM(rel->r_info);
        unsigned reloc = (unsigned)(rel->r_offset + si->load_bias);

        if( sym_offset == sym ||
            strcmp(si->strtab + ((Elf32_Sym*)(si->symtab + sym))->st_name, symbol) == 0) {
            switch(type) {
                case R_ARM_ABS32:
                case R_ARM_GLOB_DAT:

                    original = libhook_patch_address( reloc, newval );

                default:

                    HOOKLOG( "Expected R_ARM_ABS32 or R_ARM_GLOB_DAT, found 0x%X", type );
            }
        }
    }

    if( original == 0 ){
        HOOKLOG( "Unable to find symbol in the reloc tables ( plt_rel_count=%u - rel_count=%u ).", si->plt_rel_count, si->rel_count );
    }

    return original;
}
