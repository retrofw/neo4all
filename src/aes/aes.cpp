#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "neo4all.h"
#include "video/video.h"
#include "video/console.h"
#include "menu/menu.h"

#include "aes.h"

extern char aes4all_actual_dir[1024];
extern char neo4all_image_file[1024];

unsigned char aes4all_pen_usage[0x10000];
unsigned *aes4all_prefetch_bufffer=NULL;
void *aes4all_prealloc=NULL;


static void load_pen_usage(FILE *f, unsigned size)
{
	unsigned char *p=(unsigned char *)malloc(4*1024);
	size>>=10;
	unsigned addr=0;
	do{
		bzero(p,4*1024);
		fread(p,1,4*1024,f);
		unsigned i;
		for(i=0;i<4*1024;i++,addr++)
			if (p[i])
				aes4all_pen_usage[addr>>3]|=(1<<(addr&0x7));
			else
				aes4all_pen_usage[addr>>3]&=0xFF-(1<<(addr&0x7));
	}while(size--);
	free(p);
}

unsigned char aes4all_ram[0x10000];
unsigned char aes4all_nvram[0x10000];

void *aes4all_memory;
void *aes4all_memory_cpu;
void *aes4all_memory_bios;
void *aes4all_memory_sfix_game;
void *aes4all_memory_sfix_board;
void *aes4all_memory_sm1;
void *aes4all_memory_sound1;
void *aes4all_memory_sound2;
void *aes4all_memory_gfx;
void *aes4all_memory_ng_lo;
void *aes4all_memory_fix_board_usage;
void *aes4all_memory_fix_game_usage;
void *aes4all_memory_pen_usage=NULL;

unsigned aes4all_bankaddress = 0;

int aes4all_prefetch_all(void)
{
	FILE *f;
	int nc=strlen(aes4all_actual_dir);
	if (!aes4all_filename)
		return 1;
#ifndef AUTO_EVENTS
	if (!nc)
		return 1;
#endif
	f=fopen(aes4all_filename,"rb");
	if (!f)
	{
		aes4all_actual_dir[nc]=0;
		aes4all_filename=(char *)&neo4all_image_file[0];
		return 2;
	}

	console_puts("Prefetching ROM...");
	neogeo_emulating=0;
	text_draw_loading(1,12);
#ifndef USE_MMAP
	fseek(f,aes4all_memory_cpu_init,SEEK_SET);
	fread(aes4all_memory_cpu,1,aes4all_memory_cpu_size,f);
#else
#ifdef DEBUG_MMAP_PREFETCH
	puts("CPU");fflush(stdout);
#endif
	aes4all_mmap_prefetch(aes4all_memory_cpu_init,aes4all_memory_cpu_size,f);
#endif

	text_draw_loading(2,12);
#ifndef USE_MMAP
	fseek(f,aes4all_memory_bios_init,SEEK_SET);
	fread(aes4all_memory_bios,1,aes4all_memory_bios_size,f);
#else
#ifdef DEBUG_MMAP_PREFETCH
	puts("BIOS");fflush(stdout);
#endif
	aes4all_mmap_prefetch(aes4all_memory_bios_init,aes4all_memory_bios_size,f);
#endif

	text_draw_loading(3,12);
#ifndef USE_MMAP
	fseek(f,aes4all_memory_sfix_game_init,SEEK_SET);
	fread(aes4all_memory_sfix_game,1,aes4all_memory_sfix_game_size,f);
#else
#ifdef DEBUG_MMAP_PREFETCH
	puts("SFIX_GAME");fflush(stdout);
#endif
	aes4all_mmap_prefetch(aes4all_memory_sfix_game_init,aes4all_memory_sfix_game_size,f);
#endif

	text_draw_loading(4,12);
#ifndef USE_MMAP
	fseek(f,aes4all_memory_sfix_board_init,SEEK_SET);
	fread(aes4all_memory_sfix_board,1,aes4all_memory_sfix_board_size,f);
#else
#ifdef DEBUG_MMAP_PREFETCH
	puts("SFIX_BOARD");fflush(stdout);
#endif
	aes4all_mmap_prefetch(aes4all_memory_sfix_board_init,aes4all_memory_sfix_board_size,f);
#endif

	text_draw_loading(5,12);
#ifndef USE_MMAP
	fseek(f,aes4all_memory_sm1_init,SEEK_SET);
	fread(aes4all_memory_sm1,1,aes4all_memory_sm1_size,f);
#else
#ifdef DEBUG_MMAP_PREFETCH
	puts("SM1");fflush(stdout);
#endif
	aes4all_mmap_prefetch(aes4all_memory_sm1_init,aes4all_memory_sm1_size,f);
#endif

	text_draw_loading(6,12);
#ifndef USE_MMAP
	fseek(f,aes4all_memory_sound1_init,SEEK_SET);
	fread(aes4all_memory_sound1,1,aes4all_memory_sound1_size,f);
#else
#ifdef DEBUG_MMAP_PREFETCH
	puts("SOUND1");fflush(stdout);
#endif
	aes4all_mmap_prefetch(aes4all_memory_sound1_init,aes4all_memory_sound1_size,f);
#endif

	text_draw_loading(7,12);
	if (aes4all_memory_sound2_size)
	{
#ifndef USE_MMAP
		fseek(f,aes4all_memory_sound2_init,SEEK_SET);
		fread(aes4all_memory_sound2,1,aes4all_memory_sound2_size,f);
#else
#ifdef DEBUG_MMAP_PREFETCH
	puts("SOUND2");fflush(stdout);
#endif
		aes4all_mmap_prefetch(aes4all_memory_sound2_init,aes4all_memory_sound2_size,f);
#endif
	}

	text_draw_loading(8,12);
#ifndef USE_MMAP
	fseek(f,aes4all_memory_gfx_init,SEEK_SET);
	fread(aes4all_memory_gfx,1,aes4all_memory_gfx_size,f);
#else
#ifdef DEBUG_MMAP_PREFETCH
	puts("GFX");fflush(stdout);
	aes4all_mmap_prefetch(aes4all_memory_gfx_init,aes4all_memory_gfx_size,f);
#else
	aes4all_mmap_prefetch(aes4all_memory_gfx_init,aes4all_memory_gfx_size<0x1000000?aes4all_memory_gfx_size:1000000,f);
#endif
#endif

	text_draw_loading(9,12);
#ifndef USE_MMAP
	fseek(f,aes4all_memory_ng_lo_init,SEEK_SET);
	fread(aes4all_memory_ng_lo,1,aes4all_memory_ng_lo_size,f);
#else
#ifdef DEBUG_MMAP_PREFETCH
	puts("NG_LO");fflush(stdout);
#endif
	aes4all_mmap_prefetch(aes4all_memory_ng_lo_init,aes4all_memory_ng_lo_size,f);
#endif

	text_draw_loading(10,12);
#ifndef USE_MMAP
	fseek(f,aes4all_memory_fix_board_usage_init,SEEK_SET);
	fread(aes4all_memory_fix_board_usage,1,aes4all_memory_fix_board_usage_size,f);
#else
#ifdef DEBUG_MMAP_PREFETCH
	puts("FIX_BOARD");fflush(stdout);
#endif
	aes4all_mmap_prefetch(aes4all_memory_fix_board_usage_init,aes4all_memory_fix_board_usage_size,f);
#endif

	text_draw_loading(11,12);
#ifndef USE_MMAP
	fseek(f,aes4all_memory_fix_game_usage_init,SEEK_SET);
	fread(aes4all_memory_fix_game_usage,1,aes4all_memory_fix_game_usage_size,f);
#else
#ifdef DEBUG_MMAP_PREFETCH
	puts("FIX_GAME");fflush(stdout);
#endif
	aes4all_mmap_prefetch(aes4all_memory_fix_game_usage_init,aes4all_memory_fix_game_usage_size,f);
#endif

#ifdef DEBUG_MMAP_PREFETCH
	puts("PEN_USAGE");fflush(stdout);
#endif
	text_draw_loading(12,12);
	fseek(f,aes4all_memory_pen_usage_init,SEEK_SET);
	load_pen_usage(f,aes4all_memory_pen_usage_size&(AES4ALL_PEN_USAGE_SIZE-1));

	fclose(f);

	video_fix_usage=(unsigned char*)aes4all_memory_fix_game_usage;
	neogeo_fix_memory=(char*)aes4all_memory_sfix_game;

#ifndef USE_RAZE
	memcpy(mame_z80mem, aes4all_memory_sm1, 0xf800);
#endif
	sound_emulate_start();

	aes4all_actual_dir[nc]=0;
	aes4all_filename=(char *)&neo4all_image_file[0];
	return 0;
}

