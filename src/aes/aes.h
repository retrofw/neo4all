#ifndef AES_H
#define AES_H

#ifndef AES_PREFETCHING
#ifdef SHOW_MENU
//#define AES4ALL_TOTAL_MEMORY ((11*1024*1024)-(256*1024))
#define AES4ALL_TOTAL_MEMORY ((11*1024*1024)+(64*1024))
#else
#define AES4ALL_TOTAL_MEMORY ((12*1024*1024)+(128*1024))
//#define AES4ALL_TOTAL_MEMORY ((12*1024*1024)+(320*1024))
#endif
#else
#define AES4ALL_TOTAL_MEMORY (10*1024*1024)
#endif

#define AES4ALL_PEN_USAGE_SIZE 0x10000

extern unsigned char aes4all_ram[0x10000];
extern unsigned char aes4all_nvram[0x10000];

extern unsigned char aes4all_nvram_lock;
extern unsigned int  aes4all_nvram_protection_hack;
extern unsigned int  aes4all_bksw_handler;
extern unsigned char aes4all_bksw_unscramble[6];
extern unsigned char aes4all_bksw_offset[64];
extern unsigned char aes4all_system;
extern unsigned aes4all_bankaddress;

extern void *aes4all_mmu;
extern void *aes4all_mmap;

extern char *aes4all_filename;

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

extern void *aes4all_memory_sm1;
extern unsigned aes4all_memory_sm1_init;
extern unsigned aes4all_memory_sm1_size;

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

extern unsigned char aes4all_pen_usage[AES4ALL_PEN_USAGE_SIZE];


extern unsigned aes4all_prefetch_init;
extern unsigned *aes4all_prefetch_bufffer;
extern void *aes4all_prealloc;

int aes4all_load(void);
int aes4all_prefetch_all(void);

int is_aes_file(char *dir, char *filename, char *realname);
int aes4all_mmap_prefetch(unsigned mem_init, unsigned mem_size, FILE *f);

void m68k_bankswitch(unsigned int address);

#endif
