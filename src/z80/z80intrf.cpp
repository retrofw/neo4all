#include<stdio.h>
#include<stdlib.h>
#include<string.h>

// #define Z80_ONLY_CHANGES_PATCH
#define RAZE_NO_MAPPING


#include	"../neo4all.h"
#include 	<stdio.h>
#include 	<stdlib.h>
#include	"z80intrf.h"
#include	"sound/2610intf.h"


Uint16 z80_bank[4];

#if defined(AES) && defined(USE_RAZE) && !defined(RAZE_NO_MAPPING)
#define mame_z80mem aes4all_memory_sm1
#else
Uint8 mame_z80mem[0x10000];
#endif

#ifndef AES
Uint8 subcpu_memspace[0x10000];
#define memory_sm1 ((unsigned)subcpu_memspace)
#else
#include "aes/aes.h"
#define memory_sm1 ((unsigned)aes4all_memory_sm1)
#endif

Uint32 sound_code;
Uint32 result_code;
Uint32 pending_command;
Uint32 z80_cycles;

#ifdef AES
#if !defined(USE_RAZE) || defined(RAZE_NO_MAPPING)
static void _fast_memcpy(void *OUT, const void *IN, size_t N)
{
	register unsigned *d=(unsigned *)OUT;
	register unsigned *s=(unsigned *)IN;
	register unsigned n=N;
	while(n--){
#ifdef DREAMCAST
		asm("pref @%0" : : "r" (s + 8));
#endif
		*d++=*s++;
		*d++=*s++;
		*d++=*s++;
		*d++=*s++;
		*d++=*s++;
		*d++=*s++;
		*d++=*s++;
		*d++=*s++;
	}
}

#define fast_memcpy(OUT, IN, N) _fast_memcpy(OUT,IN,N>>5)

#define change_z80_map(DST,SRC,LEN) \
	fast_memcpy( (void *)(((unsigned)mame_z80mem)+(DST)), (void *)(((unsigned)memory_sm1)+(SRC)),(LEN));

#else

extern unsigned _z80_Fetch[];
extern unsigned _z80_Read[];
extern unsigned _z80_Write[];

static void change_z80_map(unsigned dst, unsigned src, unsigned len)
{
/*
	unsigned i=dst>>8;
	src=((unsigned)memory_sm1)+src-(i<<8);
	len>>8;
	len+=i;
	for(;i<len;i++)
		_z80_Fetch[i]=_z80_Read[1+(i<<1)]=_z80_Write[1+(i<<1)]=src;
*/
	z80_map_fetch(dst, dst+len-1, (unsigned char *)(((unsigned)memory_sm1)+(src)));
	z80_map_read(dst,  dst+len-1, (unsigned char *)(((unsigned)memory_sm1)+(src)));
	z80_map_write(dst, dst+len-1, (unsigned char *)(((unsigned)memory_sm1)+(src)));
}
#endif

#endif





#ifndef USE_RAZE
#ifndef USE_MAMEZ80

#ifdef USE_CZ80

cz80_struc neo4all_cz80_struc;


static __inline__ unsigned char _cpu_readmem8(unsigned int address)
{
//printf("cpu_readmem8(%i), PC=0x%X\n",address,z80_get_reg(Z80_PC));
	return (mame_z80mem[address&0xFFFF]);
}

static unsigned char cpu_readmem8(unsigned int address)
{
	return _cpu_readmem8(address);
}

#if CZ80_USE_WORD_HANDLER
static unsigned short cpu_readmem16(unsigned int address)
{
	return _cpu_readmem8(address) | (_cpu_readmem8(address + 1) << 8);
}
#endif

static __inline__ void _cpu_writemem8(unsigned int address, unsigned int data)
{
//printf("cpu_writemem8(%i,%i), PC=0x%X\n",address,data,z80_get_reg(Z80_PC));
	mame_z80mem[address&0xFFFF]=data;
}

static void cpu_writemem8(unsigned int address, unsigned int data)
{
	_cpu_writemem8(address,data);
}

#if CZ80_USE_WORD_HANDLER
static void cpu_writemem16(unsigned int address, unsigned int data)
{
	_cpu_writemem8(address, data & 0xFF);
	_cpu_writemem8(address + 1, data >> 8);
}
#endif


#else

void	PortWrite(UINT16 PortNo, UINT8 data, struct z80PortWrite *ptr);
UINT16	PortRead(UINT16 PortNo, struct z80PortRead *ptr);

struct	z80PortRead ReadPorts[] =
{
	{0x0000,	0xffff,		PortRead,	NULL},
	{(UINT16)-1,	(UINT16)-1,	NULL,		NULL}
};

struct	z80PortWrite WritePorts[] =
{
	{0x0000,	0xffff,		PortWrite,	NULL},
	{(UINT16)-1,	(UINT16)-1,	NULL,		NULL}
};

struct MemoryReadByte	MemRead[] =
{
	{(UINT16)-1,	(UINT16)-1,	NULL,		NULL}
};

struct MemoryWriteByte	MemWrite[] =
{
	{(UINT16)-1,	(UINT16)-1,	NULL,		NULL}
};

CONTEXTMZ80	subcpu_context;

#endif
#endif

int cpu_z80_irq_callback(int a)
{
//printf("cpu_z80_irq_callback(%i), PC=0x%X\n",a,z80_get_reg(Z80_PC));
	neogeo_sound_irq(a);
	return 0;
}
#endif


void _z80_init(void)
{
#if defined(AES) && (!defined(USE_RAZE) || defined(RAZE_NO_MAPPING))
	memset(mame_z80mem, 0, 0x10000);
	memcpy(mame_z80mem, aes4all_memory_sm1, 0xf800);
#endif
#ifdef Z80_ONLY_CHANGES_PATCH
	z80_bank[0]=0x8000;
	z80_bank[1]=0xc000;
	z80_bank[2]=0xe000;
	z80_bank[3]=0xf000;
#endif
#ifdef USE_RAZE
	z80_init_memmap();
	z80_map_fetch(0x0000, 0xffff, (unsigned char *)&mame_z80mem);
	z80_add_read(0x0000, 0xffff, Z80_MAP_DIRECT, (void *)mame_z80mem);
	z80_add_write(0x0000, 0xffff, Z80_MAP_DIRECT, (void *)mame_z80mem);
	z80_end_memmap();
	z80_set_in((unsigned char (*)(short unsigned int))&cz80_z80_readport16);
	z80_set_out((void (*)(short unsigned int, unsigned char))&cz80_z80_writeport16);
	z80_reset();
#else

#ifndef USE_MAMEZ80
#ifdef USE_CZ80
	Cz80_Init(&neo4all_cz80_struc);
	Cz80_Set_Fetch(&neo4all_cz80_struc,0x0000,0xFFFF,(Uint32)((void *)&mame_z80mem));
	Cz80_Set_ReadB(&neo4all_cz80_struc,(unsigned int (*)(unsigned int))&cpu_readmem8);
	Cz80_Set_WriteB(&neo4all_cz80_struc,&cpu_writemem8);
#if CZ80_USE_WORD_HANDLER
	Cz80_Set_ReadW(&neo4all_cz80_struc,&cpu_readmem16);
	Cz80_Set_WriteW(&neo4all_cz80_struc,&cpu_writemem16);
#endif
	Cz80_Set_INPort(&neo4all_cz80_struc,(CZ80_READ *)&cz80_z80_readport16);
	Cz80_Set_OUTPort(&neo4all_cz80_struc,(CZ80_WRITE *)&cz80_z80_writeport16);
	Cz80_Set_IRQ_Callback(&neo4all_cz80_struc,cpu_z80_irq_callback);
	Cz80_Reset(&neo4all_cz80_struc);
#ifndef AES
//	Cz80_Exec(&neo4all_cz80_struc,100000);
#endif
#else
	subcpu_context.z80Base = mame_z80mem;
	
	subcpu_context.z80IoRead  = ReadPorts;
	subcpu_context.z80IoWrite = WritePorts;
	
	subcpu_context.z80MemRead = MemRead;
	subcpu_context.z80MemWrite = MemWrite;
	
	mz80SetContext((void *)&subcpu_context);

	mz80reset();

	// Let Z80 do its initialization
#ifndef AES
	mz80exec(100000);
#endif
#endif
#else
	z80_init();
	z80_reset(NULL);
	z80_set_irq_callback(cpu_z80_irq_callback);
#ifndef AES
	z80_execute(100000);
#endif
#endif
#endif
}

//---------------------------------------------------------------------------
#ifndef USE_MAMEZ80
#if defined(USE_CZ80) || defined(USE_RAZE)
void cz80_z80_writeport16(Uint16 PortNo, Uint8 data)
#else
void PortWrite(UINT16 PortNo, UINT8	data, struct z80PortWrite *ptr)
#endif
#else
void mame_z80_writeport16(Uint16 PortNo, Uint8 data)
#endif
{
//printf("z80_writeport16(%.2X,%.2X)\n",PortNo&0xff,data);
	switch( PortNo & 0xff)
	{
	case	0x4:
		YM2610_control_port_0_A_w(0,data);
		break;

	case	0x5:
		YM2610_data_port_0_A_w(0,data);
		break;

	case	0x6:
		YM2610_control_port_0_B_w(0,data);
		break;

	case	0x7:
		YM2610_data_port_0_B_w(0,data);
		break;
/*
	case	0x8:
		// NMI enable / acknowledge? (the data written doesn't matter) 
		break;
*/	
	case	0xc:
		result_code = data;
		break;
/*	
	case	0x18:
		// NMI disable? (the data written doesn't matter)
		break;
*/
/*
	default:
		//printf("Unimplemented Z80 Write Port: %x data: %x\n",PortNo&0xff,data);
		break;
*/
	}
}


//---------------------------------------------------------------------------
#ifndef USE_MAMEZ80
#if defined(USE_CZ80) || defined(USE_RAZE)
Uint8 cz80_z80_readport16(Uint16 PortNo)
#else
UINT16 PortRead(UINT16 PortNo, struct z80PortRead *ptr)
#endif
#else
Uint8 mame_z80_readport16(Uint16 PortNo)
#endif
{
//printf("z80_readport16(%.2X)\n",PortNo&0xff);
	switch( PortNo & 0xff)
	{
	case	0x0:
		pending_command = 0;
		return sound_code;
	
	case	0x4:
		return YM2610_status_port_0_A_r(0);
	
	case	0x5:
		return YM2610_read_port_0_r(0);
	
	case	0x6:
		return YM2610_status_port_0_B_r(0);

#ifdef AES
	case 0x08:
#ifdef Z80_ONLY_CHANGES_PATCH
		if (z80_bank[3]!=PortNo)
#endif
		{
#ifdef Z80_ONLY_CHANGES_PATCH
			z80_bank[3]=PortNo;
#endif
			change_z80_map(0xf000, ((unsigned)((PortNo>>8)&0x7f))<<11, 0x0800);
		}
		break;
		
	case 0x09:
#ifdef Z80_ONLY_CHANGES_PATCH
		if (z80_bank[2]!=PortNo)
#endif
		{
#ifdef Z80_ONLY_CHANGES_PATCH
			z80_bank[2]=PortNo;
#endif
			change_z80_map(0xe000, ((unsigned)((PortNo>>8)&0x3f))<<12, 0x1000);
		}
		break;
		
	case 0x0a:
#ifdef Z80_ONLY_CHANGES_PATCH
		if (z80_bank[1]!=PortNo)
#endif
		{
#ifdef Z80_ONLY_CHANGES_PATCH
			z80_bank[1]=PortNo;
#endif
			change_z80_map(0xc000, ((unsigned)((PortNo>>8)&0x1f))<<13, 0x2000);
		}
		break;

	case 0x0b:
#ifdef Z80_ONLY_CHANGES_PATCH
		if (z80_bank[0]!=PortNo)
#endif
		{
#ifdef Z80_ONLY_CHANGES_PATCH
			z80_bank[0]=PortNo;
#endif
			change_z80_map(0x8000, ((unsigned)((PortNo>>8)&0x0f))<<14, 0x4000);
		}
		break;
#endif

/*
	default:
		//printf("Unimplemented Z80 Read Port: %d\n",PortNo&0xff);
		break;
*/
	};	
	return 0;
}


#ifdef USE_MAMEZ80
void cpu_z80_nmi(void)
{
//printf("cpu_z80_nmi PC=0x%X\n",z80_get_reg(Z80_PC));
	z80_set_irq_line(IRQ_LINE_NMI, ASSERT_LINE);
	z80_set_irq_line(IRQ_LINE_NMI, CLEAR_LINE);
}

void cpu_z80_init(void)
{
//printf("cpu_z80_init PC=0x%X\n",z80_get_reg(Z80_PC));
	z80_set_irq_callback(cpu_z80_irq_callback);
}

void cpu_z80_raise_irq(int l)
{
//printf("cpu_z80_raise(%i) PC=0x%X\n",l,z80_get_reg(Z80_PC));
	z80_set_irq_line(l, ASSERT_LINE);
}
void cpu_z80_lower_irq(void)
{
//printf("cpu_z80_lower_irq PC=0x%X\n",z80_get_reg(Z80_PC));
	z80_set_irq_line(0, CLEAR_LINE);
}

void cpu_z80_reset(void)
{
	z80_set_irq_callback(cpu_z80_irq_callback);
	z80_reset(NULL);
	z80_set_irq_callback(cpu_z80_irq_callback);
#ifndef AES
	z80_execute(100000);
#endif
//printf("cpu_z80_reset PC=0x%X\n",z80_get_reg(Z80_PC));
}
#endif
