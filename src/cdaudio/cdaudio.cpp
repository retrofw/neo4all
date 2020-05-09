/**************************************
****   CDAUDIO.C  -  CD-DA Player  ****
**************************************/

#ifdef ENABLE_CDDA
//-- Include files -----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#include "../neo4all.h"
#include "cdaudio.h"

#include "console.h"

#ifdef USE_THREAD_CDDA
#include<SDL_thread.h>
#define MAX_COLA 128
static int cola[MAX_COLA];
static int iw_cola=0, ir_cola=0;
static SDL_sem *sem_atiende=NULL;
static SDL_mutex *mtx_procesando=NULL;
#endif

#include "SDL_mixer.h"
extern char neo4all_image_dir[];
Mix_Music *music;

//-- Private Variables -------------------------------------------------------
static int			cdda_min_track;
static int			cdda_max_track;
static int			cdda_disk_length;
static int			cdda_track_end;
static int			cdda_loop_counter;
static SDL_CD			*cdrom=NULL;

//-- Public Variables --------------------------------------------------------
int			cdda_first_drive=0;
int			cdda_current_drive=0;
int			cdda_current_track=0;
int			cdda_current_frame=0;
int			cdda_playing=0;
int			cdda_autoloop=0;
int			cdda_volume=0;
int			cdda_disabled=0;

//-- Function Prototypes -----------------------------------------------------
int			cdda_init(void);
int			cdda_play(int);
void			cdda_stop(void);
void			cdda_resume(void);
void			cdda_shutdown(void);
void			cdda_loop_check(void);
int 			cdda_get_disk_info(void);




//----------------------------------------------------------------------------
int cdda_get_disk_info(void)
{
#ifdef ENABLE_CDDA
    if(cdda_disabled) return 1;

#ifdef USE_MP3_CDDA
    cdda_min_track = 0;
    cdda_max_track = cdda_num_tracks();
    cdda_disk_length = cdda_max_track;
    return 1;
#else
    if( CD_INDRIVE(SDL_CDStatus(cdrom)) ) {
        cdda_min_track = 0;
        cdda_max_track = cdrom->numtracks;
        cdda_disk_length = cdrom->numtracks;
        return 1;
    }
    else
    {
        console_printf("Error: No Disc in drive\n");
        cdda_disabled=1;
        return 1;
    }
#endif
#endif
    return 0;
}

//----------------------------------------------------------------------------
#ifdef USE_THREAD_CDDA
static int real_cdda_play(int track)
#else
int cdda_play(int track)
	if(cdda_disabled) return;
#endif
{
#ifndef SHOW_MENU
	track++;
#endif
#ifdef ENABLE_CDDA
    if(cdda_disabled) return 1;

    if(cdda_playing && cdda_current_track==track) return 1;

#ifdef USE_MP3_CDDA
    if (track > 1 && track <= cdda_num_tracks())
    {

    	char fname[256];
    	if (cdda_get_track_name(track, fname))
    	{
    		if (cdda_playing && music) {
				Mix_FreeMusic(music);
				music=NULL;
    		}

    		char cdda_track[1024];
			sprintf(cdda_track, "%s/%s", neo4all_image_dir, fname);

			printf("CD Play: %s\n", cdda_track);

			music = Mix_LoadMUS(cdda_track);
			Mix_PlayMusic(music, -1);

			cdda_current_track = track;
			cdda_loop_counter = 0;
			cdda_track_end = cdda_get_track_end(track);
			cdda_playing = 1;
			return 1;
    	}
    }
#else
    if( CD_INDRIVE(SDL_CDStatus(cdrom)) ) {
    	SDL_CDPlayTracks(cdrom, track-1, 0, 1, 0);
    	cdda_current_track = track;
    	cdda_loop_counter=0;
    	cdda_track_end=(cdrom->track[track-1].length*60)/CD_FPS;//Length in 1/60s of second
    	cdda_playing = 1;
#ifndef USE_THREAD_CDDA
        init_autoframeskip();
#endif
    	return 1;
    }
    else
    {
        cdda_disabled = 1;
        return 1;
    }
#endif
#endif
    return 0;
}

//----------------------------------------------------------------------------
void	cdda_pause(void)
{
	if (music) Mix_PauseMusic();
	if (cdda_disabled) return;
#if defined(ENABLE_CDDA) && !defined(USE_MP3_CDDA)
	SDL_CDPause(cdrom);
#endif
	cdda_playing = 0;
}


void	cdda_stop(void)
{
	if (cdda_disabled) return;
	if (music) Mix_HaltMusic();
#if defined(ENABLE_CDDA) && !defined(USE_MP3_CDDA)
	SDL_CDStop(cdrom);
#endif
	cdda_playing = 0;
}

//----------------------------------------------------------------------------
void	cdda_resume(void)
{
	if (cdda_disabled || cdda_playing) return;
	if (music) Mix_ResumeMusic();

#if defined(ENABLE_CDDA) && !defined(USE_MP3_CDDA)
	SDL_CDResume(cdrom);
#endif
	cdda_playing = 1;
}

//----------------------------------------------------------------------------
void	cdda_loop_check(void)
{
#ifdef ENABLE_CDDA
	if(cdda_disabled) return;
	if (cdda_playing==1) {
		cdda_loop_counter++;
		if (cdda_loop_counter>=cdda_track_end) {
			if (cdda_autoloop)
				cdda_play(cdda_current_track);
			else
				cdda_stop();
		}
	}
#endif
}

#ifdef USE_THREAD_CDDA

static __inline__ int datospendientes(void)
{
	int ret;
	SDL_mutexP(mtx_procesando);
	ret=(iw_cola!=ir_cola);
	SDL_mutexV(mtx_procesando);
	return ret;
}

static __inline__ int leecola(void)
{
	int valor;
	SDL_mutexP(mtx_procesando);
	ir_cola=(ir_cola+1)%MAX_COLA;
	valor=cola[ir_cola];
	SDL_mutexV(mtx_procesando);
	return valor;
}

int cdda_play(int track)
{
	if (track>=0)
	{
		SDL_mutexP(mtx_procesando);
		iw_cola=(iw_cola+1)%MAX_COLA;
		cola[iw_cola]=track;
		SDL_mutexV(mtx_procesando);
		SDL_SemPost(sem_atiende);
		return 1;
	}
	return 0;
}

unsigned char cpy_neogeo_memorycard[NEO4ALL_MEMCARD_SIZE];
extern int real_save_savestate(void);
int save_savestate(void)
{
	memcpy(cpy_neogeo_memorycard,neogeo_memorycard,NEO4ALL_MEMCARD_SIZE);
	SDL_mutexP(mtx_procesando);
	iw_cola=(iw_cola+1)%MAX_COLA;
	cola[iw_cola]=-1;
	SDL_mutexV(mtx_procesando);
	SDL_SemPost(sem_atiende);
	return 0;
}

static int mithread_alive=0;

static int mithread(void *data)
{
	mithread_alive=1;
	while(mithread_alive)
	{
		SDL_SemWait(sem_atiende);
		if (datospendientes())
		{
			int valor=leecola();
			if (valor>=0)
				real_cdda_play(valor);
			else
				real_save_savestate();
		}
	}
	return 0;
}

#endif


//----------------------------------------------------------------------------
int	cdda_init(void)
{
#ifdef ENABLE_CDDA
	cdda_min_track = cdda_max_track = 0;
	cdda_current_track = 0;
	cdda_playing = 0;
	cdda_loop_counter = 0;
	if (cdda_num_tracks() == 0)
		cdda_disabled = 1;
	else
		cdda_disabled = 0;

#ifndef USE_MP3_CDDA
	/* Open the default drive */
	cdrom=SDL_CDOpen(cdda_current_drive);

	/* Did if open? Check if cdrom is NULL */
	if(cdrom == NULL){
		console_printf("Couldn't open drive %s for audio.  %s\n", SDL_CDName(cdda_current_drive), SDL_GetError());
		cdda_disabled=1;
		return 1;
	} else {
		cdda_disabled=0;
#endif
#ifdef USE_THREAD_CDDA
		if (!sem_atiende)
			sem_atiende=SDL_CreateSemaphore(0);
		if (!mtx_procesando)
			mtx_procesando=SDL_CreateMutex();
		if (!mithread_alive)
			SDL_CreateThread(mithread,0);
#endif
#ifndef USE_MP3_CDDA
		console_printf("CD Audio OK!\n");
	}
#endif

	cdda_get_disk_info();
#else
	cdda_disabled=1;
#endif
	return 0;
}

//----------------------------------------------------------------------------
void	cdda_shutdown(void)
{
#ifdef ENABLE_CDDA
	if(cdda_disabled) return;
#ifndef USE_MP3_CDDA
	if (cdrom)
	{
		SDL_CDStop(cdrom);
		SDL_CDClose(cdrom);
	}
#endif
	cdrom=NULL;
	cdda_disabled=1;
#ifdef USE_THREAD_CDDA
	mithread_alive=0;
	SDL_Delay(234);
	if (sem_atiende)
		SDL_DestroySemaphore(sem_atiende);
	if (mtx_procesando)
		SDL_DestroyMutex(mtx_procesando);
#endif
#endif
}

#else
int	cdda_disabled=1; // DISABLED CDDA
#endif
