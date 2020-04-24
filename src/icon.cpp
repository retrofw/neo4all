#include <stdio.h>
#include <SDL.h>

#ifndef DREAMCAST
#include "icon.h"
#else
#include <kos.h>
#include "vmu_icon.h"
#endif

void show_icon(void)
{
#ifndef DREAMCAST
	SDL_RWops	*iconfp;
	SDL_Surface *icon;

	//set window title
#ifdef AES
	SDL_WM_SetCaption("AES4ALL","AES4ALL");
#else
	SDL_WM_SetCaption("NEO4ALL","NEO4ALL");
#endif

	//Hide Mouse Pointer
	SDL_ShowCursor(SDL_DISABLE);
	//set up window icon
	iconfp=SDL_RWFromMem((void*)rawicon, sizeof(rawicon));
	icon=SDL_LoadBMP_RW(iconfp,0);
	SDL_WM_SetIcon(icon,NULL);
#else
	vmu_set_icon(vmu_icon);
#endif

}
