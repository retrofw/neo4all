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


#include "SDL.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "unzip.h"
#include "memory.h"
#include "fileio.h"
#include "neocrypt.h"
#include "conf.h"
#include "pbar.h"
#include "driver.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef USE_GUI
#include "gui/gui.h"
#endif

Uint8 *current_buf;
//extern Uint8 fix_buffer[0x20000];
char *rom_file;

char *file_basename(char *filename) {
    char *t;
    t=strrchr(filename,'/');
    if (t) return t+1;
    return filename;
}

void dump_bios(unsigned inicio, unsigned n)
{
	unsigned char *p=(unsigned char *)memory.bios;
	unsigned i;
	printf("\nDUMB_BIOS(%08X,%i): ",inicio,n);
	for(i=inicio;i<(inicio+n);i++)
		printf("%02X ",p[i]);
	puts("");
}

/* check if dir_name exist. Create it if not */
SDL_bool check_dir(char *dir_name)
{
/*
    DIR *d;

    if (!(d = opendir(dir_name)) && (errno == ENOENT)) {
#ifdef WIN32
	mkdir(dir_name);
#else
	mkdir(dir_name, 0755);
#endif
	return SDL_FALSE;
    }
*/
    return SDL_TRUE;
}

/* return a char* to $HOME/.neo4all/
   DO NOT free it!
*/
char *get_gngeo_dir(void) {
    static char *filename=NULL;
    int len = strlen(getenv("HOME")) + strlen("/.neo4all/") + 1;
    int i;
    if (!filename) {
    	filename=calloc(len,sizeof(char));
    	sprintf(filename,"%s/.neo4all/",getenv("HOME"));
    }
    check_dir(filename);
    //printf("get_gngeo_dir %s\n",filename);
    return filename;
}


void free_game_memory(void) {

    /* clean up memory */
    free(memory.cpu);memory.cpu=NULL;
    
    free(memory.sm1);memory.sm1=NULL;
    free(memory.sfix_game);memory.sfix_game=NULL;
    if (memory.sound1!=memory.sound2) 
	free(memory.sound2);
    memory.sound2=NULL;
    free(memory.sound1);memory.sound1=NULL;
    free(memory.gfx);memory.gfx=NULL;
    free(memory.pen_usage);memory.pen_usage=NULL;

}

SDL_bool init_game(char *rom_name) {
    DRIVER *dr;
    char *drconf,*gpath;
    dr=dr_get_by_name(rom_name);
    if (!dr) {
	return SDL_FALSE;
    }
/*
    if (conf_game!=NULL) {
	save_nvram(conf.game);
	if (conf.sound) {
	    close_sdl_audio();
	    YM2610_sh_stop();
	    streams_sh_stop();
	}
	free_game_memory();
    }
*/

    /* open transpack if need */
  //  trans_pack_open(CF_STR(cf_get_item_by_name("transpack")));

    //open_rom(rom_name);
    if (dr_load_game(dr,rom_name)==SDL_FALSE) {
#ifdef USE_GUI
	gui_error_box(20,80,264,60,
		      "Error!","Couldn't load\n%s\n",
		      file_basename(rom_name));
#else
#ifdef WIN32
	MessageBox (0, "Can't load source rom file", "Convert", MB_OK | MB_ICONERROR);
#else
	printf("Can't load %s\n",rom_name);
#endif
#endif
	return SDL_FALSE;
    }
    open_bios();
//dump_bios(0x11022,8);

    return SDL_TRUE;
}

void free_bios_memory(void) {
    free(memory.ram);memory.ram=NULL;
    if (!conf_special_bios)
      free(memory.bios);memory.bios=NULL;
    free(memory.ng_lo);memory.ng_lo=NULL;
    free(memory.sfix_board);memory.sfix_board=NULL;

    free(memory.pal1);memory.pal1=NULL;
    free(memory.pal2);memory.pal2=NULL;
    free(memory.pal_pc1);memory.pal_pc1=NULL;
    free(memory.pal_pc2);memory.pal_pc2=NULL;
}

void open_bios(void)
{
    FILE *f;
    char *romfile;
    char *path = "bios"; //CF_STR(cf_get_item_by_name("rompath"));
    int len = strlen(path) + 15;

    if (conf_game!=NULL) free_bios_memory();

    /* allocation de la ram */
    memory.ram = (Uint8 *) calloc(1,0x10000);
    memset(memory.ram,0,0x10000);
    memory.sfix_board = (Uint8 *) calloc(1,0x20000);
    memory.ng_lo = (Uint8 *) calloc(1,0x10000);

    /* partie video */
    memory.pal1 = (Uint8 *) calloc(1,0x2000);
    memory.pal2 = (Uint8 *) calloc(1,0x2000);

    memory.pal_pc1 = (Uint8 *) calloc(1,0x2000);
    memory.pal_pc2 = (Uint8 *) calloc(1,0x2000);

    memset(memory.video, 0, 0x20000);

    romfile = (char *) calloc(1,len);
    memset(romfile, 0, len);
    if (!conf_special_bios) {
      memory.bios = (Uint8 *) calloc(1,0x20000);
      memory.bios_size=0x20000;
      /* try new bios */
      if (conf_system==SYS_HOME) {
          sprintf(romfile, "%s/aes-bios.bin", path);
      } else {
          if (conf_country==CTY_JAPAN) {
              sprintf(romfile, "%s/vs-bios.rom", path);
          } else if (conf_country==CTY_USA) {
              sprintf(romfile, "%s/usa_2slt.bin", path);
          } else if (conf_country==CTY_ASIA) {
              sprintf(romfile, "%s/asia-s3.rom", path);
          } else {
              sprintf(romfile, "%s/sp-s2.sp1", path);
          }
      }
//printf("open_bios: try to open '%s'\n",romfile);
      f = fopen(romfile, "rb");
      if (f == NULL) {
#ifdef WIN32
	MessageBox (0, "No found bios rom file", "Convert", MB_OK | MB_ICONERROR);
#else
          printf("Can't find %s\n", romfile);
#endif
          exit(1);
      }
      fread(memory.bios, 1, 0x20000, f);
      fclose(f);
    }
    sprintf(romfile, "%s/bios/ng-sfix.rom", path);
//printf("open_bios: try to open '%s'\n",romfile);
    f = fopen(romfile, "rb");
    if (f == NULL) {
	/* try new bios */
	sprintf(romfile, "%s/sfix.sfx", path);
//printf("open_bios: try to open '%s'\n",romfile);
	f = fopen(romfile, "rb");
	if (f == NULL) {
#ifdef WIN32
	MessageBox (0, "No found bios rom file", "Convert", MB_OK | MB_ICONERROR);
#else
	    printf("Can't find %s\n", romfile);
#endif
	    exit(1);
	}
    }
    fread(memory.sfix_board, 1, 0x20000, f);
    fclose(f);

    sprintf(romfile, "%s/ng-lo.rom", path);
//printf("open_bios: try to open '%s'\n",romfile);
    f = fopen(romfile, "rb");
    if (f == NULL) {
	/* try new bios */
	sprintf(romfile, "%s/000-lo.lo", path);
//printf("open_bios: try to open '%s'\n",romfile);
	f = fopen(romfile, "rb");
	if (f == NULL) {
#ifdef WIN32
	MessageBox (0, "No found bios rom file", "Convert", MB_OK | MB_ICONERROR);
#else
	    printf("Can't find %s\n", romfile);
#endif
	    exit(1);
	}
    }
    fread(memory.ng_lo, 1, 0x10000, f);
    fclose(f);


    /* convert bios fix char */
    convert_all_char(memory.sfix_board, 0x20000, memory.fix_board_usage);

    fix_usage = memory.fix_board_usage;
    current_pal = memory.pal1;
    current_fix = memory.sfix_board;
    current_pc_pal = (Uint16 *) memory.pal_pc1;

    free(romfile);
}


