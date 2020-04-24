#ifndef _CONF_H_
#define _CONF_H_



typedef enum SYSTEM {
    SYS_ARCADE=0,
    SYS_HOME,
    SYS_MAX
} SYSTEM;

typedef enum COUNTRY {
    CTY_JAPAN=0,
    CTY_EUROPE,
    CTY_USA,
    CTY_ASIA,
    CTY_MAX
} COUNTRY;

extern char *conf_game;
extern unsigned char conf_rom_type;
extern unsigned char conf_special_bios;
extern unsigned char conf_extra_xor;
extern unsigned char conf_banksw_type;

extern unsigned char conf_system;
extern unsigned char conf_country;


#endif
