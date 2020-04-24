
#include "SDL.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>
#include "unzip.h"
#include "driver.h"
#include "conf.h"
#include "pbar.h"
#include "memory.h"
#include "neocrypt.h"

static LIST *driver_list=NULL;
Uint32 sram_protection_hack;

void setup_misc_patch(char *name)
{
    sram_protection_hack = -1;
    if (!strcmp(name, "fatfury3") ||
	!strcmp(name, "samsho3") ||
	!strcmp(name, "samsho4") ||
	!strcmp(name, "aof3") ||
	!strcmp(name, "rbff1") ||
	!strcmp(name, "rbffspec") ||
	!strcmp(name, "kof95") ||
	!strcmp(name, "kof96") ||
	!strcmp(name, "kof97") ||
	!strcmp(name, "kof98") ||
	!strcmp(name, "kof99") ||
	!strcmp(name, "kof99n") ||
	!strcmp(name, "kof99p") ||
	!strcmp(name, "kof2000") ||
	!strcmp(name, "kizuna") ||
	!strcmp(name, "lastblad") ||
	!strcmp(name, "lastbld2") ||
	!strcmp(name, "rbff2") ||
	!strcmp(name, "mslug2") || 
	!strcmp(name, "garou"))

	sram_protection_hack = 0x100;



    if (!strcmp(name, "pulstar"))
	sram_protection_hack = 0x35a;


    if (!strcmp(name, "ssideki")) {
	WRITE_WORD_ROM(&memory.cpu[0x2240], 0x4e71);
    }


    if (!strcmp(name, "fatfury3")) {
	WRITE_WORD_ROM(memory.cpu, 0x0010);
    }

    
    /* Many mgd2 dump have a strange initial PC, so as some MVS */
    if ((!strcmp(name, "aodk")) ||
	(!strcmp(name, "bjourney")) ||
	(!strcmp(name, "maglord")) ||
	(!strcmp(name, "mosyougi")) ||
	(!strcmp(name, "twinspri")) ||
	(!strcmp(name, "whp")) || 
	(conf_rom_type == MGD2)
/*
	||
	(CF_BOOL(cf_get_item_by_name("forcepc")))
*/
	    )
    {
	unsigned char *RAM = memory.cpu;
	WRITE_WORD_ROM(&RAM[4], 0x00c0);
	WRITE_WORD_ROM(&RAM[6], 0x0402);
    }

    if (!strcmp(name, "mslugx")) {
	/* patch out protection checks */
	int i;
	unsigned char *RAM = memory.cpu;
	for (i = 0; i < memory.cpu_size; i += 2) {
	    if ((READ_WORD_ROM(&RAM[i + 0]) == 0x0243) && 
		(READ_WORD_ROM(&RAM[i + 2]) == 0x0001) &&	/* andi.w  #$1, D3 */
		(READ_WORD_ROM(&RAM[i + 4]) == 0x6600)) {	/* bne xxxx */

		WRITE_WORD_ROM(&RAM[i + 4], 0x4e71);
		WRITE_WORD_ROM(&RAM[i + 6], 0x4e71);
	    }
	}

	WRITE_WORD_ROM(&RAM[0x3bdc], 0x4e71);
	WRITE_WORD_ROM(&RAM[0x3bde], 0x4e71);
	WRITE_WORD_ROM(&RAM[0x3be0], 0x4e71);
	WRITE_WORD_ROM(&RAM[0x3c0c], 0x4e71);
	WRITE_WORD_ROM(&RAM[0x3c0e], 0x4e71);
	WRITE_WORD_ROM(&RAM[0x3c10], 0x4e71);

	WRITE_WORD_ROM(&RAM[0x3c36], 0x4e71);
	WRITE_WORD_ROM(&RAM[0x3c38], 0x4e71);
    }


}

