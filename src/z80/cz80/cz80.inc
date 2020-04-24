/********************************************************************************/
/*                                                                              */
/* CZ80 macro file                                                              */
/* C Z80 emulator version 0.92                                                  */
/* Copyright 2004-2005 Stéphane Dallongeville                                   */
/*                                                                              */
/********************************************************************************/

// use zR8 for B/C/D/E/H/L registers only
// use zR16 for BC/DE/HL registers only

#define zR8(A)          CPU->r8[(A) ^ 1]
#define zR16(A)         CPU->r16[A].W
#define pzR16(A)        &(CPU->r16[A])

#define pzFA            &(CPU->FA)
#define zFA             CPU->FA.W
#define zlFA            CPU->FA.B.L
#define zhFA            CPU->FA.B.H
#define zA              zlFA
#define zF              zhFA

#define pzBC            &(CPU->BC)
#define zBC             CPU->BC.W
#define zlBC            CPU->BC.B.L
#define zhBC            CPU->BC.B.H
#define zB              zhBC
#define zC              zlBC

#define pzDE            &(CPU->DE)
#define zDE             CPU->DE.W
#define zlDE            CPU->DE.B.L
#define zhDE            CPU->DE.B.H
#define zD              zhDE
#define zE              zlDE

#define pzHL            &(CPU->HL)
#define zHL             CPU->HL.W
#define zlHL            CPU->HL.B.L
#define zhHL            CPU->HL.B.H
#define zH              zhHL
#define zL              zlHL

#define zFA2            CPU->FA2.W
#define zlFA2           CPU->FA2.B.L
#define zhFA2           CPU->FA2.B.H
#define zA2             zlFA2
#define zF2             zhFA2

#define zBC2            CPU->BC2.W
#define zDE2            CPU->DE2.W
#define zHL2            CPU->HL2.W

#define pzIX            &(CPU->IX)
#define zIX             CPU->IX.W
#define zlIX            CPU->IX.B.L
#define zhIX            CPU->IX.B.H

#define pzIY            &(CPU->IY)
#define zIY             CPU->IY.W
#define zlIY            CPU->IY.B.L
#define zhIY            CPU->IY.B.H

#define pzSP            &(CPU->SP)
#define zSP             CPU->SP.W
#define zlSP            CPU->SP.B.L
#define zhSP            CPU->SP.B.H

#define zPC             PC
#define zRealPC         ((u32)(PC) - CPU->BasePC)

#define zI              CPU->I
#define zIM             CPU->IM

#define zwR             CPU->R.W
#define zR1             CPU->R.B.L
#define zR2             CPU->R.B.H
#define zR              zR1

#define zIFF            CPU->IFF.W
#define zIFF1           CPU->IFF.B.L
#define zIFF2           CPU->IFF.B.H


#if CZ80_USE_JUMPTABLE
#define _SSOP(A,B) A##B
#define OP(A) _SSOP(OP,A)
#define OPCB(A) _SSOP(OPCB,A)
#define OPED(A) _SSOP(OPED,A)
#define OPXY(A) _SSOP(OPXY,A)
#define OPXYCB(A) _SSOP(OPXYCB,A)
#else
#define OP(A) case A
#define OPCB(A) case A
#define OPED(A) case A
#define OPXY(A) case A
#define OPXYCB(A) case A
#endif

#define GET_BYTE                \
    (*zPC)

#define GET_BYTE_S              \
    ((s8)(*zPC))

#if CZ80_LITTLE_ENDIAN
#define GET_WORD                \
    (zPC[0] | (zPC[1] << 8))
#else
#define GET_WORD                \
    (zPC[1] | (zPC[0] << 8))
#endif

#define FETCH_BYTE              \
    (*zPC++)

#define FETCH_BYTE_S            \
    ((s8)(*zPC++))

#define FETCH_WORD(A)           \
    A = GET_WORD;               \
    zPC += 2;

#if CZ80_SIZE_OPT
    #define RET(A)              \
        CCnt -= A;              \
        goto Cz80_Exec_Check;
#else
    #define RET(A)              \
        if ((CCnt -= A) <= 0) goto Cz80_Check_Int;  \
        goto Cz80_Exec;
#endif

#define SET_PC(A)               \
    CPU->BasePC = (u32) CPU->Fetch[(A) >> CZ80_FETCH_SFT];  \
    zPC = (u8*)((A) + CPU->BasePC);

#define PRE_IO                  \
    CPU->CycleIO = CCnt;

#define POST_IO                 \
    CCnt = CPU->CycleIO;

#define READ_BYTE(A, D)                 \
    D = CPU->Read_Byte(A);

#if CZ80_USE_WORD_HANDLER
#define READ_WORD(A, D)                 \
    D = CPU->Read_Word(A);
#elif CZ80_LITTLE_ENDIAN
#define READ_WORD(A, D)                 \
    D = CPU->Read_Byte(A) | (CPU->Read_Byte((A) + 1) << 8);
#else
#define READ_WORD(A, D)                 \
    D = (CPU->Read_Byte(A) << 8) | CPU->Read_Byte((A) + 1);
#endif

#define READSX_BYTE(A, D)               \
    D = (s8)CPU->Read_Byte(A);

#define WRITE_BYTE(A, D)                \
    CPU->Write_Byte(A, D);

#if CZ80_USE_WORD_HANDLER
#define WRITE_WORD(A, D)                \
    CPU->Write_Word(A, D);
#elif CZ80_LITTLE_ENDIAN
#define WRITE_WORD(A, D)                \
    CPU->Write_Byte(A, D);              \
    CPU->Write_Byte((A) + 1, (D) >> 8);
#else
#define WRITE_WORD(A, D)                \
    CPU->Write_Byte(A, D);              \
    CPU->Write_Byte((A) + 1, (D) >> 8);
#endif

#define PUSH_16(A)          \
    {                       \
        u32 sp;             \
                            \
        zSP -= 2;           \
        sp = zSP;           \
        WRITE_WORD(sp, A);  \
    }

#define POP_16(A)           \
    {                       \
        u32 sp;             \
                            \
        sp = zSP;           \
        READ_WORD(sp, A)    \
        zSP = sp + 2;       \
    }

#define IN(A, D)            \
    D = CPU->IN_Port(A);

#define OUT(A, D)           \
    CPU->OUT_Port(A, D);
