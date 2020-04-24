#ifdef DREAMCAST
#include<kos.h>
#endif

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "neo4all.h"
#include "menu.h"

#include "sound/sound.h"

#if defined (DREAMCAST) && defined(AES_PREFETCHING)
#include "mmu_file/mmu_file.h"
#endif

#ifdef USE_VIDEO_GL
#include "video/videogl.h"
#endif


#ifdef DREAMCAST
#include <SDL_dreamcast.h>
#endif

#ifndef AES
static char *text_str_title="NEO4ALL RC-4";
#else
static char *text_str_title="AES4ALL BETA-4";
#endif

static char *text_str_cpu="CPU Settings";
static char *text_str_region="Region ";
static char *text_str_usa="USA";
static char *text_str_jap="JAP";
static char *text_str_eur="EUR";
static char *text_str_frameskip="Frameskip";
static char *text_str_0="0";
static char *text_str_1="1";
static char *text_str_2="2";
static char *text_str_3="3";
static char *text_str_4="4";
static char *text_str_5="5";
static char *text_str_auto="Auto";
static char *text_str_sound="Sound";
static char *text_str_on="On";
static char *text_str_off="Off";
static char *text_str_filter="Filter";
static char *text_str_filter_none="None";
static char *text_str_filter_bilinear="Bilinear";
static char *text_str_control="Edit Controls";
static char *text_str_cdaudio="CDDA";
static char *text_str_separator="~~~~~~~~~~~~~~~~~~~~~~";
#if !defined(AES) && !defined(CDISO)
static char *text_str_reset="Reset";
#else
static char *text_str_reset="Select Game";
#endif
static char *text_str_run="Run";
#ifdef DREAMCAST
static char *text_str_exit= "Reboot";
#else
static char *text_str_exit="Exit";
#endif

int mainMenu_filter=0;
int mainMenu_case=-1;

extern int	neogeo_frameskip;
extern char	neogeo_region;
extern int	neogeo_sound_enable;
extern int	menu_moving;
extern int	menuCPU_68k;
extern int	menuCPU_z80;
#ifndef AES
extern int	cdda_disabled;
#else
static int	cdda_disabled=1;
#endif

enum { MAIN_MENU_CASE_REBOOT, MAIN_MENU_CASE_LOAD, MAIN_MENU_CASE_RUN, MAIN_MENU_CASE_RESET, MAIN_MENU_CASE_CANCEL, MAIN_MENU_CASE_CPU, MAIN_MENU_CASE_CONTROL };


#if defined(DREAMCAST) && !defined(AES)
#include<dirent.h>
static int list_files(void)
{
	int found=0;
	DIR *d=opendir("/cd");
	unsigned i;
	while(d!=NULL && !found)
	{
		struct dirent *actual=readdir(d);
		if (actual==NULL)
			break;
		found++;
	}
	closedir(d);
	return found;
}
static int try_to_list_files(void)
{
	int rt;
	for(rt=0;(rt<20)&&(!list_files());rt++)
		SDL_Delay(500);
	return list_files();
}
#endif

static inline void draw_mainMenu(int c)
{
//	static int b=0;
//	int bb=(b%9)/3;

	text_draw_background();
	text_draw_window(40,24,260,200,text_str_title);
//	write_text(6,3,text_str_separator);

	if (c==0)
	{
		write_text_sel(6,4,252,text_str_region);
		switch(neogeo_region)
		{
			case 0: write_text_inv(19,4,"||||||");break;
			case 1: write_text_inv(25,4,"||||||");break;
			default:
				write_text_inv(31,4,"||||||");break;
		}
		write_text_inv(19,4,text_str_jap);
		write_text_inv(25,4,text_str_usa);
		write_text_inv(31,4,text_str_eur);
	}
	else
	{
		write_text(6,4,text_str_region);
		switch(neogeo_region)
		{
			case 0: write_text(19,4,"||||||");break;
			case 1: write_text(25,4,"||||||");break;
			default:
				write_text(31,4,"||||||");break;
		}
		write_text(19,4,text_str_jap);
		write_text(25,4,text_str_usa);
		write_text(31,4,text_str_eur);
	}

	if (c==1)
	{
		write_text_sel(6,6,252,text_str_frameskip);
		switch(neogeo_frameskip)
		{
			case 0: write_text_inv(19,6,"||"); break;
			case 1: write_text_inv(21,6,"||"); break;
			case 2: write_text_inv(23,6,"||"); break;
			case 3: write_text_inv(25,6,"||"); break;
			case 4: write_text_inv(27,6,"||"); break;
			case 5: write_text_inv(29,6,"||"); break;
			default:
				write_text_inv(31,6,"||||||||");
		}
		write_text_inv(19,6,text_str_0);
		write_text_inv(21,6,text_str_1);
		write_text_inv(23,6,text_str_2);
		write_text_inv(25,6,text_str_3);
		write_text_inv(27,6,text_str_4);
		write_text_inv(29,6,text_str_5);
		write_text_inv(31,6,text_str_auto);

	}
	else
	{
		write_text(6,6,text_str_frameskip);
		switch(neogeo_frameskip)
		{
			case 0: write_text(19,6,"||"); break;
			case 1: write_text(21,6,"||"); break;
			case 2: write_text(23,6,"||"); break;
			case 3: write_text(25,6,"||"); break;
			case 4: write_text(27,6,"||"); break;
			case 5: write_text(29,6,"||"); break;
			default:
				write_text(31,6,"|||||||");
		}
		write_text(19,6,text_str_0);
		write_text(21,6,text_str_1);
		write_text(23,6,text_str_2);
		write_text(25,6,text_str_3);
		write_text(27,6,text_str_4);
		write_text(29,6,text_str_5);
		write_text(31,6,text_str_auto);
	}

	if (c==2)
	{
		write_text_sel(6,8,252,text_str_sound);
		if (neogeo_sound_enable)
			write_text_inv(25,8,"||||");
		else
			write_text_inv(19,8,"||||||");
		write_text_inv(19,8,text_str_off);
		write_text_inv(25,8,text_str_on);
	}
	else
	{
		write_text(6,8,text_str_sound);
		if (neogeo_sound_enable)
			write_text(25,8,"||||");
		else
			write_text(19,8,"||||||");
		write_text(19,8,text_str_off);
		write_text(25,8,text_str_on);
	}

	if (c==3)
	{
		write_text_sel(6,10,252,text_str_cdaudio);
		if (!cdda_disabled)
			write_text_inv(25,10,"||||");
		else
			write_text_inv(19,10,"||||||");
		write_text_inv(19,10,text_str_off);
		write_text_inv(25,10,text_str_on);
	}
	else
	{
		write_text(6,10,text_str_cdaudio);
		if (!cdda_disabled)
			write_text(25,10,"||||");
		else
			write_text(19,10,"||||||");
		write_text(19,10,text_str_off);
		write_text(25,10,text_str_on);
	}

	if (c==4)
	{
		write_text_sel(6,12,252,text_str_filter);
		if (mainMenu_filter)
			write_text_inv(25,12,"||||||||||||||||");
		else
			write_text_inv(19,12,"||||||||");
		write_text_inv(19,12,text_str_filter_none);
		write_text_inv(25,12,text_str_filter_bilinear);
	}
	else
	{
		write_text(6,12,text_str_filter);
		if (mainMenu_filter)
			write_text(25,12,"||||||||||||||");
		else
			write_text(19,12,"|||||||");
		write_text(19,12,text_str_filter_none);
		write_text(25,12,text_str_filter_bilinear);
	}

	write_text(6,13,text_str_separator);

	write_text(6,14,text_str_separator);
	if ((c==5)) //&&(bb))
		write_text_sel(6,15,252,text_str_cpu);
	else
		write_text(6,15,text_str_cpu);

	if ((c==6)) //&&(bb))
		write_text_sel(6,17,252,text_str_control);
	else
		write_text(6,17,text_str_control);
	write_text(6,18,text_str_separator);

	write_text(6,19,text_str_separator);
	if ((c==7)) //&&(bb))
		write_text_sel(6,20,252,text_str_reset);
	else
		write_text(6,20,text_str_reset);

	if ((c==8)) //&&(bb))
		write_text_sel(6,22,252,text_str_run);
	else
		write_text(6,22,text_str_run);

	write_text(6,23,text_str_separator);
	write_text(6,24,text_str_separator);
	if ((c==9)) //&&(bb))
		write_text_sel(6,25,252,text_str_exit);
	else
		write_text(6,25,text_str_exit);
//	write_text(6,26,text_str_separator);

	text_flip();
//	b++;
}

static inline int key_mainMenu(int *cp)
{
	int c=(*cp);
	int end=0;
	int left=0, right=0, up=0, down=0, hit0=0, hit1=0;
	SDL_Event event;

	while (SDL_PollEvent(&event) > 0)
	{
		if (event.type == SDL_QUIT)
		{
			mainMenu_case=MAIN_MENU_CASE_REBOOT;
			end=-1;
		}
		else
		if (event.type == SDL_KEYDOWN)
		{
			sound_play_beep();
			switch(event.key.keysym.sym)
			{
				case SDLK_ESCAPE: mainMenu_case=MAIN_MENU_CASE_REBOOT; end=-1; break;
				case SDLK_d:
				case SDLK_RIGHT: right=1; break;
				case SDLK_a:
				case SDLK_LEFT: left=1; break;
				case SDLK_w:
				case SDLK_UP: up=1; break;
				case SDLK_s:
				case SDLK_DOWN: down=1; break;
				case SDLK_z:
				case SDLK_RETURN:
				case SDLK_q:
				case SDLK_LALT:
				case SDLK_e:
				case SDLK_LCTRL: hit0=1; break;
				case SDLK_1:
				case SDLK_TAB:
				case SDLK_BACKSPACE: hit1=1; break;
			}
			if (hit1)
			{
				mainMenu_case=MAIN_MENU_CASE_RUN;
				end=1;
			}
			else if (up)
			{
				if (c>0) c=(c-1)%10;
				else c=9;
			}
			else if (down)
				c=(c+1)%10;
			else
			switch(c)
			{
				case 0:	// REGION
					if (left)
					{
						if (neogeo_region>0)
							neogeo_region--;
					}
					else if (right)
					{
						if (neogeo_region<2)
							neogeo_region++;
					}
					break;
				case 1:	// FRAMESKIP
					if (left)
					{
						if (neogeo_frameskip>-1)
							neogeo_frameskip--;
						else
							neogeo_frameskip=5;
					}
					else if (right)
					{
						if (neogeo_frameskip<5)
							neogeo_frameskip++;
						else
							neogeo_frameskip=-1;
					}
					break;
				case 2:	// SOUND
					if (left)
					{
						if (neogeo_sound_enable)
							neogeo_sound_enable=0;
					}
					else if (right)
					{
						if (!neogeo_sound_enable)
							neogeo_sound_enable=1;
					}
					break;
				case 3:	// CDAUDIO
					if (left)
					{
						if (!cdda_disabled)
							cdda_disabled=1;
					}
					else if (right)
					{
						if (cdda_disabled)
							cdda_disabled=0;
					}
					break;
				case 4:	// FILTER
#ifdef USE_VIDEO_GL
					if (left)
					{
						if (mainMenu_filter)
							mainMenu_filter=0;
					}
					else if (right)
					{
						if (!mainMenu_filter)
							mainMenu_filter=1;
					}
					neogeo_adjust_filter(mainMenu_filter);
#endif
					break;
				case 5: // CPU MENU
					if (hit0)
					{
						mainMenu_case=MAIN_MENU_CASE_CPU;
						end=1;
					}
					break;
				case 6: // CONTROL MENU
					if (hit0)
					{
						mainMenu_case=MAIN_MENU_CASE_CONTROL;
						end=1;
					}
					break;
				case 7:	// RESET
					if (hit0)
					{
						mainMenu_case=MAIN_MENU_CASE_RESET;
						end=1;
					}
					break;
				case 8:	// RUN
					if (hit0)
					{
						mainMenu_case=MAIN_MENU_CASE_RUN;
						end=1;
					}
					break;
				case 9:// EXIT
					if (hit0)
					{
						mainMenu_case=MAIN_MENU_CASE_REBOOT;
						end=1;
					}
					break;
			}
		}
	}


	(*cp)=c;
	return end;
}

static inline void raise_mainMenu()
{
	int i;

	text_draw_background();
	text_flip();
	for(i=0;i<10;i++)
	{
		text_draw_background();
		text_draw_window(40,(10-i)*24,260,200,text_str_title);
		text_flip();
	}
}

static inline void unraise_mainMenu()
{
	int i;

	for(i=9;i>=0;i--)
	{
		text_draw_background();
		text_draw_window(40,(10-i)*24,260,200,text_str_title);
		text_flip();
	}
	text_draw_background();
	text_flip();
}

void drawNoCD(void)
{
	int i;
	sound_play_error();
	for(i=0;i<200;i++)
	{
		text_draw_background();
		text_draw_window(64,64,160,36,"ERROR");
#if !defined(AES) && !defined(CDISO)
		write_text(9,9,"No disk in drive");
#else
		write_text(9,9,"No game selected");
#endif
		text_flip();
	}
}

#ifndef AES
void drawNoNeoGeoCD(void)
{
	int i;
	sound_play_error();
	for(i=0;i<200;i++)
	{
		text_draw_background();
		text_draw_window(64,64,160,36,"ERROR");
		write_text(9,9,"No NeoGeo/CD disk");
		text_flip();
	}
}

void drawNoBIOS(void)
{
	int i;
	sound_play_error();
	for(i=0;i<400;i++)
	{
		text_draw_background();
		text_draw_window(48,64,224,36,"FATAL ERROR");
		write_text(8,9,"Could not load NEOCD.BIN");
		text_flip();
	}
}

#if 0
void drawNoRom(void)
{
	int i;
	SDL_Event ev;

	for(i=0;i<20;i++)
	{
		text_draw_background();
		text_draw_window(54,110,250,64,"ERROR");
		write_text(14,14,"ROM not found");
		write_text(8,16,"Press any button to retry");
		text_flip();
	}
	SDL_Delay(333);
    	while(SDL_PollEvent(&ev))
		if (ev.type==SDL_QUIT)
			exit(1);
		else
		    SDL_Delay(10);
    	while(!SDL_PollEvent(&ev))
		    SDL_Delay(10);
    	while(SDL_PollEvent(&ev))
		if (ev.type==SDL_QUIT)
			exit(1);
	text_draw_background();
	text_flip();
	SDL_Delay(333);
}
#endif

#endif

int run_mainMenu()
{
	static int c=8;
	int end,need_reset=-1;
	mainMenu_case=-1;
#ifdef DREAMCAST
	SDL_DC_VerticalWait(SDL_FALSE);
#endif
	sound_play_menu();
	while(mainMenu_case<0)
	{
		menu_moving=1;
		raise_mainMenu();
		end=0;
		while(!end)
		{
			draw_mainMenu(c);
			end=key_mainMenu(&c);
		}
		unraise_mainMenu();
		switch(mainMenu_case)
		{
/*
			case MAIN_MENU_CASE_LOAD:
//				run_menuLoad();	
				mainMenu_case=-1;
				break;
*/
			case MAIN_MENU_CASE_RUN:
#if defined(AES) || defined(CDISO)
				if (neogeo_yet_running)
				{
					if (need_reset>0)
						mainMenu_case=need_reset+1;
					else
						mainMenu_case=1;
				}
				else
					if (need_reset>0)
						mainMenu_case=2;
					else
					{
						mainMenu_case=-1;
						drawNoCD();
						break;
					}
#else
#if defined(DREAMCAST) && !defined(AES)
				if (!try_to_list_files())
				{
					mainMenu_case=-1;
					drawNoCD();
					break;
				}
#endif
				mainMenu_case=1;
#endif
				neogeo_adjust_frameskip(neogeo_frameskip);
				neogeo_adjust_cycles(menuCPU_68k, menuCPU_z80);
				break;

			case MAIN_MENU_CASE_CPU:
				run_menuCPU();
				mainMenu_case=-1;
				break;

			case MAIN_MENU_CASE_CONTROL:
				run_menuControl();
				mainMenu_case=-1;
				break;

			case MAIN_MENU_CASE_RESET:
#if defined(AES) || defined(CDISO)
				if (need_reset>0)
					run_menuLoad();
				else
					need_reset=run_menuLoad();
				mainMenu_case=-1;
#else
#if defined(DREAMCAST) && !defined(AES)
				if (!try_to_list_files())
				{
					mainMenu_case=-1;
					drawNoCD();
					break;
				}
#endif
				neogeo_adjust_frameskip(neogeo_frameskip);
				neogeo_adjust_cycles(menuCPU_68k, menuCPU_z80);
				if (neogeo_emulating)
					mainMenu_case=2;
				else
					mainMenu_case=1;
#endif
				break;
			case MAIN_MENU_CASE_REBOOT:
#if defined (DREAMCAST) && defined(AES_PREFETCHING)
				mmu_handle_dump_memaccess();
#endif
				sound_play_bye();
				menu_unraise();
				SDL_Delay(333);
				SDL_Quit();
#if defined(DREAMCAST) && defined(REBOOT_DREAMCAST)
				arch_reboot();
#else
				exit(0);
#endif
				break;
			default:
				mainMenu_case=-1;
		}
	}

/*
	text_draw_window(96,64,140,32,"-------");
	write_text(14,9,"Please wait");
	text_flip();
*/

#ifdef DREAMCAST
//	__sdl_dc_wait_vblank=mainMenu_vsync;
#endif
	menu_moving=0;
	text_draw_background();
	return mainMenu_case;
}
