
// MAX 16 FILES

#ifndef MMU_FILE_MAX_FILES
#define MMU_FILE_MAX_FILES 1
#endif

#define MMU_FILE_SOUND_PATCH (4*1024)

#include "mmu_handle.h"

void mmu_file_init(void);
void mmu_file_quit(void);
void mmu_file_restart(void);
void *mmu_file_add(char *filename, unsigned max_real, unsigned offset);
void mmu_file_set_pos(unsigned id, unsigned pos);
int mmu_file_direct_fetch(unsigned id, unsigned *the_array, mmu_handle_loading_func_t func, char *filename, unsigned offset);
