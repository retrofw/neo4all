/**************************************
****    CDROM.C  -  File reading   ****
**************************************/

//-- Include files -----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <ctype.h>

#include <string.h>
#include <unistd.h>

#include "cdrom.h"
#include "../neo4all.h"

#include "../menu/menu.h"
#include "console.h"

#ifdef USE_VIDEO_GL
#include "video/videogl.h"
#endif

#ifndef LOWERCASEFILES
#define LOWERCASEFILES
#endif

#ifdef CDISO
#define cdrom_fopen(FN,MD) (FILE *)CDR_fopen(FN,MD)
#define cdrom_fclose(ST) CDR_fclose((void *)ST)
#define cdrom_fread(PTR,SZ,NM,ST) CDR_fread(PTR,SZ,NM,(void*)ST)
#define cdrom_feof(ST) CDR_feof((void*)ST)
#define cdrom_fgets(STR,SZ,FS) CDR_fgets(STR,SZ,(void *)FS)
#else
#define cdrom_fopen(FN,MD) fopen_retry(FN,MD)
FILE *fopen_retry(char *filename, char *mode);
#define cdrom_fclose(ST) fclose(ST)
#define cdrom_fread(PTR,SZ,NM,ST) fread(PTR,SZ,NM,ST)
#define cdrom_feof(ST) feof(ST)
#define cdrom_fgets(STR,SZ,FS) fgets(STR,SZ,FS)
#endif

#define MIN_TEXT_DRAW 2

#ifdef LOWERCASEFILES

#define CHANGECASE  tolower
#define IPL_TXT     "ipl.txt"
#define PRG     "prg"
#define FIX     "fix"
#define SPR      "spr"
#define OBJ	"obj"
#define Z80      "z80"
#define PAT      "pat"
#define PCM      "pcm"
#define JUE      "jue"
#define TITLE_X_SYS "title_x.sys"

#else

#define CHANGECASE  toupper
#define IPL_TXT  "IPL.TXT"
#define PRG     "PRG"
#define FIX     "FIX"
#define SPR      "SPR"
#define OBJ	"OBJ"
#define Z80      "Z80"
#define PAT      "PAT"
#define PCM      "PCM"
#define JUE      "JUE"
#define TITLE_X_SYS "TITLE_X.SYS"

#endif

/*-- Definitions -----------------------------------------------------------*/
#define    BUFFER_SIZE    131072
#define PRG_TYPE    0
#define FIX_TYPE    1
#define SPR_TYPE    2
#define Z80_TYPE    3
#define PAT_TYPE    4
#define PCM_TYPE    5
#define    min(a, b) ((a) < (b)) ? (a) : (b)

/*-- Exported Functions ----------------------------------------------------*/
int    neogeo_cdrom_init1(void);
int    neogeo_cdrom_load_prg_file(char *, unsigned int);
int    neogeo_cdrom_load_z80_file(char *, unsigned int);
int    neogeo_cdrom_load_fix_file(char *, unsigned int);
int    neogeo_cdrom_load_spr_file(char *, unsigned int);
int    neogeo_cdrom_load_pcm_file(char *, unsigned int);
int    neogeo_cdrom_load_pat_file(char *, unsigned int, unsigned int);
void    neogeo_cdrom_load_files(void);
int    neogeo_cdrom_process_ipl(int check);
void    neogeo_cdrom_shutdown(void);
void    fix_conv(unsigned char *, unsigned char *, int, unsigned char *);
void    spr_conv(unsigned char *, unsigned char *, int, unsigned char *);
void    neogeo_upload(void);
void    neogeo_cdrom_load_title(void);
void    neogeo_cdrom_apply_patch(short *source, int offset, int bank);

//-- Private Variables -------------------------------------------------------
static char    neogeo_cdrom_buffer[BUFFER_SIZE];
static char    cdpath[256];

//-- Private Function --------------------------------------------------------
static    int    recon_filetype(char *);

//-- Exported Variables ------------------------------------------------------
int        neogeo_cdrom_current_drive;
int        img_display = 1;

#ifndef AES
//----------------------------------------------------------------------------
int    neogeo_cdrom_init1(void)
{    
    FILE *fp;
    char Path[256];
    int found=0,number;

#ifndef CDISO
    console_printf("\nDetecting CD-ROM drives... ");

    /* Check for CD drives */
    if(!(number=SDL_CDNumDrives())){
        /* None found */
        console_puts("No CDROM devices available");
	console_wait();
	drawNoCD();
        exit(-1);
    }
#else
    extern char neo4all_image_file[];
    CDR_init();
    CDR_SetIsoFile(neo4all_image_file); //ROM_PREFIX "/game.iso");
    CDR_open();

    strcpy(cdpath,"");
    number=1;
#endif

    neogeo_cdrom_current_drive=0;

    console_printf("%d CD-ROM drives found\n",number);


    //find 1st neogeoCD in system
    while(!found && neogeo_cdrom_current_drive<number)
    {
        console_printf("Trying %s\n",SDL_CDName(neogeo_cdrom_current_drive));

#ifndef CDISO
        if(getMountPoint(neogeo_cdrom_current_drive,cdpath))
#endif
        {
            strcpy(Path, cdpath);
            strcat(Path, IPL_TXT);
            if((fp=cdrom_fopen(Path,"rb"))!=NULL)
            {
                found=1;
                cdrom_fclose(fp);
                console_printf("Found Neogeo CD in drive: %s\n",SDL_CDName(neogeo_cdrom_current_drive));
            } else 
                neogeo_cdrom_current_drive++;
#ifndef CDISO
        } else {
            console_printf ("CD drive %s is not mounted\n",SDL_CDName(neogeo_cdrom_current_drive));
#endif
            
            neogeo_cdrom_current_drive++;
        }
    }

    if(!found)
    {
    	console_println();
        console_printf("No NeogeoCD Disc available\n");
	console_wait();
	drawNoNeoGeoCD();
	return 0;
    }

    return 1;
}
#endif

//----------------------------------------------------------------------------
void    neogeo_cdrom_shutdown(void)
{
    /* free loading picture surface ??? */
}

//----------------------------------------------------------------------------
int    neogeo_cdrom_load_prg_file(char *FileName, unsigned int Offset)
{
    FILE    *fp;
    char    Path[256];
    char    *Ptr;
    int        Readed;

#ifdef DEBUG_CDROM
    printf("PRG Lectura %s desde %i\n",FileName,Offset);
#endif

//    sound_disable();
    strcpy(Path, cdpath);
    strcat(Path, FileName);

    fp = cdrom_fopen(Path, "rb");
    if (fp==NULL) {
//        sprintf(global_error, "Could not open %s", Path);
//    	sound_enable();
        return 0;
    }
    
    Ptr = neogeo_prg_memory + Offset;
    
    do {
        Readed = cdrom_fread(neogeo_cdrom_buffer, 1, BUFFER_SIZE, fp);
#ifdef DEBUG_CDROM
	printf("PRG Lectura desde la posicion %i (%i bytes), resto=%i\n",Ptr-neogeo_prg_memory,Readed,0x200000-(Ptr-neogeo_prg_memory));
#endif
        swab(neogeo_cdrom_buffer, Ptr, Readed);
        Ptr += Readed;
    } while(  !cdrom_feof(fp) );//Readed==BUFFER_SIZE &&
    
    cdrom_fclose(fp);

//    sound_enable();
#ifdef DEBUG_CDROM
    puts("PRG Ok.");
#endif
    return 1;
}

//----------------------------------------------------------------------------
int    neogeo_cdrom_load_z80_file(char *FileName, unsigned int Offset)
{
#ifdef Z80_EMULATED
    FILE    *fp;
    char    Path[256];
#ifdef DEBUG_CDROM
    printf("Z80 Lectura %s desde %i\n",FileName,Offset);
#endif

//    sound_disable();
    strcpy(Path, cdpath);
    strcat(Path, FileName);

    fp = cdrom_fopen(Path, "rb");
    if (fp==NULL) {
//        sprintf(global_error, "Could not open %s", Path);
//    	sound_enable();
        return 0;
    }

#ifdef DEBUG_CDROM
    printf("Z80 Lectura desde la posicion %i (%i bytes)\n",Offset,0x10000-Offset);
#endif
    cdrom_fread( &subcpu_memspace[Offset], 1, 0x10000-Offset, fp);
    memcpy(&mame_z80mem[Offset],&subcpu_memspace[Offset],0x10000-Offset);

    cdrom_fclose(fp);
//    sound_enable();
#ifdef DEBUG_CDROM
    puts("Z80 Ok.");
#endif
#endif
    return 1;
}

//----------------------------------------------------------------------------
int    neogeo_cdrom_load_fix_file(char *FileName, unsigned int Offset)
{
    FILE    *fp;
    char    Path[256];
    char    *Ptr, *Src;
    int        Readed;
#ifdef DEBUG_CDROM
    printf("FIX Lectura %s desde %i\n",FileName,Offset);
#endif

//    sound_disable();
    strcpy(Path, cdpath);
    strcat(Path, FileName);
    
    fp = cdrom_fopen(Path, "rb");
    if (fp==NULL) {
//        sprintf(global_error, "Could not open %s", Path);
//    	sound_enable();
        return 0;
    }
    
    Ptr = neogeo_fix_memory + Offset;
    
    do {
        memset(neogeo_cdrom_buffer, 0, BUFFER_SIZE);
        Readed = cdrom_fread(neogeo_cdrom_buffer, 1, BUFFER_SIZE, fp);
#ifdef DEBUG_CDROM
    printf("FIX Lectura desde la posicion %i (%i bytes), resto=%i\n",Ptr-neogeo_fix_memory,Readed,0x20000-(Ptr-neogeo_fix_memory));
#endif
        Src = neogeo_cdrom_buffer;
        fix_conv((unsigned char *)Src, (unsigned char *)Ptr, Readed, (unsigned char *)(video_fix_usage + (Offset>>5)));
        Ptr += Readed;
        Offset += Readed;
    } while( Readed == BUFFER_SIZE );
    
    cdrom_fclose(fp);
    
//    sound_enable();
#ifdef DEBUG_CDROM
    puts("FIX Ok.");
#endif
    return 1;
}

//----------------------------------------------------------------------------
int    neogeo_cdrom_load_spr_file(char *FileName, unsigned int Offset)
{
    FILE    *fp;
    char    Path[256];
    char    *Ptr;
    int        Readed;
#ifdef DEBUG_CDROM
    printf("SPR Lectura %s desde %i\n",FileName,Offset);
#endif

//    sound_disable();
    strcpy(Path, cdpath);
    strcat(Path, FileName);
    
    fp = cdrom_fopen(Path, "rb");
    if (fp==NULL) {
//        sprintf(global_error, "Could not open %s", Path);
//    	sound_enable();
        return 0;
    }
    
    Ptr = neogeo_spr_memory + Offset;
    
    do {
        memset(neogeo_cdrom_buffer, 0, BUFFER_SIZE);
        Readed = cdrom_fread(neogeo_cdrom_buffer, 1, BUFFER_SIZE, fp);
#ifdef DEBUG_CDROM
    printf("SPR Lectura desde la posicion %i (%i bytes), resto=%i\n",Ptr-neogeo_spr_memory,Readed,0x400000-(Ptr-neogeo_spr_memory));
#endif
        spr_conv((unsigned char*)neogeo_cdrom_buffer, (unsigned char *)Ptr, Readed, (unsigned char *)(video_spr_usage + (Offset>>7)));
        Offset += Readed;
        Ptr += Readed;
    } while( Readed == BUFFER_SIZE );
    
    cdrom_fclose(fp);
    
//    sound_enable();
#ifdef DEBUG_CDROM
    puts("SPR Ok.");
#endif
    return 1;
}

//----------------------------------------------------------------------------
int    neogeo_cdrom_load_pcm_file(char *FileName, unsigned int Offset)
{
    FILE        *fp;
    char        Path[256];
    char        *Ptr;
#ifdef DEBUG_CDROM
    printf("PCM Lectura %s desde %i\n",FileName,Offset);
#endif

//    sound_disable();
    strcpy(Path, cdpath);
    strcat(Path, FileName);

    fp = cdrom_fopen(Path, "rb");
    if (fp==NULL) {
//        sprintf(global_error, "Could not open %s", Path);
//    	sound_enable();
        return 0;
    }

    Ptr = neogeo_pcm_memory + Offset;
#ifdef DEBUG_CDROM
    printf("PCM Lectura desde la posicion %i (%i bytes)\n",Offset,0x100000-Offset);
#endif
    cdrom_fread(Ptr, 1, 0x100000-Offset, fp);

    cdrom_fclose(fp);

//    sound_enable();
#ifdef DEBUG_CDROM
    puts("PCM Ok.");
#endif
    return 1;
}

//----------------------------------------------------------------------------
int    neogeo_cdrom_load_pat_file(char *FileName, unsigned int Offset, unsigned int Bank)
{
    FILE    *fp;
    char    Path[256];
    int        Readed;
#ifdef DEBUG_CDROM
    printf("PAT Lectura %s desde %i\n",FileName,Offset);
#endif

//    sound_disable();
    strcpy(Path, cdpath);
    strcat(Path, FileName);
    
    fp = cdrom_fopen(Path, "rb");
    if (fp==NULL) {
//        sprintf(global_error, "Could not open %s", Path);
//    	sound_enable();
        return 0;
    }
    
    Readed = cdrom_fread(neogeo_cdrom_buffer, 1, BUFFER_SIZE, fp);
#ifdef DEBUG_CDROM
    printf("PAT Lectura desde la posicion %i (%i bytes)\n",0,Readed);
#endif
    swab(neogeo_cdrom_buffer, neogeo_cdrom_buffer, Readed);
    neogeo_cdrom_apply_patch((short*)neogeo_cdrom_buffer, Offset, Bank);

    cdrom_fclose(fp);

//    sound_enable();
#ifdef DEBUG_CDROM
    puts("PAT Ok.");
#endif
    return 1;
}




int hextodec(char c) {
	switch (tolower(c)) {
	case '0':	return 0;
	case '1':	return 1;
	case '2':	return 2;
	case '3':	return 3;
	case '4':	return 4;
	case '5':	return 5;
	case '6':	return 6;
	case '7':	return 7;
	case '8':	return 8;
	case '9':	return 9;
	case 'a':	return 10;
	case 'b':	return 11;
	case 'c':	return 12;
	case 'd':	return 13;
	case 'e':	return 14;
	case 'f':	return 15;
	default:	return 0;
	}
}



//----------------------------------------------------------------------------
int    neogeo_cdrom_process_ipl(int check)
{
    FILE    *fp;
    char    Path[256];
    char    Line[32];
    char    FileName[16];
    int        FileType;
    int        Bnk;
    int        Off;
    int        i, j;
    int	       ret=0;

    sound_disable();
    strcpy(Path, cdpath);
    strcat(Path, IPL_TXT);
    
    fp = cdrom_fopen(Path, "rb");
    
    if (fp==NULL) {
//        strcpy(global_error, "Could not open IPL.TXT!");
    	sound_enable();
        return 0;
    }
    while (cdrom_fgets(Line, 32, fp)!=NULL)
    {
        Bnk=0;
	Off=0;
	processEvents();
    
        i=0;
        j=0;
        while((Line[i] != ',')&&(Line[i]!=0))
            FileName[j++] = CHANGECASE(Line[i++]);
        FileName[j]=0;

        j -= 3;
        if (j>0) {
            FileType = recon_filetype(&FileName[j]);
            i++;
            j=0;
            while(Line[i] != ',') {
                Bnk*=10;
		Bnk+=Line[i]-'0';
		i++;
	    }

            i++;
            j=0;

            while(Line[i] != 0x0D && Line[i] != 0) {
		Off*=16;
		Off+=hextodec(Line[i++]);
	    }

            Bnk &= 3;

	    ret++;
	    if (!check)
	    {
		strcpy(Path, cdpath);
		strcat(Path, FileName);
		FILE *f=cdrom_fopen(Path,"rb");
		if (!f)
			return 0;
		cdrom_fclose(f);
	    }
	    else
	    {
            	console_printf("Loading File: %s %02x %08x\n", FileName, Bnk, Off);
		text_draw_loading(ret,check);

            	switch( FileType ) {
            	  case PRG_TYPE:
                	if (!neogeo_cdrom_load_prg_file(FileName, Off)) {
                    		cdrom_fclose(fp);
    		    		sound_enable();
                    		return 0;
                	}
               		break;
            	  case FIX_TYPE:
                	if (!neogeo_cdrom_load_fix_file(FileName, (Off>>1))) {
                    		cdrom_fclose(fp);
    		    		sound_enable();
                   		return 0;
                	}
                	break;
            	  case SPR_TYPE:
                	if (!neogeo_cdrom_load_spr_file(FileName, (Bnk*0x100000) + Off)) {
                    		cdrom_fclose(fp);
    		    		sound_enable();
                    		return 0;
                	}
                	break;
            	  case Z80_TYPE:
                	if (!neogeo_cdrom_load_z80_file(FileName, (Off>>1))) {
                    		cdrom_fclose(fp);
    		    		sound_enable();
                    		return 0;
                	}
                	break;
            	  case PAT_TYPE:
                	if (!neogeo_cdrom_load_pat_file(FileName, Off, Bnk)) {
                    		cdrom_fclose(fp);
    		    		sound_enable();
                    		return 0;
                	}
                	break;
            	  case PCM_TYPE:
                	if (!neogeo_cdrom_load_pcm_file(FileName, (Bnk*0x80000) + (Off>>1))) {
                   		cdrom_fclose(fp);
    		    		sound_enable();
                    		return 0;
                	}
                	break;
#ifdef DEBUG_CDROM
		   default:
			printf("!!!! FileType=%i NO IMPLEMENTADO!!!!!!\n",FileType);
#endif
            	}
	    }
        }
    }
    
    cdrom_fclose(fp);
#ifndef USE_VIDEO_GL
    video_draw_blank();
#endif
    
    sound_enable();
    init_autoframeskip();
    z80_cycles_inited=0;
    return ret;
}

//----------------------------------------------------------------------------
int    recon_filetype(char *ext)
{    
    if (strcmp(ext, PRG)==0)
        return PRG_TYPE;
    
    if (strcmp(ext, FIX)==0)
        return FIX_TYPE;
    
    if ((strcmp(ext, SPR)==0)||(strcmp(ext, OBJ)==0))
        return SPR_TYPE;
        
    if (strcmp(ext, Z80)==0)
        return Z80_TYPE;
        
    if (strcmp(ext, PAT)==0)
        return PAT_TYPE;
    
    if (strcmp(ext, PCM)==0)
        return PCM_TYPE;
        
    return    -1;
}


//----------------------------------------------------------------------------
unsigned int motorola_peek(unsigned char *address) 
{
    unsigned int a,b,c,d;
	
	a=address[0]<<24;
	b=address[1]<<16;
	c=address[2]<<8;
	d=address[3]<<0;
	
	return (a|b|c|d);
}

static unsigned neogeo_cdrom_test_files(int check)
{
    unsigned ret=0;
    char    Entry[32], FileName[13];
    char    *Ptr, *Ext;
    int     i, j, Bnk, Off, Type, Reliquat;

    Ptr = neogeo_prg_memory + _68k_get_register(_68K_REG_A0);

    do {
        Reliquat = ((int)Ptr)&1;

        if (Reliquat)
            Ptr--;

        swab(Ptr, Entry, 32);
        i=Reliquat;

        while((Entry[i]!=0)&&(Entry[i]!=';')) {
            FileName[i-Reliquat] = CHANGECASE(Entry[i]);
            i++;
        }

        FileName[i-Reliquat] = 0;

        if (Entry[i]==';')    /* 01/05/99 MSLUG2 FIX */
            i += 2;

        i++;

        Bnk = Entry[i++]&3;

        if (i&1)
            i++;


        Off = motorola_peek((unsigned char *)&Entry[i]);
        i += 4;
        Ptr += i;


        j=0;

        while(FileName[j] != '.' && FileName[j] != '\0')
            j++;

        if(FileName[j]=='\0')
        {
	    if (!check)
		return 0;
            console_printf("Internal Error loading file: %s",FileName);
	    SDL_Delay(1000);
            exit(1);
        }

        j++;
        Ext=&FileName[j];

	ret++;
	if (!check)
	{
		char Path[256];
		strcpy(Path, cdpath);
		strcat(Path, FileName);
		FILE *f=cdrom_fopen(Path,"rb");
		if (!f)
			return 0;
		cdrom_fclose(f);
	}
	else
	{
        	console_printf("Loading File: %s %02x %08x\n", FileName, Bnk, Off);
        	Type = recon_filetype(Ext);
        	switch( Type ) {
        	  case PRG_TYPE:
            		neogeo_cdrom_load_prg_file(FileName, Off);
            		break;
        	  case FIX_TYPE:
            		neogeo_cdrom_load_fix_file(FileName, Off>>1);
            		break;
        	  case SPR_TYPE:
            		neogeo_cdrom_load_spr_file(FileName, (Bnk*0x100000) + Off);
            		break;
        	  case Z80_TYPE:
            		neogeo_cdrom_load_z80_file(FileName, Off>>1);
            		break;
        	  case PAT_TYPE:
            		neogeo_cdrom_load_pat_file(FileName, Off, Bnk);
            		break;
        	  case PCM_TYPE:
            		neogeo_cdrom_load_pcm_file(FileName, (Bnk*0x80000) + (Off>>1));
            		break;
#ifdef DEBUG_CDROM
		  default:
			printf("!!!!ATENCION TYPE %i NO IMPLEMENTADO!!!!!!\n",Type);
#endif
        	}
        	processEvents();
	}
    } while( Entry[i] != 0);
    return ret;
}
//----------------------------------------------------------------------------
void    neogeo_cdrom_load_files(void)
{
    unsigned nfiles=0;

    if (m68k_read_memory_8(_68k_get_register(_68K_REG_A0))==0)
    {
    	sound_enable();
        return;
    }


    SDL_PauseAudio(1);
    cdda_stop();
#ifdef ENABLE_CDDA
    cdda_current_track = 0;
#endif

    m68k_write_memory_32(0x10F68C, 0x00000000);
    m68k_write_memory_8(0x10F6C3, 0x00);
    m68k_write_memory_8(0x10F6D9, 0x01);
    m68k_write_memory_8(0x10F6DB, 0x01);
    m68k_write_memory_32(0x10F742, 0x00000000);
    m68k_write_memory_32(0x10F746, 0x00000000);
    m68k_write_memory_8(0x10FDC2, 0x01);
    m68k_write_memory_8(0x10FDDC, 0x00);
    m68k_write_memory_8(0x10FDDD, 0x00);
    m68k_write_memory_8(0x10FE85, 0x01);
    m68k_write_memory_8(0x10FE88, 0x00);
    m68k_write_memory_8(0x10FEC4, 0x01);

    nfiles=neogeo_cdrom_test_files(0);

    if (nfiles)
    {
	if (nfiles>MIN_TEXT_DRAW)
		text_draw_cdload();
	neogeo_cdrom_test_files(nfiles);
    }
/*
    else
ERROR !!!!!!!!!!!!!!!!!!
*/
	
#ifndef USE_VIDEO_GL
    	video_draw_blank();
#endif

    SDL_PauseAudio(0);
    init_autoframeskip();
    z80_cycles_inited=0;
}

//----------------------------------------------------------------------------
void    fix_conv(unsigned char *Src, unsigned char *Ptr, int Taille,
    unsigned char *usage_ptr)
{
    int        i;
    unsigned char    usage;
    
    for(i=Taille;i>0;i-=32) {
        usage = 0;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src++;
        *Ptr++ = *(Src+16);
        usage |= *(Src+16);
        *Ptr++ = *(Src+24);
        usage |= *(Src+24);
        *Ptr++ = *(Src);
        usage |= *(Src);
        *Ptr++ = *(Src+8);
        usage |= *(Src+8);
        Src+=25;
        *usage_ptr++ = usage;
    }
}


//----------------------------------------------------------------------------
#define COPY_BIT(a, b) { \
    a <<= 1; \
    a |= (b & 0x01); \
    b >>= 1; }

void extract8(char *src, char *dst) 
{ 
   int i; 

   unsigned char bh = *src++;
   unsigned char bl = *src++;
   unsigned char ch = *src++;
   unsigned char cl = *src; 
   unsigned char al, ah; 

   for(i = 0; i < 4; i++)
   { 
      al = ah = 0; 

      COPY_BIT(al, ch) 
      COPY_BIT(al, cl) 
      COPY_BIT(al, bh) 
      COPY_BIT(al, bl) 

      COPY_BIT(ah, ch) 
      COPY_BIT(ah, cl) 
      COPY_BIT(ah, bh) 
      COPY_BIT(ah, bl) 

      *dst++ = ((ah << 4) | al);
   } 
} 


//----------------------------------------------------------------------------
void spr_conv(unsigned char *src, unsigned char *dst, int len, unsigned char *usage_ptr)
{
    register int    i;
    int offset;

    for(i=0;i<len;i+=4) {
        if((i&0x7f)<64)
            offset=(i&0xfff80)+((i&0x7f)<<1)+4;
        else
            offset=(i&0xfff80)+((i&0x7f)<<1)-128;

        extract8((char *)src,(char *)(dst+offset));
        src+=4;
    }
}

//----------------------------------------------------------------------------
void    neogeo_upload(void)
{
#ifdef DEBUG_CDROM
puts("NEOGEO_UPLOAD !!!");fflush(stdout);
#endif
    int        Zone;
    int        Taille;
    int        Banque;
    int        Offset = 0;
    unsigned char    *Source;
    unsigned char    *Dest;
    // FILE            *fp;

    Zone = m68k_read_memory_8(0x10FEDA);
#ifdef DEBUG_CDROM
printf("neogeo_upload: Zone=0x%X\n",Zone); fflush(stdout);
#endif

    /*fp = cdrom_fopen("UPLOAD.LOG", "at");

    fprintf(fp, "%02x: %06x, %06x:%02x, %08x\n",
        Zone,
        m68k_read_memory_32(0x10FEF8),
        m68k_read_memory_32(0x10FEF4),
        m68k_read_memory_8(0x10FEDB),
        m68k_read_memory_32(0x10FEFC));

    cdrom_fclose(fp);   */

    switch( Zone&0x0F )
    {
    case    0:    // PRG
#ifdef DEBUG_CDROM
puts("neogeo_upload: PRG");fflush(stdout);
#endif
        Source = (unsigned char *)(neogeo_prg_memory + m68k_read_memory_32(0x10FEF8));
        Dest = (unsigned char *)(neogeo_prg_memory + m68k_read_memory_32(0x10FEF4));
        Taille = m68k_read_memory_32(0x10FEFC);

        memcpy(Dest, Source, Taille);

        m68k_write_memory_32( 0x10FEF4, m68k_read_memory_32(0x10FEF4) + Taille );

        break;

    case    2:    // SPR
#ifdef DEBUG_CDROM
puts("neogeo_upload: SPR");fflush(stdout);
#endif
        Banque = m68k_read_memory_8(0x10FEDB);
        Source = (unsigned char *)(neogeo_prg_memory + m68k_read_memory_32(0x10FEF8));
        Offset = m68k_read_memory_32(0x10FEF4) + (Banque<<20);
        Dest = (unsigned char *)(neogeo_spr_memory + Offset);
        Taille = m68k_read_memory_32(0x10FEFC);
        
        do {
            memset(neogeo_cdrom_buffer, 0, BUFFER_SIZE);
#ifdef WIN32
            swab((const char *)Source, (char *)neogeo_cdrom_buffer, min(BUFFER_SIZE, Taille));
#else
            swab(Source, neogeo_cdrom_buffer, min(BUFFER_SIZE, Taille));
#endif
            spr_conv((unsigned char *)neogeo_cdrom_buffer, (unsigned char *)Dest, min(BUFFER_SIZE, Taille), 
                (unsigned char *)(video_spr_usage+(Offset>>7)));
            Source += min(BUFFER_SIZE, Taille);
            Dest += min(BUFFER_SIZE, Taille);
            Offset += min(BUFFER_SIZE, Taille);
            Taille -= min(BUFFER_SIZE, Taille);
        } while(Taille!=0);
        
        // Mise à jour des valeurs
        Offset = m68k_read_memory_32( 0x10FEF4 );
        Banque = m68k_read_memory_8( 0x10FEDB );
        Taille = m68k_read_memory_8( 0x10FEFC );
        
        Offset += Taille;
        
        while (Offset > 0x100000 )
        {
            Banque++;
            Offset -= 0x100000;
        }
        
        m68k_write_memory_32( 0x10FEF4, Offset );
	m68k_write_memory_8(0x10FEDB, (Banque>>8)&0xFF);
	m68k_write_memory_8(0x10FEDC, Banque&0xFF);
        
        break;

    case    1:    // FIX
#ifdef DEBUG_CDROM
puts("neogeo_upload: FIX");fflush(stdout);
#endif
        Source = (unsigned char *)(neogeo_prg_memory + m68k_read_memory_32(0x10FEF8));
        Offset = m68k_read_memory_32(0x10FEF4)>>1;
        Dest = (unsigned char *)(neogeo_fix_memory + Offset);
        Taille = m68k_read_memory_32(0x10FEFC);

        do {
            memset(neogeo_cdrom_buffer, 0, BUFFER_SIZE);
#ifdef WIN32
            swab((const char *)Source, (char *)neogeo_cdrom_buffer, min(BUFFER_SIZE, Taille));
#else
            swab(Source, neogeo_cdrom_buffer, min(BUFFER_SIZE, Taille));
#endif
            fix_conv((unsigned char *)neogeo_cdrom_buffer, Dest, min(BUFFER_SIZE, Taille), 
                (unsigned char *)(video_fix_usage + (Offset>>5)));
            Source += min(BUFFER_SIZE, Taille);
            Dest += min(BUFFER_SIZE, Taille);
            Offset += min(BUFFER_SIZE, Taille);
            Taille -= min(BUFFER_SIZE, Taille);
        } while(Taille!=0);
        
        Offset = m68k_read_memory_32( 0x10FEF4 );
        Taille = m68k_read_memory_32( 0x10FEFC );
        
        Offset += (Taille<<1);
        
        m68k_write_memory_32( 0x10FEF4, Offset);
        
        break;

    case    3:    // Z80
#ifdef DEBUG_CDROM
puts("neogeo_upload: Z80");fflush(stdout);
#endif

#ifdef Z80_EMULATED
        Source = (unsigned char *)(neogeo_prg_memory + m68k_read_memory_32(0x10FEF8));
        Dest = subcpu_memspace + (m68k_read_memory_32(0x10FEF4)>>1);
        Taille = m68k_read_memory_32(0x10FEFC);
       
#ifdef WIN32	
        swab( (const char *)Source, (char *)Dest, Taille);
#else
        swab( Source, Dest, Taille);
#endif
	memcpy(mame_z80mem+(Dest-subcpu_memspace), Dest, Taille);

        m68k_write_memory_32( 0x10FEF4, m68k_read_memory_32(0x10FEF4) + (Taille<<1) );
#endif
        break;        

    case    5:    // Z80 patch
#ifdef DEBUG_CDROM
puts("neogeo_upload: Z80 patch");fflush(stdout);
#endif

        Source = (unsigned char *)(neogeo_prg_memory + m68k_read_memory_32(0x10FEF8));
        neogeo_cdrom_apply_patch((short*)Source, m68k_read_memory_32(0x10FEF4), m68k_read_memory_8(0x10FEDB));

        break;
    
    case    4:    // PCM
#ifdef DEBUG_CDROM
puts("neogeo_upload: PCM");fflush(stdout);
#endif
        Banque = m68k_read_memory_8(0x10FEDB);
        Source = (unsigned char *)(neogeo_prg_memory + m68k_read_memory_32(0x10FEF8));
        Offset = (m68k_read_memory_32(0x10FEF4)>>1) + (Banque<<19);
        Dest = (unsigned char *)(neogeo_pcm_memory + Offset);
        Taille = m68k_read_memory_32(0x10FEFC);
       
#ifdef WIN32	
        swab( (const char *)Source, (char *)Dest, Taille);        
#else
        swab( Source, Dest, Taille);        
#endif
        
        // Mise à jour des valeurs
        Offset = m68k_read_memory_32( 0x10FEF4 );
        Banque = m68k_read_memory_8( 0x10FEDB );
        Taille = m68k_read_memory_8( 0x10FEFC );
        
        Offset += (Taille<<1);
        
        while (Offset > 0x100000 )
        {
            Banque++;
            Offset -= 0x100000;
        }
        
        m68k_write_memory_32( 0x10FEF4, Offset );
	m68k_write_memory_8(0x10FEDB, (Banque>>8)&0xFF);
	m68k_write_memory_8(0x10FEDC, Banque&0xFF);

        break;    
        
    }
#ifdef DEBUG_CDROM
puts("neogeo_upload: end");fflush(stdout);
#endif
}

//----------------------------------------------------------------------------
void neogeo_cdrom_load_title(void)
{
    char            Path[256];
    char            jue[4] = JUE;
    char            file[12] = TITLE_X_SYS;
    FILE            *fp;
    char            *Ptr;
    int                Readed;
    int                x, y;

#ifdef USE_VIDEO_GL
    int filter=neo4all_filter;
    neogeo_adjust_filter(1);
#endif
//    sound_disable();
    strcpy(Path, cdpath);

    file[6] = jue[m68k_read_memory_8(0x10FD83)&3];

    strcat(Path, file);
    
    fp = cdrom_fopen(Path, "rb");
    if (fp!=NULL)
    {
    
#ifdef DEBUG_CDROM
    	printf("Palette Lectura desde la posicion %i (%i bytes)\n",0,0x5a0);
#endif
    	cdrom_fread(video_paletteram_pc, 1, 0x5A0, fp);
    	swab((char *)video_paletteram_pc, (char *)video_paletteram_pc, 0x5A0);

    	for(Readed=0;Readed<720;Readed++)
        	video_paletteram_pc[Readed] = video_color_lut[video_paletteram_pc[Readed]];

    	Ptr = neogeo_spr_memory;
    
    	Readed = cdrom_fread(neogeo_cdrom_buffer, 1, BUFFER_SIZE, fp);
#ifdef DEBUG_CDROM
    	printf("TITLE Lectura desde la posicion %i (%i bytes)\n",0,Readed);
#endif
    	spr_conv((unsigned char *)neogeo_cdrom_buffer, (unsigned char *)Ptr, Readed, (unsigned char *)video_spr_usage);
    	cdrom_fclose(fp);

#if defined(DREAMCAST) && defined(USE_VIDEO_GL)
    	glKosBeginFrame();
    	glKosFinishList();
#endif

    	Readed = 0;
    	for(y=0;y<80;y+=16)
    	{
        	for(x=0;x<144;x+=16)
        	{
#ifndef USE_VIDEO_GL
            		video_draw_spr(Readed, Readed, 0, 0, x+16, y+16, 15, 16);
#else
            		video_draw_spr(Readed, Readed, 0, 0, 16+x*2, 30+ y*2, 31, 32);
#endif
            		Readed++;
        	}
    	}

    	blitter();
    	SDL_Delay(1500);
   }

    memset(neogeo_spr_memory, 0, 4194304);
    memset(neogeo_fix_memory, 0, 131072);
    memset(video_spr_usage, 0, 32768);
    memset(video_fix_usage, 0, 4096);
#ifdef USE_VIDEO_GL
    neogeo_adjust_filter(filter);
#endif

//    sound_enable();
}

#ifdef Z80_EMULATED
#define PATCH_Z80(a, b) { \
                            subcpu_memspace[(a)] = (b)&0xFF; \
                            subcpu_memspace[(a+1)] = ((b)>>8)&0xFF; \
			    mame_z80mem[(a)] = subcpu_memspace[(a)]; \
			    mame_z80mem[(a+1)] = subcpu_memspace[(a+1)]; \
                        }
#else
#define PATCH_Z80(a, b) { }
#endif

void neogeo_cdrom_apply_patch(short *source, int offset, int bank)
{
    int master_offset;
    
    master_offset = (((bank*1048576) + offset)/256)&0xFFFF;
    
    while(*source != 0)
    {
        PATCH_Z80( source[0], ((source[1] + master_offset)>>1) );
        PATCH_Z80( source[0] + 2, (((source[2] + master_offset)>>1) - 1) );
        
        if ((source[3])&&(source[4]))
        {
            PATCH_Z80( source[0] + 5, ((source[3] + master_offset)>>1) );
            PATCH_Z80( source[0] + 7, (((source[4] + master_offset)>>1) - 1) );
        }
            
        source += 5;
    }
}

FILE *fopen_retry(char *filename, char *mode)
{
	FILE *ret=fopen(filename,mode);
	unsigned len=strlen(filename);
	char *tmp=(char *)calloc(len+1,1);
	if (!ret)
	{
		int i,j;
		strcpy(tmp,filename);
		for(i=0;i<len && !ret;i++)
		{
			for(j=0;j<i;j++)
				tmp[j]=toupper(tmp[j]);
			for(j=i+1;j<len;j++)
				tmp[j]=tolower(tmp[j]);
			for(j=0;j<len && !ret; j++)
			{
				tmp[j]=tolower(tmp[j]);
				ret=fopen(tmp,mode);
				if (!ret)
				{
					tmp[j]=toupper(tmp[j]);
					ret=fopen(tmp,mode);
				}
			}
		}
		for(i=0;i<len && !ret;i++)
		{
			for(j=0;j<i;j++)
				tmp[j]=tolower(tmp[j]);
			for(j=i+1;j<len;j++)
				tmp[j]=toupper(tmp[j]);
			for(j=0;j<len && !ret; j++)
			{
				tmp[j]=toupper(tmp[j]);
				ret=fopen(tmp,mode);
				if (!ret)
				{
					tmp[j]=tolower(tmp[j]);
					ret=fopen(tmp,mode);
				}
			}
		}


	}
	free(tmp);
	return ret;
}
