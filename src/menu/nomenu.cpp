
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<SDL.h>

#include <unistd.h>

#include "neo4all.h"
#include "fade.h"
#include "video/video.h"


extern char *autodir;
extern int neogeo_accurate;
extern int cdda_disabled;

static int exit_now=0;

char aes4all_actual_dir[128]= ROM_PREFIX "\0\0\0\0";

void neogeo_adjust_filter(int);

SDLKey menuControl_bt0[6]=
{ SDLK_LCTRL, SDLK_LALT, SDLK_LSHIFT, SDLK_SPACE, SDLK_RETURN, SDLK_TAB };

SDLKey menuControl_bt1[6]=
{ SDLK_e, SDLK_q, SDLK_c, SDLK_x, SDLK_z, SDLK_1 };


static void load_config(void)
{
	int fs=-1;
	int m68k_spd=NEO4ALL_68K_NORMAL_CYCLES;
        int z80_spd=NEO4ALL_Z80_NORMAL_CYCLES;

	FILE *f=fopen("config.cfg","rb");
	if (f)
	{
		char *buf=(char *)malloc(256);
		while(!feof(f))
		{
			int i,j,k,len;
			memset(buf,0,256);
			fgets(buf,255,f);
			len=strlen(buf);
			for(i=0,j=0,k=0;i<len;i++)
			{
				char c=buf[i];
				if (c=='=')
				{
					if (k)
						break;
					k=j+1;
					c=0;
				}
				else
				if (c>='a' && c<='z')
					c-='a'-'A';
				else
				if (c=='#')
					break;
				else
				if (c<'0' || c>'Z')
					continue;
				buf[j++]=c;
			}
			buf[j]=0;
			if (k && j)
			{
				char *val=(char *)&buf[k];
				len=k-1;
				if (!strcmp(buf,"FRAMESKIP"))
				{
					if (val[0]>='0' && val[0]<='9')
						fs=val[0]-'0';
				}
				else
				if (!strcmp(buf,"M68K"))
				{
					m68k_spd=(atoi(val))/60;
				}
				else
				if (!strcmp(buf,"Z80"))
				{
					z80_spd=(atoi(val))/60;
				}
				else
				if (!strcmp(buf,"REGION"))
				{
					if (!strcmp(val,"JAP"))
						neogeo_region=REGION_JAPAN;
					else
					if (!strcmp(val,"USA"))
						neogeo_region=REGION_USA;
					else
					if (!strcmp(val,"EUR"))
						neogeo_region=REGION_EUROPE;
				}
				else
				if (!strcmp(buf,"SOUND"))
				{
					if (!strcmp(val,"OFF"))
						neogeo_sound_enable=0;
				}
				else
				if (!strcmp(buf,"ACCURATE"))
				{
					if (!strcmp(val,"ON"))
						neogeo_accurate=1;
				}
#ifndef AES
				else
				if (!strcmp(buf,"CDDA"))
				{
					if (!strcmp(val,"OFF"))
						cdda_disabled=1;
					else
						cdda_disabled=0;
				}
#endif
			}
		}
		free(buf);
		fclose(f);
	}
	neogeo_adjust_frameskip(fs);
	neogeo_adjust_fine_cycles(m68k_spd, z80_spd);
}

void init_text(int splash)
{
#ifdef AES
	extern char neo4all_image_file[];
	strcpy(neo4all_image_file,"game.aes");
#endif
	chdir(autodir);
	neogeo_adjust_filter(0);
	if (splash)
	{
		SDL_Event event;
		SDL_Surface *tmp=SDL_LoadBMP("splash.bmp");

		load_config();
    		SDL_Delay(133);
    		while(SDL_PollEvent(&event)) 
	    		SDL_Delay(10);
		if (tmp!=NULL)
		{
			SDL_Rect r;
			int i;
			SDL_Surface *sur = SDL_DisplayFormat(tmp);
			SDL_FreeSurface(tmp);
			SDL_FillRect(screen,NULL,0);
			video_flip(screen);
			SDL_Delay(50);
			SDL_FillRect(screen,NULL,0);
			video_flip(screen);
			for (i=128;(i>=0)&&(!exit_now);i-=8)
			{
				SDL_Delay(20);
				r.x=(screen->w - sur->w)/2;
				r.y=(screen->h - sur->h)/2;
				r.w=sur->w;
				r.h=sur->h;
				SDL_BlitSurface(sur,NULL,screen,&r);
				fade16(screen,i);

				video_flip(screen);
    				while(SDL_PollEvent(&event)) 
					exit_now=1;
			}

			SDL_Delay(100);
			r.x=(screen->w - sur->w)/2;
			r.y=(screen->h - sur->h)/2;
			r.w=sur->w;
			r.h=sur->h;
			SDL_FillRect(screen,NULL,0);
			SDL_BlitSurface(sur,NULL,screen,&r);
			video_flip(screen);
			SDL_Delay(100);
			r.x=(screen->w - sur->w)/2;
			r.y=(screen->h - sur->h)/2;
			r.w=sur->w;
			r.h=sur->h;
			SDL_FillRect(screen,NULL,0);
			SDL_BlitSurface(sur,NULL,screen,&r);
			video_flip(screen);

			for(i=0;(i<12)&&(!exit_now);i++)
			{
    				while(SDL_PollEvent(&event)) 
					exit_now=1;
				SDL_Delay(100);
			}

			SDL_FreeSurface(sur);
		}
	}
}

void text_draw_loading(int per, int max)
{
	static Uint32 last=0;

	Uint32 now=SDL_GetTicks();

	if (now<last+50)
		return;
	last=now;

	SDL_Rect r;
	r.x=15;
	r.y=219;
	r.w=290;
	r.h=8;
	SDL_FillRect(screen,&r,0);

	r.x=16;
	r.y=220;
	r.h=6;
	r.w=(Uint16)((((288.0)*((double)per))/((double)max)));
	SDL_FillRect(screen,&r,(unsigned)-1);

	if (!exit_now)
	{
		video_flip(screen);
		SDL_Delay(50);
	}
}

void text_draw_saving(int per, int max)
{
}

void text_draw_nomenu(void)
{
	SDL_FillRect(screen,NULL,0);
	video_flip(screen);
	SDL_Delay(50);
}

#ifndef AES
void drawNoBIOS(void)
{
	SDL_Delay(333);
}
void drawNoNeoGeoCD(void)
{
	SDL_Delay(1333);
}
void drawNoCD(void)
{
	SDL_Delay(1333);
}
void text_draw_cdload(void)
{
}

#endif
