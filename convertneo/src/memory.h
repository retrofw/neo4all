/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <SDL.h>
#include <SDL_endian.h>
#include "driver.h"

#define READ_WORD(a)          (*(Uint16 *)(a))
#define WRITE_WORD(a,d)       (*(Uint16 *)(a) = (d))
#define READ_BYTE(a)          (*(Uint8 *)(a))
#define WRITE_BYTE(a,d)       (*(Uint8 *)(a) = (d))
#define SWAP_BYTE_ADDRESS(a)  ((Uintptr)(a)^1)
#define SWAP16(y) SDL_Swap16(y)
#define SWAP32(y) SDL_Swap32(y)
		    
#ifdef USE_GENERATOR68K
/* Programs are stored as BIGENDIAN */
#  ifdef WORDS_BIGENDIAN
#    define WRITE_WORD_ROM WRITE_WORD
#    define READ_WORD_ROM READ_WORD
#    define WRITE_BYTE_ROM WRITE_BYTE
#    define READ_BYTE_ROM READ_BYTE
#  else /* WORDS_BIGENDIAN */
#    define WRITE_WORD_ROM(a,d) (WRITE_WORD(a,SWAP16(d)))
#    define READ_WORD_ROM(a) (SWAP16(READ_WORD(a)))
#    define WRITE_BYTE_ROM WRITE_BYTE
#    define READ_BYTE_ROM READ_BYTE
#  endif
#else /* USE_GENERATOR68K */
/* Programs are stored as LITTLEENDIAN */
#  define WRITE_WORD_ROM WRITE_WORD
#  define READ_WORD_ROM READ_WORD
#  define WRITE_BYTE_ROM(a,d) WRITE_BYTE(SWAP_BYTE_ADDRESS(a),(d))
#  define READ_BYTE_ROM(a) READ_BYTE(SWAP_BYTE_ADDRESS(a))
#endif

typedef struct neo_mem {
    Uint8 *cpu;
    Uint32 cpu_size;
    Uint8 *ram;
    Uint8 *bios;
    Uint32 bios_size;
    Uint8 *ng_lo;
    Uint8 *sm1;
    Uint32 sm1_size;
    Uint8 *sfix_board;
    Uint8 *sfix_game;
    Uint32 sfix_size;
    Uint8 *sound1;
    Uint32 sound1_size;
    Uint8 *sound2;
    Uint32 sound2_size;
    Uint8 *gfx;
    Uint32 gfx_size;
    Uint32 nb_of_tiles;
    //tile **tile_in_cache;
    Uint8 video[0x20000];
    Uint8 *pal1, *pal2;
    Uint8 *pal_pc1, *pal_pc2;
    Uint8 sram[0x10000];
    Uint32 *pen_usage;
    Uint8 fix_board_usage[4096];
    Uint8 *fix_game_usage;
    Uint8 z80_ram[0x800];
    Uint8 game_vector[0x80];
    /* internal representation of key */
    Uint8 intern_p1, intern_p2, intern_coin, intern_start;
    /* crypted rom bankswitch system */
    Uint32 bksw_handler;
    Uint8 bksw_unscramble[6];
    Uint8 bksw_offset[64];
    Uint8 kof2003_bksw[0x2000];
} neo_mem;

extern neo_mem memory;

/* video related */
extern int irq2enable, irq2start, irq2repeat, irq2control;
extern int lastirq2line;
extern int irq2repeat_limit;
extern int palno, vptr, high_tile, vhigh_tile, vvhigh_tile;
extern Sint16 modulo;
extern Uint8 *current_pal;
extern Uint8 *current_pal_dirty;
extern Uint16 *current_pc_pal;
extern Uint8 *current_fix;
extern Uint8 *fix_usage;

/* memory card */
extern Uint8 neo_memcard[0x1000];;

/* sram */
extern Uint8 sram_lock;
extern Uint32 sram_protection_hack;

/* Sound control */
extern Uint8 sound_code;
extern Uint8 pending_command;
extern Uint8 result_code;


/* 68k cpu Banking control */
extern Uint32 bankaddress;		/* current bank */
extern Uint8 current_cpu_bank;
extern Uint16 z80_bank[4];

/* misc utility func */
void update_all_pal(void);
void dump_hardware_reg(void);

#define LONG_FETCH(fetchname) Uint32 fetchname ## _long(Uint32 addr) { \
      return (fetchname ## _word(addr) << 16) |	fetchname ## _word(addr+2); \
}

#define LONG_STORE(storename) void storename ## _long(Uint32 addr, Uint32 data) { \
      storename ## _word(addr,data>>16); \
      storename ## _word(addr+2,data & 0xffff); \
}

#endif
