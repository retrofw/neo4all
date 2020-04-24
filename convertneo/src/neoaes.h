
extern void *aes4all_memory;
extern unsigned aes4all_memory_total;
extern unsigned aes4all_memory_nb_of_tiles;

extern void *aes4all_memory_cpu;
extern unsigned aes4all_memory_cpu_init;
extern unsigned aes4all_memory_cpu_size;

extern void *aes4all_memory_bios;
extern unsigned aes4all_memory_bios_init;
extern unsigned aes4all_memory_bios_size;

extern void *aes4all_memory_sfix_game;
extern unsigned aes4all_memory_sfix_game_init;
extern unsigned aes4all_memory_sfix_game_size;

extern void *aes4all_memory_sfix_board;
extern unsigned aes4all_memory_sfix_board_init;
extern unsigned aes4all_memory_sfix_board_size;

extern void *aes4all_memory_sound1;
extern unsigned aes4all_memory_sound1_init;
extern unsigned aes4all_memory_sound1_size;

extern void *aes4all_memory_sound2;
extern unsigned aes4all_memory_sound2_init;
extern unsigned aes4all_memory_sound2_size;

extern void *aes4all_memory_gfx;
extern unsigned aes4all_memory_gfx_init;
extern unsigned aes4all_memory_gfx_size;

extern void *aes4all_memory_ng_lo;
extern unsigned aes4all_memory_ng_lo_init;
extern unsigned aes4all_memory_ng_lo_size;

extern void *aes4all_memory_fix_board_usage;
extern unsigned aes4all_memory_fix_board_usage_init;
extern unsigned aes4all_memory_fix_board_usage_size;

extern void *aes4all_memory_fix_game_usage;
extern unsigned aes4all_memory_fix_game_usage_init;
extern unsigned aes4all_memory_fix_game_usage_size;

extern void *aes4all_memory_pen_usage;
extern unsigned aes4all_memory_pen_usage_init;
extern unsigned aes4all_memory_pen_usage_size;

int load_aes(char *filename);
int aes4all_prefetch_all(void);
