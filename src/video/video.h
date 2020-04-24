/*******************************************
**** VIDEO.H - Video Hardware Emulation ****
****            Header File             ****
*******************************************/

#ifndef	VIDEO_H
#define VIDEO_H

#include <SDL.h>

/*-- Defines ---------------------------------------------------------------*/
#define VIDEO_TEXT	0
#define VIDEO_NORMAL	1
#define	VIDEO_SCANLINES	2

/*-- Global Variables ------------------------------------------------------*/
extern unsigned	video_palette_selected;
extern char		*video_vidram;
extern unsigned short	*video_paletteram_ng;
extern unsigned short   video_palette_ng[4096*2];
#define video_palette_bank0_ng ((unsigned short *)&video_palette_ng[0])
#define video_palette_bank1_ng ((unsigned short *)&video_palette_ng[4096])
extern unsigned short	*video_paletteram_pc;
extern unsigned short   video_palette_pc[4096*2];
#define video_palette_bank0_pc ((unsigned short *)&video_palette_pc[0])
#define video_palette_bank1_pc ((unsigned short *)&video_palette_pc[4096])
extern short		video_modulo;
extern unsigned short	video_pointer;
extern unsigned short	*video_paletteram;
extern unsigned short	*video_paletteram_pc;
extern unsigned short	video_palette_bank0[4096];
extern unsigned short	video_palette_bank1[4096];
extern unsigned short	*video_line_ptr[224];
extern unsigned char	*video_fix_usage;
extern unsigned char	rom_fix_usage[4096];
extern unsigned char	video_spr_usage[32768];
extern unsigned char	rom_spr_usage[32768];
extern unsigned int	video_hide_fps;
extern unsigned short	video_color_lut[32768];
extern char	video_palette_use[0x200];
extern int	video_palette_dirty;
extern int	used_blitter;

extern SDL_Surface	*screen;
extern SDL_Surface	*video_buffer;
extern SDL_Surface	*game_title;
extern int		video_mode;
extern double		gamma_correction;
extern int		frameskip;

extern unsigned int	neogeo_frame_counter;
extern unsigned int	neogeo_frame_counter_speed;

/*-- video.c functions ----------------------------------------------------*/
int  video_init(void);
void video_reset(void);
void video_shutdown(void);
int  video_set_mode(int);
void video_draw_screen1(void);
void video_save_snapshot(void);
void video_draw_spr(unsigned int code, unsigned int color, int flipx,
			int flipy, int sx, int sy, int zx, int zy);
void video_setup(void);
void video_fullscreen_toggle(void);
void video_mode_toggle(void);
void incframeskip(void);
void video_flip(SDL_Surface *surface);
void video_final_flip(void);

void video_draw_tile_textures(void);
void video_draw_font_textures(void);
void video_draw_screen2();
void video_draw_blank(void);

void blitter(void);

void set_message(char *msg, int t);

/*-- draw_fix.c functions -------------------------------------------------*/
void video_draw_fix(void);
void fixputs(Uint16 x, Uint16 y, const char * string);

#endif /* VIDEO_H */

