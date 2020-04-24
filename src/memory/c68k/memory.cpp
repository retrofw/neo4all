/********************************************
*   NeoCD Memory Mapping (C version)        *
*********************************************
* Fosters(2001,2003)                        *
********************************************/

#include <SDL/SDL.h>
#include <SDL/SDL_endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "../neo4all.h"

#ifdef DEBUG_MEMORY
extern int trazando;
#endif

extern unsigned short    *video_paletteram_ng;

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
static int    read_upload(int);
static void   write_upload(int, int);
static void   write_upload_word(int, int);
static void   write_upload_dword(int, int);

static int    cpu_readvidreg(int);
static void   cpu_writevidreg(int, int);
static void   cpu_writeswitch(int, int);
static int    cpu_readcoin(int);

static void   write_memcard(int,int);

void cdda_control(void);

static void   watchdog_reset_w(void);


/***************************************************************************************************/
void initialize_memmap(void) {
    return;
}


/***************************************************************************************************/

unsigned int  FASTCALL m68k_read_memory_8f(const unsigned int off) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_read_memory_8f(0x%X)\n",off); fflush(stdout);
#endif
    unsigned int offset = off & 0xffffff;
    if(offset<0x200000)
        return *((Uint8*)&neogeo_prg_memory[offset^1]);

    switch(offset>>16)
    {
        case    0x30:    return read_player1();
        case    0x32:    return cpu_readcoin(offset);
        case    0x34:    return read_player2();
        case    0x38:    return read_pl12_startsel();
        case    0x80:    if(offset&0x01) return neogeo_memorycard[(offset&0x3fff)>>1];
                         else return -1;

        /* BIOS ROM */
        case    0xc0:
        case    0xc1:
        case    0xc2:
        case    0xc3:
        case    0xc4:
        case    0xc5:
        case    0xc6:
        case    0xc7:    return neogeo_rom_memory[(offset^1)&0x0fffff];

        /* upload region */
        case    0xe0:
        case    0xe1:
        case    0xe2:
        case    0xe3:    return read_upload(offset&0xfffff);
    }
    return 0;
}


unsigned int  m68k_read_memory_8(unsigned int offset) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_read_memory_8(0x%X)\n",offset); fflush(stdout);
#endif
    return m68k_read_memory_8f(offset);
}

/***************************************************************************************************/
unsigned int  FASTCALL m68k_read_memory_16f(const unsigned int off) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_read_memory_16f(0x%X)\n",off); fflush(stdout);
#endif
    unsigned int offset = off & 0xffffff;
    if(offset<0x200000)
        return *((Uint16*)&neogeo_prg_memory[offset]);

    switch(offset>>16)
    {
        case    0x3c:    return cpu_readvidreg(offset);
        case    0x40:    return video_paletteram_ng[offset&0x1fff];
        case    0x80:    return 0xff00|neogeo_memorycard[(offset&0x3fff)>>1];

        /* BIOS ROM */
        case    0xc0:
        case    0xc1:
        case    0xc2:
        case    0xc3:
        case    0xc4:
        case    0xc5:
        case    0xc6:
        case    0xc7:    return *(Uint16*)&neogeo_rom_memory[offset&0x0fffff];
		
//	case	0xff:	 break;

    }
    return 0;
}


unsigned int  m68k_read_memory_16(unsigned int offset) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_read_memory_16(0x%X)\n",offset); fflush(stdout);
#endif
    return m68k_read_memory_16f(offset);
}


/***************************************************************************************************/
unsigned int  FASTCALL m68k_read_memory_32f(const unsigned int off) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_read_memory_32f(0x%X)\n",off); fflush(stdout);
#endif
    unsigned int offset = off & 0xffffff;
    unsigned int data;
    data=m68k_read_memory_16(offset)<<16;
    data|=m68k_read_memory_16(offset+2);
    return data;
}

unsigned int  m68k_read_memory_32(unsigned int offset) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_read_memory_32(0x%X)\n",offset); fflush(stdout);
#endif
    return m68k_read_memory_32f(offset);
}


/***************************************************************************************************/
void FASTCALL m68k_write_memory_8f(const unsigned int off, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_write_memory_8f(0x%X,0x%X)\n",off,data); fflush(stdout);
#endif
    int temp;
    unsigned int offset = off & 0xffffff;
    data&=0xff;
    if(offset<0x200000) {
        neogeo_prg_memory[offset^1]=(char)data;
        return;
    }

    switch(offset>>16)
    {
        case    0x30:    watchdog_reset_w(); break;
        case    0x32:
#ifdef Z80_EMULATED
	   if (neogeo_sound_enable)
	    if(!(offset&0xffff)) {
                sound_code=data&0xff;
                pending_command=1;
                _z80nmi();
                z80_cycles-=neo4all_z80_cycles >> 8;
                _z80exec(neo4all_z80_cycles >> 8);
                my_timer();
            }
#endif
	    break;
		case	0x38:	break;
        case    0x3a:    cpu_writeswitch(offset, data); break;
        case    0x3c:    temp=cpu_readvidreg(offset);
            if(offset&0x01) cpu_writevidreg(offset, (temp&0xff)|(data<<8));
            else cpu_writevidreg(offset, (temp&0xff00)|data);
            break;
        case    0x80:    if(offset&0x01) write_memcard(offset,data); break;


        /* upload */
        case    0xe0:
        case    0xe1:
        case    0xe2:
        case    0xe3:   write_upload(offset&0xfffff,data); break;
		
	/* cdrom */
//	case	0xff:	break;

    }
}


void  m68k_write_memory_8(unsigned int offset, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_write_memory_8(0x%X,0x%X)\n",offset,data); fflush(stdout);
#endif
    m68k_write_memory_8f(offset, data);
}


/***************************************************************************************************/
void FASTCALL m68k_write_memory_16f(const unsigned int off, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_write_memory_16f(0x%X,0x%X)\n",off,data); fflush(stdout);
#endif
    unsigned int offset = off & 0xffffff;
    data&=0xffff;

    if(offset<0x200000) {
        *(Uint16*)&neogeo_prg_memory[offset]=(Uint16)data;
        return;
    }

    switch(offset>>16)
    {
//	case	0x2f:	break;
        case    0x3a:    cpu_writeswitch(offset, data); break;
        case    0x3c:    cpu_writevidreg(offset, data); break;
        case    0x40:    offset =(offset&0x1fff)>>1;
                         data  &=0x7fff;
                         video_paletteram_ng[offset]=(Uint16)data;
                         video_paletteram_pc[offset]=video_color_lut[data]; break;
        case    0x80:    write_memcard(offset,data); break;

        /* upload */
        case    0xe0:
        case    0xe1:
        case    0xe2:
        case    0xe3:    write_upload_word(offset&0xfffff,data); break;
		
//	case	0xff: break;
    }
}


void  m68k_write_memory_16(unsigned int offset, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_write_memory_16(0x%X,0x%X)\n",offset,data); fflush(stdout);
#endif
    m68k_write_memory_16f(offset, data);
}


/***************************************************************************************************/
#if 1
void FASTCALL m68k_write_memory_32f(const unsigned int off, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_write_memory_32f(0x%X,0x%X)\n",off,data); fflush(stdout);
#endif
      unsigned int offset = off & 0xffffff;
	unsigned int word1=(data>>16)&0xffff;
	unsigned int word2=data&0xffff;
	m68k_write_memory_16(offset,word1);
	m68k_write_memory_16(offset+2,word2);
}

#else
void FASTCALL m68k_write_memory_32f(const unsigned int off, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_write_memory_32f(0x%X,0x%X)\n",off,data); fflush(stdout);
#endif
    int temp=((data&0xffff)<<16)|((data>>16)&0xffff);
    unsigned int offset = off & 0xffffff;

    if(offset<0x200000) {
        *(Uint32*)&neogeo_prg_memory[offset]=temp;
        return;
    }

    switch(offset>>16)
    {
        case    0x3a:    cpu_writeswitch(offset, temp); break;
        case    0x3c:    cpu_writevidreg(offset, temp);
                         cpu_writevidreg(offset+2, data); break;
        case    0x40:    offset =(offset&0x1fff)>>1;
                         data  &=0x7fff;
                         temp  &=0x7fff;
                         video_paletteram_ng[offset]  =(Uint16)temp;
                         video_paletteram_pc[offset]  =video_color_lut[temp];
                         video_paletteram_ng[offset+1]=(Uint16)data;
                         video_paletteram_pc[offset+1]=video_color_lut[data]; break;

        case    0x80:    offset =(offset&0x3fff)>>1;
                         neogeo_memorycard[offset]  =(char)temp;
                         neogeo_memorycard[offset+1]=(char)data; break;

        case    0xe0:
        case    0xe1:
        case    0xe2:
        case    0xe3:    write_upload_dword(offset&0xfffff,temp); break;
		
//	case	0xff:	break;
    }
}
#endif

void  m68k_write_memory_32(unsigned int offset, unsigned int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("m68k_write_memory_32(0x%X,0x%X)\n",offset,data); fflush(stdout);
#endif
    m68k_write_memory_32f(offset, data);
}

/***************************************************************************************************/
static int    cpu_readvidreg(int offset)
{
#ifdef DEBUG_MEMORY
if (trazando) printf("cpu_readvidreg(0x%X)\n",offset); fflush(stdout);
#endif
    switch(offset)
    {
        case    0x3c0000:    return *(Uint16*)&video_vidram[video_pointer<<1]; break;
        case    0x3c0002:    return *(Uint16*)&video_vidram[video_pointer<<1]; break;
        case    0x3c0004:    return video_modulo; break;
        case    0x3c0006:    return ((((neogeo_frame_counter_speed-1)&0xffff)<<8)|
                               (neogeo_frame_counter&7)|128); break;
        case    0x3c0008:    return *(Uint16*)&video_vidram[video_pointer<<1]; break;
        case    0x3c000a:    return *(Uint16*)&video_vidram[video_pointer<<1]; break;

    }
    return 0;
}


/***************************************************************************************************/
static void    cpu_writevidreg(int offset, int data)
{
#ifdef DEBUG_MEMORY
if (trazando) printf("cpu_writevidreg(0x%X,0x%X)\n",offset,data); fflush(stdout);
#endif
    switch(offset)
    {
        case    0x3c0000:    video_pointer=(Uint16)data; break;
        case    0x3c0002:    *(Uint16*)&video_vidram[video_pointer<<1]=(Uint16)data;
                             video_pointer+=video_modulo; break;
        case    0x3c0004:    video_modulo=(Sint16)data; break;

        case    0x3c0006:    neogeo_frame_counter_speed=((data>>8)&0xff)+1; break;

        case    0x3c0008:    /* IRQ position */    break;
        case    0x3c000a:    /* IRQ position */    break;
        case    0x3c000c:    /* IRQ acknowledge */ break;
    }
}

/***************************************************************************************************/

static void     neogeo_setpalbank0 (void) {
#ifdef DEBUG_MEMORY
if (trazando) puts("neogeo_setpalbank0()"); fflush(stdout);
#endif
    video_paletteram_ng=video_palette_bank0_ng;
    video_paletteram_pc=video_palette_bank0_pc;
}


static void     neogeo_setpalbank1 (void) {
#ifdef DEBUG_MEMORY
if (trazando) puts("neogeo_setpalbank1()"); fflush(stdout);
#endif
    video_paletteram_ng=video_palette_bank1_ng;
    video_paletteram_pc=video_palette_bank1_pc;
}


static void    neogeo_select_bios_vectors (void) {
#ifdef DEBUG_MEMORY
if (trazando) puts("neogeo_select_bios_vectors()"); fflush(stdout);
#endif
    memcpy(neogeo_prg_memory, neogeo_rom_memory, 0x80);
}


static void    neogeo_select_game_vectors (void) {
#ifdef DEBUG_MEMORY
if (trazando) puts("neogeo_select_game_vectors()"); fflush(stdout);
#endif
}


static void    cpu_writeswitch(int offset, int data)
{
#ifdef DEBUG_MEMORY
if (trazando) printf("cpu_writeswitch(0x%X,0x%X)\n",offset,data); fflush(stdout);
#endif
    switch(offset)
    {
        case 0x3a0000: /* NOP */ break;
        case 0x3a0001: /* NOP */ break;

        case 0x3a0002: neogeo_select_bios_vectors(); break;
        case 0x3a0003: neogeo_select_bios_vectors(); break;

        case 0x3a000e: neogeo_setpalbank1(); break;
        case 0x3a000f: neogeo_setpalbank1(); break;

        case 0x3a0010: /* NOP */ break;
        case 0x3a0011: /* NOP */ break;

        case 0x3a0012: neogeo_select_game_vectors(); break;
        case 0x3a0013: neogeo_select_game_vectors(); break;

        case 0x3a001e: neogeo_setpalbank0(); break;
        case 0x3a001f: neogeo_setpalbank0(); break;

    }
}

/***************************************************************************************************/


void neogeo_sound_irq(int irq)
{
#ifdef DEBUG_MEMORY
if (trazando) printf("neogeo_sound_irq(0x%X)\n",irq); fflush(stdout);
#endif
#ifdef Z80_EMULATED
    if (neogeo_sound_enable)
    {
    	if (irq)
        	_z80raise(0);
    	else 
		_z80lower();
    }
#endif
}



static int cpu_readcoin(int addr)
{
#ifdef DEBUG_MEMORY
if (trazando) printf("cpu_readcoin(0x%X)\n",addr); fflush(stdout);
#endif
    addr &= 0xFFFF;
    if (addr == 0x1) {
        int coinflip = pd4990a_testbit_r();
        int databit = pd4990a_databit_r();
        return 0xff ^ (coinflip << 6) ^ (databit << 7);
    }
    if (addr == 0x0) {
        int res = 0;
#ifdef Z80_EMULATED
    	if (neogeo_sound_enable)
	{
        	res |= result_code;
        	if (!pending_command)
            		res &= 0x7f;
        	else
           		 res |= 0x01;
        }
#endif
        return res;
    }
    return 0;
}


static void watchdog_reset_w (void)
{
#ifdef DEBUG_MEMORY
if (trazando) puts("watchdog_reset_w()"); fflush(stdout);
#endif
    watchdog_counter=3 * 60;  /* 3s * 60 fps */
}


static int read_upload(int offset) {
#ifdef DEBUG_MEMORY
if (trazando) printf("read_upload(0x%X)\n",offset); fflush(stdout);
#endif
    return -1;
#if 0
    int zone = m68k_read_memory_8(0x10FEDA);
    /* int bank = m68k_read_memory_8(0x10FEDB); */

    /* read_upload is disabled for now.*/
	/* fixes Metal Slug */

    switch (zone) {
        case 0x00: /* 68000 */
            return neogeo_prg_memory[offset^1];
        case 0x01: /* Z80 */
#ifdef Z80_EMULATED
    		if (neogeo_sound_enable)
           	 return mame_z80mem[(offset>>1)&0xFFFF];
#endif
		break;
    }
    return -1;
#endif
}


static void write_upload(int offset, int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("write_upload(0x%X,0x%X)\n",offset,data); fflush(stdout);
#endif
    int zone = m68k_read_memory_8(0x10FEDA);
    /* int bank = m68k_read_memory_8(0x10FEDB); */

    switch (zone) {
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
}


static void write_upload_word(int offset, int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("write_upload_word(0x%X,0x%X)\n",offset,data); fflush(stdout);
#endif
    int zone = m68k_read_memory_8(0x10FEDA);
    int bank = m68k_read_memory_8(0x10FEDB);
    int offset2;
    char *dest;
    char sprbuffer[4];
	
  	data&=0xffff;

    switch (zone) {
        case 0x12: /* SPR */

            offset2=offset & ~0x02;

            offset2+=(bank<<20);

            if((offset2&0x7f)<64)
               offset2=(offset2&0xfff80)+((offset2&0x7f)<<1)+4;
            else
               offset2=(offset2&0xfff80)+((offset2&0x7f)<<1)-128;

            dest=&neogeo_spr_memory[offset2];

            if (offset & 0x02) {
               /* second word */
			   *(Uint16*)(&dest[2])=(Uint16)data;
			   /* reformat sprite data */
               swab(dest, sprbuffer, sizeof(sprbuffer));
               extract8(sprbuffer, dest);
			} else {
			   /* first word */
			   *(Uint16*)(&dest[0])=(Uint16)data;
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
}


static void write_upload_dword(int offset, int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("write_upload_dword(0x%X,0x%X)\n",offset,data); fflush(stdout);
#endif
    int zone = m68k_read_memory_8(0x10FEDA);
    int bank = m68k_read_memory_8(0x10FEDB);
    char *dest;
    char sprbuffer[4];

    switch (zone) {
        case 0x12: /* SPR */
            offset+=(bank<<20);

            if((offset&0x7f)<64)
               offset=(offset&0xfff80)+((offset&0x7f)<<1)+4;
            else
               offset=(offset&0xfff80)+((offset&0x7f)<<1)-128;

            dest=&neogeo_spr_memory[offset];
            swab((char*)&data, sprbuffer, sizeof(sprbuffer));
            extract8(sprbuffer, dest);

            break;
    }
}


static void write_memcard(int offset, int data) {
#ifdef DEBUG_MEMORY
if (trazando) printf("write_memcard(0x%X,0x%X)\n",offset,data); fflush(stdout);
#endif
    data&=0xff;
    neogeo_memorycard[(offset&0x3fff)>>1]=(char)data; 

    /* signal that memory card has been written */
    memcard_write=3; 
}

