/********************************************
*   NeoCD Memory Mapping (C version)        *
*********************************************
* Fosters(2001,2003)                        *
********************************************/

// #define ONLY_CHANGES_PATCH

#include <SDL.h>
#include "SDL_endian.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "fame/fame.h"

#if defined(DREAMCAST) || defined(USE_FAME_C_CORE)
#define M68KCONTEXT m68kcontext
#else
#define M68KCONTEXT _m68kcontext
#endif

extern M68K_CONTEXT M68KCONTEXT;

#if defined(USE_FAME_C_CORE) && defined(FAME_DIRECT_MAPPING)
#define USE_SECURE_MEMSWITCH
#else
void famec_SetFetch(unsigned low_adr, unsigned high_adr, unsigned fetch_adr);
#endif

#include "z80/z80intrf.h"

#ifdef AES
#include "aes/aes.h"
#endif

//#define Z80_TIMESLICE_PER_NMI 192
#define Z80_TIMESLICE_PER_NMI 300
#define USE_NVRAM_READ_FUNC

#ifdef DEBUG_MEMORY
extern int trazando;
#endif

#include "console.h"

#include "../neo4all.h"

extern unsigned short    *video_paletteram_ng;
extern unsigned	video_palette_selected;


/*** Globals ***************************************************************************************/
int watchdog_counter=-1;
int memcard_write=0;

void   initialize_memmap(void);
unsigned int m68k_read_memory_8(unsigned int address);
unsigned int m68k_read_memory_16(unsigned int address);
unsigned int m68k_read_memory_32(unsigned int address);
void m68k_write_memory_8(unsigned int address, unsigned int value);
void m68k_write_memory_16(unsigned int address, unsigned int value);
void m68k_write_memory_32(unsigned int address, unsigned int value);


/***Helper Functions********************************************************************************/
static unsigned int	cpu_dummy_read		(unsigned int address);
static void		cpu_dummy_write		(unsigned int address, unsigned int value);
static void		cpu_z80_write_8		(unsigned int address, unsigned int value);
static void		cpu_videreg_write_8	(unsigned int address, unsigned int value);
static void		cpu_z80_write_16 	(unsigned int address, unsigned int value);
static void		cpu_pal_write_16 	(unsigned int address, unsigned int value);
static void		cpu_pal_write_8 	(unsigned int address, unsigned int value);
static unsigned int 	cpu_memcard_read_8 	(unsigned int offset);
static unsigned int 	cpu_memcard_read_16	(unsigned int offset);
static void   		cpu_memcard_write	(int,int);
static int    		cpu_vidreg_read_16	(int);
static int    		cpu_vidreg_read_8 	(int);
static void   		cpu_vidreg_write_16 	(int, int);
static void   		cpu_switch_write 	(int, int);
static int    		cpu_coin_read 		(int);
static void		cpu_pd4990_write_8	(unsigned int address, unsigned int value);
static void		cpu_pd4990_write_16	(unsigned int address, unsigned int value);

#ifndef AES
static int    		cpu_upload_read 	(int);
static void   		cpu_upload_write_8 	(int, int);
static void   		cpu_upload_write_16 	(int, int);
static void   		cpu_upload_write_32 	(int, int);
#else
static void   		cpu_nvram_write_8 	(unsigned int addr, unsigned char data);
static void   		cpu_nvram_write_16 	(unsigned int addr, unsigned short data);
#ifdef USE_NVRAM_READ_FUNC
static unsigned char	cpu_nvram_read_8 	(unsigned int addr);
static unsigned short	cpu_nvram_read_16 	(unsigned int addr);
#endif
static void 		cpu_bk_write		(unsigned int addr, unsigned int _data);
#endif

static unsigned int back_bankswitch_address=0xFFFFFFFF;


void cdda_control(void);

static void   cpu_watchdog_reset(void);

#ifndef AES

// NEOGEO CDROM

static M68K_PROGRAM neogeo_program[]= {
	{0x000000, 0x1FFFFF, (unsigned)&neogeo_prg_memory},		// 0
	{0xC00000, 0xC7FFFF, (((unsigned)&neogeo_rom_memory)-0xC00000)},// 1
	{(unsigned)-1,(unsigned)-1,(unsigned)NULL}
};
#else

// NEOGEO AES

static M68K_PROGRAM neogeo_program[]= {
	{0x000000, 0x0FFFFF, (unsigned)&neogeo_prg_memory},		// 0
	{0x100000, 0x10FFFF, (((unsigned)&aes4all_ram)-0x100000)},	// 1
	{0x200000, 0x2FFFFF, (unsigned)&neogeo_prg_memory},             // 2
	{0xC00000, 0xC1FFFF, (((unsigned)&neogeo_rom_memory)-0xC00000)},// 3
	{0xD00000, 0xD0ffff, (((unsigned)&aes4all_nvram)-0xD00000)},	// 4 
	{(unsigned)-1,(unsigned)-1,(unsigned)NULL}
};
#endif



unsigned int cpu_dummy_read(unsigned int address){
#ifdef DEBUG_MEMORY
if (trazando) printf("cpu_dummy_read(0x%X)\n",address);
#endif
	return 0;
}

void cpu_dummy_write(unsigned int address, unsigned int value){
#ifdef DEBUG_MEMORY
if (trazando) printf("cpu_dummy_write(0x%X,0x%X)\n",address,value);
#endif
}


#ifndef AES

// NEOGEO CDROM

#if !defined(DEBUG_MEMORY) && !defined(SINGLE_MEMORY)
static M68K_DATA neogeo_memmap_read_16[] = {
	{0x000000, 0x1FFFFF, NULL, (void *)&neogeo_prg_memory},			//0
	{0x200000, 0x3BFFFF, (void *)cpu_dummy_read, NULL},		//1
	{0x3C0000, 0x3CFFFF, (void *)cpu_vidreg_read_16, NULL},			//2
	{0x3D0000, 0x3FFFFF, (void *)cpu_dummy_read, NULL},		//3
	{0x400000, 0x401FFF, NULL, (void *)(((unsigned)&video_paletteram_ng)-0x400000)},	//4
	{0x402000, 0x7FFFFF, (void *)cpu_dummy_read, NULL},		//5
	{0x800000, 0x803FFF, (void *)cpu_memcard_read_16, NULL},		//6
	{0x804000, 0xBFFFFF, (void *)cpu_dummy_read, NULL},		//7
	{0xC00000, 0xC7FFFF, NULL, (void *)(((unsigned)&neogeo_rom_memory)-0xC00000)},	//8
	{0xC80000, 0xFFFFFF, (void *)cpu_dummy_read, NULL},		//9
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#else

#ifdef SINGLE_MEMORY
static M68K_DATA neogeo_memmap_read_16[] = {
	{0x000000, 0x1FFFFF, NULL, (void *)&neogeo_prg_memory},			//0
	{0x200000, 0xBFFFFF, (void *)m68k_read_memory_16, NULL},		//1
	{0xC00000, 0xC7FFFF, NULL, (void *)(((unsigned)&neogeo_rom_memory)-0xC00000)},  //2
	{0xC80000, 0xFFFFFF, (void *)m68k_read_memory_16, NULL},		//3
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#else

#warning neogeo_memmap_read_16 DEBUG MODE !!!!!!!
static M68K_DATA neogeo_memmap_read_16[] = {
	{0x000000, 0xFFFFFF, (void *)m68k_read_memory_16, NULL},	// 0
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#endif
#endif

#else

// NEOGEO AES

static M68K_DATA neogeo_memmap_read_16[] = {
	{0x000000, 0x0FFFFF, NULL, (void *)&neogeo_prg_memory},			// 0
	{0x100000, 0x10FFff, NULL, (void *)(((unsigned)&aes4all_ram)-0x100000)},// 1 
	{0x110000, 0x1FFFFF, (void *)cpu_dummy_read, NULL},             // 2
	{0x200000, 0x2FFFFF, NULL, (void *)&neogeo_prg_memory },		// 3
	{0x300000, 0x3BFFFF, (void *)cpu_dummy_read, NULL},		// 4
	{0x3C0000, 0x3CFFFF, (void *)cpu_vidreg_read_16, NULL},			// 5
	{0x3D0000, 0x3FFFFF, (void *)cpu_dummy_read, NULL},		// 6
	{0x400000, 0x401FFF, NULL, (void *)(((unsigned)&video_paletteram_ng)-0x400000)},// 7
	{0x402000, 0x7FFFFF, (void *)cpu_dummy_read, NULL},		// 8
	{0x800000, 0x803FFF, (void *)cpu_memcard_read_16, NULL},		// 9
	{0x804000, 0xBFFFFF, (void *)cpu_dummy_read, NULL},		// 10
	{0xC00000, 0xC1FFFF, NULL, (void *)(((unsigned)&neogeo_rom_memory)-0xC00000)},	// 11
	{0xC20000, 0xCFFFFF, (void *)cpu_dummy_read, NULL},		// 12
#ifdef USE_NVRAM_READ_FUNC
	{0xD00000, 0xD0ffff, (void *)cpu_nvram_read_16, NULL},	// 13
#else
	{0xD00000, 0xD0ffff, NULL, (void *)(((unsigned)&aes4all_nvram)-0xD00000)},	// 13
#endif
	{0xD10000, 0xFFFFFF, (void *)cpu_dummy_read, NULL},		// 14
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};

#endif



#ifndef AES

// NEOGEO CDROM

#if !defined(DEBUG_MEMORY) && !defined(SINGLE_MEMORY)
static M68K_DATA neogeo_memmap_read_8[] = {
	{0x000000, 0x1FFFFF, NULL, (void *)&neogeo_prg_memory},			// 0
	{0x200000, 0x2FFFFF, (void *)cpu_dummy_read, NULL},		// 1
	{0x300000, 0x30FFFF, (void *)read_player1, NULL},			// 2
	{0x310000, 0x31FFFF, (void *)cpu_dummy_read, NULL},		// 3
	{0x320000, 0x320FFF, (void *)cpu_coin_read, NULL},			// 4
	{0x321000, 0x33FFFF, (void *)cpu_dummy_read, NULL},		// 5
	{0x340000, 0x34FFFF, (void *)read_player2, NULL},			// 6
	{0x350000, 0x37FFFF, (void *)cpu_dummy_read, NULL},		// 7
	{0x380000, 0x38FFFF, (void *)read_pl12_startsel, NULL},			// 8
	{0x390000, 0x39FFFF, (void *)cpu_dummy_read, NULL},		// 9
	{0x400000, 0x401FFF, NULL, (void *)(((unsigned)&video_paletteram_ng)-0x400000)},	// 10
	{0x402000, 0x7FFFFF, (void *)cpu_dummy_read, NULL},		// 11
	{0x800000, 0x803FFF, (void *)cpu_memcard_read_8, NULL},		// 12
	{0x804000, 0xBFFFFF, (void *)cpu_dummy_read, NULL},		// 13
	{0xC00000, 0xC7FFFF, NULL, (void *)(((unsigned)&neogeo_rom_memory)-0xC00000)},	// 14
	{0xC80000, 0xDFFFFF, (void *)cpu_dummy_read, NULL},		// 15
	{0xE00000, 0xE3FFFF, (void *)cpu_upload_read, NULL},			// 16
	{0xE40000, 0xFFFFFF, (void *)cpu_dummy_read, NULL},		// 17
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#else

#ifdef SINGLE_MEMORY
static M68K_DATA neogeo_memmap_read_8[] = {
	{0x000000, 0x1FFFFF, NULL, (void *)&neogeo_prg_memory},			//0
	{0x200000, 0xBFFFFF, (void *)m68k_read_memory_8, NULL},		//1
	{0xC00000, 0xC7FFFF, NULL, (void *)(((unsigned)&neogeo_rom_memory)-0xC00000)},  //2
	{0xC80000, 0xFFFFFF, (void *)m68k_read_memory_8, NULL},		//3
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#else

#warning neogeo_memmap_read_8 DEBUG MODE !!!!!!!
static M68K_DATA neogeo_memmap_read_8[] = {
	{0x000000, 0xFFFFFF, (void *)m68k_read_memory_8, NULL},	// 0
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#endif
#endif

#else

// NEOGEO AES

static M68K_DATA neogeo_memmap_read_8[] = {
	{0x000000, 0x0FFFFF, NULL, (void *)&neogeo_prg_memory},			// 0
	{0x100000, 0x10FFFF, NULL, (void *)(((unsigned)&aes4all_ram)-0x100000)},// 1 
	{0x110000, 0x1FFFFF, (void *)cpu_dummy_read, NULL},             // 2
	{0x200000, 0x2FFFFF, NULL, (void *)&neogeo_prg_memory },		// 3
	{0x300000, 0x30FFFF, (void *)read_player1, NULL},			// 4
	{0x310000, 0x31FFFF, (void *)cpu_dummy_read, NULL},		// 5
	{0x320000, 0x320FFF, (void *)cpu_coin_read, NULL},			// 6
	{0x321000, 0x33FFFF, (void *)cpu_dummy_read, NULL},		// 7
	{0x340000, 0x34FFFF, (void *)read_player2, NULL},			// 8 
	{0x350000, 0x37FFFF, (void *)cpu_dummy_read, NULL},		// 9
	{0x380000, 0x38FFFF, (void *)read_pl12_startsel, NULL},			// 10
	{0x390000, 0x3BFFFF, (void *)cpu_dummy_read, NULL},		// 11
	{0x3C0000, 0x3C0FFF, (void *)cpu_vidreg_read_8, NULL},			// 12
	{0x3C1000, 0x3FFFFF, (void *)cpu_dummy_read, NULL},		// 13
	{0x400000, 0x401FFF, NULL, (void *)(((unsigned)&video_paletteram_ng)-0x400000)},// 14
	{0x402000, 0x7FFFFF, (void *)cpu_dummy_read, NULL},		// 15
	{0x800000, 0x803FFF, (void *)cpu_memcard_read_8, NULL},		// 16
	{0x804000, 0xBFFFFF, (void *)cpu_dummy_read, NULL},		// 17
	{0xC00000, 0xC1FFFF, NULL, (void *)(((unsigned)&neogeo_rom_memory)-0xC00000)},	// 18
	{0xC20000, 0xCFFFFF, (void *)cpu_dummy_read, NULL},		// 19
#ifdef USE_NVRAM_READ_FUNC
	{0xD00000, 0xD0ffff, (void *)cpu_nvram_read_8, NULL},	// 20
#else
	{0xD00000, 0xD0ffff, NULL, (void *)(((unsigned)&aes4all_nvram)-0xD00000)},	// 20
#endif
	{0xD10000, 0xFFFFFF, (void *)cpu_dummy_read, NULL},		// 21
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#endif



#ifndef AES

// NEOGEO CDROM

#if !defined(DEBUG_MEMORY) && !defined(SINGLE_MEMORY)
static M68K_DATA neogeo_memmap_write_16[] = {
	{0x000000, 0x1FFFFF, NULL, (void *)&neogeo_prg_memory},		// 0
	{0x200000, 0x31FFFF, (void *)cpu_dummy_write, NULL},		// 1
	{0x320000, 0x32FFFF, (void *)cpu_z80_write_16, NULL},		// 2
	{0x330000, 0x37FFFF, (void *)cpu_dummy_write, NULL},		// 3
	{0x380000, 0x380FFF, (void *)cpu_pd4990_write_16, NULL}, 	// 5
	{0x381000, 0x39FFFF, (void *)cpu_dummy_write, NULL},		// 6
	{0x3A0000, 0x3AFFFF, (void *)cpu_switch_write, NULL},		// 7
	{0x3B0000, 0x3BFFFF, (void *)cpu_dummy_write, NULL},		// 8
	{0x3C0000, 0x3CFFFF, (void *)cpu_vidreg_write_16, NULL},	// 9
	{0x3D0000, 0x3FFFFF, (void *)cpu_dummy_write, NULL},		// 10
	{0x400000, 0x401FFF, (void *)cpu_pal_write_16, NULL},		// 11
	{0x402000, 0x7FFFFF, (void *)cpu_dummy_write, NULL},		// 12
	{0x800000, 0x80FFFF, (void *)cpu_memcard_write, NULL},		// 13
	{0x810000, 0xDFFFFF, (void *)cpu_dummy_write, NULL},		// 14
	{0xE00000, 0xE3FFFF, (void *)cpu_upload_write_16, NULL},	// 15
	{0xE40000, 0xFFFFFF, (void *)cpu_dummy_write, NULL},		// 16
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#else

#ifdef SINGLE_MEMORY
static M68K_DATA neogeo_memmap_write_16[] = {
	{0x000000, 0x1FFFFF, NULL, (void *)&neogeo_prg_memory},			//0
	{0x200000, 0xFFFFFF, (void *)m68k_write_memory_16, NULL},		//1
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#else

#warning neogeo_memmap_write_16 DEBUG MODE !!!!!!!
static M68K_DATA neogeo_memmap_write_16[] = {
	{0x000000, 0xFFFFFF, (void *)m68k_write_memory_16, NULL},	// 0
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#endif
#endif

#else

// NEOGEO AES

static M68K_DATA neogeo_memmap_write_16[] = {
	{0x000000, 0x0FFFFF, (void *)cpu_dummy_write, NULL},			// 0
	{0x100000, 0x10FFFF, NULL, (void *)(((unsigned)&aes4all_ram)-0x100000)},// 1 
	{0x110000, 0x1FFFFF, (void *)cpu_dummy_write, NULL},			// 2
	{0x200000, 0x2FFFFF, (void *)cpu_bk_write, NULL},			// 3 
	{0x300000, 0x31FFFF, (void *)cpu_dummy_write, NULL},			// 4
	{0x320000, 0x32FFFF, (void *)cpu_z80_write_16, NULL},			// 5
	{0x330000, 0x37FFFF, (void *)cpu_dummy_write, NULL},			// 6
	{0x380000, 0x380FFF, (void *)cpu_pd4990_write_16, NULL}, 		// 7
	{0x381000, 0x39FFFF, (void *)cpu_dummy_write, NULL},			// 8
	{0x3A0000, 0x3AFFFF, (void *)cpu_switch_write, NULL},			// 9
	{0x3B0000, 0x3BFFFF, (void *)cpu_dummy_write, NULL},			// 10
	{0x3C0000, 0x3CFFFF, (void *)cpu_vidreg_write_16, NULL},		// 11
	{0x3D0000, 0x3FFFFF, (void *)cpu_dummy_write, NULL},			// 12
	{0x400000, 0x401FFF, (void *)cpu_pal_write_16, NULL},			// 13
	{0x402000, 0x7FFFFF, (void *)cpu_dummy_write, NULL},			// 14
	{0x800000, 0x80FFFF, (void *)cpu_memcard_write, NULL},			// 15
	{0x810000, 0xCFFFFF, (void *)cpu_dummy_write, NULL},			// 16
	{0xD00000, 0xD0ffff, (void *)cpu_nvram_write_16, NULL},			// 17
	{0xD10000, 0xFFFFFF, (void *)cpu_dummy_write, NULL},			// 18
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};

#endif



#ifndef AES

// NEOGEO CDROM

#if !defined(DEBUG_MEMORY) && !defined(SINGLE_MEMORY)
static M68K_DATA neogeo_memmap_write_8[] = {
	{0x000000, 0x1FFFFF, NULL, &neogeo_prg_memory},		// 0
	{0x200000, 0x2FFFFF, (void *)cpu_dummy_write, NULL},	// 1
	{0x300000, 0x30FFFF, (void *)cpu_watchdog_reset, NULL},	// 2
	{0x310000, 0x31FFFF, (void *)cpu_dummy_write, NULL},	// 3
	{0x320000, 0x32FFFF, (void *)cpu_z80_write_8, NULL},	// 4
	{0x330000, 0x37FFFF, (void *)cpu_dummy_write, NULL},	// 5
	{0x380000, 0x380FFF, (void *)cpu_pd4990_write_8, NULL}, // 6
	{0x381000, 0x39FFFF, (void *)cpu_dummy_write, NULL},	// 7
	{0x3A0000, 0x3AFFFF, (void *)cpu_switch_write, NULL},	// 8
	{0x3B0000, 0x3BFFFF, (void *)cpu_dummy_write, NULL},	// 9
	{0x3C0000, 0x3CFFFF, (void *)cpu_videreg_write_8, NULL},// 10
	{0x3D0000, 0x3FFFFF, (void *)cpu_dummy_write, NULL},	// 11
	{0x400000, 0x401FFF, (void *)cpu_pal_write_8, NULL}, 	// 12
	{0x402000, 0x7FFFFF, (void *)cpu_dummy_write, NULL},	// 13
	{0x800000, 0x80FFFF, (void *)cpu_memcard_write, NULL},	// 14
	{0x810000, 0xDfFFFF, (void *)cpu_dummy_write, NULL},	// 15
	{0xE00000, 0xE3FFFF, (void *)cpu_upload_write_8, NULL},	// 16
	{0xE40000, 0xFFFFFF, (void *)cpu_dummy_write, NULL},	// 17
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#else

#ifdef SINGLE_MEMORY
static M68K_DATA neogeo_memmap_write_8[] = {
	{0x000000, 0x1FFFFF, NULL, (void *)&neogeo_prg_memory},			//0
	{0x200000, 0xFFFFFF, (void *)m68k_write_memory_8, NULL},		//1
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#else

#warning neogeo_memmap_write_8 DEBUG MODE !!!!!!!
static M68K_DATA neogeo_memmap_write_8[] = {
	{0x000000, 0xFFFFFF, (void *)m68k_write_memory_8, NULL},	// 0
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};
#endif
#endif

#else

// NEOGEO AES

static M68K_DATA neogeo_memmap_write_8[] = {
	{0x000000, 0x0FFFFF, (void *)cpu_dummy_write, NULL},			// 0
	{0x100000, 0x10FFFF, NULL, (void *)(((unsigned)&aes4all_ram)-0x100000)},// 1 
	{0x110000, 0x1FFFFF, (void *)cpu_dummy_write, NULL},			// 2
	{0x200000, 0x2FFFFF, (void *)cpu_bk_write, NULL},			// 3 
	{0x300000, 0x30FFFF, (void *)cpu_watchdog_reset, NULL},			// 4
	{0x310000, 0x31FFFF, (void *)cpu_dummy_write, NULL},			// 5
	{0x320000, 0x32FFFF, (void *)cpu_z80_write_8, NULL}, 			// 6
	{0x330000, 0x37FFFF, (void *)cpu_dummy_write, NULL},			// 7
	{0x380000, 0x380FFF, (void *)cpu_pd4990_write_8, NULL}, 		// 8
	{0x381000, 0x39FFFF, (void *)cpu_dummy_write, NULL},			// 9
	{0x3A0000, 0x3AFFFF, (void *)cpu_switch_write, NULL},			// 10
	{0x3B0000, 0x3BFFFF, (void *)cpu_dummy_write, NULL},			// 11
	{0x3C0000, 0x3CFFFF, (void *)cpu_videreg_write_8, NULL},		// 12
	{0x3D0000, 0x3FFFFF, (void *)cpu_dummy_write, NULL},			// 13
	{0x400000, 0x401FFF, (void *)cpu_pal_write_8, NULL}, 			// 14
	{0x402000, 0x7FFFFF, (void *)cpu_dummy_write, NULL},			// 15
	{0x800000, 0x80FFFF, (void *)cpu_memcard_write, NULL},			// 16
	{0x810000, 0xCFFFFF, (void *)cpu_dummy_write, NULL},			// 17
	{0xD00000, 0xD0ffff, (void *)cpu_nvram_write_8, NULL},			// 18
	{0xD10000, 0xFFFFFF, (void *)cpu_dummy_write, NULL},			// 19
	{(unsigned)-1,(unsigned)-1,NULL,NULL}
};

#endif



static M68K_CONTEXT neogeo_context;


#ifndef AES

static unsigned neogeo_fpa[256];
void    neogeo_cdrom_load_files(void);
void    neogeo_cdda_control(void);
void    neogeo_prio_switch(void);
void    neogeo_upload(void);
extern int img_display;

void neogeo_chk_handler_10(unsigned vector)
{
	neo4all_prof_start(NEO4ALL_PROFILER_VECTOR);
#ifdef DEBUG_MEMORY
puts("CAPTURADO !!!!!!!!");
printf("\tPC=%X  FPC=%X\n", m68k_get_pc(), m68k_fetch(m68k_get_pc(),0));
printf("\tVECTOR=%X\n",vector);
fflush(stdout);
#endif

	switch(m68k_fetch(m68k_get_pc(),0))
	{
		case 0xfabe: neogeo_exit(); break;
		case 0xfabf: img_display=1; neogeo_cdrom_load_files(); break;
		case 0xfac0: img_display=0; neogeo_cdrom_load_files(); break;
		case 0xfac1: neogeo_upload(); break;
		case 0xfac2: neogeo_prio_switch(); break;
		case 0xfac3: neogeo_cdda_control(); break;
	}
	m68k_set_register(M68K_REG_PC,m68k_get_pc()+2);
	neo4all_prof_end(NEO4ALL_PROFILER_VECTOR);
}
#endif

static void init_context(void)
{
	neogeo_context.fetch=(M68K_PROGRAM*)&neogeo_program;
	neogeo_context.read_byte=(M68K_DATA*)&neogeo_memmap_read_8;
	neogeo_context.read_word=(M68K_DATA*)&neogeo_memmap_read_16;
	neogeo_context.write_byte=(M68K_DATA*)&neogeo_memmap_write_8;
	neogeo_context.write_word=(M68K_DATA*)&neogeo_memmap_write_16;

	neogeo_context.sv_fetch=(M68K_PROGRAM*)&neogeo_program;
	neogeo_context.sv_read_byte=(M68K_DATA*)&neogeo_memmap_read_8;
	neogeo_context.sv_read_word=(M68K_DATA*)&neogeo_memmap_read_16;
	neogeo_context.sv_write_byte=(M68K_DATA*)&neogeo_memmap_write_8;
	neogeo_context.sv_write_word=(M68K_DATA*)&neogeo_memmap_write_16;

	neogeo_context.user_fetch=(M68K_PROGRAM*)&neogeo_program;
	neogeo_context.user_read_byte=(M68K_DATA*)&neogeo_memmap_read_8;
	neogeo_context.user_read_word=(M68K_DATA*)&neogeo_memmap_read_16;
	neogeo_context.user_write_byte=(M68K_DATA*)&neogeo_memmap_write_8;
	neogeo_context.user_write_word=(M68K_DATA*)&neogeo_memmap_write_16;

	neogeo_context.reset_handler=NULL;
	neogeo_context.iack_handler=NULL;
}


/***************************************************************************************************/
void initialize_memmap(void) {
	console_puts("INIT MEMMAP");
	memset(&neogeo_context,0,sizeof(M68K_CONTEXT));
#ifndef AES
	memset(&neogeo_fpa,0,sizeof(unsigned)*256);
	neogeo_fpa[0x10 / 4]=(unsigned)&neogeo_chk_handler_10;
	neogeo_fpa[0x2c / 4]=(unsigned)&neogeo_chk_handler_10;
	neogeo_context.icust_handler = (unsigned int *)&neogeo_fpa;
#endif
	init_context();

	neogeo_program[0].offset=(unsigned int)neogeo_prg_memory;
#ifndef AES
	neogeo_program[1].offset=((unsigned)neogeo_rom_memory)-0xc00000;
#else
	neogeo_program[2].offset=((unsigned int)neogeo_prg_memory)-0x200000;
	neogeo_program[3].offset=((unsigned)neogeo_rom_memory)-0xc00000;
#endif



#ifndef AES
#if !defined(DEBUG_MEMORY) && !defined(SINGLE_MEMORY)
	neogeo_memmap_read_16[0].data=neogeo_prg_memory;
	neogeo_memmap_read_16[4].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
	neogeo_memmap_read_16[8].data=(void *)(((unsigned)neogeo_rom_memory)-0xc00000);

	neogeo_memmap_read_8[0].data=neogeo_prg_memory;
	neogeo_memmap_read_8[10].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
	neogeo_memmap_read_8[14].data=(void *)(((unsigned)neogeo_rom_memory)-0xc00000);

	neogeo_memmap_write_16[0].data=neogeo_prg_memory;
	neogeo_memmap_write_8[0].data=neogeo_prg_memory;
#else
#ifdef SINGLE_MEMORY
	neogeo_memmap_read_16[0].data=neogeo_prg_memory;
	neogeo_memmap_read_16[2].data=(void *)(((unsigned)neogeo_rom_memory)-0xc00000);
	neogeo_memmap_read_8[0].data=neogeo_prg_memory;
	neogeo_memmap_read_8[2].data=(void *)(((unsigned)neogeo_rom_memory)-0xc00000);
	neogeo_memmap_write_16[0].data=neogeo_prg_memory;
	neogeo_memmap_write_8[0].data=neogeo_prg_memory;
#endif
#endif
#else // AES ...

	neogeo_memmap_read_8[0].data=neogeo_prg_memory;
	neogeo_memmap_read_8[3].data=(void *)(((unsigned)neogeo_prg_memory)-0x200000);
	neogeo_memmap_read_8[14].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
	neogeo_memmap_read_8[18].data=(void *)(((unsigned)neogeo_rom_memory)-0xc00000);

	neogeo_memmap_read_16[0].data=neogeo_prg_memory;
	neogeo_memmap_read_16[3].data=(void *)(((unsigned)neogeo_prg_memory)-0x200000);
	neogeo_memmap_read_16[7].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
	neogeo_memmap_read_16[11].data=(void *)(((unsigned)neogeo_rom_memory)-0xc00000);

#endif

	m68k_set_context(&neogeo_context);
//	m68k_set_irq_type(NULL,M68K_AUTO_LOWER_IRQ);

#ifdef AES
	back_bankswitch_address=0xFFFFFFFF;
	if (aes4all_memory_cpu_size>=0x100000)
		m68k_bankswitch(0x100000);
	else
		m68k_bankswitch(0);
#endif
}


#ifndef AES


/***************************************************************************************************/
#ifdef DEBUG_MEMORY
unsigned int  _m68k_read_memory_8(unsigned int offset) {
#else
unsigned int m68k_read_memory_8(unsigned int offset) {
#endif
#ifdef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    register unsigned char ret=0;
    offset&=0xffffff;
    if(offset<0x200000)
        ret=*((Uint8*)&neogeo_prg_memory[offset^1]);
    else
    switch(offset>>16)
    {
        case    0x30:    ret=read_player1(); break;
        case    0x32:    ret=cpu_coin_read(offset); break;
        case    0x34:    ret=read_player2(); break;
        case    0x38:    ret=read_pl12_startsel(); break;
        case    0x80:
			 if(offset&0x01) ret=neogeo_memorycard[(offset&0x3fff)>>1];
                         else ret=(unsigned char)-1; break;

        /* BIOS ROM */
        case    0xc0:
        case    0xc1:
        case    0xc2:
        case    0xc3:
        case    0xc4:
        case    0xc5:
        case    0xc6:
        case    0xc7:    ret=neogeo_rom_memory[(offset^1)&0x0fffff];  break;

        /* upload region */
        case    0xe0:
        case    0xe1:
        case    0xe2:
        case    0xe3:    ret=cpu_upload_read(offset&0xfffff); break;

    }
#ifdef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
    return (unsigned int)ret;
}

#ifdef DEBUG_MEMORY
unsigned int  m68k_read_memory_8(unsigned int offset) {
unsigned int result=_m68k_read_memory_8(offset);
if (trazando) printf("read_8(0x%X)=0x%X\n",offset,result&0xFF);
return result;
}
#endif

#endif // AES

/***************************************************************************************************/

#ifndef AES

#ifdef DEBUG_MEMORY
unsigned int  _m68k_read_memory_16(unsigned int offset) {
#else
unsigned int  m68k_read_memory_16(unsigned int offset) {
#endif
#ifdef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    register unsigned short ret=0;
    offset&=0xffffff;
    if(offset<0x200000)
        ret=*((Uint16*)&neogeo_prg_memory[offset]);
    else
    switch(offset>>16)
    {
        case    0x3c:    ret=cpu_vidreg_read_16(offset); break;
        case    0x40:    ret=video_paletteram_ng[offset&0x1fff]; break;
        case    0x80:
			 ret=0xff00|neogeo_memorycard[(offset&0x3fff)>>1]; break;

        /* BIOS ROM */
        case    0xc0:
        case    0xc1:
        case    0xc2:
        case    0xc3:
        case    0xc4:
        case    0xc5:
        case    0xc6:
        case    0xc7:    ret=*(Uint16*)&neogeo_rom_memory[offset&0x0fffff]; break;
		
//	case	0xff:	 break;

    }
#ifdef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
    return (unsigned int)ret;
}


#ifdef DEBUG_MEMORY
unsigned int  m68k_read_memory_16(unsigned int offset) {
unsigned int result=_m68k_read_memory_16(offset);
if (trazando) printf("read_16(0x%X)=0x%X\n",offset,result&0xFFFF);
return result;
}
#endif

#endif // AES


/***************************************************************************************************/

#ifndef AES

#ifdef DEBUG_MEMORY
unsigned int _m68k_read_memory_32(unsigned int offset) {
#else
unsigned int  m68k_read_memory_32(unsigned int offset) {
#endif
    unsigned int data;
    offset&=0xfffffe;
    data=m68k_read_memory_16(offset)<<16;
    data|=m68k_read_memory_16(offset+2);
    return data;
}

#ifdef DEBUG_MEMORY
unsigned int  m68k_read_memory_32(unsigned int offset) {
unsigned int result=_m68k_read_memory_32(offset);
if (trazando) printf("read_32(0x%X)=0x%X\n",offset,result);
return result;
}
#endif

#endif // AES

unsigned int cpu_memcard_read_8(unsigned int offset) {
#ifdef DEBUG_MEMORY
if (trazando) printf("read_memorycard_8(0x%X)\n",offset);
#endif
	if(offset&0x01) return neogeo_memorycard[(offset&0x3fff)>>1];
	return (unsigned)-1;
}

unsigned int cpu_memcard_read_16(unsigned int offset) {
#ifdef DEBUG_MEMORY
if (trazando) printf("read_memorycard_16(0x%X)\n",offset);
#endif
	return 0xff00|neogeo_memorycard[(offset&0x3fff)>>1];
}


void cpu_z80_write_8(unsigned int offset, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("write_8_320000(0x%X)=0x%X\n",offset,data&0xFF);
#endif
#ifdef Z80_EMULATED
    neo4all_prof_start(NEO4ALL_PROFILER_Z80);
    if (offset == 0x320000) {
           sound_code=data&0xff;
           pending_command=1;
    	   if (neogeo_sound_enable)
	   {
                _z80nmi();
                _z80exec(Z80_TIMESLICE_PER_NMI);
                my_timer();
           }
    }
    neo4all_prof_end(NEO4ALL_PROFILER_Z80);
#endif
}

void cpu_z80_write_16(unsigned int offset, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("write_16_320000(0x%X)=0x%X\n",offset,data&0xFFFF);
#endif
#ifdef Z80_EMULATED
    neo4all_prof_start(NEO4ALL_PROFILER_Z80);
    if (offset == 0x320000) {
           sound_code=(data>>8)&0xff;
           pending_command=1;
    	   if (neogeo_sound_enable)
	   {
                _z80nmi();
                _z80exec(Z80_TIMESLICE_PER_NMI);
                my_timer();
           }
    }
    neo4all_prof_end(NEO4ALL_PROFILER_Z80);
#endif
}

void cpu_videreg_write_8(unsigned int offset, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("write_8_3c0000(0x%X)=0x%X\n",offset,data&0xFF);
#endif
	    int temp=cpu_vidreg_read_16(offset);
            if(offset&0x01) cpu_vidreg_write_16(offset, (temp&0xff)|(data<<8));
            else cpu_vidreg_write_16(offset, (temp&0xff00)|data);
}

#ifndef AES

/***************************************************************************************************/
void m68k_write_memory_8(unsigned int offset, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("write_8(0x%X)=0x%X\n",offset,data&0xFF);
#endif
#ifdef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    data&=0xff;
    offset&=0xffffff;

    if(offset<0x200000)
        neogeo_prg_memory[offset^1]=(char)data;
    else
    switch(offset>>16)
    {

        case    0x30:    cpu_watchdog_reset(); break;
        case    0x32:	 cpu_z80_write_8(offset,data); break;
//	case	0x38:	 break;
        case    0x3a:    cpu_switch_write(offset, data); break;
        case    0x3c:    cpu_videreg_write_8(offset,data); break;
        case    0x80:    if(offset&0x01) cpu_memcard_write(offset,data); break;


        /* upload */
        case    0xe0:
        case    0xe1:
        case    0xe2:
        case    0xe3:   cpu_upload_write_8(offset&0xfffff,data); break;
		
		/* cdrom */
//	case	0xff:	break;
    }
#ifdef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
}

#endif // AES


/***************************************************************************************************/

void cpu_pal_write_16(unsigned int offset, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("write_16_400000(0x%X)=0x%X\n",offset,data&0xFFFF);
#endif
#ifndef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
	offset =(offset&0x1fff)>>1;
#ifdef ONLY_CHANGES_PATCH
	if (video_palette_ng[offset+(video_palette_selected<<4)]!=data)
#endif
	{
//printf("store_pal_word addr=0x%X data=0x%X\n",offset<<1,data);
        	video_paletteram_ng[offset]=(Uint16)data;
        	video_paletteram_pc[offset]=video_color_lut[data&0x1fff];
		video_palette_dirty=-1;
		video_palette_use[video_palette_selected+(offset>>4)]=-1;
	}
#ifndef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
}

void cpu_pal_write_8(unsigned int offset, unsigned int data) {
	register unsigned addr=offset&0x1FFE;
	register unsigned short a = video_paletteram_ng[addr];
	if (offset & 0x1)
		a = data | (a & 0xff00);
	else
		a = (a & 0xff) | (data << 8);
//printf("store_pal_byte addr=0x%X data=0x%X -> a=0x%X\n",offset,data,a);
	cpu_pal_write_16(addr,a);
}


#ifndef AES

void m68k_write_memory_16(unsigned int offset, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("write_16(0x%X)=0x%X\n",offset,data&0xFFFF);
#endif
#ifdef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    data&=0xffff;
	offset&=0xffffff;

    if(offset<0x200000)
        *(Uint16*)&neogeo_prg_memory[offset]=(Uint16)data;
    else
    switch(offset>>16)
    {
//	case	0x2f:	break;
        case    0x3a:   cpu_switch_write(offset, data); break;
        case    0x3c:   cpu_vidreg_write_16(offset, data); break;
        case    0x40:   cpu_pal_write_16(offset,data); break;
        case    0x80:   cpu_memcard_write(offset,data); break;

        /* upload */
        case    0xe0:
        case    0xe1:
        case    0xe2:
        case    0xe3:    cpu_upload_write_16(offset&0xfffff,data); break;
		
//	case	0xff: break;
    }
#ifdef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
}

#endif // AES

/***************************************************************************************************/


#ifndef AES


void m68k_write_memory_32(unsigned int offset, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("write_32(0x%X)=0x%X\n",offset,data);
#endif
	unsigned int word1=(data>>16)&0xffff;
	unsigned int word2=data&0xffff;
	m68k_write_memory_16(offset,word1);
	m68k_write_memory_16(offset+2,word2);
}

#endif // AES


/***************************************************************************************************/

static __inline__ unsigned short cpu_control_read(void)
{
	int scan = neogeo_current_line;
//	int irq_bit = neogeo_irq2taken || (scan < 16 || scan > 239);

	return ((scan <<7) & 0x7f80)
//		|(irq_bit << 15)
//		|(conf.pal<<3)
		|(neogeo_frame_counter & 0x0007);
/*
	return ((((neogeo_frame_counter_speed-1)&0xffff)<<8)|
		(neogeo_frame_counter&7)|128); break;
*/
}

static int    cpu_vidreg_read_16(int offset)
{
#ifndef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    register int ret=0;
    switch(offset)
    {
        case    0x3c0000:
        case    0x3c0002:
        case    0x3c0008:
        case    0x3c000a:    ret=*(Uint16*)&video_vidram[video_pointer<<1]; break;
        case    0x3c0004:    ret=video_modulo; break;
	case    0x3c0006:    ret=cpu_control_read();


    }
#ifndef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
    return ret;
}


static int cpu_vidreg_read_8(int offset)
{
	if ((offset&0xFFFF)==0xe)
		return -1;
	return 0;
}


static __inline__ void cpu_control_write(unsigned short data)
{
	neogeo_frame_counter_speed = (((data >> 8) & 0xff)+1);
//	neogeo_irq2control = data & 0xff;
}


static void cpu_vidreg_write_16(int offset, int data)
{
//printf("vidreg 0x%X = 0x%X\n",offset,data);
#ifndef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    switch(offset)
    {
        case    0x3c0000:    video_pointer=(Uint16)data; break;

        case    0x3c0002:    *(Uint16*)&video_vidram[video_pointer<<1]=(Uint16)data;
                             video_pointer=(video_pointer+video_modulo)&0xFFFF; break;

        case    0x3c0004:    video_modulo=(Sint16)data; break;

        case    0x3c0006:    cpu_control_write(data);
/*
			     break;
        case    0x3c0008:    break; // IRQ position
        case    0x3c000a:    break; // IRQ position
        case    0x3c000c:    break; // IRQ acknowledge
*/

    }
#ifndef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
}

/*
static void cpu_irq2pos_write(unsigned data)
{
	irq2pos_value = data;
	if (neogeo_irq2control & 0x20) {
		 int line = (irq2pos_value + 0x3b) / 0x180;
		 irq2start = line + current_line;
	}
}
*/


/***************************************************************************************************/

static void     neogeo_setpalbank0 (void) {
#ifdef USE_SECURE_MEMSWITCH
    if (video_palette_selected)
    {
//puts("setpalbank0");
#endif
    	video_palette_selected=0;
   	video_paletteram_ng=video_palette_bank0_ng;
    	video_paletteram_pc=video_palette_bank0_pc;

#ifdef USE_SECURE_MEMSWITCH
	m68k_get_context(&neogeo_context);
//	init_context();	
#ifndef AES
#if !defined(DEBUG_MEMORY) && !defined(SINGLE_MEMORY)
	neogeo_memmap_read_16[4].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
	neogeo_memmap_read_8[10].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
#endif
#else
	neogeo_memmap_read_8[14].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
	neogeo_memmap_read_16[7].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
#endif
	m68k_set_context(&neogeo_context);
#else
#ifndef AES
#if !defined(DEBUG_MEMORY) && !defined(SINGLE_MEMORY)
	M68KCONTEXT.read_word[4].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
	M68KCONTEXT.read_byte[10].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
#endif
#else
	M68KCONTEXT.read_byte[14].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
	M68KCONTEXT.read_word[7].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
#endif
#endif
#ifdef USE_SECURE_MEMSWITCH
    }
#endif
}


static void     neogeo_setpalbank1 (void) {
#ifdef USE_SECURE_MEMSWITCH
    if (!video_palette_selected)
    {
#endif
//puts("setpalbank1");
    	video_palette_selected=0x100;
    	video_paletteram_ng=video_palette_bank1_ng;
    	video_paletteram_pc=video_palette_bank1_pc;

#ifdef USE_SECURE_MEMSWITCH
	m68k_get_context(&neogeo_context);
//	init_context();	
#ifndef AES
#if !defined(DEBUG_MEMORY) && !defined(SINGLE_MEMORY)
	neogeo_memmap_read_16[4].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
	neogeo_memmap_read_8[10].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
#endif
#else
	neogeo_memmap_read_8[14].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
	neogeo_memmap_read_16[7].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
#endif
	m68k_set_context(&neogeo_context);
#else
#ifndef AES
#if !defined(DEBUG_MEMORY) && !defined(SINGLE_MEMORY)
	M68KCONTEXT.read_word[4].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
	M68KCONTEXT.read_byte[10].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
#endif
#else
	M68KCONTEXT.read_byte[14].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
	M68KCONTEXT.read_word[7].data=(void *)(((unsigned)video_paletteram_ng)-0x400000);
#endif
#endif
#ifdef USE_SECURE_MEMSWITCH
    }
#endif

}



// NEOGEO CD

static __inline__ void    neogeo_select_bios_vectors (void) {
//puts("neogeo_select_bios_vectors");
#ifndef USE_MMAP
    memcpy(neogeo_prg_memory, neogeo_rom_memory, 0x80);
#endif
}

static __inline__ void    neogeo_select_game_vectors (void) {
#if defined(AES) && !defined(USE_MMAP)
//puts("neogeo_select_game_vectors");
    memcpy(neogeo_prg_memory, neogeo_game_vector, 0x80);
#endif
}

#ifdef AES

// NEOGEO AES

void m68k_bankswitch(unsigned int address)
{
#if defined(USE_SECURE_MEMSWITCH) || defined(USE_FAME_C_CORE)
	if (back_bankswitch_address!=address)
	{
#endif
//printf("m68k_bankswitch(0x%X)\n",address);
		unsigned int new_address=((unsigned)neogeo_prg_memory)+address-0x200000;
#ifdef USE_SECURE_MEMSWITCH
		m68k_get_context(&neogeo_context);
		neogeo_memmap_read_8[3].data=(void *)(new_address);
		neogeo_memmap_read_16[3].data=(void *)(new_address);
		neogeo_program[2].offset=new_address;
//		init_context();	
		m68k_set_context(&neogeo_context);
#else
		M68KCONTEXT.read_byte[3].data=(void *)(new_address);
		M68KCONTEXT.read_word[3].data=(void *)(new_address);
#ifndef USE_FAME_C_CORE
		M68KCONTEXT.fetch[2].offset=new_address;
#else
		famec_SetFetch(0x200000, 0x2FFFFF,new_address);
#endif
#endif
#if defined(USE_SECURE_MEMSWITCH) || defined(USE_FAME_C_CORE)
		back_bankswitch_address=address;
	}
#endif
}


static __inline__ void    neogeo_select_board_sfix (void) {
//puts("neogeo_select_board_sfix");
	neogeo_fix_memory = (char*) aes4all_memory_sfix_board;
	video_fix_usage = (unsigned char *) aes4all_memory_fix_board_usage;
}

static __inline__ void    neogeo_select_game_sfix (void) {
//puts("neogeo_select_game_sfix");
	neogeo_fix_memory = (char*) aes4all_memory_sfix_game;
	video_fix_usage = (unsigned char *) aes4all_memory_fix_game_usage;
}

static void cpu_bk_write(unsigned int address, unsigned int _data)
{
//printf("cpu_bk_write(0x%X,0x%X)\n",address,_data);
	if (aes4all_memory_cpu_size <= 0x100000)
		return;
	register unsigned char data=_data;
/*
	if (address == aes4all_bksw_handler) {
		data = 
			(((data >> aes4all_bksw_unscramble[0]) & 1) << 0) +
			(((data >> aes4all_bksw_unscramble[1]) & 1) << 1) +
			(((data >> aes4all_bksw_unscramble[2]) & 1) << 2) +
			(((data >> aes4all_bksw_unscramble[3]) & 1) << 3) +
			(((data >> aes4all_bksw_unscramble[4]) & 1) << 4) +
			(((data >> aes4all_bksw_unscramble[5]) & 1) << 5);
		aes4all_bankaddress = 0x100000 + aes4all_bksw_offset[data];
	} else
*/
	if (address >= 0x2FFFF0) {
		data = data & 0x7;
		aes4all_bankaddress = (data + 1) * 0x100000;
	} else
		return;
	if (aes4all_bankaddress >= aes4all_memory_cpu_size)
		aes4all_bankaddress = 0x100000;
	m68k_bankswitch(aes4all_bankaddress);
}


#ifdef USE_NVRAM_READ_FUNC
static unsigned char   cpu_nvram_read_8(unsigned int addr)
{
	return aes4all_nvram[addr - 0xd00000];
}

static unsigned short   cpu_nvram_read_16(unsigned int addr)
{
	addr -= 0xd00000;
	return (((unsigned short)aes4all_nvram[addr]) << 8) |
		(((unsigned short)aes4all_nvram[addr + 1]) & 0xff);
}
#endif

static void cpu_nvram_write_8(unsigned int addr, unsigned char data)
{
	if (aes4all_nvram_lock)
		return;
	if (addr == 0xd00000 + aes4all_nvram_protection_hack && ((data & 0xff) == 0x01))
		return;
	aes4all_nvram[addr - 0xd00000] = data;
}

static void cpu_nvram_write_16(unsigned int addr, unsigned short data)
{
	if (aes4all_nvram_lock)
		return;
	if (addr == 0xd00000 + aes4all_nvram_protection_hack && ((data & 0xffff) == 0x01))
		return;
	addr -= 0xd00000;
	aes4all_nvram[addr] = data >> 8;
	aes4all_nvram[addr + 1] = data & 0xff;
}
#endif


static void    cpu_switch_write(int offset, int data)
{
#ifndef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    switch(offset&0xFFFFFE)
    {
        case 0x3a0002: neogeo_select_bios_vectors(); break;
        case 0x3a0012: neogeo_select_game_vectors(); break;
#ifdef AES
	case 0x3a000a: neogeo_select_board_sfix(); break;
	case 0x3a001a: neogeo_select_game_sfix(); break;
	case 0x3a000d: aes4all_nvram_lock = 1; break;
	case 0x3a001d: aes4all_nvram_lock = 0; break;
#endif
        case 0x3a000e: neogeo_setpalbank1(); break;
        case 0x3a001e: neogeo_setpalbank0(); break;

    }
#ifndef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
}

/***************************************************************************************************/


void neogeo_sound_irq(int irq)
{
#ifdef Z80_EMULATED
   if (neogeo_sound_enable)
   {
    if (irq) 
    {   _z80raise(0); }
    else 
    {   _z80lower(); }
   }
#endif
}



static int cpu_coin_read(int addr)
{
#ifndef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    register int res=0;
    addr &= 0xFFFF;
    if (addr == 0x1) {
        register int coinflip = pd4990a_testbit_r();
        register int databit = pd4990a_databit_r();
        res=neo4all_intern_coin ^ (coinflip << 6) ^ (databit << 7);
    }
    else
    if (addr == 0x0) {
#ifdef Z80_EMULATED
	if (neogeo_sound_enable)
	{
		res |= result_code;
		if (pending_command)
			res &= 0x7f;
	}
	else
#endif
		res |= 0x01;
    }
#ifndef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
    return res;
}


static void cpu_watchdog_reset (void)
{
    watchdog_counter=3 * 60;  /* 3s * 60 fps */
}

#ifndef AES

// NEOGEO CD

static int cpu_upload_read(int offset) {
    register int ret=-1;
#if 0
#ifndef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    /* int bank = m68k_read_memory_8(0x10FEDB); */

    /* cpu_upload_read is disabled for now.*/
	/* fixes Metal Slug */

    switch (m68k_read_memory_8(0x10FEDA)) {
        case 0x00: /* 68000 */
            return neogeo_prg_memory[offset^1];
        case 0x01: /* Z80 */
#ifdef Z80_EMULATED
   	    if (neogeo_sound_enable)
            	return mame_z80mem[(offset>>1)&0xFFFF];
#endif
	    break;
    }
#ifndef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
#endif
    return ret;
}


static void cpu_upload_write_8(int offset, int data) {
#ifndef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    switch (m68k_read_memory_8(0x10FEDA)) {
        case 0x00: /* 68000 */
            neogeo_prg_memory[offset^1]=(char)data;
            break;
        case 0x01: /* Z80 */
#ifdef Z80_EMULATED
   	    if (neogeo_sound_enable)
            	mame_z80mem[(offset>>1)&0xFFFF]=(char)data;
#endif
            break;
        case 0x11: /* FIX */
            neogeo_fix_memory[offset>>1]=(char)data;
            break;
    }
#ifndef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
}


static char _sprbuffer[4];

static void cpu_upload_write_16(int offset, int data) {
#ifndef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    register int bank = m68k_read_memory_8(0x10FEDB);
    data&=0xffff;

    switch (m68k_read_memory_8(0x10FEDA)) {
        case 0x12: /* SPR */
	    {
            	register int offset2=(offset & ~0x02)+(bank<<20);
            	if((offset2&0x7f)<64)
               		offset2=(offset2&0xfff80)+((offset2&0x7f)<<1)+4;
           	 else
               		offset2=(offset2&0xfff80)+((offset2&0x7f)<<1)-128;

           	register char* dest=&neogeo_spr_memory[offset2];

            	if (offset & 0x02) {
               		/* second word */
			   *(Uint16*)(&dest[2])=(Uint16)data;
			   /* reformat sprite data */
               		swab(dest, _sprbuffer, sizeof(_sprbuffer));
               		extract8(_sprbuffer, dest);
	        } else {
			   /* first word */
			   *(Uint16*)(&dest[0])=(Uint16)data;
            	}
	    }
            break;

        case 0x13: /* Z80 */
#ifdef Z80_EMULATED
   	    if (neogeo_sound_enable)
            	mame_z80mem[(offset>>1)&0xFFFF]=(char)data;
#endif
            break;
        case 0x14: /* PCM */
            neogeo_pcm_memory[(offset>>1)+(bank<<19)]=(char)data;
            break;
    }
#ifndef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
}

static void cpu_upload_write_32(int offset, int data) {
#ifndef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    switch (m68k_read_memory_8(0x10FEDA)) {
        case 0x12: /* SPR */
            offset+=((int)(m68k_read_memory_8(0x10FEDB))<<20);

            if((offset&0x7f)<64)
               offset=(offset&0xfff80)+((offset&0x7f)<<1)+4;
            else
               offset=(offset&0xfff80)+((offset&0x7f)<<1)-128;
	    {
            	register char *dest=&neogeo_spr_memory[offset];
            	swab((char*)&data, _sprbuffer, sizeof(_sprbuffer));
            	extract8(_sprbuffer, dest);
	    }
            break;
    }
#ifndef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
}

#endif


static void cpu_memcard_write(int offset, int data) {
#ifndef SINGLE_MEMORY
    neo4all_prof_start(NEO4ALL_PROFILER_MEM);
#endif
    neogeo_memorycard[(offset&0x3fff)>>1]=(char)(data&0xFF); 

    /* signal that memory card has been written */
    memcard_write=4; 
#ifndef SINGLE_MEMORY
    neo4all_prof_end(NEO4ALL_PROFILER_MEM);
#endif
}

static void cpu_pd4990_write_8(unsigned int address, unsigned int value)
{
	register unsigned char data=value;
	pd4990a_control_w(address,value);
}

static void cpu_pd4990_write_16(unsigned int address, unsigned int value)
{
	register unsigned short data=value;
	pd4990a_control_w(address,value);
}
