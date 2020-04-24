#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "neoaes.h"

unsigned aes4all_memory_total;
unsigned aes4all_memory_nb_of_tiles;

unsigned aes4all_memory_cpu_init;
unsigned aes4all_memory_cpu_size;

unsigned aes4all_memory_bios_init;
unsigned aes4all_memory_bios_size;

unsigned aes4all_memory_sfix_game_init;
unsigned aes4all_memory_sfix_game_size;

unsigned aes4all_memory_sfix_board_init;
unsigned aes4all_memory_sfix_board_size;

unsigned aes4all_memory_sm1_init;
unsigned aes4all_memory_sm1_size;

unsigned aes4all_memory_sound1_init;
unsigned aes4all_memory_sound1_size;

unsigned aes4all_memory_sound2_init;
unsigned aes4all_memory_sound2_size;

unsigned aes4all_memory_gfx_init;
unsigned aes4all_memory_gfx_size;

unsigned aes4all_memory_ng_lo_init;
unsigned aes4all_memory_ng_lo_size;

unsigned aes4all_memory_fix_board_usage_init;
unsigned aes4all_memory_fix_board_usage_size;

unsigned aes4all_memory_fix_game_usage_init;
unsigned aes4all_memory_fix_game_usage_size;

unsigned aes4all_memory_pen_usage_init;
unsigned aes4all_memory_pen_usage_size;


unsigned char aes4all_rom_type;

unsigned char aes4all_special_bios;

unsigned char aes4all_extra_xor;

unsigned char aes4all_banksw_type;

unsigned char aes4all_system;

unsigned char aes4all_country;

unsigned aes4all_bksw_handler;

unsigned char aes4all_bksw_unscramble[6];

unsigned char aes4all_bksw_offset[64];


unsigned aes4all_nvram_protection_hack;

#define MEMBUF_K 64

static int check_chksum(FILE *f,unsigned size, unsigned chk)
{
	unsigned char *buf=(unsigned char *)calloc(MEMBUF_K,1024);
	unsigned actual=0;
	unsigned i;
	while(size)
	{
		unsigned s=size;
		if (s>(MEMBUF_K*1024))
			s=(MEMBUF_K*1024);
		fread((void *)buf,1,s,f);
		for(i=0;i<s;i++)
			actual+=buf[i];
		size-=s;
	}
	free(buf);
	return (actual!=chk);
}

static int load_part(FILE *f, unsigned *size, unsigned *init)
{
	unsigned chk=0;
	int ret=0;

	long pos=ftell(f);

	fread((void *)init,1,4,f);
	fread((void *)size,1,4,f);
	fread((void *)&chk,1,4,f);
	
	if (size)
	{
		fseek(f,*init,SEEK_SET);
		ret=check_chksum(f,*size,chk);
	}

	fseek(f,pos+(4*3),SEEK_SET);
	return ret;
}


int load_aes(char *filename)
{
	char *head;
	int ret=0;
	FILE *f=fopen(filename,"rb");
	if (!f)
		return -1;

	head=(char *)calloc(16,1);
	fread((void *)head,1,6,f); 

	if (strcmp("NEOAES",head))
	{
		free(head);
		fclose(f);
		return -2;
	}
	free(head);

	fread((void *)&aes4all_memory_total,1,4,f);
	fread((void *)&aes4all_memory_nb_of_tiles,1,4,f);
	fread((void *)&aes4all_rom_type,1,1,f);
	fread((void *)&aes4all_special_bios,1,1,f);
	fread((void *)&aes4all_extra_xor,1,1,f);
	fread((void *)&aes4all_banksw_type,1,1,f);
	fread((void *)&aes4all_system,1,1,f);
	fread((void *)&aes4all_country,1,1,f);
	fread((void *)&aes4all_nvram_protection_hack,1,4,f);
	fread((void *)&aes4all_bksw_handler,1,4,f);
	fread((void *)&aes4all_bksw_unscramble[0],1,6,f);
	fread((void *)&aes4all_bksw_offset[0],1,64,f);

	if (load_part(f,(unsigned *)&aes4all_memory_cpu_size,(unsigned *)&aes4all_memory_cpu_init))
		ret=-3;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_bios_size,(unsigned *)&aes4all_memory_bios_init))
		ret=-4;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_sfix_game_size,(unsigned *)&aes4all_memory_sfix_game_init))
		ret=-5;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_sfix_board_size,(unsigned *)&aes4all_memory_sfix_board_init))
		ret=-6;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_sm1_size,(unsigned *)&aes4all_memory_sm1_init))
		ret=-7;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_sound1_size,(unsigned *)&aes4all_memory_sound1_init))
		ret=-8;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_sound2_size,(unsigned *)&aes4all_memory_sound2_init))
		ret=-9;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_gfx_size,(unsigned *)&aes4all_memory_gfx_init))
		ret=-10;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_ng_lo_size,(unsigned *)&aes4all_memory_ng_lo_init))
		ret=-11;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_fix_board_usage_size,(unsigned *)&aes4all_memory_fix_board_usage_init))
		ret=-12;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_fix_game_usage_size,(unsigned *)&aes4all_memory_fix_game_usage_init))
		ret=-13;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_pen_usage_size,(unsigned *)&aes4all_memory_pen_usage_init))
		ret=-14;


	fclose(f);
	return ret;
}
