#ifndef __2610INTF_H__
#define __2610INTF_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm.h"
#include "ay8910.h"
#ifdef BUILD_YM2610
void YM2610UpdateRequest( /*int chip */ );
#endif

#define   MAX_2610    (2)

#ifndef MAX_8910
#define MAX_8910 5
typedef Uint32(*mem_read_handler) (Uint32 offset);
typedef void (*mem_write_handler) (Uint32 offset, Uint32 data);
#endif

#ifndef VOL_YM3012
/* #define YM3014_VOL(Vol,Pan) VOL_YM3012((Vol)/2,Pan,(Vol)/2,Pan) */
#define YM3012_VOL(LVol,LPan,RVol,RPan) (MIXER(LVol,LPan)|(MIXER(RVol,RPan) << 16))
#endif

struct YM2610interface {
    int num;			/* total number of 8910 in the machine */
    int baseclock;
    int volumeSSG[MAX_8910];
    mem_read_handler portAread[MAX_8910];
    mem_read_handler portBread[MAX_8910];
    mem_write_handler portAwrite[MAX_8910];
    mem_write_handler portBwrite[MAX_8910];
    void (*handler[MAX_8910]) (int irq);	/* IRQ handler for the YM2610 */
    int pcmromb[MAX_2610];	/* Delta-T rom region */
    int pcmroma[MAX_2610];	/* ADPCM   rom region */
    int volumeFM[MAX_2610];	/* use YM3012_VOL macro */
};

/************************************************/
/* Sound Hardware Start				*/
/************************************************/
int YM2610_sh_start(void);

/************************************************/
/* Sound Hardware Stop				*/
/************************************************/
#define YM2610_sh_stop() YM2610Shutdown()
#define YM2610_sh_reset()  YM2610ResetChip()

/************************************************/
/* Chip 0 functions				*/
/************************************************/
#define YM2610_status_port_0_A_r(OFF) YM2610Read(0)
#define YM2610_status_port_0_B_r(OFF) YM2610Read(2)
#define YM2610_read_port_0_r(OFF) YM2610Read(1)
#define YM2610_control_port_0_A_w(OFF,DAT) YM2610Write(0, DAT)
#define YM2610_control_port_0_B_w(OFF,DAT) YM2610Write(2, DAT)
#define YM2610_data_port_0_A_w(OFF,DAT) YM2610Write(1, DAT)
#define YM2610_data_port_0_B_w(OFF,DAT) YM2610Write(3, DAT)

void FMTimerInit(void);

/************************************************/
/* Chip 1 functions				*/
/************************************************/
//Uint32  YM2610_status_port_1_A_r (Uint32 offset);
//Uint32  YM2610_status_port_1_B_r (Uint32 offset);
//Uint32  YM2610_read_port_1_r (Uint32 offset);
//void  YM2610_control_port_1_A_w (Uint32 offset,Uint32 data);
//void  YM2610_control_port_1_B_w (Uint32 offset,Uint32 data);
//void  YM2610_data_port_1_A_w (Uint32 offset,Uint32 data);
//void  YM2610_data_port_1_B_w (Uint32 offset,Uint32 data);

#endif
/**************** end of file ****************/
