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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <unistd.h>
#include "fileio.h"
#include "memory.h"
#include "driver.h"
#include "conf.h"


#include "neoaes.h"

#ifdef WIN32
#include <windows.h>
#endif

#define PAD (256*1024)
static unsigned total_index=PAD;
static unsigned actual_index=0;
static void *null_all;

#define PREFETCH_POS 242
#define PREFETCH_SIZE 32768
static unsigned prefetch[PREFETCH_SIZE];

char *conf_game=NULL;
unsigned char conf_rom_type;
unsigned char conf_special_bios;
unsigned char conf_extra_xor;
unsigned char conf_banksw_type;
neo_mem memory;
Uint8 *fix_usage=NULL;
Uint8 *current_pal=NULL;
Uint16 *current_pc_pal=NULL;
Uint8 *current_fix=NULL;

unsigned char conf_country=CTY_USA;
unsigned char conf_system=SYS_HOME; //SYS_ARCADE;

static unsigned get_chksum(void *membuf,unsigned size)
{
	unsigned char *buf=(unsigned char *)membuf;
	unsigned ret=0;
	unsigned i;
	if (membuf!=NULL)
		for(i=0;i<size;i++)
			ret+=*buf++;
	return ret;
}

static char *get_noexist_filename(char *filename)
{
	int cuantos=1;
	int final=strlen(filename)-4;
	FILE *f=fopen(filename,"rb");
	while(f)
	{
		fclose(f);
		sprintf(filename+final,"_%i.aes",cuantos);
		f=fopen(filename,"rb");
		cuantos++;
	}
	return filename;
}

static char *get_new_filename_2(char *filename)
{
	char *str=(char *)calloc(strlen(filename)+16,1);
	int i;
	strcpy(str,filename);
	for(i=strlen(str)-1;i>=0;i--)
		if (str[i]=='.')
		{
			str[i+1]='a';
			str[i+2]='e';
			str[i+3]='s';
			str[i+4]=0;
			i+=4;
			break;
		}
	if (i<3)
	{
		str[strlen(str)]='.';
		str[strlen(str)]='a';
		str[strlen(str)]='e';
		str[strlen(str)]='s';
		str[strlen(str)]=0;
	}

	printf("\nSave as '%s' ...\n",str);
	return get_noexist_filename(str);
}

static char *get_new_filename(char *filename)
{
	if (!conf_game)
		return get_new_filename_2(filename);
	if (!strlen(conf_game))
		return get_new_filename_2(filename);
	char *str=(char *)calloc(strlen(filename)+16+strlen(conf_game),1);
	int i;
	strcpy(str,filename);
	for(i=strlen(str)-1;i>0;i--)
		if ((str[i]=='/')||(str[i]=='\\'))
			break;
	if (!i)
	{
		free(str);
		return get_new_filename_2(filename);
	}
	str[i+1]=0;
	strcat(str,conf_game);
	strcat(str,".aes");
	return get_noexist_filename(str);
}

static char *get_new_filename_prefetch_2(char *filename)
{
	char *str=(char *)calloc(strlen(filename)+16,1);
	char *real=(char *)calloc(strlen(filename)+128,1);
	int i;
	strcpy(str,filename);
	for(i=strlen(str)-1;i>=0;i--)
		if (str[i]=='.')
		{
			str[i+1]='p';
			str[i+2]='r';
			str[i+3]='e';
			str[i+4]=0;
			i+=4;
			break;
		}
	if (i<3)
	{
		str[strlen(str)]='.';
		str[strlen(str)]='p';
		str[strlen(str)]='r';
		str[strlen(str)]='e';
		str[strlen(str)]=0;
	}
	strcpy(real,"prefetch/");
	for(i=strlen(str)-1;i>=0;i--)
		if ((str[i]=='/')||(str[i]=='\\'))
			break;
	strcat(real,(char *)&str[i+1]);
	free(str);

//	printf("\nTrying prefetch '%s' ...\n",real);
	return real;
}

static char *get_new_filename_prefetch(char *filename)
{
	if (!conf_game)
		return get_new_filename_prefetch_2(filename);
	if (!strlen(conf_game))
		return get_new_filename_prefetch_2(filename);
	char *str=(char *)calloc(128+strlen(conf_game),1);
	strcpy(str,"prefetch/");
	strcat(str,conf_game);
	strcat(str,".pre");
//	printf("\nTrying prefetch '%s' ...\n",str);
	return str;
}


static int save_part(FILE *f, char *name, void *buf, unsigned size)
{
	if ((!buf)||(!size))
	{
		printf("\n NO %s\n",name);
		fwrite(null_all,1,3*4,f);
	}
	else
	{
		unsigned chksum=get_chksum(buf,size);
		printf("\n %s = %i KBytes, chksum=0x%X\n",name,size/1024,chksum);
		fwrite((void *)&total_index,1,4,f);
		fwrite((void *)&size,1,4,f);
		fwrite((void *)&chksum,1,4,f);
		total_index+=size;
	}
	actual_index+=(3*4);
	return 0;
}

static int fw(void *buf,int size,FILE *f)
{
	if ((size>0)&&(buf))
	{
		actual_index+=size;
		return fwrite(buf,1,size,f);
	}
	return 0;
}

int load_prefetch(char *filename)
{
	int ret=1;
	unsigned last=0;
	FILE *f;
	char *str=get_new_filename_prefetch(filename);
	memset(prefetch,0,PREFETCH_SIZE*sizeof(unsigned));
	f=fopen(str,"rb");
	if (!f)
		printf("WITHOUT PREFETCH!!!!\n");
	else
	{
		last=fread((void *)prefetch,sizeof(unsigned),PREFETCH_SIZE,f);
		fclose(f);
		ret=0;
	}
	free(str);
	prefetch[last]=0xFFFFFFFF;
	return ret;
}

int add_long_name(char *filename, char *longname)
{
	int ret=0;
	FILE *f=fopen(filename,"rb");
	if (f)
	{
		void *buf=malloc(1024*1024*128);
		long len=fread(buf,1,1024*1024*128,f);
		if (len>PAD)
		{
			char *b=buf;
			if (b[0]=='N' && b[1]=='E' && b[2]=='O' && b[3]=='A' && b[4]=='E' && b[5]=='S' && b[6]==0 )
			{
				fclose(f);
				b=&b[PAD-32];
				strncpy(b,longname,31);
				b[31]=0;
				f=fopen(filename,"wb");
				if (f)
				{
					fwrite(buf,1,len,f);
					ret=-1;
				}
			}
			
		}
		free(buf);
		if (f)
			fclose(f);
	}
	return ret;
}

int add_direct_prefetch(char *filename)
{
	unsigned i;
	char back_c3=filename[strlen(filename)-3];
	char back_c2=filename[strlen(filename)-2];
	char back_c1=filename[strlen(filename)-1];
	FILE *f=fopen(filename,"rb");
	if (f)
	{
		char *b=(char *)&prefetch[0];
		fread((void *)b,1,6,f);
		if (b[0]!='N' || b[1]!='E' || b[2]!='O' || b[3]!='A' || b[4]!='E' || b[5]!='S' || b[6]!=0 )
		{
			fclose(f);
			f=NULL;
		}
	}
	if (f)
	{
		int j=0;
		void *buf=malloc(PREFETCH_SIZE*4096);
		fseek(f,PREFETCH_POS,SEEK_SET);
		fread((void*)&prefetch[0],sizeof(unsigned),PREFETCH_SIZE,f);
		for(i=0;(prefetch[i]!=0xFFFFFFFF)&&(i<PREFETCH_SIZE);i++)
		{
			if (prefetch[i])
			{
				fseek(f,(i*4096)+PAD,SEEK_SET);
				fread((void*)(((unsigned)buf)+(4096*j)),1,4096,f);
				j++;
			}
		}
		fclose(f);

		if (j>1)
		{
			i--;
			filename[strlen(filename)-3]='p';
			filename[strlen(filename)-2]='r';
			filename[strlen(filename)-1]='f';
			f=fopen(filename,"wb");
			if (f)
			{
				printf("\nWriting fast prefetch file '%s'\n",filename);
				fwrite(buf,4096,j,f);
				fclose(f);
			}
		}
		else
			f=NULL;
		free(buf);
	}
	filename[strlen(filename)-3]=back_c3;
	filename[strlen(filename)-2]=back_c2;
	filename[strlen(filename)-1]=back_c1;
	return f==NULL;
}


extern char longname[];

int create_aes_file(char *filename)
{
	int res=0;
	char *str=get_new_filename(filename);
	char *head="NEOAES\0\0\0\0\0\0\0\0";
	FILE *f=fopen(str,"wb");
	if (!f)
	{
#ifdef WIN32
	MessageBox (0, "Unable to open AES file for write", "Convert", MB_OK | MB_ICONERROR);
#else
		printf("Unable to open '%s' file for write\n",str);
#endif	
		return -1;
	}

	load_prefetch(filename);

	unsigned total_bytes=memory.cpu_size+memory.bios_size+memory.sfix_size+0x20000+memory.sm1_size+memory.sound1_size+memory.sound2_size+memory.gfx_size+0x10000+4096+(memory.sfix_size>>5)+memory.nb_of_tiles;

	actual_index=0;
	total_index=PAD;

	fwrite((void *)head,1,6,f);  actual_index+=6;
	fwrite((void *)&total_bytes,1,4,f); actual_index+=4;
	fwrite((void *)&memory.nb_of_tiles,1,4,f); actual_index+=4;
	fwrite((void *)&conf_rom_type,1,1,f); actual_index++;
	fwrite((void *)&conf_special_bios,1,1,f); actual_index++;
	fwrite((void *)&conf_extra_xor,1,1,f); actual_index++;
	fwrite((void *)&conf_banksw_type,1,1,f); actual_index++;
	fwrite((void *)&conf_system,1,1,f); actual_index++;
	fwrite((void *)&conf_country,1,1,f); actual_index++;
	fwrite((void *)&sram_protection_hack,1,4,f); actual_index+=4;
	fwrite((void *)&memory.bksw_handler,1,4,f); actual_index+=4;
	fwrite((void *)&memory.bksw_unscramble[0],1,6,f); actual_index+=6;
	fwrite((void *)&memory.bksw_offset[0],1,64,f); actual_index+=64;

	save_part(f,"CPU",(void *)memory.cpu,memory.cpu_size);
	save_part(f,"BIOS",(void *)memory.bios,memory.bios_size);
	save_part(f,"SFIXG",(void *)memory.sfix_game,memory.sfix_size);
	save_part(f,"SFIXB",(void *)memory.sfix_board,0x20000);
	save_part(f,"SM1",(void *)memory.sm1,memory.sm1_size);
	save_part(f,"SOUND1",(void *)memory.sound1,memory.sound1_size);
	save_part(f,"SOUND2",(void *)memory.sound2,memory.sound2_size);
	save_part(f,"GFX",(void *)memory.gfx,memory.gfx_size);
	save_part(f,"NG_LO",(void *)memory.ng_lo,0x10000);
	save_part(f,"FUSAB",(void *)&memory.fix_board_usage[0],4096);
	save_part(f,"FUSAG",(void *)memory.fix_game_usage,memory.sfix_size>>5);
	save_part(f,"PEN",(void *)memory.pen_usage,memory.nb_of_tiles);

	fw((void *)prefetch,PREFETCH_SIZE*sizeof(unsigned),f);
	fw((void *)null_all,PAD-actual_index-32,f);

	fw((void *)&longname,32,f);

	fw((void *)memory.cpu,memory.cpu_size,f);
	fw((void *)memory.bios,memory.bios_size,f);
	fw((void *)memory.sfix_game,memory.sfix_size,f);
	fw((void *)memory.sfix_board,0x20000,f);
	fw((void *)memory.sm1,memory.sm1_size,f);
	fw((void *)memory.sound1,memory.sound1_size,f);
	fw((void *)memory.sound2,memory.sound2_size,f);
	fw((void *)memory.gfx,memory.gfx_size,f);
	fw((void *)memory.ng_lo,0x10000,f);
	fw((void *)&memory.fix_board_usage[0],4096,f);
	fw((void *)memory.fix_game_usage,memory.sfix_size>>5,f);
	fw((void *)memory.pen_usage,memory.nb_of_tiles,f);

	fclose(f);

	if (prefetch[0]!=0xFFFFFFFF)
		add_direct_prefetch(str);

	res=load_aes(str);
	if (res)
	{
		printf("\n\nERROR %i !!!!!\n\n",res);
		unlink(str);
	}
	else
		printf("\n\nSaved as '%s'.\n\n",str);
	return res;
}

int try_to_add_prefetch(char *filename)
{
	load_prefetch(filename);
	if (prefetch[0]!=0xFFFFFFFF)
	{
		FILE *f=fopen(filename,"rb");
		if (f)
		{
			char *ln=get_longname_by_name(filename);
			void *buf=malloc(128*1024*1024);
			if (buf)
			{
				int ret=fread(buf,1,128*1024*1024,f);
				if ((ret>PAD) && (((char *)buf)[0]=='N') && (((char *)buf)[1]=='E') && (((char *)buf)[2]=='O') && (((char *)buf)[3]=='A') && (((char *)buf)[4]=='E') && (((char *)buf)[5]=='S') && (((char *)buf)[6]=='\0') )
				{
					memcpy((void *)(((unsigned)buf)+242),(void *)&prefetch[0],PREFETCH_SIZE*sizeof(unsigned));
					fclose(f);
					if (ln)
					{
						char *b=&((char *)buf)[PAD-32];
						strncpy(b,ln,31);
						b[31]=0;
					}
					printf ("\n Rewriting '%s' for rebuild prefetch\n",filename);
					f=fopen(filename,"wb");
					fwrite(buf,1,ret,f);
					fclose (f);
					free(buf);
					return 1;
				}
				free(buf);
			}
			fclose(f);
		}
	}
	return 0;
}


int process_rom(char *filename)
{
    int b;
    printf("\n------------------------\n--- %s ---\n------------------------\n",filename);
    memset((void *)&memory,0,sizeof(neo_mem));
    conf_game=NULL;
    if (init_game(filename))
	    return create_aes_file(filename);
    b=try_to_add_prefetch(filename);
    if (add_direct_prefetch(filename))
    {
	char *ln=get_longname_by_name(filename);
	if (b) return 0;
	if (ln)
		if (add_long_name(filename,ln))
			return 0;
#ifdef USE_GUI
	gui_error_box(20,80,264,60,
		      "Error!","No valid romset found for\n%s\n",
		      file_basename(rom_name));
#else
#ifdef WIN32
	MessageBox (0, "No valid romset found for source rom file", "Convert", MB_OK | MB_ICONERROR);
#else
	printf("No valid romset found for %s\n",filename);
#endif
#endif
	return -1;
    }
    return 0;
}


static char *usage_msg= \
"\nUsage:\n\n%s [options] neogeogame.zip\n\n" \
"\tOptions:\n" \
"\t\t[-usa]    :   USA region (default)\n" \
"\t\t[-jap]    :   Japan region\n" \
"\t\t[-eur]    :   European region\n" \
"\t\t[-asia]   :   Asia region\n" \
/*
"\t\t[-arcade] :   MVS Arcade (default)\n" \
"\t\t[-home]   :   AES console\n" \
*/
"\t\t\n\n" \
;


int main(int argc, char *argv[])
{
    int i;
    dr_load_driver("romrc");
    if (argc<2)
    {
#ifdef WIN32
	MessageBox (0, usage_msg, "Convert", MB_OK | MB_ICONERROR);
#else
	printf(usage_msg,argv[0]);
#endif
	return 1;
    }
    for(i=1;i<argc;i++)
    {
	if (!strcmp("-usa",argv[i]))
		conf_country=CTY_USA;
	else
	if (!strcmp("-jap",argv[i]))
		conf_country=CTY_JAPAN;
	else
	if (!strcmp("-eur",argv[i]))
		conf_country=CTY_EUROPE;
	else
	if (!strcmp("-asia",argv[i]))
		conf_country=CTY_ASIA;
	else
	if (!strcmp("-home",argv[i]))
		conf_system=SYS_HOME;
	else
	if (!strcmp("-arcade",argv[i]))
		conf_system=SYS_ARCADE;
    }
    null_all=calloc(PAD,2);
    for(i=1;i<argc;i++)
    {
	if (strcmp("-usa",argv[i])&&strcmp("-jap",argv[i])&&strcmp("-eur",argv[i])&&strcmp("-asia",argv[i]) &&strcmp("-home",argv[i]) && strcmp("-arcade",argv[i]) )
		if (process_rom(argv[i]))
			return 2;
    }
    free(null_all);
    return 0;
}
