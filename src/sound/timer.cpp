/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

#include <stdlib.h>
#include "neo4all.h"
#include "timer.h"
#include "state.h"


#define MAX_TIMER 3

double neogeo_timer_count=0.0;

static timer_struct timers[MAX_TIMER];
static int initted=0;
static double timer_inc=1.0;

timer_struct *insert_timer(double duration, int param)
{
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
	if (timers[i].del_it) {
	    timers[i].time = neogeo_timer_count + duration;
	    timers[i].param = param;
	    timers[i].del_it = 0;
	    return &timers[i];
	}
    }
//    printf("YM2610: No timer free!\n");
    return NULL;		/* No timer free */
}


void free_all_timer(void) {
	register int i;
	register timer_struct *t=(timer_struct *)&timers[0];
	for (i = 0; i < MAX_TIMER; i++, t++)
		t->del_it=1;
	initted=1;
}


void timer_callback_2610(int param);



void adjust_timer(unsigned z80_cycles)
{
	if (!initted)
	       free_all_timer();
	double tm=((double)(NEOGEO_NB_INTERLACE));
	double diff= (((double)NEO4ALL_Z80_NORMAL_CYCLES) - ((double)z80_cycles)) / ((double)(NEO4ALL_Z80_OVER_CYCLES-NEO4ALL_Z80_NORMAL_CYCLES));
	double ratio = diff / 12.0;
	tm += tm * ratio;
//	timer_inc = 0.014 / tm;
	timer_inc = 0.01666 / tm;
//	timer_inc = 0.02 / tm;
}

void my_timer(void)
{
    register int i;
    register timer_struct *t=(timer_struct *)&timers[0];
    neogeo_timer_count += timer_inc;		/* 16ms par frame */
    register double cnt=neogeo_timer_count;
    for (i = 0; i < MAX_TIMER; i++,t++) {
	if (cnt >= t->time && t->del_it == 0) {
	    timer_callback_2610(t->param);
	    t->del_it = 1;
	}
    }
}
