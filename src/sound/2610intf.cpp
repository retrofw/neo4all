/***************************************************************************

  2610intf.c

  The YM2610 emulator supports up to 2 chips.
  Each chip has the following connections:
  - Status Read / Control Write A
  - Port Read / Data Write A
  - Control Write B
  - Data Write B

***************************************************************************/

#include <stdio.h>
#include "ay8910.h"
#include "fm.h"
#include "ay8910.h"
#include "2610intf.h"
#include "sound/streams.h"
#include "neo4all.h"
#include "memory/memory.h"
#include "sound/timer.h"

#ifdef AES
#include "aes/aes.h"
#endif

#if BUILD_YM2610

/* use FM.C with stream system */

static int stream=0;

static timer_struct *Timer[2]={NULL,NULL};

/*------------------------- TM2610 -------------------------------*/
/* IRQ Handler */
static void IRQHandler(int n, int irq)
{
    //printf("IRQ!!!\n");
    neogeo_sound_irq(irq);
}

/* Timer overflow callback from timer.c */
void timer_callback_2610(int param)
{
    register int c = param;

    Timer[c] = 0;
    YM2610TimerOver(c);
}

/* TimerHandler from fm.c */
static void TimerHandler(int c, int count, double stepTime)
{
    //printf("TimerHandler %d %d %f\n",c,count,stepTime);
    if (count == 0) {		/* Reset FM Timer */
	if (Timer[c]) {
	    del_timer(Timer[c]);
	    Timer[c] = 0;
	}
    } else {			/* Start FM Timer */
	if (Timer[c] == 0) {
	    register double timeSec = (double) count * stepTime;
	    Timer[c] =
		(timer_struct *) insert_timer(timeSec, c);
	}
    }
}

void FMTimerInit(void)
{
    Timer[0] = Timer[1] = 0;
    free_all_timer();
}

/* update request from fm.c */
void YM2610UpdateRequest(void)
{
    static double old_tc;
    double tc=neogeo_timer_count-old_tc;
    int len=(int)(SAMPLE_RATE*tc)<<2;
    if (len >4 ) {
	old_tc=neogeo_timer_count;
	streamupdate(len);
    }
}


int YM2610_sh_start(void)
{
    int j;
    int rate = (SAMPLE_RATE*80)/100;
    void *pcmbufa, *pcmbufb;
    int pcmsizea, pcmsizeb;

    stream=0;

    if (AY8910_sh_start())
	return 1;

    /* Timer Handler set */
    FMTimerInit();
    stream = stream_init_multi(YM2610_NUMBUF, 0, YM2610UpdateOne);
#ifndef AES
    pcmbufa = (void *) neogeo_pcm_memory;
    pcmsizea = 0x100000; 
    pcmbufb = NULL;
    pcmsizeb = 0;
#else
    pcmbufa = aes4all_memory_sound1;
    pcmsizea = aes4all_memory_sound1_size;
    pcmbufb = aes4all_memory_sound2;
    pcmsizeb = aes4all_memory_sound2_size;
    if (pcmbufa==pcmbufb)
    {
    	pcmbufb = NULL;
    	pcmsizeb = 0;
    }

#endif

    //}

  /**** initialize YM2610 ****/
    /*
       if (YM2610Init(8000000,rate,
       pcmbufa,pcmsizea,pcmbufb,pcmsizeb,
       TimerHandler,IRQHandler) == 0)
     */
    if (YM2610Init((8000000*80)/100, rate,
		   pcmbufa, pcmsizea, pcmbufb, pcmsizeb,
		   TimerHandler, IRQHandler) == 0)
	return 0;

    /* error */
    return 1;
}


#endif
