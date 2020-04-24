#ifdef DREAMCAST
#include <kos.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "neo4all.h"

#ifdef PROFILER_NEO4ALL
unsigned long long neo4all_prof_initial[NEO4ALL_PROFILER_MAX];
unsigned long long neo4all_prof_sum[NEO4ALL_PROFILER_MAX];
unsigned long long neo4all_prof_executed[NEO4ALL_PROFILER_MAX];
unsigned neo4all_prof_total=0;
static unsigned neo4all_prof_total_initial=0;
static char *neo4all_prof_msg[NEO4ALL_PROFILER_MAX];
#endif

void neo4all_prof_init(void)
{
#ifdef PROFILER_NEO4ALL
	unsigned i;
#ifndef DREAMCAST
	unsigned long long s=SDL_GetTicks();
#else
	unsigned long long s=timer_us_gettime64();
#endif
	for(i=0;i<NEO4ALL_PROFILER_MAX;i++)
	{
		neo4all_prof_initial[i]=s;
		neo4all_prof_sum[i]=0;
		neo4all_prof_executed[i]=0;
		if (!neo4all_prof_total)
			neo4all_prof_msg[i]=NULL;
	}
	neo4all_prof_total_initial=s;
#endif
}

void neo4all_prof_add(char *msg)
{
#ifdef PROFILER_NEO4ALL
	if (neo4all_prof_total<NEO4ALL_PROFILER_MAX)
	{
		neo4all_prof_msg[neo4all_prof_total]=msg;	
		neo4all_prof_total++;
	}
#endif
}

void neo4all_prof_show(void)
{
#ifdef PROFILER_NEO4ALL
	unsigned i;
#ifndef DREAMCAST
	unsigned long long to=SDL_GetTicks()-neo4all_prof_total_initial;
#else
	unsigned long long to=neo4all_prof_sum[0];
	for(i=1;i<neo4all_prof_total;i++)
		if (neo4all_prof_sum[i]>to)
			neo4all_prof_sum[i]=0;
#endif

	puts("\n\n\n\n");
	puts("--------------------------------------------");
	unsigned tosum=0;
	double toper=0;
	for(i=0;i<neo4all_prof_total;i++)
	{
		unsigned long long t0=neo4all_prof_sum[i];
		double percent=(double)t0;
		percent*=100.0;
		percent/=(double)to;
		if ((i!=NEO4ALL_PROFILER_MAIN)&&(i!=NEO4ALL_PROFILER_MEM)&&(i!=NEO4ALL_PROFILER_VECTOR))
		{
			toper+=percent;
			tosum+=t0;
		}
#ifdef DREAMCAST
		t0/=1000;
#endif
		printf("%s: %.2f%% -> Ticks=%i -> %iK veces\n",neo4all_prof_msg[i],percent,((unsigned)t0),(unsigned)(neo4all_prof_executed[i]>>10));
	}
	printf("TOTAL: %.2f%% -> Ticks=%i\n",toper,tosum);
	puts("--------------------------------------------"); fflush(stdout);
#endif
}

