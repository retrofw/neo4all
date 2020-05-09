#include<stdio.h>
#include<SDL/SDL.h>

#ifdef SHOW_MENU

#ifdef AES
#define MENU_FILE_SPLASH DATA_PREFIX "splash_aes.png"
#else
#define MENU_FILE_SPLASH DATA_PREFIX "splash_cd.png"
#endif

#define MENU_FILE_LOADING DATA_PREFIX "loading.png"

#define MENU_DIR_DEFAULT "."

void text_draw_background();
void quit_text(void);
/*
void write_text(int x, int y, char * str);
void write_text_inv(int x, int y, char * str);
void write_text_sel(int x, int y, char * str);
void write_centered_text(int y, char * str);
void write_centered_text_inv(int y, char * str);
void write_num(int x, int y, int v);
void write_num_inv(int x, int y, int v);
void write_text_pos(int x, int y, char * str);
void write_text_pos_inv(int x, int y, char * str);
*/
void write_text(int x, int y, char * str);
void write_text_inv(int x, int y, char * str);
void write_text_sel(int x, int y, int w, char * str);
void write_centered_text(int y, char * str);
void write_num(int x, int y, int v);
void write_num_inv(int x, int y, int v);


void text_draw_barra(int x, int y, int w, int h, int per, int max);
void text_draw_window(int x, int y, int w, int h, char *title);
// void text_draw_menu_msg();
void text_flip(void);
void menu_raise(void);
void menu_unraise(void);


int run_mainMenu();
int run_menuLoad();
int run_menuCPU();
int run_menuControl();

#else
void text_draw_nomenu(void);
#endif

void init_text(int splash);
void text_draw_loading(int per, int max);
void text_draw_saving(int per, int max);
void drawNoBIOS(void);
void drawNoNeoGeoCD(void);
void drawNoCD(void);
void text_draw_cdload(void);
SDL_Surface * menu_DisplayFormat (SDL_Surface *surface);
SDL_Surface * menu_IMG_Load(char *filename);
