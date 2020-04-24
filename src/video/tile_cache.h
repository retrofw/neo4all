#ifndef TCACHE_H
#define TCACHE_H

extern unsigned neo4all_glframes;
extern char	video_palette_use[0x200];

#define TCACHE_HASH_SIZE 701
#define TCACHE_SIZE 7680
//#define TCACHE_HASH_SIZE 521
//#define TCACHE_SIZE 4096

#define TCACHE_BREAKTIME 16

#define tcache_compEq(a,b) (a == b)
#define tcache_hash(key) (key % TCACHE_HASH_SIZE)

typedef void * tcache_rec_t;

typedef struct tcache_nodeTag {
	struct tcache_nodeTag *next;
	unsigned used;
	union{
		unsigned key;
		struct {unsigned short col, tileno;};
	};
	tcache_rec_t rec;
}tcache_node_t;

extern tcache_node_t *tcache_hash_table[TCACHE_HASH_SIZE];
extern tcache_node_t cache_tile[TCACHE_SIZE];
extern tcache_node_t *first_tile, *last_tile;


#ifndef CACHE_INLINE
void tcache_hash_init(void);
#else
CACHE_STATIC_INLINE void tcache_hash_init(void)
{
	unsigned i;
	for(i=0;i<TCACHE_HASH_SIZE;i++)
		tcache_hash_table[i]=NULL;
	for(i=0;i<TCACHE_SIZE;i++)
	{
		cache_tile[i].used=0;
		cache_tile[i].next=&cache_tile[i+1];
	}
	cache_tile[TCACHE_SIZE-1].next=NULL;
	first_tile=&cache_tile[0];
	last_tile=&cache_tile[TCACHE_SIZE-1];
}
#endif


#ifndef CACHE_INLINE
void tcache_hash_old_cleaner(int n_frame);
#else
CACHE_STATIC_INLINE void tcache_hash_old_cleaner(int n_frame)
{
#ifdef DEBUG_GL
    	puts("TILE cache Cleaner");
#endif
	register unsigned breaktime=neo4all_glframes-n_frame;
	register unsigned i;
	for(i=0;i<TCACHE_HASH_SIZE;i++)
	{
		register tcache_node_t *p=tcache_hash_table[i];
		register tcache_node_t *p0=p;
		while(p)
		{
			register tcache_node_t *next=p->next;
			if (p->used < breaktime)
			{
				p->next=NULL;
				if (first_tile)
				{
					last_tile->next=p;
					last_tile=p;
				}
				else
				{
					first_tile=p;
					last_tile=p;
				}
				if (p==tcache_hash_table[i])
					tcache_hash_table[i]=next;
				else
					p0->next=next;
			}
			else
				p0=p;
			p=next;
		}
	}
}
#endif

#ifndef CACHE_INLINE
void tcache_hash_pal_cleaner(void);
#else
CACHE_STATIC_INLINE void tcache_hash_pal_cleaner(void)
{
	register unsigned i;
	for(i=0;i<TCACHE_HASH_SIZE;i++)
	{
		register tcache_node_t *p=tcache_hash_table[i];
		register tcache_node_t *p0=p;
		while(p)
		{
			register tcache_node_t *next=p->next;
			if (video_palette_use[p->col])
			{
				p->next=NULL;
				if (first_tile)
				{
					last_tile->next=p;
					last_tile=p;
				}
				else
				{
					first_tile=p;
					last_tile=p;
				}
				if (p==tcache_hash_table[i])
					tcache_hash_table[i]=next;
				else
					p0->next=next;
			}
			else
				p0=p;
			p=next;
		}
	}
}
#endif


#ifndef CACHE_INLINE
tcache_rec_t tcache_hash_insert(unsigned key);
#else
CACHE_STATIC_INLINE tcache_rec_t tcache_hash_insert(unsigned key)
{
	register unsigned bucket=tcache_hash(key);
	
	if (!first_tile)
	{
		tcache_hash_old_cleaner(TCACHE_BREAKTIME);
		if (!first_tile)
		{
			tcache_hash_old_cleaner(1);
			if (!first_tile)
			{
				last_tile=NULL;
				return NULL;
			}
		}
	}
	{
	register tcache_node_t *p, *p0;
	p=first_tile;
	first_tile=p->next;
	p0=tcache_hash_table[bucket];
	tcache_hash_table[bucket] = p;
	p->next = p0;
#ifdef DEBUG_GL
	if (p->used>=(neo4all_glframes-2))
		printf("!!!!! TILE USADO EN N-%i\n",neo4all_glframes-(p->used));
#endif
	p->used = neo4all_glframes;
	p->key = key;
	return p->rec;
	}
}
#endif

#ifndef CACHE_INLINE
int tcache_hash_delete(unsigned key);
#else
CACHE_STATIC_INLINE int tcache_hash_delete(unsigned key)
{
	register tcache_node_t *p0=NULL, *p;
	register unsigned bucket=tcache_hash(key);
	p=tcache_hash_table[bucket];
	while (p && !tcache_compEq(p->key,key))
	{
		p0 = p;
		p = p->next;
	}
	if (!p)
		return 0;
	if (p0)
		p0->next = p->next;
	else
		tcache_hash_table[bucket] = p->next;
	p->next=NULL;
	if (first_tile)
	{
		last_tile->next=p;
		last_tile=p;
	}
	else
	{
		first_tile=p;
		last_tile=p;
	}
	return 1;
}
#endif

#ifndef CACHE_INLINE
tcache_rec_t tcache_hash_find(unsigned key);
#else
CACHE_STATIC_INLINE tcache_rec_t tcache_hash_find(unsigned key)
{
	register tcache_node_t *p;

	p = tcache_hash_table[tcache_hash(key)];
	while (p && !tcache_compEq(p->key, key))
		p = p->next;
	if (!p)
		return NULL;
	p->used=neo4all_glframes;
	return p->rec;
}
#endif

#ifndef CACHE_INLINE
unsigned tcache_hash_unused(void);
#else
CACHE_STATIC_INLINE unsigned tcache_hash_unused(void)
{
	register unsigned ret=0;
	register tcache_node_t *p=first_tile;
	while(p)
	{
		ret++;
		p=p->next;
	}
	return ret;
}
#endif

#endif
