#ifdef DREAMCAST
#include <kos.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#if !defined(PAGESIZE) || !defined(PAGESIZE_BITS)
#undef PAGESIZE
#undef PAGESIZE_BITS
#define PAGESIZE_BITS 12
#define PAGESIZE (1<<PAGESIZE_BITS)
#endif

#ifndef MMU_HANDLE_BEGINMEM
#define MMU_HANDLE_BEGINMEM 0x40000000
#endif

#ifndef MMU_HANDLE_SLOT_SHIFT
#define MMU_HANDLE_SLOT_SHIFT 0
#endif

#ifndef MMU_HANDLE_SLICE
// #define MMU_HANDLE_SLICE 4
#define MMU_HANDLE_SLICE 0
#endif

#ifndef MMU_HANDLE_MAX
#ifdef MMU_FILE_MAX_FILES
#define MMU_HANDLE_MAX MMU_FILE_MAX_FILES
#else
#define MMU_HANDLE_MAX 1
#endif
#endif

#ifndef MMU_HANDLE_CACHEABLE
#define MMU_HANDLE_CACHEABLE
#endif

#ifndef MMU_HANDLE_READONLY
//#define MMU_HANDLE_READONLY
#endif

#ifndef MMU_HANDLE_USE_REAL_HANDLER
#define MMU_HANDLE_USE_REAL_HANDLER
#endif

#ifndef MMU_HANDLE_USE_SQ_PATCH
#define MMU_HANDLE_USE_SQ_PATCH
#endif

typedef void (*mmu_handle_func_t)(void *mem, unsigned start, unsigned size);
typedef void (*mmu_handle_loading_func_t)(int per, int max);

void mmu_handle_init(void);
void mmu_handle_quit(void);
void mmu_handle_restart(void);
void *mmu_handle_add(unsigned real_size, unsigned virtual_size, mmu_handle_func_t func);
int mmu_handle_set_prefetch_func(unsigned id, mmu_handle_func_t func, void *tmp_mem);
void mmu_handle_prefetch_all(mmu_handle_loading_func_t func);
void mmu_handle_prefetch(unsigned id, unsigned pos, unsigned size);
void mmu_handle_prefetch_by_array(unsigned id, unsigned *the_array, mmu_handle_loading_func_t func);
void mmu_handle_direct_fetch(unsigned id, unsigned *the_array, mmu_handle_loading_func_t func);
void mmu_handle_inc_frame(void);
void mmu_handle_flush(void);
unsigned mmu_handle_get_free_memory(void);
void mmu_handle_reset(void);
void mmu_handle_dump_memaccess(void);
void mmu_handle_disable_slice(int b);
unsigned mmu_handle_get_frame(void);
void *mmu_handle_get_membuf(unsigned id);
unsigned mmu_handle_get_memsize(unsigned id);

extern int mmu_handle_prefetching;
