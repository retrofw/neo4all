/**************************************
****    CDROM.H  -  File reading   ****
****         Header File           ****
**************************************/

#ifndef	CDROM_H
#define CDROM_H

#ifndef AES

#include <SDL.h>

/*-- Exported Variables -----------------------------------------------------*/
extern	int	neogeo_cdrom_current_drive;
extern SDL_Surface	*loading_pict;


/*-- cdrom.c functions ------------------------------------------------------*/
int	neogeo_cdrom_init1(void);
int	neogeo_cdrom_load_prg_file(char *, unsigned int);
int	neogeo_cdrom_load_z80_file(char *, unsigned int);
int	neogeo_cdrom_load_fix_file(char *, unsigned int);
int	neogeo_cdrom_load_spr_file(char *, unsigned int);
int	neogeo_cdrom_load_pcm_file(char *, unsigned int);
int	neogeo_cdrom_load_pat_file(char *, unsigned int, unsigned int);
int	neogeo_cdrom_process_ipl(int check);
void	neogeo_cdrom_shutdown(void);
void	neogeo_cdrom_load_title(void);

void	fix_conv(unsigned char *, unsigned char *, int, unsigned char *);
void	spr_conv(unsigned char *, unsigned char *, int, unsigned char *);


FILE *fopen_s(char *filename, char *mode);

long CDR_init(void);
long CDR_shutdown(void);
long CDR_open(void);
long CDR_close(void);
void CDR_SetIsoFile(const char *filename);
const char *CDR_GetIsoFile(void);
unsigned char CDR_UsingIso(void);
void CDR_SetCdOpenCaseTime(long long time);
long long CDR_GetCdOpenCaseTime(void);
void CDR_playCDDA(void);
long CDR_getTN(unsigned char *buffer);
long CDR_getTD(unsigned char track, unsigned char *buffer);
long CDR_readTrack(unsigned char *time);
long CDR_play(unsigned char *time);
long CDR_stop(void);
unsigned char* CDR_getBufferSub(void);
long CDR_getStatus(struct CdrStat *stat);
int CDR_IsoActive(void);
int CDR_LoadCdromFile(const char *filename, unsigned char *addr, unsigned len, unsigned offset);
void *CDR_fopen(const char *filename, const char *filemode);
void CDR_fclose(void *fp);
unsigned CDR_fread(void *addr_b, unsigned size_f, unsigned count_f, void *fp);
int CDR_feof(void *fp);
char *CDR_fgets(char *addr_b, int size_b, void *fp);

/*-- extract8.asm functions -------------------------------------------------*/
void		extract8(char *, char *);
unsigned int	motorola_peek(unsigned char*);

/*-- sysdep functions --------------------------------------------------------*/
int getMountPoint(int drive, char *mountpoint);

#else

#include "aes/aes.h"
#define neogeo_cdrom_init1() aes4all_load()

//#define neogeo_cdrom_init1() (1)

#endif

#endif /* CDROM_H */
