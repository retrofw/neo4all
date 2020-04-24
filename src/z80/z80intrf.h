#ifndef	Z80INTRF_H
#define	Z80INTRF_H

#ifdef Z80_EMULATED

void z80_init(void);


extern Uint32 sound_code;
extern Uint32 result_code;
extern Uint32 pending_command;
extern Uint32 z80_cycles;
extern Uint8 mame_z80mem[0x10000];
#ifndef AES
extern Uint8 subcpu_memspace[0x10000];
#endif
#ifdef Z80_ONLY_CHANGES_PATCH
extern Uint16 z80_bank[4];
#endif

void _z80_init(void);

#ifdef USE_RAZE

#include "z80/raze/raze.h"
#ifndef DEBUG_Z80
#define _z80nmi z80_cause_NMI
#define _z80raise z80_raise_IRQ
#define _z80lower z80_lower_IRQ
#define _z80reset_real z80_reset
#define _z80exec z80_emulate
#else

extern int trazando;

#define _z80nmi() { if (trazando) puts("_z80nmi"); z80_cause_NMI();}
#define _z80raise(VECT) { if (trazando) printf("_z80raise %X\n",(VECT)&0xff); z80_raise_IRQ(VECT);}
#define _z80lower() { if (trazando) puts("_z80lower"); z80_lower_IRQ();}
#define _z80reset_real() { if (trazando) puts("_z80reset_real"); z80_reset();}
#define _z80exec(NCLY) { \
	if (trazando)  { printf("_z80exec(%i)\n",NCLY); trazando++; if (trazando>10) exit(0); } \
	int z80falta=NCLY; \
	do { \
		unsigned short nowpc=z80_get_reg(Z80_REG_PC); \
		if (trazando) \
			printf("PC=%.4X (%.2X%.2X%.2X%.2X) A=%.2X BC=%.4X DE=%.4X HL=%.4X IX=%.4X IY=%.4X SP=%.4X R=%.2X IM=%i/%i\n",nowpc,mame_z80mem[nowpc],mame_z80mem[nowpc+1],mame_z80mem[nowpc+2],mame_z80mem[nowpc+3],z80_get_reg(Z80_REG_AF)>>8,z80_get_reg(Z80_REG_BC),z80_get_reg(Z80_REG_DE),z80_get_reg(Z80_REG_HL), z80_get_reg(Z80_REG_IX), z80_get_reg(Z80_REG_IY), z80_get_reg(Z80_REG_SP), z80_get_reg(Z80_REG_IR)&0xFF, z80_get_reg(Z80_REG_IM),z80_get_reg(Z80_REG_IRQLine)?1:0); \
		z80falta-=6; \
       	z80_emulate(1); \
	}while(z80falta>0); \
}
#endif

void cz80_z80_writeport16(Uint16 PortNo, Uint8 data);
Uint8 cz80_z80_readport16(Uint16 PortNo);

#else

#ifdef USE_MAMEZ80

#include "z80/mamez80/z80.h"
#define _z80exec z80_execute
#define _z80nmi cpu_z80_nmi
#define _z80raise cpu_z80_raise_irq
#define _z80lower cpu_z80_lower_irq
#define _z80reset_real cpu_z80_reset
void cpu_z80_reset(void);

#else

#ifdef USE_CZ80

#include "z80/cz80/cz80.h"
extern cz80_struc neo4all_cz80_struc;
#define _z80raise(VEC) Cz80_Set_IRQ(&neo4all_cz80_struc, VEC)
#define _z80lower() Cz80_Clear_IRQ(&neo4all_cz80_struc)
#ifndef DEBUG_Z80
#define _z80exec(CIC) Cz80_Exec(&neo4all_cz80_struc,CIC)
#else
#define _z80exec(NCLY) { \
printf("_z80exec(%i)\n",NCLY); \
	int z80falta=NCLY; \
	do { \
		unsigned short nowpc=Cz80_Get_PC(&neo4all_cz80_struc); \
		printf("PC=%.4X (%.2X%.2X%.2X%.2X) A=%.2X BC=%.4X DE=%.4X HL=%.4X\n",nowpc,mame_z80mem[nowpc],mame_z80mem[nowpc+1],mame_z80mem[nowpc+2],mame_z80mem[nowpc+3],Cz80_Get_AF(&neo4all_cz80_struc)>>8,Cz80_Get_BC(&neo4all_cz80_struc),Cz80_Get_DE(&neo4all_cz80_struc),Cz80_Get_HL(&neo4all_cz80_struc)); \
		z80falta-=6; \
	Cz80_Exec(&neo4all_cz80_struc,1); \
	}while(z80falta>0); \
}
#endif
#define _z80nmi() Cz80_Set_NMI(&neo4all_cz80_struc)
static __inline__ void _z80reset_real(void)
{
#ifndef AES
	memcpy(mame_z80mem,subcpu_memspace,0x10000);
#else
//	memcpy(mame_z80mem, aes4all_memory_sm1, 0xf800);
#endif
	Cz80_Reset(&neo4all_cz80_struc);
}

void cz80_z80_writeport16(Uint16 PortNo, Uint8 data);
Uint8 cz80_z80_readport16(Uint16 PortNo);

#else

#include "z80/mz80/mz80.h"
#define _z80raise mz80int
#define _z80lower() 
#define _z80exec mz80exec
#define _z80nmi mz80nmi

#endif
#endif
#endif

#ifdef AES
#define _z80reset() _z80reset_real()
#else
#define _z80reset() \
	memcpy(mame_z80mem,subcpu_memspace,0x10000); \
	_z80reset_real()
#endif

#endif

#endif /* Z80INTRF_H */


