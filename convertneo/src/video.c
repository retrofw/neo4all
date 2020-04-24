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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "conf.h"
#include "pbar.h"
#include "driver.h"



#ifdef I386_ASM
/* global declaration for video_i386.asm */
Uint8 **mem_gfx=&memory.gfx;
Uint8 *mem_video=memory.video;

/* prototype */
void draw_tile_i386_norm(unsigned int tileno,int sx,int sy,int zx,int zy,
			 int color,int xflip,int yflip,unsigned char *bmp);
void draw_tile_i386_50(unsigned int tileno,int sx,int sy,int zx,int zy,
		       int color,int xflip,int yflip,unsigned char *bmp);
void draw_one_char_i386(int byte1,int byte2,unsigned short *br);

void draw_scanline_tile_i386_norm(unsigned int tileno,int yoffs,int sx,int line,int zx,
				  int color,int xflip,unsigned char *bmp);

void draw_scanline_tile_i386_50(unsigned int tileno,int yoffs,int sx,int line,int zx,
				int color,int xflip,unsigned char *bmp);
#endif

Uint8 strip_usage[0x300];
char *dda_x_skip;
char ddaxskip[16][16] =
{
    { 0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0 },
    { 0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0 },
    { 0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0 },
    { 0,0,1,0,1,0,0,0,1,0,0,0,1,0,0,0 },
    { 0,0,1,0,1,0,0,0,1,0,0,0,1,0,1,0 },
    { 0,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0 },
    { 0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0 },
    { 1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0 },
    { 1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0 },
    { 1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,0 },
    { 1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,1 },
    { 1,0,1,1,1,0,1,1,1,1,1,0,1,0,1,1 },
    { 1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1 },
    { 1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1 },
    { 1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1 },
    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }
};


static __inline__ Uint16 alpha_blend(Uint16 dest,Uint16 src,Uint8 a)
{
    static Uint8 dr,dg,db,sr,sg,sb;
  
    dr=((dest&0xF800)>>11)<<3;
    dg=((dest&0x7E0)>>5)<<2;
    db=((dest&0x1F))<<3;

    sr=((src&0xF800)>>11)<<3;
    sg=((src&0x7E0)>>5)<<2;
    sb=((src&0x1F))<<3;
  
    dr = (((sr-dr)*(a))>>8)+dr;
    dg = (((sg-dg)*(a))>>8)+dg;
    db = (((sb-db)*(a))>>8)+db;
  
    return ((dr>>3)<<11)|((dg>>2)<<5)|(db>>3);
}
#define BLEND16_50(a,b) ((((a)&0xf7de)>>1)+(((b)&0xf7de)>>1))
#define BLEND16_25(a,b) alpha_blend(a,b,63)


char dda_y_skip[17];
char full_y_skip[16]={0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
unsigned int neogeo_frame_counter_speed=8;

/*
SDL_Rect buf_rect={16,16,304,224};
SDL_Rect screen_rect={0,0,304,224};
*/


void convert_all_char(Uint8 *Ptr, int Taille, 
		      Uint8 *usage_ptr)
{
    int		i,j;
    unsigned char	usage;
    
    Uint8 *Src;
    Uint8 *sav_src;

    Src=(Uint8*)calloc(1,Taille);
    if (!Src) {
	printf("Not enought memory!!\n");
	return;
    }
    sav_src=Src;
    memcpy(Src,Ptr,Taille);
#ifdef WORDS_BIGENDIAN
#define CONVERT_TILE *Ptr++ = *(Src+8);\
	             usage |= *(Src+8);\
                     *Ptr++ = *(Src);\
		     usage |= *(Src);\
		     *Ptr++ = *(Src+24);\
		     usage |= *(Src+24);\
		     *Ptr++ = *(Src+16);\
		     usage |= *(Src+16);\
		     Src++;
#else
#define CONVERT_TILE *Ptr++ = *(Src+16);\
	             usage |= *(Src+16);\
                     *Ptr++ = *(Src+24);\
		     usage |= *(Src+24);\
		     *Ptr++ = *(Src);\
		     usage |= *(Src);\
		     *Ptr++ = *(Src+8);\
		     usage |= *(Src+8);\
		     Src++;
#endif
    for(i=Taille;i>0;i-=32) {
        usage = 0;
        for (j=0;j<8;j++) {
            CONVERT_TILE
                }
        Src+=24;
        *usage_ptr++ = usage;
    }
    free(sav_src);
#undef CONVERT_TILE
}

/* For MGD-2 dumps */
static int mgd2_tile_pos=0;
void convert_mgd2_tiles(unsigned char *buf,int len)
{
    int i;
    unsigned char t;

    if (len==memory.gfx_size && mgd2_tile_pos==memory.gfx_size) {
	mgd2_tile_pos=0;
    }
    if (len == 2) {
	
	
	return;
    }

    if (len == 6)
    {
        unsigned char swp[6];

        memcpy(swp,buf,6);
        buf[0] = swp[0];
        buf[1] = swp[3];
        buf[2] = swp[1];
        buf[3] = swp[4];
        buf[4] = swp[2];
        buf[5] = swp[5];

        return;
    }

    if (len % 4) exit(1);	/* must not happen */

    len /= 2;

    for (i = 0;i < len/2;i++)
    {
        t = buf[len/2 + i];
        buf[len/2 + i] = buf[len + i];
        buf[len + i] = t;
    }
    if (len==2) {
	mgd2_tile_pos+=2;
	if ((mgd2_tile_pos&0x3f)==0)  update_progress_bar(mgd2_tile_pos,memory.gfx_size);
    }
    convert_mgd2_tiles(buf,len);
    convert_mgd2_tiles(buf + len,len);
}


