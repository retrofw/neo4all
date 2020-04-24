#ifdef SHOW_CONSOLE
void console_draw_background(int y, int h);
void console_text_pos(int x, int y, char * str);
void console_text(int x, int y, char * str);
void console_text_inv(int x, int y, char * str);
void console_text_centered(int y, char * str);
void console_text_num(int x, int y, int v);
void console_text_num_inv(int x, int y, int v);
void console_draw_window(int x, int y, int w, int h, char *title);
void console_draw_line(char *str);
void console_puts(char *str_orig);
void console_printf(const char *fmt, ...);
void console_init(void);
void console_draw_all_background(void);
void console_println(void);
void console_wait(void);
void console_pag(void);

extern int view_console;

#else


#define console_draw_background(A,B)
#define console_text_pos(X, Y, STR)
#define console_text(X, Y, STR)
#define console_text_inv(X, Y, STR)
#define console_text_centered(Y, STR)
#define console_text_num(X, Y, V)
#define console_text_num_inv(X, Y, V)
#define console_draw_window(X, Y, W, H, T)
#ifdef STDOUTPUT
#define console_draw_line(STR) printf("\n------------------\n%s\n",STR)
#define console_puts(STR) puts(STR)
#define console_printf printf
#define console_init() puts("NO CONSOLE !")
#define console_println() puts("")
#else
#define console_draw_line(STR) 
#define console_puts(STR)
#define console_printf(...)
#define console_init() 
#define console_println()
#endif
#define console_draw_all_background()
#define console_wait()
#define console_pag()

//static int view_console;


#endif
