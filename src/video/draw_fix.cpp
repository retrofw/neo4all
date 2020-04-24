/******************************************
**** Fixed Text Layer Drawing Routines ****
******************************************/

#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "video.h"
#include "../neo4all.h"

#ifndef INLINE
#define INLINE static __inline__
#endif

#if 1
//ndef AES
/* Draw Single FIX character */
INLINE void draw_fix(Uint16 code, Uint16 colour, Uint16 sx, Uint16 sy, Uint16 * palette, char * fix_memory)
{
	Uint8 y;
	Uint32 mydword;
	Uint32 * fix=(Uint32*)&(fix_memory[code<<5]);
	Uint16 * dest;
	Uint16 * paldata=&palette[colour];
	Uint16 col;

	for(y=0;y<8;y++)
	{
		dest    = video_line_ptr[sy+y]+sx;
		mydword  = *fix++;
		
		col = (mydword>> 0)&0x0f; if (col) dest[0] = paldata[col];
		col = (mydword>> 4)&0x0f; if (col) dest[1] = paldata[col];
		col = (mydword>> 8)&0x0f; if (col) dest[2] = paldata[col];
		col = (mydword>>12)&0x0f; if (col) dest[3] = paldata[col];
		col = (mydword>>16)&0x0f; if (col) dest[4] = paldata[col];
		col = (mydword>>20)&0x0f; if (col) dest[5] = paldata[col];
		col = (mydword>>24)&0x0f; if (col) dest[6] = paldata[col];
		col = (mydword>>28)&0x0f; if (col) dest[7] = paldata[col];
	}
}


/* Draw entire Character Foreground */
void video_draw_fix(void)
{
	Uint16 x, y;
	Uint16 code, colour;
	Uint16 * fixarea=(Uint16 *)&video_vidram[0xe004];

#ifndef DREAMCAST
	if(SDL_MUSTLOCK(video_buffer))
		SDL_LockSurface(video_buffer);
#endif

	for (y=0; y < 28; y++)
	{
		for (x = 0; x < 40; x++)
		{
			code = fixarea[x << 5];

			colour = (code&0xf000)>>8;
			code  &= 0xfff;

			if(video_fix_usage[code])
				draw_fix(code,colour,(x<<3),(y<<3), video_paletteram_pc, neogeo_fix_memory);
		}
		fixarea++;
	}
#ifndef DREAMCAST
	if(SDL_MUSTLOCK(video_buffer))
		SDL_UnlockSurface(video_buffer);
#endif
}



/* FIX palette for fixputs*/
Uint16 palette[16]={0x0000,0xffff,0x0000,0x0000,
		    0x0000,0x0000,0x0000,0x0000,
		    0xffff,0x0000,0x0000,0x0000,
		    0x0000,0x0000,0x0000,0xffff};

void fixputs( Uint16 x, Uint16 y, const char * string )
{
	Uint8 i;
	int length=strlen(string);
	
	if ( y>27 ) return;
	
	if ( x+length > 40 ) {
		length=40-x;
	}
	
	if (length<0) return;

#ifndef DREAMCAST
	if(SDL_MUSTLOCK(video_buffer))
		SDL_LockSurface(video_buffer);
#endif
	
	y<<=3;
		
	for (i=0; i<length; i++) {	
		draw_fix(toupper(string[i])+0x300,0,(x+i)<<3,y,palette, &neogeo_rom_memory[458752]);
	}
	
	
#ifndef DREAMCAST
	if(SDL_MUSTLOCK(video_buffer))
		SDL_UnlockSurface(video_buffer);
#endif

	return;
}

#else
void video_draw_fix(void){ }
#endif // AES
