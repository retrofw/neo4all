#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "video.h"
#include "../neo4all.h"
#include "videogl.h"
#include "sprgl.h"

static __inline__ void draw_tile(unsigned short *br, unsigned short *paldata, unsigned *gfxdata)
{
    int y;
    for(y=0;y<16;y++) {
#ifdef AES
	register unsigned int myword = gfxdata[0];
#else
	register unsigned int myword = gfxdata[1];
#endif
	br[0]=paldata[(myword>>28)&0xf];
	br[1]=paldata[(myword>>24)&0xf];
	br[2]=paldata[(myword>>20)&0xf];
	br[3]=paldata[(myword>>16)&0xf];
	br[4]=paldata[(myword>>12)&0xf];
	br[5]=paldata[(myword>>8)&0xf];
	br[6]=paldata[(myword>>4)&0xf];
	br[7]=paldata[(myword)&0xf];
	
#ifdef AES
	myword = gfxdata[1];
#else
	myword = gfxdata[0];
#endif
	br[8]=paldata[(myword>>28)&0xf];
	br[9]=paldata[(myword>>24)&0xf];
	br[10]=paldata[(myword>>20)&0xf];
	br[11]=paldata[(myword>>16)&0xf];
	br[12]=paldata[(myword>>12)&0xf];
	br[13]=paldata[(myword>>8)&0xf];
	br[14]=paldata[(myword>>4)&0xf];
	br[15]=paldata[(myword)&0xf];
	br+=16;
	gfxdata+=2;
    }
}

static __inline__ void create_tile(unsigned int tileno,int color,unsigned short *_br)
{
#ifdef DEBUG_GL
    tiles_fail++;
#endif
#if !defined(USE_SQ) && !defined(USE_DMA)
    register unsigned short *br=_br;
#else
    register unsigned short *br=(unsigned short*)neo4all_texture_buffer;
#endif
//printf("DRAW_TILE %i\n",tileno);
    register unsigned short *paldata=(unsigned short *)&video_paletteram_pc[color<<4];
    register unsigned int *gfxdata = (unsigned int *)&neogeo_spr_memory[tileno<<7];

    paldata[0]=0; // BLEND !!!
    draw_tile(br,paldata,gfxdata);

#if defined(USE_SQ) || defined(USE_DMA)
    memcpy(_br,neo4all_texture_buffer,16*16*2);
#endif
}


//---------------------------------------------------------------------------- 
void   video_draw_spr(unsigned int code, unsigned int color, int flipx, 
         int flipy, int sx, int sy, int zx, int zy) 
{
#ifdef _AES_
//   sx+=16;
     sy-=16;
#endif
  if (zx<=0 || zy<=0)
	  return;
  zx++;
//  if (zx==15) zx=16;

   register unsigned key=((code<<16)|(video_palette_selected+color));
   void *texture_buffer=tcache_hash_find(key);
   if (!texture_buffer)
   {
	texture_buffer=tcache_hash_insert(key);
   	if (!texture_buffer)
	{
#ifdef DEBUG_GL
puts("TILE CACHE OVERRUN!!!!!!");
#endif
		return;
	}
	create_tile(code,color,(unsigned short *)texture_buffer);
   }

#ifdef DEBUG_GL
  ntiles++;
#endif

   tile_list[n_tile_list].sx=sx;
   tile_list[n_tile_list].sy=sy;
   tile_list[n_tile_list].zx=zx;
   tile_list[n_tile_list].zy=zy;
   tile_list[n_tile_list].type=flipx|flipy;
   tile_list[n_tile_list].buffer=texture_buffer;
   n_tile_list++;
}

void video_draw_tile_textures(void)
{
#ifdef DEBUG_GL
	static unsigned minimo=0x12345678;
	unsigned actual=tcache_hash_unused();
	static unsigned maximo=0;
	if (n_tile_list>maximo)
		maximo=n_tile_list;
	if (actual<minimo)
		minimo=actual;
	printf("- %i tile_textures (%i max). Cache: %i tiles unused (%i min).\n",n_tile_list,maximo,actual,minimo);
#endif
   unsigned i;

   for(i=0;i<n_tile_list;i++)
   {
	register void *texture_buffer=tile_list[i].buffer;
	register int sx=tile_list[i].sx;
	register int sy=tile_list[i].sy;
	register int zx=tile_list[i].zx;
	register int zy=tile_list[i].zy;
//	glBindTexture(GL_TEXTURE_2D, tile_opengl_tex[i]);
	loadTextureParams();

	glTexImage2D(GL_TEXTURE_2D, 0, 4, 16, 16, 0, 
		GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, texture_buffer);
        glBegin(GL_QUADS);

	switch(tile_list[i].type)
	{
		case 0:
		   	VERTICE_FLIP_X
			break;
		case 1:
			VERTICE_FLIP_NONE
			break;
		case 2:
			VERTICE_FLIP_XY
			break;
		default:
			VERTICE_FLIP_Y
	}
	glEnd();
	tile_z+=TILE_Z_INC;
   }

#ifdef CACHE_GL_AUTOCLEAN
   if (!(neo4all_glframes&127))
   {
	   tcache_hash_old_cleaner(TCACHE_BREAKTIME);
#ifdef DEBUG_GL
	   unsigned ahora=tcache_hash_unused();
	   printf("  glframes=%i, ELIMINADOS=%i\n",neo4all_glframes,ahora-actual);
   }
   else
   {
	   printf("  glframes=%i\n",neo4all_glframes);
#endif
   }
#endif
}


#ifdef MENU_ALPHA
void video_draw_tile_textures_gl(void)
{
   unsigned i;
   extern GLint screen_texture;

   for(i=0;i<n_tile_list;i++)
   {
	register void *texture_buffer=tile_list[i].buffer;
	register int sx=tile_list[i].sx;
	register int sy=tile_list[i].sy;
	register int zx=tile_list[i].zx;
	register int zy=tile_list[i].zy;

	glBindTexture(GL_TEXTURE_2D, screen_texture); //tile_opengl_tex[i]);
	loadTextureParams();

	glTexImage2D(GL_TEXTURE_2D, 0, 4, 16, 16, 0, 
		GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, texture_buffer);
        glBegin(GL_QUADS);
	switch(tile_list[i].type)
	{
		case 0:
		   	GL_VERTICE_FLIP_X
			break;
		case 1:
			GL_VERTICE_FLIP_NONE
			break;
		case 2:
			GL_VERTICE_FLIP_XY
			break;
		default:
			GL_VERTICE_FLIP_Y
	}
	glEnd();
	tile_z+=TILE_Z_INC;
   }
}
#endif

void neo4all_draw_boders(void)
{
	glBindTexture(GL_TEXTURE_2D, black_opengl_tex);
	loadTextureParams();

	glTexImage2D(GL_TEXTURE_2D, 0, 4, 16, 16, 0, 
		GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, neo4all_black_texture_buffer);
        glBegin(GL_QUADS);
	{
		#define sx -16.0f
		#define sy -16.0f
		#define zx 24.0f
		#define zy 256.0f
		VERTICE_FLIP_NONE
		#undef zy
		#undef zx
		#undef sy
		#undef sx
	}
	glEnd();

	glBindTexture(GL_TEXTURE_2D, black_opengl_tex);
	loadTextureParams();

	glTexImage2D(GL_TEXTURE_2D, 0, 4, 16, 16, 0, 
		GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, neo4all_black_texture_buffer);
        glBegin(GL_QUADS);
	{
		#define sx 312.0f
		#define sy 0.0f
		#define zx 8.0f
		#define zy 240.0f
		VERTICE_FLIP_NONE
		#undef zy
		#undef zx
		#undef sy
		#undef sx
	}
	glEnd();

	glBindTexture(GL_TEXTURE_2D, black_opengl_tex);
	loadTextureParams();

	glTexImage2D(GL_TEXTURE_2D, 0, 4, 16, 16, 0, 
		GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, neo4all_black_texture_buffer);
        glBegin(GL_QUADS);
	{
		#define sx 0.0f
		#define sy -8.0f
		#define zx 320.0f
		#define zy 8.0f
		VERTICE_FLIP_NONE
		#undef zy
		#undef zx
		#undef sy
		#undef sx
	}
	glEnd();

	glBindTexture(GL_TEXTURE_2D, black_opengl_tex);
	loadTextureParams();

	glTexImage2D(GL_TEXTURE_2D, 0, 4, 16, 16, 0, 
		GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, neo4all_black_texture_buffer);
        glBegin(GL_QUADS);
	{
		#define sx 0.0f
		#define sy 224.0f
		#define zx 320.0f
		#define zy 8.0f
		VERTICE_FLIP_NONE
		#undef zy
		#undef zx
		#undef sy
		#undef sx
	}
	glEnd();
}
