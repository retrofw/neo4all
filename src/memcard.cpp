#define MEMCARD_FILENAME_FORMAT "memcard.bin"

#ifdef DREAMCAST
#include<kos.h>
#include"save_icon.h"
#define VMUFILE_PAD 128+512
#define MEMCARD_FILENAME "neo4all.bin"
#else
#define MEMCARD_FILENAME MEMCARD_FILENAME_FORMAT
#define VMUFILE_PAD 0
#endif

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
#ifdef DREAMCAST
	vmu_pkg_t	pkg;

	memset(&pkg, 0, sizeof(pkg));
	strcpy(pkg.desc_short, "NEO4ALL");
	strcpy(pkg.desc_long, "NEOGEO/CD Memcard Image");
	strcpy(pkg.app_id, "NEO4ALL");
	pkg.icon_cnt = 1;
	pkg.icon_anim_speed = 0;
	pkg.eyecatch_type = VMUPKG_EC_NONE;
	pkg.eyecatch_data = NULL;
	pkg.data_len = NEO4ALL_MEMCARD_SIZE;
	pkg.data = (const uint8*)&neogeo_memorycard;

	memcpy((void *)&pkg.icon_pal[0],(void *)&vmu_savestate_icon_pal,32);
	pkg.icon_data = (const uint8*)&vmu_savestate_icon_data;

	vmu_pkg_build(&pkg, &paquete, &paquete_size);

	return (paquete!=NULL);
#else
	paquete=(unsigned char *)&neogeo_memorycard;
	paquete_size=NEO4ALL_MEMCARD_SIZE;
	return 1;
#endif
#else
	return 0;
#endif
}

#ifdef DREAMCAST
static __inline__ void rebuild_crc(void)
{
  int i, c, n = 0;
  unsigned short *crc=(unsigned short*) &paquete[0x46];
  (*crc)=0;
  for (i = 0; i < paquete_size; i++)
  {
    n ^= (paquete[i]<<8);
    for (c = 0; c < 8; c++)
      if (n & 0x8000)
        n = (n << 1) ^ 4129;
      else
        n = (n << 1);
  }
  (*crc)=(n & 0xffff);
}
#endif

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
#ifdef DREAMCAST
		memcpy(paquete+VMUFILE_PAD,(void *)&cpy_neogeo_memorycard,NEO4ALL_MEMCARD_SIZE);
		rebuild_crc();
		ret=(fwrite(paquete,1,VMUFILE_PAD,f)==VMUFILE_PAD);
		fflush(f);
#endif
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
#ifdef DREAMCAST
			free(paquete);
#endif
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
#ifdef DREAMCAST
		if (ret)
			memcpy((void *)&neogeo_memorycard,paquete+VMUFILE_PAD,NEO4ALL_MEMCARD_SIZE);
#endif
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
#if defined(DREAMCAST) && defined(USE_MEMCARD)
		   init_autoframeskip();
#endif
	   }
	}
}
