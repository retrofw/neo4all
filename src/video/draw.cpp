/*******************************************
**** VIDEO.C - Video Hardware Emulation ****
*******************************************/

//-- Include Files -----------------------------------------------------------
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "video.h"
#include "../neo4all.h"
#ifdef USE_VIDEO_GL
#include "videogl.h"
#endif

#ifdef AES
#include "aes/aes.h"
#endif

#include "console.h"

#ifndef USE_VIDEO_GL
unsigned char	video_shrinky[17];
#endif

unsigned eficiencia_media=50;

#ifdef DREAMCAST
#include<kos.h>
#else
#include<string.h>
#define sq_set16(aaa,bbb,ccc)	{ \
				int sqinc; \
				short *sqsrc=(short *)aaa; \
				short sqset=bbb; \
				for(sqinc=0;sqinc<ccc;sqinc++) \
					sqsrc[sqinc]=sqset; \
				}
#endif

static int show_message=0;
static char _show_message_str[40]= {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static char *show_message_str=(char *)&_show_message_str[0];

void set_message(char *msg, int t) {
	show_message=t;
	strncpy(show_message_str, msg, 36);
}


int used_blitter=0;

void _write_text_inv_n(SDL_Surface *sf, int x, int y, int n, char * str);

void blitter(void) {
#ifdef USE_VIDEO_GL
	if (!used_blitter)
		neo4all_black_texture();
#endif
	used_blitter=1;

#ifndef USE_VIDEO_GL
	if (show_message) {
		show_message--;
		if (show_message)
			_write_text_inv_n(screen,1,28,38,show_message_str);
		else {
			SDL_Rect sr={ 0, 200, 320, 40 };
			SDL_FillRect(screen,(SDL_Rect *)&sr, 0);
		}
	}
#ifdef DOUBLE_BUFFER
	/*static const*/ SDL_Rect s={ 8, 0, 304, 240 };
	SDL_BlitSurface(video_buffer,(SDL_Rect *)&s,screen,(SDL_Rect *)&s);
#else
	/*static const*/ SDL_Rect r1={ 0, 0, 8, 240 };
	/*static const*/ SDL_Rect r2={ 312, 0, 8, 240 };
	SDL_FillRect(screen,(SDL_Rect *)&r1, 0);
	SDL_FillRect(screen,(SDL_Rect *)&r2, 0);
#endif
	SDL_Flip(screen);
#else

#ifndef DREAMCAST
  glClearColor(
	((video_paletteram_pc[4095]>>0)&0x1F)/31.0,
	((video_paletteram_pc[4095]>>5)&0x1F)/31.0,
	((video_paletteram_pc[4095]>>10)&0x1F)/31.0,
	1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
  glKosBeginFrame();
  pvr_set_bg_color(
	((float)((video_paletteram_pc[4095]>>10)&0x1F))/31.0,
	((float)((video_paletteram_pc[4095]>>5)&0x1F))/31.0,
	((float)((video_paletteram_pc[4095]>>0)&0x1F))/31.0);
  glKosFinishList();
#endif

  video_draw_tile_textures();
  video_draw_font_textures();
  neo4all_draw_boders();

#ifndef DREAMCAST
    SDL_GL_SwapBuffers();
#else
    glKosFinishFrame();
#endif
    neo4all_glframes++;
#endif
#ifdef DEBUG_GL
    if (ntiles)
    {
	unsigned eficiencia=100-((tiles_fail*100)/ntiles);
	eficiencia_media=(eficiencia_media+eficiencia)/2;
	printf("NTILES=%i, FALLADOS=%i, EFICIENCIA %i%%, EFICIENCIA MEDIA %i%%\n",ntiles,tiles_fail,eficiencia, eficiencia_media);
    }
    ntiles=0;
    tiles_fail=0;
#endif
}

void video_draw_blank(void)
{
#ifndef USE_VIDEO_GL
	SDL_FillRect(screen,NULL,0);
	blitter();
#endif
/*
#ifndef USE_VIDEO_GL
	SDL_FillRect(screen,NULL,0);
#else
    	glClearColor(0.0, 0.0, 0.0, 0.0);
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
	blitter();
*/
}

static __inline__ void video_draw_screen_first(void)
{
//ntiles=cacheados=0;

#ifndef USE_VIDEO_GL
  int    a;

//printf("draw_screen bgcolor=0x%X\n",video_paletteram_pc[4095]);
  for (a=0; a<224; a++) 
  	sq_set16(video_line_ptr[a], video_paletteram_pc[4095], 320*2);  
#else
  if (video_palette_dirty)
  {
	tcache_hash_pal_cleaner();
	fcache_hash_pal_cleaner();
	bzero(video_palette_use,0x200);
	video_palette_dirty=0;
  }
  tile_z=TILE_Z_INIT;
  n_font_list=0;
  n_tile_list=0;
#endif

}

void  print_video_checksum(void);

//---------------------------------------------------------------------------- 
void video_draw_screen1() 
{
   int         sx =0,sy =0,oy =0,my =0,zx = 1, rzy = 1; 
   int         offs,i,count,y; 
   int         tileno,tileatr,t1,t2,t3; 
   char         fullmode=0; 
   int         ddax=0,dday=0,rzx=15,yskip=0; 

   if (!neo4all_skip_next_frame)
   	video_draw_screen_first();

//printf("neogeo_frame_counter_fc=%i, neogeo_frame_counter_speed=%i\n",neogeo_frame_counter_fc,neogeo_frame_counter_speed);

   if (neogeo_frame_counter_fc >= neogeo_frame_counter_speed) { 
      neogeo_frame_counter++; 
      neogeo_frame_counter_fc=0; //3;
   } 

//printf("neogeo_frame_counter=%i\n",neogeo_frame_counter);
//print_video_checksum();

   neogeo_frame_counter_fc++; 
   for (count=0;count<0x300;count+=2) { 
      t3 = *((unsigned short *)( &video_vidram[0x10000 + count] )); 
      t1 = *((unsigned short *)( &video_vidram[0x10400 + count] )); 
      t2 = *((unsigned short *)( &video_vidram[0x10800 + count] )); 
//printf("%i, t1=0x%X, t2=0x%X, t3=0x%X\n",count,t1,t2,t3);

      // If this bit is set this new column is placed next to last one 
      if (t1 & 0x40) { 
#ifndef _AES_
         sx += (rzx + 1); 
         if ( sx >= 0x1F0 ) 
            sx -= 0x200;
#else
	 sx += rzx;
#endif

         // Get new zoom for this column 
         zx = (t3 >> 8)&0x0F; 

         sy = oy; 
      } else {   // nope it is a new block 
         // Sprite scaling 
         zx = (t3 >> 8)&0x0F; 

         rzy = t3 & 0xff; 
#ifdef _AES_
	 if (!rzy) continue;
#endif

         sx = (t2 >> 7);
#ifndef _AES_
         if ( sx >= 0x1F0 ) 
            sx -= 0x200; 
#endif

         // Number of tiles in this strip 
         my = t1 & 0x3f; 
         if (my == 0x20) 
            fullmode = 1; 
         else if (my >= 0x21) 
            fullmode = 2;   // most games use 0x21, but 
         else 
            fullmode = 0;   // Alpha Mission II uses 0x3f 

#ifndef _AES_
         sy = 0x1F0 - (t1 >> 7); 
         if (sy > 0x100) sy -= 0x200; 
#else
	 sy = 0x200 - (t1 >> 7); /* sprite bank position */
	 if (sy > 0x110) sy -= 0x200;
#endif
          
         if (fullmode == 2 || (fullmode == 1 && rzy == 0xff)) 
         { 
#ifndef _AES_
            while (sy < -16) sy += 2 * (rzy + 1); 
#else
	    while (sy < 0) sy += ((rzy + 1)<<1);
#endif
         } 
         oy = sy; 

#ifndef _AES_
         if(my==0x21)
		 my=0x20; 
         else
		 if(rzy!=0xff && my!=0) 
            		my=((my*16*256)/(rzy+1) + 15)/16; 
#else
	 if (rzy < 0xff && my < 0x10 && my){
		 my = my*255/rzy;
		 if (my > 0x10) my = 0x10;
	 }
#endif

         if(my>0x20) my=0x20; 

         ddax=0;   // setup x zoom 
      } 

#ifndef _AES_
      rzx = zx; 

      // No point doing anything if tile strip is 0 
      if ((my==0)||(sx>311)) 
         continue; 
#else
      if (my==0) continue;
      if(zx!=15) rzx=zx+1;
      else rzx=16;
      if ( sx >= 0x1F0 ) sx -= 0x200;
      if(sx>=320) continue;
#endif

      // Setup y zoom 
      if(rzy==255) 
         yskip=16; 
      else 
         dday=0;   // =256; NS990105 mslug fix 

      offs = count<<6; 

//printf("offs=%i\n",offs);
      // my holds the number of tiles in each vertical multisprite block 
      for (y=0; y < my ;y++) { 
         tileno  = *((unsigned short *)(&video_vidram[offs])); 
//printf("tileno=%i, offs=%i\n",tileno,offs);
         offs+=2; 
         tileatr = *((unsigned short *)(&video_vidram[offs])); 
         offs+=2;

#ifdef AES
	 if (aes4all_memory_nb_of_tiles>0x10000 && tileatr&0x10) tileno+=0x10000;
	 if (aes4all_memory_nb_of_tiles>0x20000 && tileatr&0x20) tileno+=0x20000;
	 if (aes4all_memory_nb_of_tiles>0x40000 && tileatr&0x40) tileno+=0x40000;

	 if (tileatr&0x8)
	    tileno=(tileno&~7)+((tileno+neogeo_frame_counter)&7);
	 else if (tileatr&0x4)
	    tileno=(tileno&~3)+((tileno+neogeo_frame_counter)&3);

//printf("y=%i, tileno=%i, tileatr=0x%X\n",y,tileno,tileatr);
	 if (tileno>aes4all_memory_nb_of_tiles)
	    continue;
#else
         if (tileatr&0x8) 
            tileno = (tileno&~7)|(neogeo_frame_counter&7); 
         else if (tileatr&0x4) 
            tileno = (tileno&~3)|(neogeo_frame_counter&3); 
              
         if (tileno>0x7FFF) 
            continue; 
#endif


         if (fullmode == 2 || (fullmode == 1 && rzy == 0xff)) 
         { 
            if (sy >= 248) sy -= ((rzy + 1)<<1); 
         } 
         else if (fullmode == 1) 
         { 
            if (y == 0x10) sy -= ((rzy + 1)<<1); 
         } 
         else if (sy > 0x110) sy -= 0x200; 

         if(rzy!=255) 
         { 
            yskip=0; 
#ifndef USE_VIDEO_GL
            video_shrinky[0]=0; 
#endif
            for(i=0;i<16;i++) 
            { 
#ifndef USE_VIDEO_GL
               video_shrinky[i+1]=0; 
#endif
               dday-=rzy+1; 
               if(dday<=0) 
               { 
                  dday+=256; 
                  yskip++; 
#ifndef USE_VIDEO_GL
                  video_shrinky[yskip]++; 
#endif
               } 
#ifndef USE_VIDEO_GL
               else 
                  video_shrinky[yskip]++; 
#endif
            } 
         } 

	 if (!neo4all_skip_next_frame)
#ifdef _AES_
//{
//	   if (!((unsigned char *)aes4all_memory_pen_usage)[tileno])
	   if (!(aes4all_pen_usage[tileno>>3]&(1<<(tileno&0x7))))
//{
		if (sx >= -16 && sx+15 < 336 && sy>=0 && sy+15 <256)
#else
         	if (((tileatr>>8)||(tileno!=0))&&(sy<224)) 
#endif
         	{
//printf("PINTA %i, %i, %i, %i, %i, %i, %i, %i,\n",tileno,tileatr >> 8,tileatr & 0x01,tileatr & 0x02,sx,sy,rzx,yskip);
         		video_draw_spr( 
               			tileno,
				tileatr >> 8, 
               			tileatr & 0x01,tileatr & 0x02, 
               			sx,sy,rzx,yskip); 
         	}
//else printf("%i desechado\n",tileno);
//}else printf("TILE INVISIBLE %i\n",tileno);
//}

         sy +=yskip; 
      }  // for y 
   }  // for count 

    
   if (!neo4all_skip_next_frame)
   {
   	video_draw_fix(); 
   	blitter();
   }
} 


#ifndef AES
//---------------------------------------------------------------------------- 
void video_draw_screen2() 
{ 
   static int      pass1_start; 
   int         sx =0,sy =0,oy =0,my =0,zx = 1, rzy = 1; 
   int         offs,i,count,y; 
   int         tileno,tileatr,t1,t2,t3; 
   char         fullmode=0; 
   int         ddax=0,dday=0,rzx=15,yskip=0; 

   if (!neo4all_skip_next_frame)
   	video_draw_screen_first();
 
   if (neogeo_frame_counter_fc >= neogeo_frame_counter_speed) { 
      neogeo_frame_counter++; 
      neogeo_frame_counter_fc=0; //3;
   } 
   neogeo_frame_counter_fc++; 
   t1 = *((unsigned short *)( &video_vidram[0x10400 + 4] )); 

   if ((t1 & 0x40) == 0) 
   { 
      for (pass1_start=6;pass1_start<0x300;pass1_start+=2) 
      { 
         t1 = *((unsigned short *)( &video_vidram[0x10400 + pass1_start] )); 

         if ((t1 & 0x40) == 0) 
            break; 
      } 
        
      if (pass1_start == 6) 
         pass1_start = 0; 
   } 
   else 
      pass1_start = 0;    

   for (count=pass1_start;count<0x300;count+=2) { 
      t3 = *((unsigned short *)( &video_vidram[0x10000 + count] )); 
      t1 = *((unsigned short *)( &video_vidram[0x10400 + count] )); 
      t2 = *((unsigned short *)( &video_vidram[0x10800 + count] )); 

      // If this bit is set this new column is placed next to last one 
      if (t1 & 0x40) { 
         sx += (rzx + 1); 
         if ( sx >= 0x1F0 ) 
            sx -= 0x200; 

         // Get new zoom for this column 
         zx = (t3 >> 8)&0x0F; 

         sy = oy; 
      } else {   // nope it is a new block 
         // Sprite scaling 
         zx = (t3 >> 8)&0x0F; 

         rzy = t3 & 0xff; 

         sx = (t2 >> 7); 
         if ( sx >= 0x1F0 ) 
            sx -= 0x200; 

         // Number of tiles in this strip 
         my = t1 & 0x3f; 
         if (my == 0x20) 
            fullmode = 1; 
         else if (my >= 0x21) 
            fullmode = 2;   // most games use 0x21, but 
         else 
            fullmode = 0;   // Alpha Mission II uses 0x3f 

         sy = 0x1F0 - (t1 >> 7); 
         if (sy > 0x100) sy -= 0x200; 
          
         if (fullmode == 2 || (fullmode == 1 && rzy == 0xff)) 
         { 
            while (sy < -16) sy += 2 * (rzy + 1); 
         } 
         oy = sy; 

         if(my==0x21)
		 my=0x20; 
         else
		 if(rzy!=0xff && my!=0) 
            		my=((my*16*256)/(rzy+1) + 15)/16; 

         if(my>0x20) my=0x20; 

         ddax=0;   // setup x zoom 
      } 

      rzx = zx; 

      // No point doing anything if tile strip is 0 
      if ((my==0)||(sx>311)) 
         continue; 

      // Setup y zoom 
      if(rzy==255) 
         yskip=16; 
      else 
         dday=0;   // =256; NS990105 mslug fix 

      offs = count<<6; 

      // my holds the number of tiles in each vertical multisprite block 
      for (y=0; y < my ;y++) { 
         tileno  = *((unsigned short *)(&video_vidram[offs])); 
         offs+=2; 
         tileatr = *((unsigned short *)(&video_vidram[offs])); 
         offs+=2; 

         if (tileatr&0x8) 
            tileno = (tileno&~7)|(neogeo_frame_counter&7); 
         else if (tileatr&0x4) 
            tileno = (tileno&~3)|(neogeo_frame_counter&3); 
              
         if (tileno>0x7FFF) 
            continue; 


         if (fullmode == 2 || (fullmode == 1 && rzy == 0xff)) 
         { 
            if (sy >= 224) sy -= ((rzy + 1)<<1); 
         } 
         else if (fullmode == 1) 
         { 
            if (y == 0x10) sy -= ((rzy + 1)<<1); 
         } 
         else if (sy > 0x100) sy -= 0x200; 

         if(rzy!=255) 
         { 
            yskip=0; 
#ifndef USE_VIDEO_GL
            video_shrinky[0]=0; 
#endif
            for(i=0;i<16;i++) 
            { 
#ifndef USE_VIDEO_GL
               video_shrinky[i+1]=0; 
#endif
               dday-=rzy+1; 
               if(dday<=0) 
               { 
                  dday+=256; 
                  yskip++; 
#ifndef USE_VIDEO_GL
                  video_shrinky[yskip]++; 
#endif
               } 
#ifndef USE_VIDEO_GL
               else 
                  video_shrinky[yskip]++; 
#endif
            } 
         } 

    	 if (!neo4all_skip_next_frame)
         	if (((tileatr>>8)||(tileno!=0))&&(sy<224)) 
         	{ 
         		video_draw_spr( 
               			tileno, 
               			tileatr >> 8, 
               			tileatr & 0x01,tileatr & 0x02, 
               			sx,sy,rzx,yskip); 
         	} 

         sy +=yskip; 
      }  // for y 
   }  // for count 

   for (count=0;count<pass1_start;count+=2) { 
      t3 = *((unsigned short *)( &video_vidram[0x10000 + count] )); 
      t1 = *((unsigned short *)( &video_vidram[0x10400 + count] )); 
      t2 = *((unsigned short *)( &video_vidram[0x10800 + count] )); 

      // If this bit is set this new column is placed next to last one 
      if (t1 & 0x40) { 
         sx += (rzx + 1); 
         if ( sx >= 0x1F0 ) 
            sx -= 0x200; 

         // Get new zoom for this column 
         zx = (t3 >> 8)&0x0F; 

         sy = oy; 
      } else {   // nope it is a new block 
         // Sprite scaling 
         zx = (t3 >> 8)&0x0F; 

         rzy = t3 & 0xff; 

         sx = (t2 >> 7); 
         if ( sx >= 0x1F0 ) 
            sx -= 0x200; 

         // Number of tiles in this strip 
         my = t1 & 0x3f; 
         if (my == 0x20) 
            fullmode = 1; 
         else if (my >= 0x21) 
            fullmode = 2;   // most games use 0x21, but 
         else 
            fullmode = 0;   // Alpha Mission II uses 0x3f 

         sy = 0x1F0 - (t1 >> 7); 
         if (sy > 0x100) sy -= 0x200; 
          
         if (fullmode == 2 || (fullmode == 1 && rzy == 0xff)) 
         { 
	    while (sy < 0) sy += ((rzy + 1)<<1);
         } 
         oy = sy; 

	 if (rzy < 0xff && my < 0x10 && my){
		 my = my*255/rzy;
		 if (my > 0x10) my = 0x10;
	 }

         if(my>0x20) my=0x20; 

         ddax=0;   // setup x zoom 
      } 

      rzx = zx; 

      // No point doing anything if tile strip is 0 
      if ((my==0)||(sx>311)) 
         continue; 

      // Setup y zoom 
      if(rzy==255) 
         yskip=16; 
      else 
         dday=0;   // =256; NS990105 mslug fix 

      offs = count<<6; 

      // my holds the number of tiles in each vertical multisprite block 
      for (y=0; y < my ;y++) { 
         tileno  = *((unsigned short *)(&video_vidram[offs])); 
         offs+=2; 
         tileatr = *((unsigned short *)(&video_vidram[offs])); 
         offs+=2; 

         if (tileatr&0x8) 
            tileno = (tileno&~7)|(neogeo_frame_counter&7); 
         else if (tileatr&0x4) 
            tileno = (tileno&~3)|(neogeo_frame_counter&3); 
              
         if (tileno>0x7FFF) 
            continue; 


         if (fullmode == 2 || (fullmode == 1 && rzy == 0xff)) 
         { 
            if (sy >= 248) sy -= ((rzy + 1)<<1); 
         } 
         else if (fullmode == 1) 
         { 
            if (y == 0x10) sy -= ((rzy + 1)<<1); 
         } 
         else if (sy > 0x110) sy -= 0x200; 

         if(rzy!=255) 
         { 
            yskip=0; 
#ifndef USE_VIDEO_GL
            video_shrinky[0]=0; 
#endif
            for(i=0;i<16;i++) 
            { 
#ifndef USE_VIDEO_GL
               video_shrinky[i+1]=0; 
#endif
               dday-=rzy+1; 
               if(dday<=0) 
               { 
                  dday+=256; 
                  yskip++; 
#ifndef USE_VIDEO_GL
                  video_shrinky[yskip]++; 
#endif
               } 
#ifndef USE_VIDEO_GL
               else 
                  video_shrinky[yskip]++; 
#endif
            } 
         } 

    	 if (!neo4all_skip_next_frame)
         	if (((tileatr>>8)||(tileno!=0))&&(sy<224)) 
         	{ 
         		video_draw_spr( 
         			tileno, 
         			tileatr >> 8, 
         			tileatr & 0x01,tileatr & 0x02, 
         			sx,sy,rzx,yskip); 
         	} 


         sy +=yskip; 
      }  // for y 
   }  // for count 

    
   if (!neo4all_skip_next_frame)
   {
   	video_draw_fix(); 
   	blitter(); 
   }
}
#endif



void video_flip(SDL_Surface *surface)
{
#ifndef USE_VIDEO_GL
	SDL_Flip(surface);
#else
#ifdef SHOW_CONSOLE
  if (used_blitter)
  {
	tcache_hash_init();
	fcache_hash_init();
	if (view_console)
		console_draw_all_background();
  }
#endif
#ifdef DREAMCAST
  glKosBeginFrame();
#endif
  glClearColor( 0.0,0.0,0.0, 255.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  extern GLint screen_texture;

  tile_z=0.5;

#ifdef MENU_ALPHA
  video_draw_tile_textures_gl();
  video_draw_font_textures_gl();
#endif

  glBindTexture(GL_TEXTURE_2D,screen_texture);
  loadTextureParams();

#ifdef MENU_ALPHA

#ifndef DREAMCAST
  glTexImage2D(GL_TEXTURE_2D, 0, 4, 512, 512, 0, 
    GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, neo4all_texture_surface);
#else
  glKosTex2D(GL_ARGB1555,512,512,neo4all_texture_surface);
#endif

#else

#ifndef DREAMCAST
  glTexImage2D(GL_TEXTURE_2D, 0, 3, 512, 512, 0, 
    GL_RGB, GL_UNSIGNED_SHORT_5_6_5, neo4all_texture_surface);
#else
  glKosTex2D(GL_RGB565,512,512,neo4all_texture_surface);
#endif

#endif

  double t_x1=0.0,t_y1=0.0,t_x2=512.0,t_y2=512.0;
#ifdef DREAMCAST
  t_y1+=4.0; t_y2+=4.0;
#endif

  glBegin(GL_QUADS);
  	glTexCoord2f(0.0,0.0);
	glVertex3f(t_x1,t_y1,tile_z);
	
  	glTexCoord2f(1.0,0.0);
	glVertex3f(t_x2,t_y1,tile_z);

  	glTexCoord2f(1.0,1.0);
	glVertex3f(t_x2,t_y2,tile_z);

  	glTexCoord2f(0.0,1.0);
	glVertex3f(t_x1,t_y2,tile_z);
  glEnd();

#ifndef DREAMCAST
  SDL_GL_SwapBuffers();
#else
  glKosFinishFrame();
#endif
  used_blitter=0;
#endif
}
