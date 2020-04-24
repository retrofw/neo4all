/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SOUND_H
#define SOUND_H

#include "../neo4all.h"

#define NB_SEGMENT 20


#ifdef SOUND

extern SDL_AudioSpec desired;
int init_sdl_audio(void);
void sound_stop(void);
void sound_reset(void);
void sound_shutdown(void);
void sound_enable(void);
void sound_disable(void);
void sound_mute(void);
void sound_unmute(void);
void sound_toggle(void);
void sound_play_menu_music(void);
void sound_play_menu(void);
void sound_play_loading(void);
void sound_play_saving(void);
void sound_play_bye(void);
void sound_play_beep(void);
void sound_play_error(void);
void sound_emulate_start(void);
void sound_emulate_stop(void);

#else

#define sound_play_menu_music()
#define init_sdl_audio()
#define sound_stop()
#define sound_reset()
#define sound_shutdown()
#define sound_enable() 
#define sound_disable()
#define sound_mute()
#define sound_unmute()
#define sound_toggle()
#define sound_play_menu_music()
#define sound_play_menu()
#define sound_play_loading()
#define sound_play_saving()
#define sound_play_bye()
#define sound_play_beep()
#define sound_play_error()
#define sound_emulate_start()
#define sound_emulate_stop()

#endif

#endif
