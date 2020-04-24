#include"fade.h"

#ifdef SHOW_MENU

void fade16(SDL_Surface *screen, unsigned short n)
{
	int i,total=screen->pitch*screen->h/2;
	SDL_LockSurface(screen);
	unsigned short rs=screen->format->Rshift;
	unsigned short gs=screen->format->Gshift;
	unsigned short bs=screen->format->Bshift;
	unsigned short rm=screen->format->Rmask;
	unsigned short gm=screen->format->Gmask;
	unsigned short bm=screen->format->Bmask;
	unsigned short * buff=(unsigned short*)screen->pixels;
	for(i=0;i<total;i++)
	{
		register unsigned short r=(buff[i]&rm)>>rs;
		register unsigned short g=(buff[i]&gm)>>gs;
		register unsigned short b=(buff[i]&bm)>>bs;
		if (r>n)
			r-=n;
		else
			r=0;
		if (g>n)
			g-=n;
		else
			g=0;
		if (b>n)
			b-=n;
		else
			b=0;
		buff[i]=(
				(r<<rs) | (g<<gs) | (b<<bs)
#if defined(USE_VIDEO_GL) && defined(MENU_ALPHA)
				| 0x8000
#endif
			);
	}
	SDL_UnlockSurface(screen);
}


#else

void fade16(SDL_Surface *screen, unsigned short n)
{
	int i,j,total_x=(screen->pitch/2),total_y=screen->h;
	SDL_LockSurface(screen);
	unsigned short rs=screen->format->Rshift;
	unsigned short gs=screen->format->Gshift;
	unsigned short bs=screen->format->Bshift;
	unsigned short rm=screen->format->Rmask;
	unsigned short gm=screen->format->Gmask;
	unsigned short bm=screen->format->Bmask;
	for(j=0;j<total_y;j++)
	{
	unsigned short *buff=(unsigned short*)(((unsigned)screen->pixels)+(screen->pitch*j));
	for(i=0;i<total_x;i++)
	{
		register unsigned short r=(buff[i]&rm)>>rs;
		register unsigned short g=(buff[i]&gm)>>gs;
		register unsigned short b=(buff[i]&bm)>>bs;
		if (r>n)
			r-=n;
		else
			r=0;
		if (g>n)
			g-=n;
		else
			g=0;
		if (b>n)
			b-=n;
		else
			b=0;
		buff[i]=(
				(r<<rs) | (g<<gs) | (b<<bs)
#if defined(USE_VIDEO_GL) && defined(MENU_ALPHA)
				| 0x8000
#endif
			);
	}
	}
	SDL_UnlockSurface(screen);
}

#endif
