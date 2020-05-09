#include "mmu_file.h"

#if MMU_FILE_MAX_FILES > 16
#undef MMU_FILE_MAX_FILES
#define MMU_FILE_MAX_FILES 16
#else
#if MMU_FILE_MAX_FILES < 1
#undef MMU_FILE_MAX_FILES
#define MMU_FILE_MAX_FILES 1
#endif
#endif

#define MMU_HANDLE_SLOT (1<<MMU_HANDLE_SLOT_SHIFT)
#define MMU_HANDLE_SLOT_PAGMASK (MMU_HANDLE_SLOT-1)
#define MMU_HANDLE_SLOT_SIZE (PAGESIZE<<MMU_HANDLE_SLOT_SHIFT)
#define MMU_HANDLE_SLOT_BITS (PAGESIZE_BITS+MMU_HANDLE_SLOT_SHIFT)
#define MMU_HANDLE_SLOT_MASK (MMU_HANDLE_SLOT_SIZE-1)
#define MMU_HANDLE_BEGINMEM_PAG (MMU_HANDLE_BEGINMEM>>PAGESIZE_BITS)

#define SDL_DC_SPU_RAM_BASE     (0xa0800000+0x11000)
#define SDL_DC_SPU_RAM_START	(SDL_DC_SPU_RAM_BASE+0x3000)
#define SDL_DC_SPU_RAM_LEN	(0xa0a00000-SDL_DC_SPU_RAM_START)
#define SDL_DC_SPU_RAM_BLOCK	(SDL_DC_SPU_RAM_LEN/MMU_FILE_MAX_FILES)
#define SDL_DC_SPU_RAM_SLOTS    (SDL_DC_SPU_RAM_BLOCK/MMU_HANDLE_SLOT_SIZE)


// #define MMU_FILE_DEBUG

//#if defined(DREAMCAST) && !defined(DEBUG_NEO4ALL)
#if 0
#define MMU_FILE_LOW_LEVEL
extern "C" {
int __kos__iso_tell(void * h);
int __kos__iso_seek(void * h, off_t offset, int whence);
int __kos__iso_read(void * h, void *buf, size_t bytes);
void __kos__iso_close(void * h);
void * __kos__iso_open(void * vfs, const char *fn, int mode);
unsigned __kos__iso_get_first_sector(void *h);
}

//#define low_level_read_sectors(BUF,POS,SZ) cdrom_read_sectors((BUF),(POS),(SZ))

#define MAKE_SYSCALL(rs, p1, p2, idx) \
{ \
	unsigned *syscall_bc = (unsigned*)0x8c0000bc; \
	int (*syscall)(unsigned,unsigned,unsigned,unsigned) = (int (*)(unsigned,unsigned,unsigned,unsigned))(*syscall_bc); \
	rs=syscall((unsigned)(p1), (unsigned)(p2), 0, (unsigned)(idx)); \
}


static int low_level_status[4] = {0,0,0,0};
static struct { int     sec, num; void    *buffer; int     dunno; } low_level_params;

static __inline__ void low_level_read_sectors(void *buf, unsigned pos, unsigned cnt)
{
	int f,n;

	low_level_params.sec = pos;
	low_level_params.num = cnt;
	low_level_params.buffer = buf;
	low_level_params.dunno = 0;

	MAKE_SYSCALL(f,CMD_PIOREAD,((void *)&low_level_params),0)
	do {
		MAKE_SYSCALL(n, 0, 0, 2);
		MAKE_SYSCALL(n, f, low_level_status, 1);
	} while (n==1);
}
#else
#ifdef MMU_FILE_LOW_LEVEL
#undef MMU_FILE_LOW_LEVEL
#endif
#endif

#include<stdio.h>

#ifndef MMU_FILE_LOW_LEVEL
static FILE *mmu_file_fhandle[MMU_FILE_MAX_FILES];
#else
static void *mmu_file_fhandle[MMU_FILE_MAX_FILES];
#endif
static unsigned mmu_file_offset[MMU_FILE_MAX_FILES];
static unsigned mmu_file_pos[MMU_FILE_MAX_FILES];
static unsigned mmu_file_id[MMU_FILE_MAX_FILES];
static void *mmu_file_buffer[MMU_FILE_MAX_FILES];
static unsigned mmu_file_used[MMU_FILE_MAX_FILES][SDL_DC_SPU_RAM_SLOTS];
static unsigned mmu_file_cache[MMU_FILE_MAX_FILES][SDL_DC_SPU_RAM_SLOTS];
static unsigned mmu_file_nfiles=0;
static unsigned mmu_file_initted=0;


static void *mmu_file_memprefetch=NULL;


#define mmu_file_sound_silence()


static void mmu_file_real_handler_file(unsigned n, void *mem, unsigned start, unsigned size)
{
	unsigned offset=mmu_file_offset[n]+start;
#ifndef MMU_FILE_LOW_LEVEL
	FILE *fhandle=mmu_file_fhandle[n];
#endif

#ifdef MMU_FILE_DEBUG
	printf("mmu_file: Reading %i bytes, position %i\n",size,offset);
#endif
#ifdef MMU_FILE_SOUND_PATCH
	if (!mmu_handle_prefetching)
		mmu_file_sound_silence();
#endif
#ifdef MMU_FILE_LOW_LEVEL
#ifdef MMU_FILE_DEBUG
	printf("mmu_file: Low Level Reading %i sectors from %i\n",(size>>11),(mmu_file_pos[n]+(offset>>11)));
#endif
	low_level_read_sectors(mem,(mmu_file_pos[n]+(offset>>11)),(size>>11));
#else
	if (offset!=mmu_file_pos[n])
		fseek(fhandle,offset,SEEK_SET);
	fread(mem,1,size,fhandle);
	mmu_file_pos[n]=offset+size;
#endif
}

static void *mmu_file_find(unsigned n, unsigned start)
{
	unsigned i;
	for(i=0;i<SDL_DC_SPU_RAM_SLOTS;i++)
		if (mmu_file_cache[n][i]==start)
		{
			mmu_file_used[n][i]=mmu_file_initted++;
			return (void *)(((unsigned)mmu_file_buffer[n])+(i*MMU_HANDLE_SLOT_SIZE));
		}
	return NULL;
}

static void *mmu_file_search_slot(unsigned n, unsigned start)
{
	unsigned i,min=0xFFFFFFFF,ret=0;
	for(i=0;i<SDL_DC_SPU_RAM_SLOTS;i++)
	{
		unsigned actual=mmu_file_used[n][i];
		if (actual<min)
		{
			min=actual;
			ret=i;
		}
	}
	mmu_file_cache[n][ret]=start;
	mmu_file_used[n][ret]=mmu_file_initted++;
	return (void *)(((unsigned)mmu_file_buffer[n])+(ret*MMU_HANDLE_SLOT_SIZE));
	
}

static void mmu_file_real_handler(unsigned n, void *mem, unsigned start, unsigned size)
{
	unsigned st=start>>MMU_HANDLE_SLOT_BITS;
	if (mmu_handle_prefetching)
	{
		mmu_file_real_handler_file(n,mem,start,size);
		return;
	}
#ifdef MMU_FILE_DEBUG
	printf("mmu_file: Geting %i bytes, position %i\n",size,start);
#endif
	void *ret=mmu_file_find(n,st);
	if (!ret)
	{
		ret=mmu_file_search_slot(n,st);
		mmu_file_real_handler_file(n,ret,start,size);
	}

	memcpy(mem,ret,size);
}

static void mmu_file_prefetch_handler(void *mem, unsigned start, unsigned size)
{
	memcpy((void *)mem,(void *)mmu_file_memprefetch,size);
}


static void mmu_file_real_handler_0(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0x0,mem,start,size); }
#if MMU_FILE_MAX_FILES > 1
static void mmu_file_real_handler_1(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0x1,mem,start,size); }
#if MMU_FILE_MAX_FILES > 2
static void mmu_file_real_handler_2(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0x2,mem,start,size); }
#if MMU_FILE_MAX_FILES > 3
static void mmu_file_real_handler_3(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0x3,mem,start,size); }
#if MMU_FILE_MAX_FILES > 4
static void mmu_file_real_handler_4(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0x4,mem,start,size); }
#if MMU_FILE_MAX_FILES > 5
static void mmu_file_real_handler_5(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0x5,mem,start,size); }
#if MMU_FILE_MAX_FILES > 6
static void mmu_file_real_handler_6(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0x6,mem,start,size); }
#if MMU_FILE_MAX_FILES > 7 
static void mmu_file_real_handler_7(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0x7,mem,start,size); }
#if MMU_FILE_MAX_FILES > 8 
static void mmu_file_real_handler_8(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0x8,mem,start,size); }
#if MMU_FILE_MAX_FILES > 9 
static void mmu_file_real_handler_9(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0x9,mem,start,size); }
#if MMU_FILE_MAX_FILES > 10
static void mmu_file_real_handler_a(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0xa,mem,start,size); }
#if MMU_FILE_MAX_FILES > 11
static void mmu_file_real_handler_b(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0xb,mem,start,size); }
#if MMU_FILE_MAX_FILES > 12
static void mmu_file_real_handler_c(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0xc,mem,start,size); }
#if MMU_FILE_MAX_FILES > 13
static void mmu_file_real_handler_d(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0xd,mem,start,size); }
#if MMU_FILE_MAX_FILES > 14
static void mmu_file_real_handler_e(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0xe,mem,start,size); }
#if MMU_FILE_MAX_FILES > 15
static void mmu_file_real_handler_f(void *mem, unsigned start, unsigned size) {
	mmu_file_real_handler(0xf,mem,start,size); }
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif



static mmu_handle_func_t mmu_file_handler[MMU_FILE_MAX_FILES]={
	mmu_file_real_handler_0,
#if MMU_FILE_MAX_FILES > 1
       	mmu_file_real_handler_1,
#if MMU_FILE_MAX_FILES > 2
	mmu_file_real_handler_2,
#if MMU_FILE_MAX_FILES > 3
       	mmu_file_real_handler_3,
#if MMU_FILE_MAX_FILES > 4
	mmu_file_real_handler_4,
#if MMU_FILE_MAX_FILES > 5
       	mmu_file_real_handler_5,
#if MMU_FILE_MAX_FILES > 6
	mmu_file_real_handler_6,
#if MMU_FILE_MAX_FILES > 7
       	mmu_file_real_handler_7,
#if MMU_FILE_MAX_FILES > 8
	mmu_file_real_handler_8,
#if MMU_FILE_MAX_FILES > 9
       	mmu_file_real_handler_9,
#if MMU_FILE_MAX_FILES > 10
	mmu_file_real_handler_a,
#if MMU_FILE_MAX_FILES > 11
       	mmu_file_real_handler_b,
#if MMU_FILE_MAX_FILES > 12
	mmu_file_real_handler_c,
#if MMU_FILE_MAX_FILES > 13
       	mmu_file_real_handler_d,
#if MMU_FILE_MAX_FILES > 14
	mmu_file_real_handler_e,
#if MMU_FILE_MAX_FILES > 15
       	mmu_file_real_handler_f
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

};





void mmu_file_init(void)
{
	unsigned i;
	if (mmu_file_initted)
		mmu_file_quit();
	else
		for(i=0;i<MMU_FILE_MAX_FILES;i++)
		{
			mmu_file_fhandle[i]=NULL;
			mmu_file_buffer[i]=NULL;
		}
	mmu_file_memprefetch=malloc(MMU_HANDLE_SLOT_SIZE);
	mmu_handle_init();
	mmu_file_initted=1;
}

void mmu_file_restart(void)
{
	unsigned i;
	if (mmu_file_initted)
	{
		mmu_handle_restart();
		for(i=0;i<MMU_FILE_MAX_FILES;i++)
			if (mmu_file_fhandle[i])
			{
				free(mmu_file_buffer[i]);
#ifdef MMU_FILE_LOW_LEVEL
				__kos__iso_close(mmu_file_fhandle[i]);
#else
				fclose(mmu_file_fhandle[i]);
#endif
				mmu_file_fhandle[i]=NULL;
				mmu_file_buffer[i]=NULL;
			}
		mmu_file_nfiles=0;
	}
	else
		mmu_file_init();
}

void mmu_file_quit(void)
{
	if (mmu_file_initted)
	{
		mmu_file_restart();
		mmu_handle_quit();
	}
	if (mmu_file_memprefetch)
	{
		free(mmu_file_memprefetch);
		mmu_file_memprefetch=NULL;
	}
	mmu_file_nfiles=0;
}

void mmu_file_set_pos(unsigned id, unsigned pos)
{
#ifndef MMU_FILE_LOW_LEVEL
	unsigned i;
	for(i=0;i<mmu_file_nfiles;i++)
		if (mmu_file_id[i]==id)
			break;
	if (i<mmu_file_nfiles)
	{
		fseek(mmu_file_fhandle[i],pos,SEEK_SET);
		mmu_file_pos[i]=pos;
	}
#endif
}

void *mmu_file_add(char *filename, unsigned max_real, unsigned offset)
{
	void *ret=NULL;
	if (mmu_file_nfiles!=MMU_FILE_MAX_FILES)
	{
#ifdef MMU_FILE_LOW_LEVEL
		if (!strncmp(filename,"/cd/",4))
			filename=(char *)&filename[3];
		mmu_file_fhandle[mmu_file_nfiles]=__kos__iso_open(NULL,filename,O_RDONLY);
#else
		mmu_file_fhandle[mmu_file_nfiles]=fopen(filename,"rb");
#endif
		mmu_file_buffer[mmu_file_nfiles]=malloc(SDL_DC_SPU_RAM_BLOCK);
		memset((void *)&mmu_file_used[mmu_file_nfiles][0],0,SDL_DC_SPU_RAM_SLOTS*sizeof(unsigned));
		memset((void *)&mmu_file_cache[mmu_file_nfiles][0],-1,SDL_DC_SPU_RAM_SLOTS*sizeof(unsigned));
		if (mmu_file_fhandle[mmu_file_nfiles])
		{
			unsigned filesize;
#ifdef MMU_FILE_LOW_LEVEL
			__kos__iso_seek(mmu_file_fhandle[mmu_file_nfiles],0,SEEK_END);
#else
			fseek(mmu_file_fhandle[mmu_file_nfiles],0,SEEK_END);
#endif
#ifdef MMU_FILE_LOW_LEVEL
			filesize=__kos__iso_tell(mmu_file_fhandle[mmu_file_nfiles]);
#else
			filesize=ftell(mmu_file_fhandle[mmu_file_nfiles]);
#endif
#ifdef MMU_FILE_LOW_LEVEL
			__kos__iso_seek(mmu_file_fhandle[mmu_file_nfiles],offset,SEEK_SET);
#else
			fseek(mmu_file_fhandle[mmu_file_nfiles],offset,SEEK_SET);
#endif
			if (filesize>offset)
				ret=mmu_handle_add(max_real, filesize-offset, mmu_file_handler[mmu_file_nfiles]);
			if (!ret)
			{
#ifdef MMU_FILE_LOW_LEVEL
				__kos__iso_close(mmu_file_fhandle[mmu_file_nfiles]);
#else
				fclose(mmu_file_fhandle[mmu_file_nfiles]);
#endif
				mmu_file_fhandle[mmu_file_nfiles]=NULL;
			}
			else
			{
#ifdef MMU_FILE_LOW_LEVEL
				mmu_file_pos[mmu_file_nfiles]=__kos__iso_get_first_sector(mmu_file_fhandle[mmu_file_nfiles]); //+(offset>>11);
#ifdef MMU_FILE_DEBUG
				printf("mmu_file: FIRST SECTOR=%i\n",mmu_file_pos[mmu_file_nfiles]);
#endif
			
#else
				mmu_file_pos[mmu_file_nfiles]=offset;
#endif
				mmu_file_id[mmu_file_nfiles]=(unsigned)ret;
				mmu_handle_set_prefetch_func((unsigned)ret,mmu_file_prefetch_handler,mmu_file_memprefetch);
				mmu_file_offset[mmu_file_nfiles]=offset;
				mmu_file_nfiles++;
			}
		}
	}
	return ret;
}

int mmu_file_direct_fetch(unsigned id, unsigned *the_array, mmu_handle_loading_func_t func, char *filename, unsigned offset)
{
	unsigned i,vp,msize,max;
	void *membuf;
#ifndef MMU_FILE_LOW_LEVEL
	FILE *f=fopen(filename,"rb");
#else
	if (!strncmp(filename,"/cd/",4))
		filename=(char *)&filename[3];
	void *f=__kos__iso_open(NULL,filename,O_RDONLY);
#endif
	if (!f)
		return 0;

	for(i=0;i<mmu_file_nfiles;i++)
		if (mmu_file_id[i]==id)
			break;
	vp=i;
	if (vp>=mmu_file_nfiles)
	{
#ifndef MMU_FILE_LOW_LEVEL
		fclose(f);
#else
		__kos__iso_close(f);
#endif
		return 0;
	}

	membuf=mmu_handle_get_membuf(id);
	max=mmu_handle_get_memsize(id);

	if ((!membuf)&&(!max))
	{
#ifndef MMU_FILE_LOW_LEVEL
		fclose(f);
#else
		__kos__iso_close(f);
#endif
		return 0;
	}

#ifndef MMU_FILE_LOW_LEVEL
	fseek(f,0,SEEK_END);
	msize=ftell(f)-offset;
	fseek(f,offset,SEEK_SET);
#else
	 __kos__iso_seek(f,0,SEEK_END);
	 msize=__kos__iso_tell(f)-offset;
	 __kos__iso_seek(f,offset,SEEK_SET);
	 offset=__kos__iso_get_first_sector(f)+(offset/2048);
#endif

	if (msize<(512*1024))
	{
#ifndef MMU_FILE_LOW_LEVEL
		fclose(f);
#else
		__kos__iso_close(f);
#endif
		return 0;
	}
	else
	if (msize>max)
		msize=max&(~((512*1024)-1));

	for(i=0;i<msize;i+=512*1024)
	{
		if (func)
			(*func)((i+256*1024)%msize,msize);
#ifndef MMU_FILE_LOW_LEVEL
		fread((void *)(((unsigned)membuf)+i),1,512*1024,f);
#else
		low_level_read_sectors((void *)(((unsigned)membuf)+i),offset+(i/2048), (512*1024)/2048);
#endif
	}

#ifndef MMU_FILE_LOW_LEVEL
	fclose(f);
#else
	__kos__iso_close(f);
#endif

	mmu_handle_direct_fetch(id, the_array, func);

	return 1;
}

