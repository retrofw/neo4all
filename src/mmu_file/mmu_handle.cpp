#include "mmu_handle.h"

#include<stdio.h>
#include<stdlib.h>

/*
#define MMU_HANDLE_EXTRA_DEBUG
#define MMU_HANDLE_DEBUG
*/


#define MMU_HANDLE_SLOT (1<<MMU_HANDLE_SLOT_SHIFT)
#define MMU_HANDLE_SLOT_PAGMASK (MMU_HANDLE_SLOT-1)
#define MMU_HANDLE_SLOT_SIZE (PAGESIZE<<MMU_HANDLE_SLOT_SHIFT)
#define MMU_HANDLE_SLOT_BITS (PAGESIZE_BITS+MMU_HANDLE_SLOT_SHIFT)
#define MMU_HANDLE_SLOT_MASK (MMU_HANDLE_SLOT_SIZE-1)
#define MMU_HANDLE_BEGINMEM_PAG (MMU_HANDLE_BEGINMEM>>PAGESIZE_BITS)


static int mmu_handle_started=0;
static mmucontext_t * mmu_handle_ctx;
static mmu_mapfunc_t mmu_handle_handler_back;
static void *mmu_handle_realmem[MMU_HANDLE_MAX];
static void *mmu_handle_membuf[MMU_HANDLE_MAX];

static unsigned *mmu_handle_memaccess=NULL;
static unsigned mmu_handle_memaccess_size=0;
static unsigned mmu_handle_frame=1;
static unsigned mmu_handle_nbufs=0;
static unsigned mmu_handle_npags=0;
static unsigned mmu_handle_nvirtualpags=0;
static unsigned mmu_handle_memsize[MMU_HANDLE_MAX];
static unsigned mmu_handle_virtualmemsize[MMU_HANDLE_MAX];
static void *mmu_handle_virtualbuf[MMU_HANDLE_MAX];
static mmu_handle_func_t mmu_handle_func[MMU_HANDLE_MAX];
static mmu_handle_func_t mmu_handle_prefetch_func[MMU_HANDLE_MAX];
static void *mmu_handle_prefetch_mem[MMU_HANDLE_MAX];
int mmu_handle_prefetching=0;
static int mmu_handle_prefetch_func_enable=1;

static volatile unsigned * const mmu_handle_pteh  = (unsigned *)(0xff000000);
static volatile unsigned * const mmu_handle_ptel  = (unsigned *)(0xff000004);
static volatile unsigned * const mmu_handle_tea   = (unsigned *)(0xff00000c);
static volatile unsigned * const mmu_handle_mmucr = (unsigned *)(0xff000010);


#ifdef MMU_HANDLE_CACHEABLE
#define MMU_HANDLE_MMU_CACHE MMU_CACHE_WT
#else
#define MMU_HANDLE_MMU_CACHE MMU_NO_CACHE 
#endif

#if defined(MMU_HANDLE_EXTRA_DEBUG) && !defined(MMU_HANDLE_DEBUG)
#define MMU_HANDLE_DEBUG
#endif

#if defined(MMU_HANDLE_DEBUG) && defined(MMU_HANDLE_USE_REAL_HANDLER)
#undef MMU_HANDLE_USE_REAL_HANDLER
#endif

#ifdef MMU_HANDLE_USE_REAL_HANDLER
#ifdef __cplusplus
extern "C" {
static void mmu_handle_real_handler(irq_t source, irq_context_t *context);
}
#endif
#endif

#define MMU_HANDLER_PTEL(PA, V, SZ, PR, C, D, SH, WT) \
	        ( ((PA) & 0xfffffc00) | ((V) << 8) \
	                  | ( ((SZ) & 2) << 6 ) | ( ((SZ) & 1) << 4 ) \
	                  | ( (PR) << 5 ) \
	                  | ( (C) << 3 ) \
	                  | ( (D) << 2 ) \
	                  | ( (SH) << 1 ) \
	                  | ( (WT) << 0 ) )

#define MMU_HANDLER_LDTBL(PTEHV, PTELV) \
	*mmu_handle_pteh = PTEHV; \
	*mmu_handle_ptel = PTELV; \
	asm("ldtlb");


static int mmu_handle_exec(unsigned sel, unsigned virtual_ptr, unsigned real_buf)
{
#ifdef MMU_HANDLE_DEBUG
	printf("mmu_handle.c: exec -> sel=%i, virtual_ptr=0x%X, real_buf=%p\n",sel,virtual_ptr,real_buf);
#endif
#ifdef MMU_HANDLE_DEBUG
	if ((mmu_handle_func[sel])==NULL)
	{
		puts("\nmmu_handle.c: exec -> ERROR CRITICO: SIN MANEJADOR !!!\n");
		return -1;
	}
#endif
	unsigned size=MMU_HANDLE_SLOT_SIZE;
	unsigned start=virtual_ptr-((unsigned)mmu_handle_virtualbuf[sel]);
#ifdef MMU_HANDLE_DEBUG
	if (start>mmu_handle_virtualmemsize[sel])
	{
		puts("\nmmu_handle.c: exec -> ERROR CRITICO: START MAYOR QUE TOTAL !!!\n");
		return -2;
	}
#endif
	if ((start+MMU_HANDLE_SLOT_SIZE)>mmu_handle_virtualmemsize[sel])
		size=MMU_HANDLE_SLOT_SIZE-((start+MMU_HANDLE_SLOT_SIZE)%mmu_handle_virtualmemsize[sel]);

	(mmu_handle_func[sel])((void *)real_buf,start,size);
	return 0;
}

static int mmu_handle_read(unsigned virtualpage, unsigned real_buf)
{
	unsigned i,sel=0;
	unsigned maximo=0;
	unsigned virtual_ptr=virtualpage<<PAGESIZE_BITS;
#ifdef MMU_HANDLE_DEBUG
	printf("mmu_handle.c: read -> virtual_ptr=%p, real_buf=%p\n",virtual_ptr,real_buf);
#endif
	for(i=0;i<mmu_handle_nbufs;i++)
	{
		unsigned actual=(unsigned)mmu_handle_virtualbuf[i];
		if ((actual)&&(actual>maximo)&&(actual<=virtual_ptr))
		{
			maximo=actual;
			sel=i;
		}
	}
	return mmu_handle_exec(sel,virtual_ptr,real_buf);
}


static unsigned mmu_handle_find(unsigned orig_vp)
{
	unsigned poff=orig_vp&MMU_HANDLE_SLOT_PAGMASK;
	unsigned pmask=orig_vp-poff;
	unsigned minimo=0xFFFFFFFF;
        unsigned vp=MMU_HANDLE_BEGINMEM_PAG;
	unsigned save_vp=0;
	mmupage_t *save_p=NULL;
#ifdef MMU_HANDLE_DEBUG
	printf("mmu_handle.c: find -> orig_vp=0x%X\n",orig_vp);
#endif
	{
		mmusubcontext_t *sc=mmu_handle_ctx->sub[pmask>>9];
		if (!sc) return 0;
		mmupage_t *p=&sc->page[pmask&0x1ff];
		if (!p) return 0;
		if (p->prkey==MMU_ALL_RDWR)
		{
			save_vp=pmask;
			save_p=p;
		}
	}
	if (!save_vp)
	for(vp=MMU_HANDLE_BEGINMEM_PAG;vp<(MMU_HANDLE_BEGINMEM_PAG+mmu_handle_nvirtualpags);vp+=MMU_HANDLE_SLOT)
	{
		mmusubcontext_t *sc=mmu_handle_ctx->sub[vp>>9];
		if (!sc) break;
		mmupage_t *p=&sc->page[vp&0x1ff];
		if (!p) break;
		if (p->prkey==MMU_ALL_RDWR)
		{
			unsigned actual=mmu_handle_memaccess[(vp-MMU_HANDLE_BEGINMEM_PAG)>>MMU_HANDLE_SLOT_SHIFT];
#ifdef MMU_HANDLE_EXTRA_DEBUG
			printf("mmu_handle.c: find -> vp=0x%X, actual=%i, minimo=%i\n",vp,actual,minimo);
#endif
			if ((actual<minimo)||((actual==minimo)&&(p->valid)))
			{
				minimo=actual;
				save_p=p;
				save_vp=vp;
			}
		}
	}
#ifdef MMU_HANDLE_DEBUG
	printf("mmu_handle.c: find -> save_vp=0x%X\n",save_vp);
#endif
	if (save_p)
	{
		int i;
		unsigned ret=save_p->physical;
		if (mmu_handle_read(pmask,(ret << PAGESIZE_BITS)|0x80000000))
			return 0;
		for(i=0,vp=pmask;i<MMU_HANDLE_SLOT;i++)
		{
			mmusubcontext_t *sc=mmu_handle_ctx->sub[vp>>9];
			if (!sc) break;
			mmupage_t *p=&sc->page[vp&0x1ff];
			if (!p) break;
			p->valid=1;
			p->prkey=MMU_ALL_RDWR;
			p->physical=ret+i;
#ifdef MMU_HANDLE_CACHEABLE
			p->cache=1;
			p->wthru=1;
#endif
#ifdef MMU_HANDLE_READONLY
			p->dirty=0;
#endif
			p->ptel = MMU_HANDLER_PTEL(p->physical << PAGESIZE_BITS, 1, 1, p->prkey, p->cache, p->dirty, p->shared, p->wthru);
			vp++;
		}
		if (save_vp!=pmask)
			for(i=0,vp=save_vp;i<MMU_HANDLE_SLOT;i++)
			{
				mmusubcontext_t *sc=mmu_handle_ctx->sub[vp>>9];
				if (!sc) break;
				mmupage_t *p=&sc->page[vp&0x1ff];
				if (!p) break;
				p->valid=0;
				p->prkey=MMU_KERNEL_RDWR;
				p->physical=0;
				vp++;
			}
		dcache_flush_range((ret << PAGESIZE_BITS)|0x80000000, MMU_HANDLE_SLOT_SIZE );
		mmu_handle_flush();
		return ret+poff;
	}
	return 0;
}

void *mmu_handle_get_membuf(unsigned id)
{
	int i;
	for(i=0;i<mmu_handle_nbufs;i++)
		if (((unsigned)mmu_handle_virtualbuf[i])==id)
			return mmu_handle_membuf[i];
	return NULL;
}

void mmu_handle_direct_fetch(unsigned id, unsigned *the_array, mmu_handle_loading_func_t func)
{
	unsigned i,vp,phy,msize;

	for(i=0;i<mmu_handle_nbufs;i++)
		if (((unsigned)mmu_handle_virtualbuf[i])==id)
			break;
	vp=i;
	if (vp>=mmu_handle_nbufs)
		return;

	if (func) (*func)(100,100);

	msize=mmu_handle_memsize[vp];

	vp=MMU_HANDLE_BEGINMEM_PAG;
	{
		mmusubcontext_t *sc=mmu_handle_ctx->sub[vp>>9];
		mmupage_t *p=&sc->page[vp&0x1ff];
		phy=p->physical;
		msize>>=PAGESIZE_BITS;
		msize+=phy;
	}
	for(i=0;vp<(MMU_HANDLE_BEGINMEM_PAG+mmu_handle_nvirtualpags);vp+=MMU_HANDLE_SLOT)
	{
		mmusubcontext_t *sc=mmu_handle_ctx->sub[vp>>9];
		mmupage_t *p=&sc->page[vp&0x1ff];
		switch (the_array[i])
		{
			case 0:
				mmu_handle_memaccess[i]=0;
				p->valid=0;
				p->physical=0;
				p->prkey=MMU_KERNEL_RDWR;
				i++;
				break;
			case 0xFFFFFFFF:
				p->valid=0;
				if (phy<msize)
				{
					p->physical=phy;
					phy++;
				}
				else
				{
					p->physical=0;
					p->prkey=MMU_KERNEL_RDWR;
				}
				mmu_handle_memaccess[i]=0;
				break;
			default:
				p->valid=1;
				p->prkey=MMU_ALL_RDWR;
				p->physical=phy;
#ifdef MMU_HANDLE_CACHEABLE
				p->cache=1;
				p->wthru=1;
#endif
#ifdef MMU_HANDLE_READONLY
				p->dirty=0;
#endif
				mmu_handle_memaccess[i]=the_array[i];
				phy++;
				i++;
		}
		p->ptel = MMU_HANDLER_PTEL(p->physical << PAGESIZE_BITS, 1, 1, p->prkey, p->cache, p->dirty, p->shared, p->wthru);
	}
}

#if MMU_HANDLE_SLICE > 0
void mmu_handle_find_slice(unsigned orig_vp)
{
	unsigned i, nvp=orig_vp+1;
	for(i=1;(i<(1<<MMU_HANDLE_SLICE))&&(nvp<(MMU_HANDLE_BEGINMEM_PAG+mmu_handle_nvirtualpags));i++,nvp++)
	{
		mmupage_t *np=&(((mmusubcontext_t *)mmu_handle_ctx->sub[nvp>>9])->page[nvp&0x1ff]);
		if (!np->valid)
			mmu_handle_find(nvp);
		mmu_handle_memaccess[(nvp-MMU_HANDLE_BEGINMEM_PAG)>>MMU_HANDLE_SLOT_SHIFT]=mmu_handle_frame;
	}
}
#endif


#ifdef MMU_HANDLE_USE_REAL_HANDLER
static mmupage_t * mmu_handle_handler(mmucontext_t * c, int _vp) {
	return NULL;
}
static void mmu_handle_real_handler(irq_t source, irq_context_t *context) {
	register unsigned _vp= (*mmu_handle_tea)>>PAGESIZE_BITS;
#else
static mmupage_t * mmu_handle_handler(mmucontext_t * c, int _vp) {
#endif
	register unsigned vp=_vp&0x7FFFF;
#ifdef MMU_HANDLE_DEBUG
	if (c!=mmu_handle_ctx)
	{
		puts("\nmmu_handle.c: ERROR CRITICO -> CONTEXTO NO IDENTIFICADO !!!");
		printf("\t\tvp=0x%X\n\n",vp);
		return (mmupage_t *)NULL;
	}

	unsigned hvp=vp>>9;
	mmusubcontext_t *sc=c->sub[hvp];
	
	if (sc==NULL)
	{
		puts("\nmmu_handle.c: ERROR CRITICO -> SUBCONTEXTO NO ENCONTRADO !!!");
		printf("\t\tvp=0x%X, hvp=0x%X\n\n",vp,hvp);
		return (mmupage_t *)NULL;
	}
		
	unsigned lvp=vp&0x1ff;
	mmupage_t *p=&sc->page[lvp];

	if (p==NULL)
	{
		puts("\nmmu_handle.c: ERROR CRITICO -> PAGINA NO ENCONTRADA !!!");
		printf("\t\tvp=0x%X, hvp=0x%X, lvp=0x%X\n\n",vp,hvp,lvp);
		return (mmupage_t *)NULL;
	}
#else
	register mmupage_t *p=&(((mmusubcontext_t *)mmu_handle_ctx->sub[vp>>9])->page[vp&0x1ff]);
#endif

#ifdef MMU_HANDLE_EXTRA_DEBUG
	printf("mmu_handle.c: handler -> VP=0x%X -> HVP=0x%X, LVP=0x%X -> physical=0x%X, prkey=0x%X\n", vp,hvp,lvp,p->physical,p->prkey);
#endif

	if (!p->valid)
	{
#ifdef MMU_HANDLE_DEBUG
		printf("mmu_handle.c: handler -> FALLO EN LA PAGINA 0x%X\n",vp);
		if (((unsigned)vp<MMU_HANDLE_BEGINMEM_PAG) || ((unsigned)vp>=(MMU_HANDLE_BEGINMEM_PAG+mmu_handle_nvirtualpags)))
		{
			puts("\nmmu_handle.c: ERROR CRITICO -> PAGINA FUERA DE RANGO !!!\n");
			return NULL;
		}
#endif
		p->physical=mmu_handle_find(vp);
#if MMU_HANDLE_SLICE > 0
		if (!mmu_handle_prefetching)
			mmu_handle_find_slice(vp);
#endif
		if (!p->physical)
#ifdef MMU_HANDLE_USE_REAL_HANDLER
			return;
#else
			return NULL;
#endif
#ifdef MMU_HANDLE_DEBUG
		printf("mmu_handle.c: handler -> NUEVA PAGINA physical=0x%X\n",p->physical);
#endif
		mmu_handle_frame++;
	}
#ifdef MMU_HANDLE_EXTRA_DEBUG
	printf("mmu_handle.c: handler -> mmu_handle_memaccess[0x%X]=%i\n",(vp-MMU_HANDLE_BEGINMEM_PAG)>>MMU_HANDLE_SLOT_SHIFT,mmu_handle_frame);
#endif
	if (vp==(unsigned)_vp)
		mmu_handle_memaccess[(vp-MMU_HANDLE_BEGINMEM_PAG)>>MMU_HANDLE_SLOT_SHIFT]=mmu_handle_frame;
#ifdef MMU_HANDLE_USE_REAL_HANDLER
	MMU_HANDLER_LDTBL(p->pteh,p->ptel);
#else
	return p;
#endif
}

static void *mmu_handle_get_pages(unsigned npages)
{
	unsigned i;
	unsigned ptr=0;
	void *dummy=NULL;
	for(i=0;i<(PAGESIZE*2);i+=4)
	{
		ptr=(unsigned)malloc(npages<<PAGESIZE_BITS);
		if (!(ptr&PAGEMASK))
			break;
		free((void *)ptr);
		ptr=0;
		if (dummy)
			free(dummy);
		dummy=malloc(i+1);
	}
	if (dummy)
		free(dummy);
	return (void *)ptr;
}

static void *mmu_handle_allocbuf(unsigned m)
{
	if (mmu_handle_realmem[mmu_handle_nbufs])
		free(mmu_handle_realmem[mmu_handle_nbufs]);
	mmu_handle_memsize[mmu_handle_nbufs]=m<<PAGESIZE_BITS;
	mmu_handle_realmem[mmu_handle_nbufs]=mmu_handle_get_pages(m);
	if (mmu_handle_realmem[mmu_handle_nbufs])
		mmu_handle_membuf[mmu_handle_nbufs]=mmu_handle_realmem[mmu_handle_nbufs];
	else
	{
		mmu_handle_realmem[mmu_handle_nbufs]=calloc(mmu_handle_memsize[mmu_handle_nbufs]+PAGESIZE,1);
		if (mmu_handle_realmem[mmu_handle_nbufs])
			mmu_handle_membuf[mmu_handle_nbufs]=(void *)((((unsigned)mmu_handle_realmem[mmu_handle_nbufs]+PAGESIZE)/PAGESIZE)*PAGESIZE);
		else
			mmu_handle_membuf[mmu_handle_nbufs]=NULL;
	}
	return mmu_handle_membuf[mmu_handle_nbufs];
}


static int mmu_handle_allocpages(unsigned real_pags, unsigned virtual_pags)
{
	unsigned i;
	if (mmu_handle_allocbuf(real_pags))
	{
		mmu_page_map(mmu_handle_ctx,MMU_HANDLE_BEGINMEM_PAG+mmu_handle_nvirtualpags,((unsigned)mmu_handle_membuf[mmu_handle_nbufs])>>PAGESIZE_BITS,real_pags,MMU_ALL_RDWR,MMU_HANDLE_MMU_CACHE,MMU_NOT_SHARED,MMU_DIRTY);
		for(i=0;i<virtual_pags;i++)
		{
			unsigned vp=MMU_HANDLE_BEGINMEM_PAG+mmu_handle_nvirtualpags+i;
			unsigned hvp=vp>>9;
			if (i>=real_pags)
				mmu_page_map(mmu_handle_ctx,vp,0,1,MMU_KERNEL_RDWR,MMU_HANDLE_MMU_CACHE,MMU_NOT_SHARED,MMU_DIRTY);
			mmusubcontext_t *sc=mmu_handle_ctx->sub[hvp];
			if (!sc)
			{
				free(mmu_handle_realmem[mmu_handle_nbufs]);
				mmu_handle_realmem[mmu_handle_nbufs]=NULL;
				mmu_handle_membuf[mmu_handle_nbufs]=NULL;
				return -2;
			}
			unsigned lvp=vp&0x1ff;
			mmupage_t *p=&sc->page[lvp];
			if (!p)
			{
				free(mmu_handle_realmem[mmu_handle_nbufs]);
				mmu_handle_realmem[mmu_handle_nbufs]=NULL;
				mmu_handle_membuf[mmu_handle_nbufs]=NULL;
				return -3;
			}
			p->valid=0;
		}
		mmu_handle_virtualbuf[mmu_handle_nbufs]=(void *)((MMU_HANDLE_BEGINMEM_PAG+mmu_handle_nvirtualpags)<<PAGESIZE_BITS);
		mmu_handle_npags+=real_pags;
		mmu_handle_nvirtualpags+=virtual_pags;
		mmu_handle_memaccess_size=(1+(mmu_handle_nvirtualpags>>MMU_HANDLE_SLOT_SHIFT))*sizeof(unsigned);
		mmu_handle_memaccess=(unsigned *)realloc(mmu_handle_memaccess,mmu_handle_memaccess_size);
		if (mmu_handle_memaccess==NULL)
		{
			mmu_handle_npags-=real_pags;
			mmu_handle_nvirtualpags-=virtual_pags;
			free(mmu_handle_realmem[mmu_handle_nbufs]);
			mmu_handle_realmem[mmu_handle_nbufs]=NULL;
			mmu_handle_membuf[mmu_handle_nbufs]=NULL;
			return -4;
		}
		bzero(mmu_handle_memaccess,mmu_handle_memaccess_size);
		return 0;
	}
	return -1;
}

#ifdef MMU_HANDLE_USE_SQ_PATCH
static void mmu_handle_set_sq(void *sq, unsigned pages, void *dest)
{
	unsigned i;
	unsigned addr=(unsigned)sq;
	unsigned vp=(0xe0000000 | ( addr & 0x03ffffe0))>>PAGESIZE_BITS;
	unsigned vp_dest=((unsigned)dest)>>PAGESIZE_BITS;
	mmu_page_map(mmu_handle_ctx,vp,vp_dest,pages,MMU_ALL_RDWR,MMU_HANDLE_MMU_CACHE,MMU_NOT_SHARED,MMU_DIRTY);
	mmu_handle_flush();
	vp&=0x7FFFF;
	for(i=0;i<pages;i++)
	{
		unsigned hvp=(vp&0x7FFFF)>>9;
		unsigned lvp=vp&0x1ff;
		mmusubcontext_t *sc=mmu_handle_ctx->sub[hvp];
		if (!sc)
			break;
		mmupage_t *p=&sc->page[lvp];
		if (!p)
			break;
		p->physical=vp_dest;
		p->ptel = MMU_HANDLER_PTEL(vp_dest << (PAGESIZE_BITS), 1, 1, 3, 1, 1, 1, 1);
		vp++;
		vp_dest++;
	}
}

static void mmu_handle_sq_patch(void)
{
	mmu_handle_set_sq((void *)0x00000000,2048,(void *)0x00000000);
	mmu_handle_set_sq((void *)0xE0800000,2048,(void *)0x04000000);
	mmu_handle_set_sq((void *)0xE1800000,2048,(void *)0x05000000);
	mmu_handle_set_sq((void *)0xE2000000,2048,(void *)0x10000000);
}
#endif

static void mmu_handle_init_helper(void)
{
	int i;
	for(i=0;i<MMU_HANDLE_MAX;i++)
	{
		mmu_handle_realmem[i]=NULL;
		mmu_handle_membuf[i]=NULL;
		mmu_handle_func[i]=NULL;
		mmu_handle_prefetch_func[i]=NULL;
	}
	mmu_handle_handler_back=mmu_map_set_callback(mmu_handle_handler);
	mmu_handle_ctx=mmu_context_create(0);
	mmu_use_table(mmu_handle_ctx);
	mmu_switch_context(mmu_handle_ctx);
	mmu_handle_started=1;
	mmu_handle_frame=1;

#ifdef MMU_HANDLE_USE_REAL_HANDLER
	irq_set_handler(EXC_ITLB_MISS, mmu_handle_real_handler);
	irq_set_handler(EXC_DTLB_MISS_READ, mmu_handle_real_handler);
	irq_set_handler(EXC_DTLB_MISS_WRITE, mmu_handle_real_handler);
#endif

#ifdef MMU_HANDLE_USE_SQ_PATCH
	mmu_handle_sq_patch();
#endif
}

// *****************************
// -----------------------------
// PUBLIC FUNCS ....
// -----------------------------
// *****************************

void mmu_handle_init(void)
{
	if (mmu_handle_started)
		mmu_handle_quit();
	mmu_init();
	mmu_handle_init_helper();
}

void mmu_handle_restart(void)
{
	if (mmu_handle_started)
	{
		mmu_handle_reset();
		mmu_map_set_callback(mmu_handle_handler_back);
		mmu_use_table(mmu_cxt_current);
		mmu_switch_context(mmu_cxt_current);
		mmu_context_destroy(mmu_handle_ctx);
		if (mmu_handle_memaccess)
		{
			free(mmu_handle_memaccess);
			mmu_handle_memaccess=NULL;
		}
		if (mmu_handle_nbufs)
		{
			unsigned i;
			for(i=0;i<mmu_handle_nbufs;i++)
				if (mmu_handle_realmem[i])
				{
					free(mmu_handle_realmem[i]);
					mmu_handle_realmem[i]=NULL;
					mmu_handle_membuf[i]=NULL;
				}
			mmu_handle_nbufs=0;
		}
		mmu_handle_npags=0;
		mmu_handle_nvirtualpags=0;
		mmu_handle_init_helper();
	}
	else
		mmu_handle_init();
}

void mmu_handle_quit(void)
{
	if (mmu_handle_started)
	{
		mmu_handle_restart();
		mmu_handle_flush();
		mmu_map_set_callback(mmu_handle_handler_back);
		mmu_shutdown();
		mmu_context_destroy(mmu_handle_ctx);
		mmu_handle_started=0;
	}
}

int mmu_handle_set_prefetch_func(unsigned id, mmu_handle_func_t func, void *tmp_mem)
{
	unsigned i;
	for(i=0;i<mmu_handle_nbufs;i++)
		if (((unsigned)mmu_handle_virtualbuf[i])==id)
			break;
	if (i<mmu_handle_nbufs)
	{
		mmu_handle_prefetch_func[i]=func;
		mmu_handle_prefetch_mem[i]=tmp_mem;
		return 0;
	}
	return -1;
}

void *mmu_handle_add(unsigned real_size, unsigned virtual_size, mmu_handle_func_t func)
{
	unsigned real_pags=0;
	unsigned virtual_pags=0;
	if ((mmu_handle_nbufs>=MMU_HANDLE_MAX)||(!virtual_size)||(!real_size))
		return NULL;
	if (virtual_size&MMU_HANDLE_SLOT_MASK)
		virtual_pags+=MMU_HANDLE_SLOT;
	virtual_pags+=(virtual_size>>MMU_HANDLE_SLOT_BITS)<<MMU_HANDLE_SLOT_SHIFT;
	if (real_size&MMU_HANDLE_SLOT_MASK)
		real_pags+=MMU_HANDLE_SLOT;
	real_pags+=(real_size>>MMU_HANDLE_SLOT_BITS)<<MMU_HANDLE_SLOT_SHIFT;
#ifdef MMU_HANDLE_DEBUG
	printf("mmu_handle.c: add -> virtual_pags=%i, real_pags=%i\n",virtual_pags,real_pags);
#endif
	mmu_handle_func[mmu_handle_nbufs]=func;
	if (virtual_pags<=real_pags)
	{
// NO USE MMU FOR THIS CASE
		if (!mmu_handle_allocbuf(real_pags))
			return NULL;
		mmu_handle_virtualbuf[mmu_handle_nbufs]=NULL;
		mmu_handle_memsize[mmu_handle_nbufs]=real_size;
		mmu_handle_virtualmemsize[mmu_handle_nbufs]=real_size;
		return mmu_handle_membuf[mmu_handle_nbufs++];
	}
// USE MMU 
	if (mmu_handle_allocpages(real_pags,virtual_pags))
		return NULL;
	mmu_handle_memsize[mmu_handle_nbufs]=real_size;
	mmu_handle_virtualmemsize[mmu_handle_nbufs]=virtual_size;
	return mmu_handle_virtualbuf[mmu_handle_nbufs++];
}


unsigned ___mmu_handle_prefetched___=0;

void mmu_handle_prefetch_all(mmu_handle_loading_func_t func)
{
	unsigned i,d=0;
#ifdef MMU_HANDLE_DEBUG
	puts("mmu_handle.c: prefetching all...");
#endif
	mmu_handle_reset();
	for(i=0;i<mmu_handle_nbufs;i++)
		if (!mmu_handle_virtualbuf[i])
		{
			mmu_handle_prefetching=1;
			unsigned j,s=(mmu_handle_memsize[i]/32), p=(unsigned)mmu_handle_membuf[i];
			for(j=0;j<32;j++,p+=s)
			{
				if (func)
					(*func)(j,32);
				(mmu_handle_func[i])((void *)p, j*s, s);
			}
			if (func)
				(*func)(32,32);
			if ((s*32)<mmu_handle_memsize[i])
				(mmu_handle_func[i])((void *)p, j*s, (mmu_handle_memsize[i]-(s*32)));
			mmu_handle_prefetching=0;
		}
		else
		{
			unsigned j;
			mmu_handle_prefetching=1;
			mmu_handle_func_t back_prefetch_func=NULL;
			if (mmu_handle_prefetch_func[i])
			{
				back_prefetch_func=mmu_handle_func[i];
				mmu_handle_func[i]=mmu_handle_prefetch_func[i];
			}
			for(j=0;j<mmu_handle_memsize[i];j+=MMU_HANDLE_SLOT_SIZE)
			{
				if (func)
				{
					static unsigned loading_func_time=0;
					if (!(loading_func_time&7))
						(*func)(j,mmu_handle_memsize[i]);
					loading_func_time++;
				}
				if (back_prefetch_func)
					(back_prefetch_func)(mmu_handle_prefetch_mem[i],j,MMU_HANDLE_SLOT_SIZE);
				d+=*((unsigned char *)((unsigned)mmu_handle_virtualbuf[i])+j);
			}
			if (back_prefetch_func)
				mmu_handle_func[i]=back_prefetch_func;
			if (func)
				(*func)(64,64);
			mmu_handle_prefetching=0;
		}
	___mmu_handle_prefetched___=d;
#ifdef MMU_HANDLE_DEBUG
	puts("mmu_handle.c: prefetched all");
#endif
}

void mmu_handle_prefetch(unsigned id, unsigned pos, unsigned size)
{
	unsigned int i,d=0;
#ifdef MMU_HANDLE_DEBUG
	printf("mmu_handle.c: prefetching %p ...\n",id);
#endif
	for(i=0;i<mmu_handle_nbufs;i++)
		if (((unsigned)mmu_handle_virtualbuf[i])==id)
			break;
	if (i<mmu_handle_nbufs)
	{
		if (pos>=mmu_handle_virtualmemsize[i])
			return;
		if (size+pos>mmu_handle_virtualmemsize[i])
			size=mmu_handle_virtualmemsize[i]-pos;
		if (!mmu_handle_virtualbuf[i])
			(mmu_handle_func[i])(mmu_handle_membuf[i], pos, size);
		else
		{
			unsigned j;
			mmu_handle_prefetching=1;
			mmu_handle_func_t back_prefetch_func=NULL;
			if ((mmu_handle_prefetch_func[i])&&(mmu_handle_prefetch_func_enable))
			{
				back_prefetch_func=mmu_handle_func[i];
				mmu_handle_func[i]=mmu_handle_prefetch_func[i];
			}
			if (!back_prefetch_func)
			{
				for(j=pos;j<(pos+size);j+=MMU_HANDLE_SLOT_SIZE)
					d+=*((unsigned char *)((unsigned)mmu_handle_virtualbuf[i])+j);
			}
			else
			{
				for(j=pos;j<(pos+size);j+=MMU_HANDLE_SLOT_SIZE)
				{
					(back_prefetch_func)(mmu_handle_prefetch_mem[i],j,MMU_HANDLE_SLOT_SIZE);
					d+=*((unsigned char *)((unsigned)mmu_handle_virtualbuf[i])+j);
				}
			}
			if (back_prefetch_func)
				mmu_handle_func[i]=back_prefetch_func;
			mmu_handle_prefetching=0;
		}
		___mmu_handle_prefetched___=d;
	}
#ifdef MMU_HANDLE_DEBUG
	puts("mmu_handle.c: prefetched");
#endif
}


void mmu_handle_prefetch_by_array(unsigned id, unsigned *the_array,mmu_handle_loading_func_t func)
{
#ifdef MMU_HANDLE_DEBUG
	printf("mmu_handle.c: prefetching by array %p ...\n",id);
#endif
	unsigned i,total=0,min=0xFFFFFFFF,max=0;
	unsigned partial, size=mmu_handle_memaccess_size>>1,mul=2;
	unsigned min_array, max_array;
	for(i=0;i<size;i++)
	{
		if (the_array[i]==0xFFFFFFFF)
			break;
		if ((the_array[i]<min)&&(the_array[i]))
			min=the_array[i];
		if ((the_array[i]>max)&&(the_array[i]<200000))
			max=the_array[i];
		if ((the_array[i]>=min)&&(the_array[i]<=max))
			total++;
	}
	if (i>10)
		size=i-1;
	else
		return;
	mmu_handle_reset();
	mmu_handle_prefetching=1;
	mmu_handle_prefetch_func_enable=1;
	if (total<mmu_handle_npags)
	{
		mul=3;
		partial=mmu_handle_npags-total;
		for(i=0;(i<size)&&(partial);i++)
		{
			if (func)
			{
				static unsigned loading_func_time=0;
				if (!(loading_func_time&127))
					(*func)(i,size*3);
				loading_func_time++;
			}
			if (the_array[i]<min)
			{
				mmu_handle_prefetch(id,i<<PAGESIZE_BITS,PAGESIZE);
				partial--;
			}
		}
	}
	unsigned ac=0, init_ac=0;
	for(i=0;i<size;i++)
	{
		if ((the_array[i]>=min)&&(the_array[i]<=max))
		{
			if (!ac)
				init_ac=i;
			ac++;
		}
		else
		{
			if (ac)
			{
				if (func)
				{
					static unsigned loading_func_time=0;
					if (!(loading_func_time&31))
						(*func)((size*(mul-2))+i,size*mul);
					loading_func_time++;
				}
				mmu_handle_prefetch(id,init_ac<<PAGESIZE_BITS,PAGESIZE*ac);
				ac=0;
			}
		}
	}
	if (ac)
		mmu_handle_prefetch(id,init_ac<<PAGESIZE_BITS,PAGESIZE*ac);
	mmu_handle_prefetch_func_enable=0;
	partial=total;
	min_array=0;
	max_array=size-1;
	for(i=min;(i<=max)&&(partial);i++)
	{
		unsigned j;
		for(j=min_array;j<=max_array;j++)
			if (the_array[j]==i)
			{
				partial--;
				if (func)
				{
					static unsigned loading_func_time=0;
					if (!(loading_func_time&127))
						(*func)((total*mul)-partial,total*mul);
					loading_func_time++;
				}
				mmu_handle_prefetch(id,j<<PAGESIZE_BITS,PAGESIZE);
				if (j<=min_array)
					min_array++;
				else if (j>=max_array)
					max_array--;
			}
	}
	mmu_handle_prefetching=0;
	mmu_handle_prefetch_func_enable=1;
	if (func)
		(*func)(64,64);

#ifdef MMU_HANDLE_DEBUG
	puts("mmu_handle.c: prefetched by array");
#endif
}


void mmu_handle_inc_frame(void)
{
	mmu_handle_frame++;
}

void mmu_handle_flush(void)
{
	register uint32 status;
	status = *mmu_handle_mmucr;
	status |= 0x04;
	*mmu_handle_mmucr = status;
}

unsigned mmu_handle_get_free_memory(void)
{
        unsigned vp=MMU_HANDLE_BEGINMEM_PAG;
	unsigned ret=mmu_handle_npags;
	for(;(vp<(MMU_HANDLE_BEGINMEM_PAG+mmu_handle_nvirtualpags))&&(ret);vp++)
	{
		mmusubcontext_t *sc=mmu_handle_ctx->sub[vp>>9];
		if (!sc) break;
		mmupage_t *p=&sc->page[vp&0x1ff];
		if (!p) continue;
		if ((p->valid!=0)&&(p->prkey==MMU_ALL_RDWR))
			ret--;
	}
	return ret<<PAGESIZE_BITS;
}

void mmu_handle_reset(void)
{
        unsigned vp=MMU_HANDLE_BEGINMEM_PAG;
	unsigned n;
	for(n=0;n<mmu_handle_nvirtualpags;n++,vp++)
	{
		mmusubcontext_t *sc=mmu_handle_ctx->sub[vp>>9];
		if (!sc) break;
		mmupage_t *p=&sc->page[vp&0x1ff];
		if (!p) continue;
		p->valid=0;
	}
	if (mmu_handle_memaccess)
		bzero(mmu_handle_memaccess,mmu_handle_memaccess_size);
	mmu_handle_frame=1;
	mmu_handle_flush();
}

void mmu_handle_dump_memaccess(void)
{
	unsigned n,size=(mmu_handle_memaccess_size/sizeof(unsigned));
	unsigned next=0,saved=0,used=0;
	for(n=0;n<size;n++)
		if (mmu_handle_memaccess[n])
		{
			used++;
			if (!saved) saved=1;
			else next++;
		}
		else saved=0;
	next+=saved;
	fflush(stdout);
	puts("\n\n_MMU_HANDLE_DUMP_MEMACESS_ -------");
	fflush(stdout);
	fwrite((void *)mmu_handle_memaccess,1,mmu_handle_memaccess_size,stdout);
	fflush(stdout);
	puts("\n------- _MMU_HANDLE_DUMP_MEMACESS_\n");
	printf("\n\npages=%i, used=%i, closer=%i\n\n\n",size,used,next);
	fflush(stdout);
}

void mmu_handle_disable_slice(int b)
{
	mmu_handle_prefetching=b;
}

unsigned mmu_handle_get_frame(void)
{
	return mmu_handle_frame;
}

unsigned mmu_handle_get_memsize(unsigned id)
{
	int i;
	for(i=0;i<mmu_handle_nbufs;i++)
		if (((unsigned)mmu_handle_virtualbuf[i])==id)
			return mmu_handle_memsize[i];
	return 0;
}
