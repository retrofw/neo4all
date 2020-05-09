#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "menu.h"

#ifdef SHOW_MENU

#include <sys/stat.h>
#include <unistd.h>
#include<dirent.h>

#include "sound/sound.h"

#define MAX_FILELEN 13
#define MAX_DESC 29

#define MAX_DNAME (MAX_FILELEN+MAX_DESC+3)

typedef struct{
	char d_name[MAX_DNAME];
	char d_type;
}fichero;


extern char neo4all_image_file[];

char *text_str_load_separator="~~~~~~~~~~~~~~~~~~~~~~~~";
char *text_str_load_dir="#DIR#";
char *text_str_load_title="Choose a game";
fichero *text_dir_files=NULL;
int text_dir_num_files=0, text_dir_num_files_index=0;

char *text_load=NULL;

#define MAX_FILES_PER_DIR 128
#define SHOW_MAX_FILES 12


char aes4all_actual_dir[128];


static int min_in_dir=0, max_in_dir=SHOW_MAX_FILES;

static int compare_names(fichero *a, fichero *b)
{
/*
	char *ca, *cb;
	if (a->d_name[MAX_FILELEN+2])
		ca=(char *)&a->d_name[MAX_FILELEN+2];
	else
		ca=(char *)&a->d_name[0];
	if (b->d_name[MAX_FILELEN+2])
		cb=(char *)&b->d_name[MAX_FILELEN+2];
	else
		cb=(char *)&b->d_name[0];
*/
char *ca=(char *)a->d_name;
char *cb=(char *)b->d_name;
	return strcmp(ca,cb);
}

static void copyCompleteName(char *dest, int n)
{
	char *src=text_dir_files[n].d_name;
	if (strlen(src)<MAX_FILELEN)
		strcpy(dest,src);
	else
	{
		DIR *d=opendir(".");
		if (d)
		{
			int i,indice=0,buscado=src[MAX_FILELEN+1];
			for(i=0;i<MAX_FILES_PER_DIR;i++)
			{
				struct dirent *actual=readdir(d);
				if (actual==NULL)
				{
					dest[0]=0;
					break;
				}
				if (!strncmp(src,actual->d_name,MAX_FILELEN))
				{
					if (indice==buscado)
					{
						strcpy(dest,actual->d_name);
						break;
					}
					indice++;
				}
			}
			closedir(d);
		}
		else
			dest[0]=0;
	}

}

static int checkFiles(void)
{
	char *buf=(char *)calloc(1,2046);
	int i,max=text_dir_num_files;
	int ret=(text_dir_num_files<1);
	if (max>16)
		max=16;
	for(i=0;(i<max)&&(!ret);i++)
	{
		copyCompleteName(buf,i);
		if (!buf[0])
			ret=1;
		else
		if (buf[0]=='.')
			ret=0;
		else
		if (!text_dir_files[i].d_type)
		{
			if (strncmp(buf,"Desktop DB",10) && strncmp(buf,"Desktop DF",10))
			{
				FILE *f=fopen(buf,"rb");
				if (!f)
					ret=1;
				else
					fclose(f);
			}
		}
		else
		{
			DIR *d=opendir(buf);
			if (!d)
				ret=1;
			else
				closedir(d);
		}
	}
	free(buf);
	return ret;
}

static int getFiles(char *dir)
{
	int i,j;
	DIR *d;
	text_dir_num_files_index=0;
	text_dir_num_files=0;
	min_in_dir=0;
	max_in_dir=SHOW_MAX_FILES;

	if (text_dir_files!=NULL)
		free(text_dir_files);

	text_draw_window(96,64,140,32,"-------");
	write_text(14,9,"Please wait");
	text_flip();

	text_dir_files=(fichero *)calloc(sizeof(fichero),MAX_FILES_PER_DIR);
	d=opendir(dir);
	if (d==NULL)
		return -1;
	for(i=0;i<MAX_FILES_PER_DIR;i++)
	{
		struct dirent *actual=readdir(d);
		if (actual==NULL)
			break;
		char  *name=actual->d_name;
		int len=strlen(name);
		if (len<2)
		{ i--; continue; }
		char *ext=(char *)&name[len-4];
		if ((!strcmp(name,"neo4all"))|| (!strcmp(ext,".prf")) || 
		    (!strcmp(name,"ip.bin"))||(!strcmp(name,"1st_read.bin")))
		{ i--; continue; }
		bzero(text_dir_files[i].d_name,MAX_DNAME);
		strncpy(text_dir_files[i].d_name,name,MAX_FILELEN);
		if (strlen(text_dir_files[i].d_name)==MAX_FILELEN)
		{
			int jjg,indice=0;
			for(jjg=0;jjg<i;jjg++)
				if (!(strcmp(text_dir_files[i].d_name,text_dir_files[jjg].d_name)))
						indice++;
			text_dir_files[jjg].d_name[MAX_FILELEN+1]=indice;
		}
		{
			struct stat sstat;
			char *tmp=(char *)calloc(1,strlen(name)+strlen(dir)+16);
			strcpy(tmp,dir);
			strcat(tmp,"/");
			strcat(tmp,name); //text_dir_files[i].d_name);
			if (!stat(tmp, &sstat))
		        	if (S_ISDIR(sstat.st_mode))
					text_dir_files[i].d_type=4;
			free(tmp);
		}
#ifdef AES
		if (!text_dir_files[i].d_type)
			if (!is_aes_file(dir,name,(char *)&text_dir_files[i].d_name[MAX_FILELEN+2]))
			{ i--; continue; }
#endif
		text_dir_files[i].d_name[MAX_FILELEN+1]=0;
		text_dir_files[i].d_name[MAX_DNAME-1]=0;
	}
	closedir(d);
	text_dir_num_files=i;

	if (!text_dir_num_files)
		return -1;

        chdir(dir);

	for(i=0;i<text_dir_num_files;i++)
	{
		if (text_dir_files[i].d_type==0)
			for(j=i;j<text_dir_num_files;j++)
				if (text_dir_files[j].d_type==4)
				{
					char *ctmp=(char *)calloc(1,MAX_DNAME+4);
					memcpy(ctmp,text_dir_files[j].d_name,MAX_DNAME);
					memcpy(text_dir_files[j].d_name,text_dir_files[i].d_name,MAX_DNAME);
					memcpy(text_dir_files[i].d_name,ctmp,MAX_DNAME);
					text_dir_files[i].d_type=4;
					text_dir_files[j].d_type=0;
					free(ctmp);
					break;
				}
	}

	for(i=0;i<text_dir_num_files;i++)
		if (text_dir_files[i].d_type==0)
		{
			qsort((void *)&text_dir_files[i],text_dir_num_files-i,sizeof(fichero),(int (*)(const void*, const void*))compare_names);
			break;
		}

	return 0;
}

static void draw_loadMenu(int c)
{
	int i,j;
//	static int b=0;
//	int bb=(b%6)/3;
	SDL_Rect r;
	extern SDL_Surface *text_screen;
//	r.x=80-64; r.y=0; r.w=110+64+64; r.h=240;
	r.x=80-64; r.y=18; r.w=110+64+64; r.h=208;

	text_draw_background();
//	text_draw_window(80-64,12,160+64+64,220,text_str_load_title);
	text_draw_window(80-64,18,160+64+64-4,208-2,text_str_load_title);

	if (text_dir_num_files_index<min_in_dir)
	{
		min_in_dir=text_dir_num_files_index;
		max_in_dir=text_dir_num_files_index+SHOW_MAX_FILES;
	}
	else
		if (text_dir_num_files_index>=max_in_dir)
		{
			max_in_dir=text_dir_num_files_index+1;
			min_in_dir=max_in_dir-SHOW_MAX_FILES;
		}
	if (max_in_dir>text_dir_num_files)
		max_in_dir=text_dir_num_files-min_in_dir;


	for (i=min_in_dir,j=1;i<max_in_dir;i++,j+=2)
	{
		if (i!=min_in_dir)
			write_text(3,j+1,text_str_load_separator);

		SDL_SetClipRect(text_screen,&r);

		char *name;

		if (text_dir_files[i].d_name[MAX_FILELEN+2])
			name=(char *)&text_dir_files[i].d_name[MAX_FILELEN+2];
		else
			name=(char *)&text_dir_files[i].d_name[0];

		if ((text_dir_num_files_index==i)) //&&(bb))
			write_text_sel(3,j+2,276,name);
		else
			write_text(3,j+2,name);

		SDL_SetClipRect(text_screen,NULL);

		if (text_dir_files[i].d_type==4)
			write_text(32,j+2,text_str_load_dir);
	}
//	write_text(3,j,text_str_load_separator);
	text_flip();
//	b++;
}

static int key_loadMenu(int *c)
{
	int end=0;
	int left=0, right=0, up=0, down=0, hit0=0, hit1=0;
	SDL_Event event;

	while (SDL_PollEvent(&event) > 0)
	{
		if (event.type == SDL_QUIT)
			end=-1;
		else
		if (event.type == SDL_KEYDOWN)
		{
			sound_play_beep();
			switch(event.key.keysym.sym)
			{
				case SDLK_RIGHT: right=1; break;
				case SDLK_LEFT: left=1; break;
				case SDLK_UP: up=1; break;
				case SDLK_DOWN: down=1; break;
				case SDLK_RETURN:
				case SDLK_LCTRL: hit0=1; break;
				case SDLK_LALT: hit1=1; break;
				case SDLK_TAB:
						if (text_dir_num_files)
							text_dir_num_files_index=text_dir_num_files-1;
						break;
				case SDLK_BACKSPACE:
						text_dir_num_files_index=0;
						break;

			}
			if (hit0)
			{
				if ((text_dir_files[text_dir_num_files_index].d_type==4)||(!strcmp((char *)&text_dir_files[text_dir_num_files_index].d_name,"."))||(!strcmp((char *)&text_dir_files[text_dir_num_files_index].d_name,"..")))
				{
					char *tmp=(char *)calloc(1,MAX_DNAME+4);
					memcpy(tmp,text_dir_files[text_dir_num_files_index].d_name,MAX_DNAME);
					if (getFiles(tmp))
						end=-1;
					free(tmp);
				}
				else
				{
					char *tmp=(char *)calloc(1,256);
					copyCompleteName(tmp,text_dir_num_files_index);
					if (!strncmp(tmp,neo4all_image_file,256))
						end=2;
					else
					{
						strncpy(neo4all_image_file,tmp,256);
						end=1;
					}
					free(tmp);
				}
			}
			else if (hit1)
				end=-1;
			else if ((up)&&(text_dir_num_files_index>0))
				text_dir_num_files_index--;
			else if ((down)&&(text_dir_num_files_index+1!=text_dir_num_files))
				text_dir_num_files_index++;
			else if (left)
			{
				text_dir_num_files_index-=SHOW_MAX_FILES;
				if (text_dir_num_files_index<0)
					text_dir_num_files_index=0;
			}
			else if (right)
			{
				text_dir_num_files_index+=SHOW_MAX_FILES;
				if (text_dir_num_files_index+1>=text_dir_num_files)
					text_dir_num_files_index=text_dir_num_files-1;
			}
		}
	}

	return end;
}

static void raise_loadMenu()
{
	int i;

	text_draw_background();
	text_flip();
	for(i=0;i<10;i++)
	{
		text_draw_background();
		text_draw_window(80-64,(10-i)*24,160+64+64,220,text_str_load_title);
		text_flip();
	}
}

static void unraise_loadMenu()
{
	int i;

	for(i=9;i>=0;i--)
	{
		text_draw_background();
		text_draw_window(80-64,(10-i)*24,160+64+64,220,text_str_load_title);
		text_flip();
	}
	text_draw_background();
	text_flip();
}


int getDefaultFiles(void)
{
	return(getFiles(MENU_DIR_DEFAULT));
}

int run_menuLoad()
{
	int end=0,c=0;

	if (text_dir_files==NULL)
		end=getDefaultFiles();
	else
		if (checkFiles())
			end=getDefaultFiles();

//	text_dir_num_files_index=0;

	raise_loadMenu();
	while(!end)
	{
		draw_loadMenu(c);
		end=key_loadMenu(&c);
	}
	unraise_loadMenu();

	getcwd(aes4all_actual_dir,128);

	return end;
}


#endif
