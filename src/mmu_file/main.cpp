#include "mmu_file.h"

void *buffer;


#define PREFIX "/tmp/"

void *buf=NULL;
mmu_handle_func_t fun;

int mmu_handle_prefetching=0;


void mmu_handle_init(void)
{
}

void mmu_handle_quit(void)
{
	if (buf) free(buf);
	buf=NULL;
}

void mmu_handle_restart(void)
{
}

void *mmu_handle_add(unsigned real_size, unsigned virtual_size, mmu_handle_func_t func)
{
	printf("mmu_handle_add(%X, %X, %p)\n",real_size,virtual_size, func); fflush(stdout);

	fun=func;
	buf=calloc(1,virtual_size);
	return buf;
}

int mmu_handle_set_prefetch_func(unsigned id, mmu_handle_func_t func, void *tmp_mem)
{
	return 0;
}

int getMe(int index)
{
	int ret;
	void *b=calloc(4,1024);
	unsigned pad=index&1023;
	(fun)(b,(index<<2)&0xFFFFF000,4*1024);
	ret=((int *)b)[pad];
	free(b);
	return ret;
}


int main()
{
	int i,j;

	puts("MMU TEST"); fflush(stdout);
	mmu_file_init();
	buffer=mmu_file_add(PREFIX "fichero",512*1024,0);
	
	for(j=0;j<2;j++)
//	for(i=0;i<(16*1024);i+=512)
	for(i=0;i<(8*512);i+=512)
	{
		int ret=getMe(i);
		if (i!=ret)
			printf("%i!=%i\n",i,ret);
	}
getMe(0);
getMe(0);
getMe(0);
getMe(0);
getMe(10*512);
getMe(0);
getMe(1024);
getMe(1024*2);
getMe(1024*3);
getMe(1024*4);

	puts("The End!");
	mmu_file_quit();
	return 0;
}
