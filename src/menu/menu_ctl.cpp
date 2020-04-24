#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "menu.h"

#include "sound/sound.h"

static char *text_str_title="Edit Controls";

static char *text_str_separator="~~~~~~~~~~~~~~~~~~~~~~~~~";
static char *text_str_joy1a="NeoGeo Joy1 A:";
static char *text_str_joy1b="NeoGeo Joy1 B:";
static char *text_str_joy1c="NeoGeo Joy1 C:";
static char *text_str_joy1d="NeoGeo Joy1 D:";
static char *text_str_joy2a="NeoGeo Joy2 A:";
static char *text_str_joy2b="NeoGeo Joy2 B:";
static char *text_str_joy2c="NeoGeo Joy2 C:";
static char *text_str_joy2d="NeoGeo Joy2 D:";
static char *text_str_joy[]={ text_str_joy1a, text_str_joy1b, text_str_joy1c, text_str_joy1d, text_str_joy2a, text_str_joy2b, text_str_joy2c, text_str_joy2d };
static char *text_str_buta="A";
static char *text_str_butb="B";
static char *text_str_butx="X";
static char *text_str_buty="Y";
static char *text_str_exit="Main Menu (B)";

SDLKey menuControl_bt0[6]=
{ SDLK_LCTRL, SDLK_LALT, SDLK_LSHIFT, SDLK_SPACE, SDLK_RETURN, SDLK_TAB };

SDLKey menuControl_bt1[6]=
{ SDLK_e, SDLK_q, SDLK_c, SDLK_x, SDLK_z, SDLK_1 };

int menuControl_case=-1;

enum { CONTROL_MENU_CASE_EXIT, CONTROL_MENU_CASE_CANCEL };

static inline void draw_controlMenu(int c)
{
	int i;

	text_draw_background();
	text_draw_window(16,24+10,260+32,200-32,text_str_title);
	write_text(3,12,text_str_separator);

	for (i=0;i<8;i++)
	{
		int y=5+(i*2);
		SDLKey *menuControl_bt;
		if (i<4)
			menuControl_bt=(SDLKey *)&menuControl_bt0;
		else
			menuControl_bt=(SDLKey *)&menuControl_bt1;
		if (c==i)
		{
			write_text_sel(3,y,252+32,text_str_joy[i]);
			switch(menuControl_bt[i&3])
			{
			case SDLK_e:
			case SDLK_LCTRL: write_text_inv(21,y,"||||||"); break; 
			case SDLK_q:
			case SDLK_LALT:  write_text_inv(25,y,"||||||"); break;
			case SDLK_c:
			case SDLK_LSHIFT:write_text_inv(29,y,"||||||"); break;
			default:
					 write_text_inv(33,y,"||||||"); break;
			}
			write_text_inv(22,y,text_str_buta);
			write_text_inv(26,y,text_str_butb);
			write_text_inv(30,y,text_str_butx);
			write_text_inv(34,y,text_str_buty);
		}
		else
		{
			write_text(3,y,text_str_joy[i]);
			switch(menuControl_bt[i&3])
			{
			case SDLK_e:
			case SDLK_LCTRL: write_text(21,y,"||||||"); break; 
			case SDLK_q:
			case SDLK_LALT:  write_text(25,y,"||||||"); break;
			case SDLK_c:
			case SDLK_LSHIFT:write_text(29,y,"||||||"); break;
			default:
					 write_text(33,y,"||||||"); break;
			}
			write_text(22,y,text_str_buta);
			write_text(26,y,text_str_butb);
			write_text(30,y,text_str_butx);
			write_text(34,y,text_str_buty);
		}
	}
			
			

	write_text(3,20,text_str_separator);

	write_text(3,21,text_str_separator);
	if ((c==8)) //&&(bb))
		write_text_sel(3,22,252+32,text_str_exit);
	else
		write_text(3,22,text_str_exit);

	text_flip();
}

static inline int key_menuControl(int *cp)
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
			menuControl_case=CONTROL_MENU_CASE_EXIT;
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
				menuControl_case=CONTROL_MENU_CASE_EXIT;
				end=1;
			}
			else if (up)
			{
				if (c>0) c=(c-1)%9;
				else c=8;
			}
			else if (down)
				c=(c+1)%9;
			else if (right)
			{
				if (c<4)
				{
					switch(menuControl_bt0[c])
					{
						case SDLK_LCTRL: menuControl_bt0[c]=SDLK_LALT; break;
						case SDLK_LALT: menuControl_bt0[c]=SDLK_LSHIFT; break;
						case SDLK_LSHIFT: menuControl_bt0[c]=SDLK_SPACE; break;
						default: menuControl_bt0[c]=SDLK_LCTRL; break;
					}
				}
				else if (c<8)
				{
					int c1=c-4;
					switch(menuControl_bt1[c1])
					{
						case SDLK_e: menuControl_bt1[c1]=SDLK_q; break;
						case SDLK_q: menuControl_bt1[c1]=SDLK_c; break;
						case SDLK_c: menuControl_bt1[c1]=SDLK_x; break;
						default: menuControl_bt1[c1]=SDLK_e; break;
					}
				}
			}
			else if (left)
			{
				if (c<4)
				{
					switch(menuControl_bt0[c])
					{
						case SDLK_LCTRL: menuControl_bt0[c]=SDLK_SPACE; break;
						case SDLK_LALT: menuControl_bt0[c]=SDLK_LCTRL; break;
						case SDLK_LSHIFT: menuControl_bt0[c]=SDLK_LALT; break;
						default: menuControl_bt0[c]=SDLK_LSHIFT; break;
					}
				}
				else if (c<8)
				{
					int c1=c-4;
					switch(menuControl_bt1[c1])
					{
						case SDLK_e: menuControl_bt1[c1]=SDLK_x; break;
						case SDLK_q: menuControl_bt1[c1]=SDLK_e; break;
						case SDLK_c: menuControl_bt1[c1]=SDLK_q; break;
						default: menuControl_bt1[c1]=SDLK_c; break;
					}
				}
			}
		}
	}


	(*cp)=c;
	return end;
}

static inline void raise_menuControl()
{
	int i;

	sound_play_beep();
	text_draw_background();
	text_flip();
	for(i=0;i<8;i++)
	{
		text_draw_background();
		text_draw_window(128-16-(8*i),16+16+((8-i)*24),144+(8*i),172,"");
		text_flip();
		SDL_Delay(15);
	}
}

static inline void unraise_menuControl()
{
	int i;

	for(i=7;i>=0;i--)
	{
		text_draw_background();
		text_draw_window(128-16-(8*i),16+16+((8-i)*24),144+(8*i),160,"");
		text_flip();
		SDL_Delay(15);
	}
	text_draw_background();
	text_flip();
}

int run_menuControl()
{
	static int c=0;
	int end;
	menuControl_case=-1;
	while(menuControl_case<0)
	{
		raise_menuControl();
		end=0;
		while(!end)
		{
			draw_controlMenu(c);
			end=key_menuControl(&c);
		}
		unraise_menuControl();
		switch(menuControl_case)
		{
			case CONTROL_MENU_CASE_EXIT:	
			case CONTROL_MENU_CASE_CANCEL:	
				menuControl_case=1;
				break;
			default:
				menuControl_case=-1;
		}
	}

	input_reinit();
	return menuControl_case;
}
