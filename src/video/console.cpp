#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<SDL.h>

#include "menu.h"
#include "console.h"

int view_console=0;

#define NUM_BACKGROUNDS 2

extern SDL_Surface *screen;
static SDL_Surface *text_image=NULL, *window_background=NULL;
static SDL_Surface *background[NUM_BACKGROUNDS]={NULL,NULL};

static char *console_filename[NUM_BACKGROUNDS+2]={
	DATA_PREFIX "background0.bmp",
	DATA_PREFIX "background1.bmp",
	DATA_PREFIX "text.bmp",
	DATA_PREFIX "window.bmp"
};

static unsigned int console_y=0;
static unsigned int console_offset=0;

static char *console_str_separator="----------------------------------------";

void console_draw_background(int y, int h)
{
	if (view_console)
	{
		int i,x,x_max;
		for(i=y;i<y+h;i++)
		{
			SDL_Surface *back=background[(i+console_offset)&0x1];
			x_max=screen->w/back->w;
			for(x=0;x<x_max;x++)
			{
				SDL_Rect r;
				r.x=x*back->w;
				r.y=i*back->h;
				r.w=back->w;
				r.h=back->h;

				SDL_BlitSurface(back,NULL,screen,&r);
			}
//			SDL_UpdateRect(screen,0,i*back->h,screen->w,back->h);
		}
	}
}

void console_draw_all_background(void)
{
	console_draw_background(0,screen->h/background[0]->h);
}

void console_text_pos(int x, int y, char * str)
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
      else if (str[i] == '/')
	c = 65;
      else if (str[i] == ',')
	c = 66;
      else if (str[i] == ':')
	c = 67;
      
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
			  screen, &dest);
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
	  
	  SDL_FillRect(screen, &dest,
		       SDL_MapRGB(screen->format, 0xCC, 0xCC, 0xCC));
	}
    }
}

void console_text(int x, int y, char * str)
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
      else if (str[i] == '/')
	c = 65;
      else if (str[i] == ',')
	c = 66;
      else if (str[i] == ':')
	c = 67;
      
      if (c >= 0)
	{
	  src.x = c * 8;
	  src.y = 0;
	  src.w = 8;
	  src.h = 8;
	  
	  dest.x = (x + i) * 8;
	  dest.y = (y * 12) + 2;
	  dest.w = 8;
	  dest.h = 8;
	  
	  SDL_BlitSurface(text_image, &src,
			  screen, &dest);
	}
      else if (c == -2 || c == -3)
	{
	  dest.x = (x + i) * 8;
	  
	  if (c == -2)
	    dest.y = y * 12 /*10*/ + 7;
	  else if (c == -3)
	    dest.y = y * 12 /*10*/ + 3;
	  
	  dest.w = 8;
	  dest.h = 1;
	  
	  SDL_FillRect(screen, &dest,
		       SDL_MapRGB(screen->format, 0xCC, 0xCC, 0xCC));
	}
    }
}


/* Write text, inverted: */

void console_text_inv(int x, int y, char * str)
{
  SDL_Rect dest;
  
  
  dest.x = (x * 8) -2 ;
  dest.y = (y * 12) /*10*/ - 2;
  dest.w = (strlen(str) * 8) + 4;
  dest.h = 12;

  SDL_FillRect(screen, &dest, 0xCCCC); //SDL_MapRGB(screen->format, 32, 32, 64));

  console_text(x, y, str);
}


/* Write text, horizontally centered... */

void console_text_centered(int y, char * str)
{
  console_text(20 - (strlen(str) / 2), y/2, str);
}


/* Write numbers on the option screen: */

void console_text_num(int x, int y, int v)
{
  char str[24];
  
  sprintf(str, "%d", v);
  console_text(x, y, str);
}

void console_text_num_inv(int x, int y, int v)
{
  SDL_Rect dest;
  int i,l=1;

  for(i=10;i<1000000;i*=10)
	if (v/i)
		l++;
  	else
		break;
  	
  dest.x = (x * 8) -2 ;
  dest.y = (y * 12) /*10*/ - 2;
  dest.w = (l * 8) + 4;
  dest.h = 12;

  SDL_FillRect(screen, &dest, 0xCCCC); //SDL_MapRGB(screen->format, 32, 32, 64));

  console_text_num(x, y, v);
}


void console_draw_window(int x, int y, int w, int h, char *title)
{
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

	dest.x = rx + 6;
	dest.y = ry - 4;
	dest.w = rw + 6;
	dest.h = rh + 18;
	SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0x20, 0x20, 0x40));

	dest.x = rx - 2;
	dest.y = ry - 10; //12;
	dest.w = rw + 4;
	dest.h = rh + 14; //16;
	SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0x40, 0x40, 0x60));

	for(i=0;i<r32w;i++)
		for(j=0;j<r24h;j++)
		{
			dest.x=rx+i*32;
			dest.y=ry+j*24;
			dest.w=32;
			dest.h=24;
			SDL_BlitSurface(window_background,NULL,screen,&dest);
		}

	console_text(r8x, r8y - 1, "o -");
	console_text(r8x + r8w - 6, r8y - 1, "-- =ox");
	console_text(r8x + ((r8w-strlen(title)) / 2), r8y - 1, title);

}

void console_draw_line(char *str)
{
	unsigned int y=console_y;
	unsigned int y_max=(screen->h/background[0]->h);

	if (y>=y_max)
	{
		y=y_max-1;
#ifndef DREAMCAST
		SDL_LockSurface(screen);
#endif
		memcpy(screen->pixels,screen->pixels+(screen->pitch*background[0]->h),screen->pitch*(screen->h-background[0]->h));
#ifndef DREAMCAST
		SDL_UnlockSurface(screen);
#endif
//		SDL_UpdateRect(screen,0,0,screen->w,screen->h-background[0]->h);
		console_offset++;
	}
	console_draw_background(y,1);
	console_text(0,y,str);
//	SDL_UpdateRect(screen,0,y*background[0]->h,screen->w,background[0]->h);
	console_y++;
}

static char tmp_text[4096];

void console_puts(char *str_orig)
{
	char *str;
	unsigned int len=strlen(str_orig);
	unsigned int max_len=screen->w/8;

	str=(char *)tmp_text;
	strcpy(str,str_orig);
#ifdef STDOUTPUT
	puts(str);
#endif
	if (view_console)
	{
		while(len>max_len)
		{
			char ctmp=str[max_len];
			str[max_len]=0;
			console_draw_line(str);
			str[max_len]=ctmp;
			str+=max_len-4;
			str[0]=' ';
			str[1]=' ';
			str[2]=' ';
			str[3]=' ';
			len=strlen(str);
		}
		if (len>0)
			console_draw_line(str);
		video_flip(screen);
#ifndef DREAMCAST
		SDL_Delay(20);
#else
		SDL_Delay(10);
#endif
		video_flip(screen);
	}
}

void console_printf(const char *fmt, ...)
{
	char * ptr, * iptr ;
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(tmp_text, 4000, fmt, ap);
	va_end(ap);
	tmp_text[4001] = 0;

	console_puts(tmp_text);
}

void console_init(void)
{
	int i;

	for(i=0;i<NUM_BACKGROUNDS;i++)
		background[i]=menu_IMG_Load(console_filename[i]);

	text_image=menu_IMG_Load(console_filename[NUM_BACKGROUNDS]);
	SDL_SetColorKey(text_image,(SDL_SRCCOLORKEY | SDL_RLEACCEL),SDL_MapRGB(text_image -> format, 0, 0, 0));
	window_background=menu_IMG_Load(console_filename[NUM_BACKGROUNDS+1]);

	if (view_console)
	{
		console_draw_all_background();
//		console_puts(" - CONSOLE : " __DATE__ " " __TIME__);
		video_flip(screen);
	}
}

void console_wait(void)
{
    int salida=0;
    SDL_Event event;

    while(SDL_PollEvent(&event)) 
	    SDL_Delay(10);

    while(!salida)
    {
    	while(SDL_PollEvent(&event))
        	switch(event.type) {
			case SDL_QUIT:
			case SDL_KEYUP:
			case SDL_JOYBUTTONUP:
				salida=1;
				break;
		}
    	video_flip(screen);
	SDL_Delay(100);
    }
}

void console_println(void)
{
	view_console=1;
	console_puts(console_str_separator);
}

void console_pag(void)
{
	console_y=(screen->h/background[0]->h);
}


