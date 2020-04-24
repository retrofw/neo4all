#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "menu.h"

#include "sound/sound.h"

static char *text_str_title="CPU Settings";
static char *text_str_68k="68K CLK";
static char *text_str_z80="Z80 CLK";
static char *text_str_under="Under";
static char *text_str_normal="Normal";
static char *text_str_over="Over";
static char *text_str_accurate="Sync SFX";
static char *text_str_separator="~~~~~~~~~~~~~~~~~~~~~~";
static char *text_str_on="On";
static char *text_str_off="Off";
static char *text_str_exit="Main Menu (B)";


int menuCPU_68k=1;
int menuCPU_z80=1;
extern int	neogeo_accurate;
int menuCPU_case=-1;

enum { CPU_MENU_CASE_EXIT, CPU_MENU_CASE_CANCEL };

static inline void draw_cpuMenu(int c)
{
	text_draw_background();
	text_draw_window(40,24+32,260,200-16-64-32,text_str_title);

	if (c==0)
	{
		write_text_sel(6,8,252,text_str_68k);
		switch(menuCPU_68k)
		{
			case 0: write_text_inv(19,8,"||||||||||"); break; 
			case 1: write_text_inv(25,8,"||||||||||||"); break;
			default:
				write_text_inv(32,8,"||||||||"); break;
		}
		write_text_inv(19,8,text_str_under);
		write_text_inv(25,8,text_str_normal);
		write_text_inv(32,8,text_str_over);
	}
	else
	{
		write_text(6,8,text_str_68k);
		switch(menuCPU_68k)
		{
			case 0: write_text(19,8,"|||||||||"); break; 
			case 1: write_text(25,8,"|||||||||||"); break;
			default:
				write_text(32,8,"|||||||"); break;
		}
		write_text(19,8,text_str_under);
		write_text(25,8,text_str_normal);
		write_text(32,8,text_str_over);
	}

	if (c==1)
	{
		write_text_sel(6,10,252,text_str_z80);
		switch(menuCPU_z80)
		{
			case 0: write_text_inv(19,10,"||||||||||"); break; 
			case 1: write_text_inv(25,10,"||||||||||||"); break;
			default:
				write_text_inv(32,10,"||||||||"); break;
		}
		write_text_inv(19,10,text_str_under);
		write_text_inv(25,10,text_str_normal);
		write_text_inv(32,10,text_str_over);
	}
	else
	{
		write_text(6,10,text_str_z80);
		switch(menuCPU_z80)
		{
			case 0: write_text(19,10,"|||||||||"); break; 
			case 1: write_text(25,10,"|||||||||||"); break;
			default:
				write_text(32,10,"|||||||"); break;
		}
		write_text(19,10,text_str_under);
		write_text(25,10,text_str_normal);
		write_text(32,10,text_str_over);
	}

	if (c==2)
	{
		write_text_sel(6,12,252,text_str_accurate);
		if (neogeo_accurate)
			write_text_inv(25,12,"||||");
		else
			write_text_inv(19,12,"||||||");
		write_text_inv(19,12,text_str_off);
		write_text_inv(25,12,text_str_on);
	}
	else
	{
		write_text(6,12,text_str_accurate);
		if (neogeo_accurate)
			write_text(25,12,"||||");
		else
			write_text(19,12,"||||||");
		write_text(19,12,text_str_off);
		write_text(25,12,text_str_on);
	}

	write_text(6,13,text_str_separator);

	write_text(6,14,text_str_separator);
	if ((c==3)) //&&(bb))
		write_text_sel(6,15,252,text_str_exit);
	else
		write_text(6,15,text_str_exit);

	text_flip();
}

static inline int key_menuCPU(int *cp)
{
	int c=(*cp);
	int end=0;
	int left=0, right=0, up=0, down=0;
	int hit0=0, hit1=0, hit2=0, hit3=0, hit4=0, hit5=0;
	SDL_Event event;

	while (SDL_PollEvent(&event) > 0)
	{
		if (event.type == SDL_QUIT)
		{
			menuCPU_case=CPU_MENU_CASE_EXIT;
			end=-1;
		}
		else
		if (event.type == SDL_KEYDOWN)
		{
			sound_play_beep();
			switch(event.key.keysym.sym)
			{
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
				case SDLK_e:
				case SDLK_LCTRL: hit0=1; break;
				case SDLK_2:
				case SDLK_BACKSPACE: hit2=1; break;
				case SDLK_1:
				case SDLK_TAB: hit3=1; break;
				case SDLK_x:
				case SDLK_SPACE: hit4=1; break;
				case SDLK_c:
				case SDLK_LSHIFT: hit5=1; break;
				case SDLK_q:
				case SDLK_LALT: hit1=1; break;
			}
			if ((hit0)||(hit1)||(hit2)||(hit3)||(hit4)||(hit5))
			{
				menuCPU_case=CPU_MENU_CASE_EXIT;
				end=1;
			}
			else if (up)
			{
				if (c>0) c=(c-1)%4;
				else c=3;
			}
			else if (down)
				c=(c+1)%4;
			else
			switch(c)
			{
				case 0: // 68K
					if (left)
					{
						if (menuCPU_68k>0)
							menuCPU_68k--;
					}
					else if (right)
					{
						if (menuCPU_68k<2)
							menuCPU_68k++;
					}
					break;
				case 1:	// Z80
					if (left)
					{
						if (menuCPU_z80>0)
							menuCPU_z80--;
					}
					else if (right)
					{
						if (menuCPU_z80<2)
							menuCPU_z80++;
					}
					break;
				case 2:	// ACCURATE
					if (left)
					{
						if (neogeo_accurate)
							neogeo_accurate=0;
					}
					else if (right)
					{
						if (!neogeo_accurate)
							neogeo_accurate=1;
					}
					break;
				case 3:
					if (hit0)
					{
						menuCPU_case=CPU_MENU_CASE_EXIT;
						end=1;
					}
					break;
			}
		}
	}


	(*cp)=c;
	return end;
}

static inline void raise_menuCPU()
{
	int i;

	sound_play_beep();
	text_draw_background();
	text_flip();
	for(i=0;i<8;i++)
	{
		text_draw_background();
		text_draw_window(128-(8*i),32+((8-i)*24),144+(8*i),172-64-32,"");
		text_flip();
		SDL_Delay(15);
	}
}

static inline void unraise_menuCPU()
{
	int i;

	for(i=7;i>=0;i--)
	{
		text_draw_background();
		text_draw_window(128-(8*i),32+((8-i)*24),144+(8*i),160-64-32,"");
		text_flip();
		SDL_Delay(15);
	}
	text_draw_background();
	text_flip();
}

int run_menuCPU()
{
	static int c=0;
	int end;
	menuCPU_case=-1;
	while(menuCPU_case<0)
	{
		raise_menuCPU();
		end=0;
		while(!end)
		{
			draw_cpuMenu(c);
			end=key_menuCPU(&c);
		}
		unraise_menuCPU();
		switch(menuCPU_case)
		{
			case CPU_MENU_CASE_EXIT:	
			case CPU_MENU_CASE_CANCEL:	
				menuCPU_case=1;
				break;
			default:
				menuCPU_case=-1;
		}
	}

	return menuCPU_case;
}
