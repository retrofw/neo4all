#ifdef DREAMCAST
#include <kos.h>
#endif

#ifdef SHOW_MENU

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#ifndef DREAMCAST
#include <unistd.h>
#endif

#include <SDL_mixer.h>
#include <SDL_image.h>

#include "menu.h"
#include "sound/sound.h"
#include "sfont.h"
//#include "msg.h"
#include "fade.h"
#include "background.h"

#define TRANS_COLOR 0xFFFFFFFF
#define TRANS_COLOR2 0xC616C616
#define MUSIC_VOLUME 80

#define MENU_FILE_TEXT		DATA_PREFIX "font.png"
#define MENU_FILE_TEXT2		DATA_PREFIX "font2.png"
#define MENU_FILE_WIN_ULEFT	DATA_PREFIX "win_uleft.png"
#define MENU_FILE_WIN_URIGHT	DATA_PREFIX "win_uright.png"
#define MENU_FILE_WIN_UPPER 	DATA_PREFIX "win_upper.png"
#define MENU_FILE_WIN_BLEFT 	DATA_PREFIX "win_bleft.png"
#define MENU_FILE_WIN_BRIGHT 	DATA_PREFIX "win_bright.png"
#define MENU_FILE_WIN_BOTTOM 	DATA_PREFIX "win_bottom.png"
#define MENU_FILE_WIN_LEFT 	DATA_PREFIX "win_left.png"
#define MENU_FILE_WIN_RIGHT 	DATA_PREFIX "win_right.png"

#ifdef DREAMCAST
#define VIDEO_FLAGS_INIT SDL_HWSURFACE|SDL_FULLSCREEN
#else
#define VIDEO_FLAGS_INIT SDL_HWSURFACE
#endif

#ifdef DOUBLEBUFFER
#define VIDEO_FLAGS VIDEO_FLAGS_INIT | SDL_DOUBLEBUF
#else
#define VIDEO_FLAGS VIDEO_FLAGS_INIT
#endif

SDL_Surface *text_screen=NULL, *text_image, *text_image2, *text_win_uleft, *text_win_uright, *text_win_upper, *text_win_left, *text_win_right, *text_win_bright, *text_win_bleft, *text_win_bottom, *text_cdrom;

static SFont_FontInfo font_inv;
static Uint32 menu_inv_color=0, menu_win0_color=0, menu_win1_color=0;
static Uint32 menu_barra0_color=0, menu_barra1_color=0;
static Uint32 menu_win0_color_base=0, menu_win1_color_base=0;

void write_num(int x, int y, int v);
//int menu_msg_pos=330;
int menu_moving=1;
//Uint32 menu_msg_time=0x12345678;

#ifdef DREAMCAST
extern int __sdl_dc_emulate_keyboard;
#endif

static void obten_colores(void)
{
	char *filename=DATA_PREFIX "colors.txt";
	FILE *f=fopen(filename, "rt");
#ifdef DREAMCAST
	if (!f) {
		filename[1]='s';
		f=fopen(filename, "rt");
	}
#endif
	if (f)
	{
		Uint32 r,g,b;
		fscanf(f,"menu_inv_color=0x%X,0x%X,0x%X\n",&r,&g,&b);
		menu_inv_color=SDL_MapRGB(text_screen->format,r,g,b);
		fscanf(f,"menu_win0_color=0x%X,0x%X,0x%X\n",&r,&g,&b);
		menu_win0_color=SDL_MapRGB(text_screen->format,r,g,b);
		fscanf(f,"menu_win1_color=0x%X,0x%X,0x%X\n",&r,&g,&b);
		menu_win1_color=SDL_MapRGB(text_screen->format,r,g,b);
		fscanf(f,"menu_barra0_color=0x%X,0x%X,0x%X\n",&r,&g,&b);
		menu_barra0_color=SDL_MapRGB(text_screen->format,r,g,b);
		fscanf(f,"menu_barra1_color=0x%X,0x%X,0x%X\n",&r,&g,&b);
		menu_barra1_color=SDL_MapRGB(text_screen->format,r,g,b);
		fclose(f);
	}
	else
	{
		menu_inv_color=SDL_MapRGB(text_screen->format, 0x20, 0x20, 0x40);
		menu_win0_color=SDL_MapRGB(text_screen->format, 0x10, 0x08, 0x08);
		menu_win1_color=SDL_MapRGB(text_screen->format, 0x20, 0x10, 0x10);
		menu_barra0_color=SDL_MapRGB(text_screen->format, 0x30, 0x20, 0x20);
		menu_barra1_color=SDL_MapRGB(text_screen->format, 0x50, 0x40, 0x40);
	}
	menu_win0_color_base=menu_win0_color;
	menu_win1_color_base=menu_win1_color;
}

void menu_raise(void)
{
	int i;
	for(i=64;i>=0;i-=4)
	{
		Mix_VolumeMusic(96-(i<<1));
#ifndef DREAMCAST
		SDL_Delay(10);
#endif
		text_draw_background();
		fade16(text_screen,i);
		text_flip();
	}
}

void menu_unraise(void)
{
	int i;
	for(i=0;i<=64;i+=4)
	{
		Mix_VolumeMusic(96-(i<<1));
#ifndef DREAMCAST
		SDL_Delay(10);
#endif
		text_draw_background();
		fade16(text_screen,i);
		text_flip();
	}

}

static void text_draw_menu_msg()
{
/*
	write_text_pos(menu_msg_pos,0,menu_msg);
	if (menu_msg_pos<MAX_SCROLL_MSG)
		menu_msg_pos=330;
	else
		menu_msg_pos--;
*/
}


static void update_window_color(void)
{
	static int cambio=0;
	static int spin=0;

	Uint8 r,g,b;
	int cambio2=cambio>>3;
	SDL_GetRGB(menu_win0_color_base,text_screen->format,&r,&g,&b);
	if (((int)r)-cambio2>0) r-=cambio2;
	else r=0;
	if (((int)g)-cambio2>0) g-=cambio2;
	else g=0;
	if (((int)b)-cambio2>0) b-=cambio2;
	else b=0;
	menu_win0_color=SDL_MapRGB(text_screen->format,r,g,b);
	SDL_GetRGB(menu_win1_color_base,text_screen->format,&r,&g,&b);
	if (((int)r)-cambio>0) r-=cambio;
	else r=0;
	if (((int)g)-cambio>0) g-=cambio;
	else g=0;
	if (((int)b)-cambio>0) b-=cambio;
	else b=0;
	menu_win1_color=SDL_MapRGB(text_screen->format,r,g,b);
	if (spin)
	{
		if (cambio<=0) spin=0;
		else cambio-=2;

	}
	else
	{
		if (cambio>=24) spin=1;
		else cambio+=2;
	}
}

void text_draw_background()
{
#if !defined(SHOW_CONSOLE) && defined(MENU_ALPHA)
	if (!used_blitter)
	{
#endif
		draw_background(text_screen);
#if !defined(SHOW_CONSOLE) && defined(MENU_ALPHA)
	}
	else
	{
#ifdef USE_VIDEO_GL
		bzero(text_screen->pixels,text_screen->h*text_screen->pitch);
#else
		SDL_FillRect(text_screen,NULL,0);
#endif
	}
#endif

}

void text_flip(void)
{
#ifndef DREAMCAST
	SDL_Delay(10);
#endif
#ifdef MENU_ALPHA
	{
		unsigned i;
		for(i=0;i<screen->h;i++)
			memcpy(
				screen->pixels + (screen->pitch * i),
				text_screen->pixels + (text_screen->pitch * i),
				screen->w * screen->format->BytesPerPixel
			);
	}
#else
	SDL_BlitSurface(text_screen,NULL,screen,NULL);
#endif
	video_flip(screen);
}

SDL_Surface * menu_DisplayFormat (SDL_Surface *surface)
{
	Uint32 flags;

	if ( ! screen )
		return(NULL);
	flags = SDL_SWSURFACE;

#if 0
	flags |= surface->flags & (SDL_SRCCOLORKEY|SDL_SRCALPHA|SDL_RLEACCELOK);
#else
	flags |= (surface->flags & (SDL_SRCCOLORKEY|SDL_SRCALPHA));
	flags |= SDL_RLEACCELOK;
#endif
	return(SDL_ConvertSurface(surface, screen->format, flags));
}

SDL_Surface *menu_IMG_Load(char *filename)
{
	SDL_Surface *ret, *tmp=IMG_Load(filename);
	if (tmp==NULL)
	{
#ifdef DREAMCAST
		filename[1]='s';
		tmp=IMG_Load(filename);
		if (tmp==NULL)
#endif
		{
#ifdef STDOUTPUT
		printf("Unable to load '%s'\n",filename);
#endif
		exit(-2);
		}
	}
	ret=menu_DisplayFormat(tmp);
	SDL_FreeSurface(tmp);
	if (ret==NULL)
	{
#ifdef STDOUTPUT
		printf("Unable to convert '%s'\n",filename);
#endif
		exit(-3);
	}
	return ret;
}

static SDL_Surface *load_img_0(char *filename)
{
	SDL_Surface *ret=menu_IMG_Load(filename);
	SDL_SetColorKey(ret,(SDL_SRCCOLORKEY | SDL_RLEACCEL),SDL_MapRGB(text_screen -> format, 0, 0, 0));
	return ret;
}

static SDL_Surface *load_img_255(char *filename)
{
	SDL_Surface *ret=menu_IMG_Load(filename);
	SDL_SetColorKey(ret,(SDL_SRCCOLORKEY | SDL_RLEACCEL),SDL_MapRGB(text_screen -> format, 255, 255, 255));
	return ret;
}

void init_text(int splash)
{
	SDL_Event ev;
	int toexit=0;
	SDL_Surface *sur;
	SDL_Rect r;
	int i,j;

	if (!text_screen)
	{

//		text_screen=SDL_ConvertSurface(screen,screen->format,0);
		text_screen=SDL_CreateRGBSurface(screen->flags,screen->w,screen->h,screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,screen->format->Amask);
//		text_screen=SDL_DisplayFormat(screen);
		if (!text_screen)
		{
#ifdef STDOUTPUT
			puts("Unable to make a SDL_Surface (text_screen)");
#endif
			exit(-1);

		}
	}

#ifdef DREAMCAST
        __sdl_dc_emulate_keyboard=1;
#endif
	if (!splash)
	{
		menu_raise();
		return;
	}

	text_image2=load_img_0(MENU_FILE_TEXT2);
	font_inv.Surface=text_image2;
	SFont_InitFontInfo(&font_inv);

	text_image=load_img_0(MENU_FILE_TEXT);
	SFont_InitFont(text_image);

	text_win_uleft=load_img_255(MENU_FILE_WIN_ULEFT);
	text_win_uright=load_img_255(MENU_FILE_WIN_URIGHT);
	text_win_upper=load_img_255(MENU_FILE_WIN_UPPER);
	text_win_left=load_img_255(MENU_FILE_WIN_LEFT);
	text_win_right=load_img_255(MENU_FILE_WIN_RIGHT);
	text_win_bright=load_img_255(MENU_FILE_WIN_BRIGHT);
	text_win_bleft=load_img_255(MENU_FILE_WIN_BLEFT);
	text_win_bottom=load_img_255(MENU_FILE_WIN_BOTTOM);
	text_cdrom=menu_IMG_Load(MENU_FILE_LOADING);

	init_background();

	obten_colores();
	sound_play_menu_music();
#ifndef AUTO_EVENTS
	sur=menu_IMG_Load(MENU_FILE_SPLASH);
	r.x=(text_screen->w - sur->w)/2;
	r.y=(text_screen->h - sur->h)/2;
	r.h=sur->w;
	r.w=sur->h;
	SDL_FillRect(text_screen,NULL,0); //0xFFFFFFFF);
	while(SDL_PollEvent(&ev)) SDL_Delay(50);
	for (i=128;(i>-8)&&(!toexit);i-=8)
	{
#ifdef DREAMCAST
		vid_waitvbl();
#else
		SDL_Delay(50);
#endif
		SDL_FillRect(text_screen,NULL,0); //0xFFFFFFFF);
		SDL_BlitSurface(sur,NULL,text_screen,&r);
		fade16(text_screen,i);
		text_flip();
		while(SDL_PollEvent(&ev)) toexit=1;
	}
	for(i=0;(i<23)&&(!toexit);i++)
	{
		while(SDL_PollEvent(&ev)) toexit=1;
		SDL_Delay(100);
	}
	for(i=0;(i<128)&&(!toexit);i+=16)
	{
#ifdef DREAMCAST
		vid_waitvbl();
#else
		SDL_Delay(50);
#endif
		SDL_FillRect(text_screen,NULL,0); //0xFFFFFFFF);
		SDL_BlitSurface(sur,NULL,text_screen,&r);
		fade16(text_screen,i);
		text_flip();
		while(SDL_PollEvent(&ev)) toexit=1;
	}
	for(i=128;(i>-8)&&(!toexit);i-=8)
	{
#ifdef DREAMCAST
		vid_waitvbl();
#else
		SDL_Delay(50);
#endif
		text_draw_background();
		fade16(text_screen,i);
		text_flip();
		while(SDL_PollEvent(&ev)) toexit=1;
	}
	SDL_FreeSurface(sur);
#else
#ifndef DREAMCAST
	chdir(ROM_PREFIX);
#else
//	fs_chdir(ROM_PREFIX);
	fs_chdir("/");//ROM_PREFIX);
#endif
#endif
//	menu_msg_time=SDL_GetTicks();
}


void quit_text(void)
{
/*
	SDL_FreeSurface(text_image);
	SDL_FreeSurface(text_background);
	SDL_FreeSurface(text_window_background);
//	SDL_FreeSurface(text_screen);
	quit_background();
*/
}

/*
void write_text_pos(int x, int y, char * str)
{
  int i, c;
  SDL_Rect src, dest;

  for (i = 0; i < strlen(str); i++)
    {
      c = -1;

      if (str[i] >= '0' && str[i] <= '9')
	c = str[i] - '0';
      else if (str[i] >= 'A' && str[i] <= 'Z')
	c = str[i] - 'A' + 10;
      else if (str[i] >= 'a' && str[i] <= 'z')
	c = str[i] - 'a' + 36;
      else if (str[i] == '#')
	c = 62;
      else if (str[i] == '=')
	c = 63;
      else if (str[i] == '.')
	c = 64;
      else if (str[i] == '_')
	c = -2;
      else if (str[i] == '-')
	c = -3;
      else if (str[i] == '(')
	c = 65;
      else if (str[i] == ')')
	c = 66;

      if (c >= 0)
	{
	  src.x = c * 8;
	  src.y = 0;
	  src.w = 8;
	  src.h = 8;

	  dest.x = x + (i * 8);
	  dest.y = y;
	  dest.w = 8;
	  dest.h = 8;

	  SDL_BlitSurface(text_image, &src,
			  text_screen, &dest);
	}
      else if (c == -2 || c == -3)
	{
	  dest.x = x + (i * 8);

	  if (c == -2)
	    dest.y = y  + 7;
	  else if (c == -3)
	    dest.y = y  + 3;

	  dest.w = 8;
	  dest.h = 1;

	  SDL_FillRect(text_screen, &dest, menu_barra0_color);
	}
    }
}
*/

void write_text(int x, int y, char * str)
{
  SFont_PutString(text_screen, x*8, 1+(y*8), str);
}

#define exact_write_text(X,Y,STR) SFont_PutString(text_screen,(X),(Y),(STR))

/* Write text, inverted: */
void write_text_inv(int x, int y, char * str)
{
	SFont_PutStringInfo(text_screen, &font_inv, x*8, y*8, str);
}

void _write_text_inv_n(SDL_Surface *sf, int x, int y, int n, char * str)
{
	SDL_Rect dest;
	dest.x = (x * 8) -2 ;
	dest.y = (y * 8) /*10*/ - 2;
	dest.w = (n*8)+4;
	dest.h = 16;
	SDL_FillRect(sf, &dest, menu_inv_color);
	SFont_PutString(sf, x*8, y*8, str);
}

#define exact_write_text_inv(X,Y,STR) SFont_PutStringInfo(text_screen, &font_inv, (X), (Y), (STR) )

void write_text_sel(int x, int y, int w, char * str)
{
	int i,j,h=14;
	int x8=(x*8)-4;
	int y8=(y*8)+1;
#ifndef DREAMCAST
	SDL_LockSurface(text_screen);
#endif
	if ((y8+h)>text_screen->h)
		h=text_screen->h-y8;
	if (y8<text_screen->h)
	{
		register Uint16 *buf=(Uint16 *)text_screen->pixels;
		buf=(Uint16 *)&buf[x8+(y8*(text_screen->pitch/2))];
		register unsigned dx=(text_screen->pitch/2)-w;
		register int wmax=w/2;
		for(j=0;j<h;j++,buf+=dx)
		{
			for(i=0;i<wmax;i++,buf++)
				*buf++=TRANS_COLOR2;
			if (j&1)
				buf++;
			else
				buf--;
		}
	}
#ifndef DREAMCAST
	SDL_UnlockSurface(text_screen);
#endif
	write_text_inv(x,y,str);
}


/* Write text, horizontally centered... */

void write_centered_text(int y, char * str)
{
  write_text(20 - (strlen(str) / 2), y/2, str);
}


/* Write numbers on the option screen: */

void write_num(int x, int y, int v)
{
  char str[24];

  sprintf(str, "%d", v);
  write_text(x, y, str);
}

void write_num_inv(int x, int y, int v)
{
  char str[24];

  sprintf(str, "%d", v);
  write_text_inv(x, y, str);
}

void write_num_sel(int x, int y, int w, int v)
{
  char str[24];

  sprintf(str, "%d", v);
  write_text_sel(x, y, w, str);
}

void text_draw_barra(int x, int y, int w, int h, int per, int max)
{
	SDL_Rect dest;
	if (h>5) h-=4;
	dest.x=x-1;
	dest.y=y-1;
	dest.w=w+2;
	dest.h=h+2;
	SDL_FillRect(text_screen, &dest, 0xdddd); //menu_barra0_color);
	dest.x=x;
	dest.y=y;
	dest.h=h;
	dest.w=w;
	SDL_FillRect(text_screen, &dest, 0xffff);
	dest.x=x;
	dest.y=y;
	dest.h=h;
	dest.w=(w*per)/max;
	SDL_FillRect(text_screen, &dest, 0x8888); //menu_barra1_color);
}


void text_draw_window(int x, int y, int w, int h, char *title)
{
	int i,j;
	SDL_Rect dest;

	dest.x=x-6;
	dest.y=y-22;
	dest.w=80;
	dest.h=40;
	SDL_BlitSurface(text_win_uleft,NULL,text_screen,&dest);

	for(i=0;i<(w-80);i+=16)
	{
		dest.x=x-8+80+i;
		dest.y=y-22;
		dest.w=16;
		dest.h=40;
		SDL_BlitSurface(text_win_upper,NULL,text_screen,&dest);
	}

	dest.x=x+w-80+6;
	dest.y=y-22;
	dest.w=80;
	dest.h=40;
	SDL_BlitSurface(text_win_uright,NULL,text_screen,&dest);

	for(i=0;i<(h-16);i+=16)
	{
		dest.x=x-6;
		dest.y=y+i;
		dest.w=80;
		dest.h=16;
		SDL_BlitSurface(text_win_left,NULL,text_screen,&dest);

		dest.x=x+w-80+6;
		dest.y=y+i;
		dest.w=80;
		dest.h=16;
		SDL_BlitSurface(text_win_right,NULL,text_screen,&dest);
	}

	dest.x=x-6;
	dest.y=y+h-24;
	dest.w=80;
	dest.h=40;
	SDL_BlitSurface(text_win_bleft,NULL,text_screen,&dest);

	for(i=0;i<(w-80);i+=16)
	{
		dest.x=x-8+80+i;
		dest.y=y+h-24;
		dest.w=16;
		dest.h=40;
		SDL_BlitSurface(text_win_bottom,NULL,text_screen,&dest);
	}

	dest.x=x+w-80+6;
	dest.y=y+h-24;
	dest.w=80;
	dest.h=40;
	SDL_BlitSurface(text_win_bright,NULL,text_screen,&dest);

	exact_write_text_inv(x+72, y-18 , title);

	y+=4; h-=16;
	x+=2; w-=4;
#ifndef DREAMCAST
	SDL_LockSurface(text_screen);
#endif
	if ((y+h)>text_screen->h)
		h=text_screen->h-y;
	if (y<text_screen->h)
	{
		register Uint16 *buf=(Uint16 *)text_screen->pixels;
		buf=(Uint16 *)&buf[x+(y*(text_screen->pitch/2))];
		register unsigned dx=(text_screen->pitch/2)-w;
		register int wmax=w/2;
		for(j=0;j<h;j++,buf+=dx)
		{
			for(i=0;i<wmax;i++,buf++)
				*buf++=TRANS_COLOR;
			if (j&1)
				buf++;
			else
				buf--;
		}
	}
#ifndef DREAMCAST
	SDL_UnlockSurface(text_screen);
#endif

/*
	int i,j;
	int r8x = x / 8;
	int r8y = y / 8;
	int rx = r8x * 8;
	int ry = r8y * 8;
	int r32w =  w / 32;
	int r24h =  h / 24;
	int rw = r32w * 32;
	int rh = r24h * 24;
	int r8w = rw / 8;


	SDL_Rect dest;

	dest.x = rx - 2;
	dest.y = ry - 10;
	dest.w = rw + 4;
	dest.h = 10; //rh + 18;
	SDL_FillRect(text_screen, &dest, menu_win1_color);


	dest.x = rx - 2;
	dest.y = ry;
	dest.w = 2; //rw + 4;
	dest.h = rh;// + 14; //16;
	SDL_FillRect(text_screen, &dest, menu_win0_color);
	dest.x = rx - 2;
	dest.y = ry+rh;
	dest.w = rw+4; //rw + 4;
	dest.h = 2;// + 14; //16;
	SDL_FillRect(text_screen, &dest, menu_win0_color);
	dest.x = rx+rw;
	dest.y = ry;
	dest.w = 2; //rw + 4;
	dest.h = rh;// + 14; //16;
	SDL_FillRect(text_screen, &dest, menu_win0_color);

	write_text(r8x, r8y - 1, "OOO");
	write_text(r8x + ((r8w-strlen(title)) / 2), r8y - 1, title);

	r32w = w / text_window_background->w;
	r24h = h / text_window_background->h;
	dest.x=rx;
	dest.y=ry;
	dest.w=rw;//-4;
	dest.h=rh;//-8;
	SDL_SetClipRect(text_screen, &dest);
	for(i=0;i<4;i++)
		for(j=0;j<2;j++)
		{
			dest.x=rx+i*text_window_background->w;
			dest.y=ry+j*text_window_background->h;
			dest.w=text_window_background->w; //32;
			dest.h=text_window_background->h; //24;
			SDL_BlitSurface(text_window_background,NULL,text_screen,&dest);
		}
	SDL_SetClipRect(text_screen, NULL);
*/
}

void text_draw_loading(int per, int max)
{
	text_draw_background();
	text_draw_window(80,64,172,36,"Loading");
	text_draw_barra(84, (9*8)+1, 158, 16, per, max);
	write_text(14,9,"Please wait");
	text_flip();
}

void text_draw_cdload(void)
{
	SDL_Rect r;
	SDL_FillRect(text_screen,NULL,0);
	r.x=(text_screen->w-text_cdrom->w)-20;
	r.y=(text_screen->h-text_cdrom->h)-20;
	r.w=text_cdrom->w;
	r.h=text_cdrom->h;
	SDL_BlitSurface(text_cdrom,NULL,text_screen,&r);
	text_flip();
}

void text_draw_saving(int per, int max)
{
	text_draw_background();
	text_draw_window(80,64,172,36,"Saving");
	text_draw_barra(84, (9*8)+1, 158, 16, per, max);
	write_text(14,9,"Please wait");
	text_flip();
}


#endif
