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
#ifndef LINKER_H_
#define LINKER_H_

#include <elf.h>

#define ANDROID_ARM_LINKER 1

#define SOINFO_NAME_LEN 128
#define FLAG_GNU_HASH 0x00000040

typedef Elf32_Half Elf32_Versym;

struct link_map
{
    uintptr_t l_addr;
    char * l_name;
    uintptr_t l_ld;
    struct link_map * l_next;
    struct link_map * l_prev;
};

struct soinfo_list_t
{
    void * head;
    void * tail;
};

struct soinfo
{
    const char name[SOINFO_NAME_LEN];
    Elf32_Phdr *phdr;
    int phnum;
    unsigned entry;
    unsigned base;
    unsigned size;
    int unused;  // DO NOT USE, maintained for compatibility.
    unsigned *dynamic;
    unsigned wrprotect_start;
    unsigned wrprotect_end;
    struct soinfo *next;
    unsigned flags;
    const char *strtab;
    Elf32_Sym *symtab;
    unsigned nbucket;
    unsigned nchain;
    unsigned *bucket;
    unsigned *chain;
    unsigned *plt_got;
    Elf32_Rel *plt_rel;
    unsigned plt_rel_count;
    Elf32_Rel *rel;
    unsigned rel_count;
    unsigned *preinit_array;
    unsigned preinit_array_count;
    unsigned *init_array;
    unsigned init_array_count;
    unsigned *fini_array;
    unsigned fini_array_count;
    void (*init_func)(void);
    void (*fini_func)(void);
#ifdef ANDROID_ARM_LINKER
    /* ARM EABI section used for stack unwinding. */
    unsigned *ARM_exidx;
    unsigned ARM_exidx_count;
#endif
    unsigned refcount;
    struct link_map linkmap;
    int constructors_called;

    unsigned *load_bias;

#if !defined(__LP64__)
  bool has_text_relocations;
#endif
  bool has_DT_SYMBOLIC;

  // version >= 0
  dev_t st_dev_;
  ino_t st_ino_;

  soinfo_list_t children_;
  soinfo_list_t parents_;

  // version >= 1
  off64_t file_offset_;
  uint32_t rtld_flags_;
  uint32_t dt_flags_1_;
  size_t strtab_size_;

  // version >= 2
  size_t gnu_nbucket_;
  uint32_t *gnu_bucket_;
  uint32_t *gnu_chain_;
  uint32_t gnu_maskwords_;
  uint32_t gnu_shift2_;
  unsigned *gnu_bloom_filter_;

  soinfo* local_group_root_;

  uint8_t* android_relocs_;
  size_t android_relocs_size_;

  const char* soname_;
  std::string realpath_;

  const Elf32_Versym* versym_;

  unsigned *verdef_ptr_;
  size_t verdef_cnt_;

  unsigned *verneed_ptr_;
  size_t verneed_cnt_;

  uint32_t target_sdk_version_;

  std::vector<std::string> dt_runpath_;
};

#define R_ARM_ABS32      2
#define R_ARM_COPY       20
#define R_ARM_GLOB_DAT   21
#define R_ARM_JUMP_SLOT  22
#define R_ARM_RELATIVE   23

#endif
