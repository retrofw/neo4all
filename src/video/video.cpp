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

#include "console.h"

#ifdef USE_VIDEO_GL
#include "videogl.h"
#endif


//-- Defines -----------------------------------------------------------------
#define VIDEO_TEXT		0
#define VIDEO_NORMAL	1
#define	VIDEO_SCANLINES	2



//-- Global Variables --------------------------------------------------------
unsigned	video_palette_selected=0;
char *          video_vidram;
unsigned short	*video_paletteram_ng;
unsigned short   video_palette_ng[4096*2];
unsigned short	*video_paletteram_pc;
unsigned short   video_palette_pc[4096*2];
unsigned short	video_color_lut[32768];
char	video_palette_use[0x200];
int	video_palette_dirty=0;

short			video_modulo;
unsigned short	video_pointer;

#ifndef USE_VIDEO_GL
unsigned short	*video_line_ptr[224];
#endif

#ifndef AES
static unsigned char	_video_fix_usage[4096];
unsigned char *video_fix_usage=(unsigned char *)&_video_fix_usage[0];
unsigned char   rom_fix_usage[4096];
#else
unsigned char *video_fix_usage;
#endif

// unsigned      neogeo_nb_of_tiles;

#ifndef AES
unsigned char	video_spr_usage[32768];
#endif

SDL_Surface		*screen;
SDL_Surface		*video_buffer;
SDL_Surface		*game_title;
Uint8			*SrcPtr;
Uint8			*DestPtr;

unsigned int	neogeo_frame_counter = 0;
unsigned int	neogeo_frame_counter_speed = 4;

unsigned int	video_hide_fps=1;
int				video_scanlines_capable = 1;
double			gamma_correction = 1.0;

int				fullscreen_flag=0;
int				display_mode=1;
int				snap_no;
int				frameskip=0;

SDL_Rect		src,dest;

//-- Function Prototypes -----------------------------------------------------
int	video_init(void);
void	video_shutdown(void);
void	video_draw_line(int);
int	video_set_mode(int);
void	incframeskip(void);
void	video_precalc_lut(void);
void	video_flip_pages(void);
void	video_draw_spr(unsigned int code, unsigned int color, int flipx,
			int flipy, int sx, int sy, int zx, int zy);
void	video_draw_screen1(void);
void	video_draw_screen2(void);
void	snapshot_init(void);
void	video_save_snapshot(void);
void	video_setup(void);


void video_reset(void)
{

	video_precalc_lut();

	memset(video_palette_ng, 0, 8192*2);
	memset(video_palette_pc, 0, 8192*2);

	video_paletteram_ng = video_palette_bank0_ng;
	video_paletteram_pc = video_palette_bank0_pc;

	video_modulo = 0;
	video_pointer = 0;

	video_palette_dirty=0;
	memset(video_palette_use,0,0x200);
	video_palette_selected=0;

#ifndef AES
	unsigned i;
        for(i=0;i<32768;i++)
            video_spr_usage[i]=1;
#endif

#ifdef USE_VIDEO_GL
	video_reset_gl();
#endif
	init_autoframeskip();
}

//----------------------------------------------------------------------------
int	video_init(void)
{
	unsigned short	*ptr;

	video_precalc_lut();

	video_vidram = (char *)calloc(1, 131072);

	if (video_vidram==NULL) {
//		strcpy(global_error, "VIDEO: Could not allocate vidram (128k)");
		return	0;
	}

	video_reset();
	
	if (video_set_mode(VIDEO_NORMAL)==0)
		return 0;

#ifdef DOUBLE_BUFFER
//	video_buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 226, 16, 0xF800, 0x07E0, 0x001F, 0x0000);
	video_buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, screen->format->Amask);
#else
	video_buffer = screen;
#endif

	game_title   = SDL_CreateRGBSurface(SDL_SWSURFACE, 144,  80, 16, 0xF800, 0x07E0, 0x001F, 0x0000);

	ptr = (unsigned short *)(video_buffer->pixels);

#ifndef USE_VIDEO_GL
	int		y;
	for(y=0;y<224;y++) {
		video_line_ptr[y] = ptr;
		ptr += (video_buffer->pitch/2);
	}
#endif

	//dest={0,7,320,240};
	src.x=8;
	src.y=0;
	src.w=304;
	src.h=224;

	dest.x=166;
	dest.y=128;
	dest.w=304;
	dest.h=224;

	snapshot_init();
	console_init();

	return 1;
}

//----------------------------------------------------------------------------
void	video_shutdown(void)
{

	//if (video_buffer != NULL) SDL_FreeSurface( video_buffer );
	//if (game_title != NULL)   SDL_FreeSurface( game_title );

	//free(video_vidram);
	
}
#ifdef DREAMCAST
#include <SDL_dreamcast.h>
#endif

//----------------------------------------------------------------------------
int	video_set_mode(int mode)
{
#ifndef USE_VIDEO_GL
#ifdef DREAMCAST
//	if ((screen=SDL_SetVideoMode(320, 240, 16, SDL_HWPALETTE|SDL_HWSURFACE|SDL_DOUBLEBUF))==NULL) {
	if ((screen=SDL_SetVideoMode(320, 240, 16, SDL_HWPALETTE|SDL_HWSURFACE|SDL_FULLSCREEN|SDL_DOUBLEBUF))==NULL) {
//	if ((screen=SDL_SetVideoMode(320, 240, 16, SDL_HWPALETTE|SDL_HWSURFACE|SDL_FULLSCREEN))==NULL) {
//	if ((screen=SDL_SetVideoMode(512, 256, 16, SDL_HWPALETTE|SDL_HWSURFACE|SDL_DOUBLEBUF))==NULL) {
#else
#ifndef DINGUX
	if ((screen=SDL_SetVideoMode(320, 240, 16, SDL_HWPALETTE|SDL_HWSURFACE|SDL_DOUBLEBUF))==NULL) {
#else
	if ((screen=SDL_SetVideoMode(320, 240, 16, SDL_SWSURFACE))==NULL) {
#endif
#endif

#else
	if (!init_video_gl()) {
#endif
		fprintf( stderr, "Could not set video mode: %s\n", SDL_GetError() );

        	SDL_Quit();
        	exit( -1 );
		return 0;
	}
#ifdef DREAMCAST
#ifndef USE_VIDEO_GL
	SDL_DC_SetWindow(320,240);
	SDL_DC_VerticalWait(SDL_FALSE);
#endif
#endif

	return 1;
}


//----------------------------------------------------------------------------
void	video_precalc_lut(void)
{
	int	ndx, rr, rg, rb;
	int 	r1=0,r2=5,r3=11,mr=0,mult=63;
	static double back_gamma=-12345.32523;

	if (back_gamma==gamma_correction)
		return;
	back_gamma=gamma_correction;

#ifdef USE_VIDEO_GL
#ifdef DREAMCAST
	r1=0;
	r3=10;
#else
	r1=10;
	r3=0;
#endif
	r2=5;
	mult=31;
	mr=0x8000;
#endif

	memset(video_color_lut,1,32768);
	for(rr=0;rr<32;rr++) {
		for(rg=0;rg<32;rg++) {
			for(rb=0;rb<32;rb++) {
				ndx = ((rr&1)<<14)|((rg&1)<<13)|((rb&1)<<12)|((rr&30)<<7)
					|((rg&30)<<3)|((rb&30)>>1);
				video_color_lut[ndx] =
				     ((int)( 31 * pow( (double)rb / 31.0, 1.0 / gamma_correction ) )<<r1)
				        | mr
					|((int)( mult * pow( (double)rg / 31.0, 1.0 / gamma_correction ) )<<r2)
					|((int)( 31 * pow( (double)rr / 31.0, 1.0 / gamma_correction ) )<<r3);
			}
		}
	}
}


#ifndef DREAMCAST
void snapshot_init(void)
{
	/* This function sets up snapshot number */
	char name[30];
	FILE *fp;

	snap_no=0;

	sprintf(name,"snap%04d.bmp", snap_no);

	while((fp=fopen(name,"r"))!=NULL)
	{
		fclose(fp);
		snap_no++;
		sprintf(name,"snap%04d.bmp", snap_no);
	}
}

void video_save_snapshot(void)
{
	char name[30];
	sprintf(name,"snap%04d.bmp", snap_no++);
	SDL_SaveBMP(screen,name);
}

void video_fullscreen_toggle(void)
{
#ifndef USE_VIDEO_GL
	SDL_WM_ToggleFullScreen(screen);
#else
	video_fullscreen_toggle_gl();
#endif

}
#else
void snapshot_init(void) { }
void video_save_snapshot(void) { }
void video_fullscreen_toggle(void) { }
#endif

