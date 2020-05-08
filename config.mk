# --------------------------------------

NAME = neo4all

#AES=1
#MUSASHI_CORE=1
FAME_CORE=1
#FAME_C_CORE=1
Z80_EMULATED=1
RAZE_CORE=1
#CZ80_CORE=1
#MAMEZ80_CORE=1
SOUND=1
#CONSOLE=1
MENU=1
MENU_MUSIC=1
#MENU_ALPHA=1
#CDDA=1
CDISO=1
PD4990A=1
MEMCARD=1
#DOUBLE_BUFFER=1
VIDEOGL=1
#USE_DMA=1
#USE_SQ=1
USE_MMAP=1
CACHE_GL_AUTOCLEAN=1
USE_THREAD_CDDA=1
#Z80_ONLY_CHANGES_PATCH=1
#SINGLE_MEMORY=1
STDOUTPUT=1
#CACHE_INLINE=1
#FM_INLINE=1
#INPUT_INLINE=1
# REBOOT_DREAMCAST=1
#DEBUG_FAME=1
#DEBUG_Z80=1
##DEBUG_FRAMESKIP=1
##DEBUG_GL=1
##DEBUG_CDROM=1
#DEBUG_MEMORY=1
#DEBUG_NEO4ALL=1
##PROFILER_NEO4ALL=1
#AUTO_EVENTS=0
#AUTO_MAX_EVENTS=5
##AES_PREFETCHING=1


# --------------------------------------

ifdef DREAMCAST
ifdef AES_PREFETCHING
NEO4ALL_CFLAGS+= -DAES_PREFETCHING -DSTDOUTPUT -DMMU_HANDLE_SLOT_SHIFT=0 -DMMU_HANDLE_SLICE=0
endif
endif

ifdef DOUBLE_BUFFER
NEO4ALL_CFLAGS+=-DDOUBLE_BUFFER
endif

ifdef USE_DMA
NEO4ALL_CFLAGS+=-DUSE_DMA -DUSE_SQ
endif

ifdef USE_MMAP
ifdef AES
NEO4ALL_CFLAGS+=-DUSE_MMAP
endif
endif

ifdef USE_SQ
ifndef USE_DMA
NEO4ALL_CFLAGS+=-DUSE_SQ
endif
endif

ifdef CACHE_GL_AUTOCLEAN
NEO4ALL_CFLAGS+=-DCACHE_GL_AUTOCLEAN
endif

ifdef STDOUTPUT
NEO4ALL_CFLAGS+=-DSTDOUTPUT
endif

ifdef REBOOT_DREAMCAST
NEO4ALL_CFLAGS+=-DREBOOT_DREAMCAST
endif

ifdef SOUND
NEO4ALL_CFLAGS+=-DSOUND
endif

ifdef DEBUG_CDROM
NEO4ALL_CFLAGS+=-DDEBUG_CDROM
endif

ifdef PD4990A
NEO4ALL_CFLAGS+=-DUSE_PD4990A
endif

ifdef AES
NAME = aes4all
ifndef DREAMCAST
NEO4ALL_CFLAGS+=-DAES=\"${AES}\"
else
KOS_AFLAGS+= --defsym AES=1
NEO4ALL_CFLAGS+=-DAES="${AES}" -Isrc/mmu_file
endif
endif

ifdef CDDA
ifndef AES
NEO4ALL_CFLAGS+=-DENABLE_CDDA
endif
endif

ifdef CDISO
NEO4ALL_CFLAGS+=-DCDISO
endif

ifdef DEBUG_GL
NEO4ALL_CFLAGS+=-DDEBUG_GL
endif

ifdef DEBUG_FAME
NEO4ALL_CFLAGS+=-DDEBUG_FAME
endif

ifdef DEBUG_Z80
NEO4ALL_CFLAGS+=-DDEBUG_Z80
endif

ifdef DEBUG_FRAMESKIP
NEO4ALL_CFLAGS+=-DDEBUG_FRAMESKIP
endif

ifdef CONSOLE
NEO4ALL_CFLAGS+=-DSHOW_CONSOLE
endif

ifdef CACHE_INLINE
NEO4ALL_CFLAGS+=-DCACHE_INLINE
endif

ifdef INPUT_INLINE
NEO4ALL_CFLAGS+=-DINPUT_INLINE
endif

ifdef FM_INLINE
NEO4ALL_CFLAGS+=-DFM_INLINE
endif

ifdef MENU
NEO4ALL_CFLAGS+=-DSHOW_MENU
endif

ifdef MENU_MUSIC
NEO4ALL_CFLAGS+=-DMENU_MUSIC
endif

ifdef MENU_ALPHA
NEO4ALL_CFLAGS+=-DMENU_ALPHA
endif

ifdef Z80_EMULATED
NEO4ALL_CFLAGS+= -DZ80_EMULATED
ifdef RAZE_CORE
NEO4ALL_CFLAGS+= -DUSE_RAZE
else
ifdef MAMEZ80_CORE
NEO4ALL_CFLAGS+= -DUSE_MAMEZ80
else
ifdef CZ80_CORE
NEO4ALL_CFLAGS+= -DUSE_CZ80
endif
endif
endif
endif

ifdef FAME_CORE
NEO4ALL_CFLAGS+=-DUSE_FAME_CORE
ifdef FAME_C_CORE
#NEO4ALL_CFLAGS+=-DUSE_FAME_C_CORE -DFAME_IRQ_CLOCKING -DFAME_CHECK_BRANCHES -DFAME_DIRECT_MAPPING -DFAME_BYPASS_TAS_WRITEBACK -DFAME_ACCURATE_TIMING -DFAME_GLOBAL_CONTEXT -DFAME_FETCHBITS=12 -DFAME_DATABITS=12 -DFAME_GOTOS -DFAME_EXTRA_INLINE=__inline__ -DINLINE=__inline__ -DFAME_NO_RESTORE_PC_MASKED_BITS
NEO4ALL_CFLAGS+=-DUSE_FAME_C_CORE -DFAME_IRQ_CLOCKING -DFAME_GLOBAL_CONTEXT -DFAME_FETCHBITS=12 -DFAME_DATABITS=12 -DFAME_GOTOS -DFAME_EXTRA_INLINE=__inline__ -DINLINE=__inline__
endif
else
ifdef MUSASHI_CORE
NEO4ALL_CFLAGS+=-DCPU68K_USE_MUSASHI
else
NEO4ALL_CFLAGS+=-DCPU68K_USE_C68K
endif
endif

ifdef MEMCARD
NEO4ALL_CFLAGS+=-DUSE_MEMCARD
endif

ifdef DEBUG_MEMORY
NEO4ALL_CFLAGS+=-DDEBUG_MEMORY
endif

ifdef DEBUG_NEO4ALL
NEO4ALL_CFLAGS+=-DDEBUG_NEO4ALL -DBEGINTRACE=19 -DTOTRACE=19 -DDEBUG_REGISTERS
endif

ifdef CDDA
ifdef USE_THREAD_CDDA
ifndef AES
NEO4ALL_CFLAGS+=-DUSE_THREAD_CDDA
endif
endif
endif

ifdef SINGLE_MEMORY
NEO4ALL_CFLAGS+=-DSINGLE_MEMORY
endif

ifdef PROFILER_NEO4ALL
NEO4ALL_CFLAGS+=-DPROFILER_NEO4ALL
endif

ifdef AUTO_EVENTS
NEO4ALL_CFLAGS+=-DAUTO_EVENTS=${AUTO_EVENTS}
endif

ifdef AUTO_MAX_EVENTS
NEO4ALL_CFLAGS+=-DAUTO_MAX_EVENTS=${AUTO_MAX_EVENTS}
endif

# --------------------------------------

#NEO4ALL_CFLAGS+= -DEBUG_NEO4ALL -DDEBUG_REGISTERS -DDEBUG_MEMORY -DBEGINTRACE=350 -DTOTRACE=370
#NEO4ALL_CFLAGS+= -DDEBUG_NEO4ALL -DDEBUG_REGISTERS -DBEGINTRACE=1 -DTOTRACE=1
#NEO4ALL_CFLAGS+= -DDEBUG_NEO4ALL -DDEBUG_REGISTERS -DDEBUG_MEMORY -DBEGINTRACE=1 -DTOTRACE=4
#NEO4ALL_CFLAGS+= -DDEBUG_NEO4ALL -DDEBUG_REGISTERS -DBEGINTRACE=1 -DTOTRACE=4
#NEO4ALL_CFLAGS+= -DPROFILER
#NEO4ALL_CFLAGS+= -DPROFILER_TIME=100
#NEO4ALL_CFLAGS+= -DAUTO_FIRE


# --------------------------------------


OBJS =	\
	src/video/video.o \
	src/video/draw.o \
	src/input/input.o \
	src/main.o \
	src/icon.o \
	src/memcard.o

ifdef AES
OBJS+= \
	src/aes/load.o \
	src/aes/aes.o

ifdef DREAMCAST
OBJS+= \
	src/mmu_file/mmu_file.o \
	src/mmu_file/mmu_handle.o
endif

else
OBJS+= \
	src/cdrom/cdrom.o \
	src/cdaudio/cdaudio.o \
	$(CDROM_OBJ)

endif

ifdef CDISO
ifndef AES
OBJS+= \
	src/cdrom/cdriso.o
endif
endif

ifdef PROFILER_NEO4ALL
OBJS+= \
	src/profiler.o
endif

ifdef PD4990A
OBJS+= \
	src/pd4990a.o
endif

ifdef MENU
OBJS+= \
	src/menu/fade.o \
	src/menu/menu.o \
	src/menu/background.o \
	src/menu/sfont.o \
	src/menu/menu_cpu.o \
	src/menu/menu_ctl.o \
	src/menu/menu_main.o
ifdef AES
OBJS+= \
	src/menu/menu_load.o
else
ifdef CDISO
OBJS+= \
	src/menu/menu_load.o
endif
endif
else
OBJS+= \
	src/menu/fade.o \
	src/menu/nomenu.o
endif

ifdef CONSOLE
OBJS+= \
	src/video/console.o
endif

ifdef SOUND
OBJS+= \
	src/sound/sound.o \
	src/sound/streams.o \
	src/sound/2610intf.o \
	src/sound/ay8910.o \
	src/sound/fm.o \
	src/sound/ymdeltat.o \
	src/sound/timer.o
endif

ifdef VIDEOGL
	OBJS+=	src/video/sprgl.o \
		src/video/videogl.o \
		src/video/draw_fixgl.o
ifndef ASM_TYPE
	OBJS+=	src/video/draw_tile.o \
		src/video/draw_font.o
endif
	NEO4ALL_CFLAGS+= -DUSE_VIDEO_GL
else
	OBJS+=	src/video/spr.o \
		src/video/draw_fix.o
endif


ifdef Z80_EMULATED
	ifdef RAZE_CORE
		ifndef DREAMCAST
			Z80_OBJS = src/z80/raze/raze.o
		else
			Z80_OBJS = src/z80/faze/raze.o
		endif
	else
	ifdef MAMEZ80_CORE
		Z80_OBJS = src/z80/mamez80/z80.o
	else
		ifdef CZ80_CORE
			Z80_OBJS = src/z80/cz80/cz80.o
		else
			Z80_OBJS = src/z80/mz80/z80.o
		endif
	endif
	endif
	Z80_OBJS+= src/z80/z80intrf.o
endif


ifdef FAME_CORE
ifdef FAME_C_CORE
FAME_OBJ=src/68k/fame/famec.o
else
ifdef ASM_TYPE
$(FAME_OBJ): $(FAME_SRC)
	nasm -f $(ASM_TYPE) $(FAME_SRC)
endif
endif
OBJS_68K= $(FAME_OBJ)
MEMORY_OBJ= src/memory/fame/memory.o

else
ifdef MUSASHI_CORE
OBJS_68K=	\
	src/68k/c68k/m68kops.o \
	src/68k/c68k/m68kcpu.o \
	src/68k/c68k/m68kopac.o \
	src/68k/c68k/m68kopdm.o \
	src/68k/c68k/m68kopnz.o \
	src/68k/c68k/m68kdasm.o
else
OBJS_68K=	\
	src/68k/c68k/c68kexec.o	\
	src/68k/c68k/c68k.o
endif

OBJS_68K+= src/68k/c68k/cpu68k.o

MEMORY_OBJ= src/memory/c68k/memory.o

endif


# --------------------------------------

