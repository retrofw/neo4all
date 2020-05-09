#define MEMCARD_FILENAME_FORMAT "memcard.bin"

#define MEMCARD_FILENAME MEMCARD_FILENAME_FORMAT
#define VMUFILE_PAD 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "neo4all.h"
#include "console.h"


unsigned char           neogeo_memorycard[NEO4ALL_MEMCARD_SIZE];
static char* memcard_filestate=MEMCARD_PREFIX MEMCARD_FILENAME;
static char* memcard_filestate_formated=DATA_PREFIX MEMCARD_FILENAME_FORMAT;
static unsigned char	*paquete=NULL;
static int		paquete_size=0;

static int init_savestate(void)
{
#ifdef USE_MEMCARD
	paquete=(unsigned char *)&neogeo_memorycard;
	paquete_size=NEO4ALL_MEMCARD_SIZE;
	return 1;
#else
	return 0;
#endif
}


#ifdef USE_THREAD_CDDA
extern unsigned char cpy_neogeo_memorycard[NEO4ALL_MEMCARD_SIZE];
int save_savestate(void);
int real_save_savestate(void)
#else
#define real_save_savestate() save_savestate()
#define cpy_neogeo_memorycard neogeo_memorycard
int save_savestate(void)
#endif
{
	int	ret=0;
#ifdef USE_MEMCARD
	FILE	*f;

	if (paquete==NULL)
		return 0;
#ifdef MENU_PLASTA
        sound_disable();
	menu_raise();
	sound_play_saving();
	text_draw_saving(0,9);
#endif
	unlink(memcard_filestate);
	f=fopen(memcard_filestate,"w");
	if (f!=NULL)
	{
		int i;
		ret=1;
		for(i=0;i<8 && ret;i++)
		{
#ifdef MENU_PLASTA
			text_draw_saving(i+1,9);
#endif
			ret=(fwrite(&cpy_neogeo_memorycard[1024*i],1,1024,f)==1024);
			fflush(f);
		}
		fclose(f);
		if (!ret)
		{
			unlink(memcard_filestate);
			paquete=NULL;
		}
	}
#endif
#ifdef MENU_PLASTA
	menu_unraise();
        sound_enable();
#endif 
        init_autoframeskip();
	return ret;
}

static int load_savestate(void)
{
	int	ret=0;
#ifdef USE_MEMCARD
	FILE	*f;

	if (paquete==NULL)
		return 0;
	f=fopen(memcard_filestate,"r");
	if (f!=NULL)
		f=fopen(memcard_filestate_formated,"r");
	if (f!=NULL)
	{
		int ok;
		fseek(f,VMUFILE_PAD,SEEK_SET);
		ok=fread(paquete+VMUFILE_PAD,1,NEO4ALL_MEMCARD_SIZE,f);
		fclose(f);
		ret=(ok==NEO4ALL_MEMCARD_SIZE);
	}
#endif
	return ret;
}


void memcard_init(void)
{
	if (init_savestate())
		load_savestate();
}

void memcard_shutdown(void)
{
	if (paquete==NULL)
		load_savestate();
#ifdef USE_THREAD_CDDA
	memcpy(cpy_neogeo_memorycard,neogeo_memorycard,NEO4ALL_MEMCARD_SIZE);
#endif
	real_save_savestate();
}

void memcard_update(void)
{
	/* check for memcard writes */
	if (memcard_write > 0) {
	   memcard_write--;
	   if(!memcard_write)
	   {
		   save_savestate();
	   }
	}
}
