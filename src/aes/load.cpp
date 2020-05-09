#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(USE_MMAP)
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
void *aes4all_mmap;
#endif


#include "neo4all.h"
#include "video/video.h"
#include "video/console.h"
#include "aes.h"


extern char aes4all_actual_dir[128];
char neo4all_image_file[64];
char *aes4all_filename=(char *)&neo4all_image_file[0];


unsigned char aes4all_nvram_lock;

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

unsigned aes4all_nvram_protection_hack;

unsigned aes4all_bksw_handler;

unsigned char aes4all_bksw_unscramble[6];

unsigned char aes4all_bksw_offset[64];

unsigned aes4all_prefetch_init;


#define MEMBUF_K 64
#define MEMBUF (MEMBUF_K*1024)
/*
static int check_chksum(FILE *f,unsigned size, unsigned chk)
{
	unsigned char *buf=(unsigned char *)calloc(MEMBUF_K,1024);
	unsigned actual=0;
	unsigned i;
	while(size)
	{
		unsigned s=size;
		if (s>MEMBUF)
			s=MEMBUF;
		fread((void *)buf,1,s,f);
		for(i=0;i<s;i++)
			actual+=buf[i];
		size-=s;
	}
	free(buf);
	return (actual!=chk);
	return 0;
}
*/

static int load_part(FILE *f, unsigned *size, unsigned *init)
{
	unsigned chk=0;
	int ret=0;

//	long pos=ftell(f);

	fread((void *)init,1,4,f);
	fread((void *)size,1,4,f);
	fread((void *)&chk,1,4,f);
/*
	if (size)
	{
		fseek(f,*init,SEEK_SET);
		ret=check_chksum(f,*size,chk);
	}
	fseek(f,pos+(4*3),SEEK_SET);
*/
	return ret;
}


static int load_aes(char *filename)
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
		return -13;
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
		ret=-2;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_bios_size,(unsigned *)&aes4all_memory_bios_init))
		ret=-3;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_sfix_game_size,(unsigned *)&aes4all_memory_sfix_game_init))
		ret=-4;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_sfix_board_size,(unsigned *)&aes4all_memory_sfix_board_init))
		ret=-5;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_sm1_size,(unsigned *)&aes4all_memory_sm1_init))
		ret=-12;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_sound1_size,(unsigned *)&aes4all_memory_sound1_init))
		ret=-6;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_sound2_size,(unsigned *)&aes4all_memory_sound2_init))
		ret=-7;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_gfx_size,(unsigned *)&aes4all_memory_gfx_init))
		ret=-8;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_ng_lo_size,(unsigned *)&aes4all_memory_ng_lo_init))
		ret=-9;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_fix_board_usage_size,(unsigned *)&aes4all_memory_fix_board_usage_init))
		ret=-10;
	else
	if (load_part(f,(unsigned *)&aes4all_memory_fix_game_usage_size,(unsigned *)&aes4all_memory_fix_game_usage_init))
		ret=-11;
	if (load_part(f,(unsigned *)&aes4all_memory_pen_usage_size,(unsigned *)&aes4all_memory_pen_usage_init))
		ret=-13;

	aes4all_prefetch_init=(unsigned)ftell(f);

	fclose(f);
	return ret;
}

#if defined(USE_MMAP)
int aes4all_mmap_prefetch(unsigned mem_init, unsigned mem_size, FILE *f)
{
	int i,ret=0;
	int *m=(int *)(((unsigned)aes4all_mmap)+mem_init-aes4all_memory_cpu_init);
	fseek(f,mem_init,SEEK_SET);
	for(i=0;i<(mem_size/4);i+=256) {
#ifdef DEBUG_MMAP_PREFETCH
		int t;
		fread(&t,1,4,f);
		if (t!=m[i]) {
			printf("ERROR MMAP_FETCH: init=%p, i=%i (%i!=%i)\n",mem_init,i,t,m[i]);
			exit(0);
		}
		fseek(f,255*4,SEEK_CUR);
#endif
		ret+=m[i];
	}
//printf("aes4all_mmap_prefetch(%i,%i)=%i\n",mem_init,mem_size,ret);fflush(stdout);
	return ret;
}
#endif

int aes4all_load(void)
{
#ifdef AUTO_EVENTS
	aes4all_filename=(char *)&neo4all_image_file[0];
	strcpy(aes4all_filename,"aof3.aes");
	int nc=0;
#else
	if (!aes4all_filename)
		return 0;
	int nc=strlen(aes4all_actual_dir);
	console_printf("Preloading '%s'...\n",aes4all_filename);
	if (!nc)
		return 0;
#endif
	if (load_aes(aes4all_filename))
	{
		aes4all_actual_dir[nc]=0;
		aes4all_filename=(char *)&neo4all_image_file[0];
		return 0;
	}
	neogeo_emulating=0;
	sound_emulate_stop();

	console_puts("Preloading OK.");
	aes4all_nvram_lock=0;
	aes4all_bankaddress=0;
	input_init_aes_system(aes4all_system);
	aes4all_filename=(char *)&neo4all_image_file[0];
	aes4all_actual_dir[nc]=0;
#if defined(USE_MMAP)
	static int aes4all_mmap_fd=0;
	static unsigned aes4all_mmap_size=0;
	if (aes4all_mmap_fd) {
		munmap(aes4all_mmap,aes4all_mmap_size);
		close(aes4all_mmap_fd);
	}
	aes4all_mmap_fd=open(aes4all_filename,O_RDONLY|O_NONBLOCK);
	aes4all_mmap_size=aes4all_memory_total;
	aes4all_mmap=mmap(NULL,aes4all_mmap_size,PROT_READ,MAP_PRIVATE|MAP_NONBLOCK,aes4all_mmap_fd,aes4all_memory_cpu_init);
//printf("aes4all_mmap_fd=%i, size=%i, offset=%i, mmap=%p\n",aes4all_mmap_fd,aes4all_mmap_size,aes4all_memory_cpu_init,aes4all_mmap);
#endif
	return 1;
}


static char is_aes_file_buf[1024+10+2];

int is_aes_file(char *dir, char *filename, char *realname)
{
	int ret=0;
	char *name=(char*) &is_aes_file_buf[0];
	char *schar=(char *) &is_aes_file_buf[1024+1];
	bzero(name,1024);
// #ifdef DREAMCAST
#if 0
	strcpy(name,aes4all_actual_dir);
        strcat(name,"/");
	strcat(name,dir);
#else
	strcpy(name,dir);
#endif
        strcat(name,"/");
	strcat(name,filename);

	FILE *f=fopen(name,"rb");
	if (f)
	{
		fread((void *)schar,1,10,f);
		if (!(strncmp(schar,"NEOAES\0\0\0\0",6)))
		{
			fseek(f,0,SEEK_END);
			long len=ftell(f);
			if (len>(256*1024))
			{
				fseek(f,(256*1024)-32,SEEK_SET);
				fread((void *)realname,1,29,f);
				realname[29]=0;
				ret=-1;
			}
		}
		fclose(f);
	}
	return ret;
}
