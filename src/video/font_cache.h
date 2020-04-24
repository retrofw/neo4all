#ifndef FCACHE_H
#define FCACHE_H

extern unsigned neo4all_glframes;
extern char	video_palette_use[0x200];

#define FCACHE_HASH_SIZE 127
#define FCACHE_SIZE 2048
//#define FCACHE_SIZE 1024

#define FCACHE_BREAKTIME 16

#define fcache_compEq(a,b) (a == b)
#define fcache_hash(key) (key % FCACHE_HASH_SIZE)

typedef void * fcache_rec_t;

typedef struct fcache_nodeTag {
	struct fcache_nodeTag *next;
	unsigned used;
	union{
		unsigned key;
		struct {unsigned short col, fontno;};
	};
	fcache_rec_t rec;
}fcache_node_t;

extern fcache_node_t *fcache_hash_table[FCACHE_HASH_SIZE];
extern fcache_node_t cache_font[FCACHE_SIZE];
extern fcache_node_t *first_font, *last_font;

#ifndef CACHE_INLINE
void fcache_hash_init(void);
#else
CACHE_STATIC_INLINE void fcache_hash_init(void)
{
	register unsigned i;
	for(i=0;i<FCACHE_HASH_SIZE;i++)
		fcache_hash_table[i]=NULL;
	for(i=0;i<FCACHE_SIZE;i++)
	{
		cache_font[i].used=0;
		cache_font[i].next=&cache_font[i+1];
	}
	cache_font[FCACHE_SIZE-1].next=NULL;
	first_font=&cache_font[0];
	last_font=&cache_font[FCACHE_SIZE-1];
}
#endif


#ifndef CACHE_INLINE
void fcache_hash_old_cleaner(int n_frame);
#else
CACHE_STATIC_INLINE void fcache_hash_old_cleaner(int n_frame)
{
#ifdef DEBUG_GL
    	puts("FONT cache Cleaner");
#endif
	register unsigned breaktime=neo4all_glframes-n_frame;
	register unsigned i;
	for(i=0;i<FCACHE_HASH_SIZE;i++)
	{
		register fcache_node_t *p=fcache_hash_table[i];
		register fcache_node_t *p0=p;
		while(p)
		{
			register fcache_node_t *next=p->next;
			if (p->used < breaktime)
			{
				p->next=NULL;
				if (first_font)
				{
					last_font->next=p;
					last_font=p;
				}
				else
				{
					first_font=p;
					last_font=p;
				}
				if (p==fcache_hash_table[i])
					fcache_hash_table[i]=next;
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
void fcache_hash_pal_cleaner(void);
#else
CACHE_STATIC_INLINE void fcache_hash_pal_cleaner(void)
{
	register unsigned i;
	for(i=0;i<FCACHE_HASH_SIZE;i++)
	{
		register fcache_node_t *p=fcache_hash_table[i];
		register fcache_node_t *p0=p;
		while(p)
		{
			register fcache_node_t *next=p->next;
			if (video_palette_use[p->col])
			{
				p->next=NULL;
				if (first_font)
				{
					last_font->next=p;
					last_font=p;
				}
				else
				{
					first_font=p;
					last_font=p;
				}
				if (p==fcache_hash_table[i])
					fcache_hash_table[i]=next;
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
fcache_rec_t fcache_hash_insert(unsigned key);
#else
CACHE_STATIC_INLINE fcache_rec_t fcache_hash_insert(unsigned key)
{
	register unsigned bucket=fcache_hash(key);
	
	if (!first_font)
	{
		fcache_hash_old_cleaner(TCACHE_BREAKTIME);
		if (!first_font)
		{
			fcache_hash_old_cleaner(1);
			if (!first_font)
			{
				last_font=NULL;
				return NULL;
			}
		}
	}
	{
	register fcache_node_t *p, *p0;
	p=first_font;
	first_font=p->next;
	p0=fcache_hash_table[bucket];
	fcache_hash_table[bucket] = p;
	p->next = p0;
#ifdef DEBUG_GL
	if (p->used>=(neo4all_glframes-2))
		printf("!!!!! FONT USADA EN N-%i\n",neo4all_glframes-(p->used));
#endif
	p->used = neo4all_glframes;
	p->key = key;
	return p->rec;
	}
}
#endif

#ifndef CACHE_INLINE
int fcache_hash_delete(unsigned key);
#else
CACHE_STATIC_INLINE int fcache_hash_delete(unsigned key)
{
	register fcache_node_t *p0=NULL, *p;
	register unsigned bucket=fcache_hash(key);
	p=fcache_hash_table[bucket];
	while (p && !fcache_compEq(p->key,key))
	{
		p0 = p;
		p = p->next;
	}
	if (!p)
		return 0;
	if (p0)
		p0->next = p->next;
	else
		fcache_hash_table[bucket] = p->next;
	p->next=NULL;
	if (first_font)
	{
		last_font->next=p;
		last_font=p;
	}
	else
	{
		first_font=p;
		last_font=p;
	}
	return 1;
}
#endif

#ifndef CACHE_INLINE
fcache_rec_t fcache_hash_find(unsigned key);
#else
CACHE_STATIC_INLINE fcache_rec_t fcache_hash_find(unsigned key)
{
	register fcache_node_t *p;

	p = fcache_hash_table[fcache_hash(key)];
	while (p && !fcache_compEq(p->key, key))
		p = p->next;
	if (!p)
		return NULL;
	p->used=neo4all_glframes;
	return p->rec;
}
#endif

#ifndef CACHE_INLINE
unsigned fcache_hash_unused(void);
#else
CACHE_STATIC_INLINE unsigned fcache_hash_unused(void)
{
	register unsigned ret=0;
	register fcache_node_t *p=first_font;
	while(p)
	{
		ret++;
		p=p->next;
	}
	return ret;
}
#endif


#endif
