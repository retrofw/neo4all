#if defined(CDISO) && !defined(AES)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

char neo4all_image_file[1024] = { 0, 0, 0, 0};
static int cdr_modes[]={0, 8, 12, 24};
static int cdr_frames[]={2048, 2352, 2056, 2332, 2336, 2448};
static int cdr_mode=0;
static int cdr_frame=2048;
#define SWAP16(v) ((((v)&0xff00)>>8) +(((v)&0xff)<<8))
#define SWAP32(v) ((((v)&0xff000000ul)>>24) + (((v)&0xff0000ul)>>8) + (((v)&0xff00ul)<<8) +(((v)&0xfful)<<24))
#define btoi(b)     ((b)/16*10 + (b)%16)
#define itob(i)     ((i)/10*16 + (i)%10)
#define MSF2SECT(m, s, f)               (((m) * 60 + (s) - 2) * 75 + (f))
#define CD_FRAMESIZE_RAW                cdr_frame
//#define CD_FRAMESIZE_RAW                2048
//#define CD_FRAMESIZE_PAD		16
//#define CD_FRAMESIZE_PAD		12
//#define CD_FRAMESIZE_PAD		0
#define CD_FRAMESIZE_PAD	cdr_mode
#define DATA_SIZE                               (CD_FRAMESIZE_RAW - CD_FRAMESIZE_PAD)
#define SUB_FRAMESIZE                   96
#define MAXPATHLEN 256
//#define MAXDIRLEN 65536
//#define MAXDIRLEN (128*1024)

//#define MAXDIRLEN (168*1024)
//#define MAXDIRLEN (176*1024)
#define MAXDIRLEN (192*1024)
//#define MAXDIRLEN (256*1024)

#define incTime() \
	time[0] = btoi(time[0]); time[1] = btoi(time[1]); time[2] = btoi(time[2]); \
	time[2]++; \
	if(time[2] == 75) { \
		time[2] = 0; \
		time[1]++; \
		if (time[1] == 60) { \
			time[1] = 0; \
			time[0]++; \
		} \
	} \
	time[0] = itob(time[0]); time[1] = itob(time[1]); time[2] = itob(time[2]);

#define READTRACK() \
	if (CDR_readTrack(time) == -1) return -1; \
	buf = CDR_getBuffer(); \
	if (buf == NULL) return -1;

#define READTRACK_n() \
	if (CDR_readTrack(time) == -1) return 0; \
	buf = CDR_getBuffer(); \
	if (buf == NULL) return 0;

#define READTRACK_p() \
	if (CDR_readTrack(time) == -1) return NULL; \
	buf = CDR_getBuffer(); \
	if (buf == NULL) return NULL;


#define READDIR(_dir) \
{ \
	unsigned _dl; \
	for(_dl=0;_dl<dirlen;_dl+=2048) { \
		READTRACK(); \
		memcpy(_dir+_dl, buf + CD_FRAMESIZE_PAD, 2048); \
		incTime(); \
	} \
}

#define READDIR_n(_dir) \
{ \
	unsigned _dl; \
	for(_dl=0;_dl<dirlen;_dl+=2048) { \
		READTRACK_n(); \
		memcpy(_dir+_dl, buf + CD_FRAMESIZE_PAD, 2048); \
 \
		incTime(); \
	} \
}

#define READDIR_p(_dir) \
{ \
	unsigned _dl; \
	for(_dl=0;_dl<dirlen;_dl+=2048) { \
		READTRACK_p(); \
		memcpy(_dir+_dl, buf + CD_FRAMESIZE_PAD, 2048); \
 \
		incTime(); \
	} \
}

#define ISODCL(from, to) (to - from + 1)

struct iso_directory_record {
	char length			[ISODCL (1, 1)]; /* 711 */
	char ext_attr_length		[ISODCL (2, 2)]; /* 711 */
	char extent			[ISODCL (3, 10)]; /* 733 */
	char size			[ISODCL (11, 18)]; /* 733 */
	char date			[ISODCL (19, 25)]; /* 7 by 711 */
	char flags			[ISODCL (26, 26)];
	char file_unit_size		[ISODCL (27, 27)]; /* 711 */
	char interleave			[ISODCL (28, 28)]; /* 711 */
	char volume_sequence_number	[ISODCL (29, 32)]; /* 723 */
	unsigned char name_len		[ISODCL (33, 33)]; /* 711 */
	char name			[1];
};


static FILE *cdHandle = NULL;
static FILE *cddaHandle = NULL;
static FILE *subHandle = NULL;

static char subChanInterleaved = 0;

static unsigned char cdbuffer[3*1024]; //DATA_SIZE];
static unsigned char subbuffer[SUB_FRAMESIZE];

static unsigned char sndbuffer[24480]; //CD_FRAMESIZE_RAW * 10];

#define CDDA_FRAMETIME			(1000 * (sizeof(sndbuffer) / CD_FRAMESIZE_RAW) / 75)

static unsigned int initial_offset = 0;
static volatile char playing = 0;
static char cddaBigEndian = 0;
static volatile unsigned int cddaCurOffset = 0;

struct CdrStat {
	unsigned Type;
	unsigned Status;
	unsigned char Time[3];
};

typedef enum {
	DATA,
	CDDA
} cd_type;

struct trackinfo {
	cd_type type;
	char start[3];		// MSF-format
	char length[3];		// MSF-format
#ifdef USE_MP3_CDDA
	char fname[MAXPATHLEN]; // filename
#endif
};

typedef struct {
	int	y0, y1;
} ADPCM_Decode_t;

typedef struct {
	int				freq;
	int				nbits;
	int				stereo;
	int				nsamples;
	ADPCM_Decode_t	left, right;
	short			pcm[16384];
} xa_decode_t;


typedef struct {
	unsigned char OCUP;
	unsigned char Reg1Mode;
	unsigned char Reg2;
	unsigned char CmdProcess;
	unsigned char Ctrl;
	unsigned char Stat;

	unsigned char StatP;

	unsigned char Transfer[2448]; //CD_FRAMESIZE_RAW];
	unsigned char *pTransfer;

	unsigned char Prev[4];
	unsigned char Param[8];
	unsigned char Result[8];

	unsigned char ParamC;
	unsigned char ParamP;
	unsigned char ResultC;
	unsigned char ResultP;
	unsigned char ResultReady;
	unsigned char Cmd;
	unsigned char Readed;
	unsigned Reading;

	unsigned char ResultTN[6];
	unsigned char ResultTD[4];
	unsigned char SetSector[4];
	unsigned char SetSectorSeek[4];
	unsigned char Track;
	unsigned char Play, Muted;
	int CurTrack;
	int Mode, File, Channel;
	int Reset;
	int RErr;
	int FirstSector;

	xa_decode_t Xa;

	int Init;

	unsigned char Irq;
	unsigned eCycle;

	unsigned char Seeked;
} cdrStruct;

typedef struct {
	unsigned char time[4];
	unsigned size;
	unsigned pos;
} cdr_file;

static cdrStruct cdr;
static cdr_file cdr_files[64];


#define MAXTRACKS 100 /* How many tracks can a CD hold? */

static int numtracks = 0;
static struct trackinfo ti[MAXTRACKS];
#ifdef USE_MP3_CDDA
static int numaudiotracks = 0;
static int firstaudiotrack = -1;
#endif

static char IsoFile[MAXPATHLEN] = "";
static long long cdOpenCaseTime = 0;

void CDR_SetIsoFile(const char *filename) {
	if (filename == NULL) {
		IsoFile[0] = '\0';
		return;
	}
	strcpy(IsoFile, filename);
}

const char *CDR_GetIsoFile(void) {
	return IsoFile;
}

unsigned char CDR_UsingIso(void) {
	return (IsoFile[0] != '\0');
}

void CDR_SetCdOpenCaseTime(long long time) {
	cdOpenCaseTime = time;
}

long long CDR_GetCdOpenCaseTime(void)
{
	return cdOpenCaseTime;
}

// get a sector from a msf-array
static unsigned int msf2sec(char *msf) {
	return ((msf[0] * 60 + msf[1]) * 75) + msf[2];
}

static void sec2msf(unsigned int s, char *msf) {
	msf[0] = s / 75 / 60;
	s = s - msf[0] * 75 * 60;
	msf[1] = s / 75;
	s = s - msf[1] * 75;
	msf[2] = s;
}

// divide a string of xx:yy:zz into m, s, f
static void tok2msf(char *time, char *msf) {
	char *token;

	token = strtok(time, ":");
	if (token) {
		msf[0] = atoi(token);
	}
	else {
		msf[0] = 0;
	}

	token = strtok(NULL, ":");
	if (token) {
		msf[1] = atoi(token);
	}
	else {
		msf[1] = 0;
	}

	token = strtok(NULL, ":");
	if (token) {
		msf[2] = atoi(token);
	}
	else {
		msf[2] = 0;
	}
}

// stop the CDDA playback
static void stopCDDA(void) {
	if (!playing) {
		return;
	}

	playing = 0;

	if (cddaHandle != NULL) {
		fclose(cddaHandle);
		cddaHandle = NULL;
	}

	initial_offset = 0;
}

static unsigned long long CONV64w(void *punt) {
	unsigned short *p=(unsigned short *)punt;
	unsigned long long ret=0;
	ret=(((unsigned long long)p[3])<<48);
	ret|=(((unsigned long long)p[2])<<32);
	ret|=(((unsigned long long)p[1])<<16);
	ret|=(((unsigned long long)p[0]));

	return ret;
}


// start the CDDA playback
static void startCDDA(unsigned int offset) {
	if (playing) {
		if (initial_offset == offset) {
			return;
		}
		stopCDDA();
	}

	if (!offset) return;

	cddaHandle = fopen(IsoFile, "rb");
	if (cddaHandle == NULL) {
		return;
	}

	initial_offset = offset;
	cddaCurOffset = initial_offset;
	fseek(cddaHandle, initial_offset, SEEK_SET);

	playing = 1;

}

void CDR_playCDDA(void)
{
	long			d, i, s;
	unsigned char	tmp;

	if (playing) {

		if (subChanInterleaved) {
			s = 0;

			for (i = 0; i < sizeof(sndbuffer) / CD_FRAMESIZE_RAW; i++) {
				// read one sector
				d = fread(sndbuffer + CD_FRAMESIZE_RAW * i, 1, CD_FRAMESIZE_RAW, cddaHandle);
				if (d < CD_FRAMESIZE_RAW) {
					break;
				}

				s += d;

				// skip the subchannel data
				fseek(cddaHandle, SUB_FRAMESIZE, SEEK_CUR);
			}
		}
		else {
			s = fread(sndbuffer, 1, sizeof(sndbuffer), cddaHandle);
		}

		if (s == 0) {
			playing = 0;
			fclose(cddaHandle);
			cddaHandle = NULL;
			initial_offset = 0;
			return;
		}

		if (!cdr.Muted && playing) {
			if (cddaBigEndian) {
				for (i = 0; i < s / 2; i++) {
					tmp = sndbuffer[i * 2];
					sndbuffer[i * 2] = sndbuffer[i * 2 + 1];
					sndbuffer[i * 2 + 1] = tmp;
				}
			}

#if 0
			SPU_playCDDAchannel((unsigned char *)sndbuffer, s);
#endif
		}

		cddaCurOffset += s;
	}
}

// this function tries to get the .toc file of the given .bin
// the necessary data is put into the ti (trackinformation)-array
static int parsetoc(const char *isofile) {
	char			tocname[MAXPATHLEN];
	FILE			*fi;
	char			linebuf[256], dummy[256], name[256];
	char			*token;
	char			time[20], time2[20];
	unsigned int	t;

	numtracks = 0;

	// copy name of the iso and change extension from .bin to .toc
	strncpy(tocname, isofile, sizeof(tocname));
	tocname[MAXPATHLEN - 1] = '\0';
	if (strlen(tocname) >= 4) {
		strcpy(tocname + strlen(tocname) - 4, ".toc");
	}
	else {
		return -1;
	}

	if ((fi = fopen(tocname, "r")) == NULL) {
		// check for image.bin.toc (for AcetoneISO)
		sprintf(tocname, "%s.toc", isofile);
		if ((fi = fopen(tocname, "r")) == NULL) {
			// if filename is image.toc.bin, try removing .bin (for Brasero)
			strcpy(tocname, isofile);
			t = strlen(tocname);
			if (t >= 8 && strcmp(tocname + t - 8, ".toc.bin") == 0) {
				tocname[t - 4] = '\0';
				if ((fi = fopen(tocname, "r")) == NULL) {
					return -1;
				}
			}
			else {
				return -1;
			}
		}
	}

	memset(&ti, 0, sizeof(ti));

	// parse the .toc file
	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		// search for tracks
		strncpy(dummy, linebuf, sizeof(linebuf));
		token = strtok(dummy, " ");

		if (token == NULL) {
			continue;
		}

		if (!strcmp(token, "TRACK")) {
			// get type of track
			token = strtok(NULL, " ");
			numtracks++;

			if (!strcmp(token, "MODE2_RAW\n")) {
				ti[numtracks].type = DATA;
				sec2msf(2 * 75, ti[numtracks].start); // assume data track on 0:2:0
			}
			else if (!strcmp(token, "AUDIO\n")) {
				ti[numtracks].type = CDDA;
			}
		}
		else if (!strcmp(token, "DATAFILE")) {
			sscanf(linebuf, "DATAFILE \"%[^\"]\" %8s", name, time);
			tok2msf((char *)&time, (char *)&ti[numtracks].length);
		}
		else if (!strcmp(token, "FILE")) {
			sscanf(linebuf, "FILE \"%[^\"]\" #%d %8s %8s", name, &t, time, time2);
			tok2msf((char *)&time, (char *)&ti[numtracks].start);
			t /= CD_FRAMESIZE_RAW;
			t += msf2sec(ti[numtracks].start) + 2 * 75;
			sec2msf(t, (char *)&ti[numtracks].start);
			tok2msf((char *)&time2, (char *)&ti[numtracks].length);
		}
	}

	fclose(fi);

	return 0;
}

#ifdef USE_MP3_CDDA
int cdda_num_tracks()
{
	return numtracks;
}

int cdda_get_track_name(int index, char* name)
{
	if (index > 1 && index <= numtracks)
	{
		strcpy(name, ti[index].fname);
		return 1;
	}
	return 0;
}

int cdda_get_track_end(int track)
{
	return (msf2sec(ti[track].length) * 60) / 75;
}
#endif
// this function tries to get the .cue file of the given .bin
// the necessary data is put into the ti (trackinformation)-array
static int parsecue(const char *isofile) {
	char			cuename[MAXPATHLEN];
	FILE			*fi;
	char			*token;
	char			time[20];
	char			*tmp;
	char			linebuf[256], dummy[256];
	unsigned int	t;
#ifdef USE_MP3_CDDA
	bool			foundMP3  = 0;
#endif

	numtracks = 0;

	// copy name of the iso and change extension from .bin to .cue
	strncpy(cuename, isofile, sizeof(cuename));
	cuename[MAXPATHLEN - 1] = '\0';
	if (strlen(cuename) >= 4) {
		strcpy(cuename + strlen(cuename) - 4, ".cue");
	}
	else {
		return -1;
	}

	if ((fi = fopen(cuename, "r")) == NULL) {
		return -1;
	}

	memset(&ti, 0, sizeof(ti));

	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		strncpy(dummy, linebuf, sizeof(linebuf));
		token = strtok(dummy, " ");

		if (token == NULL) {
			continue;
		}

#ifdef USE_MP3_CDDA
		if (!strcmp(token, "FILE"))
		{
			if (strstr(linebuf, "MP3") != NULL)
			{
				if (numtracks >= 1)
				{
					printf("%s - Index %d, Type %s\n", ti[numtracks].fname, numtracks, ti[numtracks].type == CDDA ? "CDDA" : "DATA");
				}
				numtracks++;
				foundMP3 = 1;
				memset(ti[numtracks].fname, 0, MAXPATHLEN);
				sscanf(linebuf, " FILE \"%[^\"]\" MP3", ti[numtracks].fname);
			}
			else if (strstr(linebuf, "OGG") != NULL)
			{
				if (numtracks >= 1)
				{
					printf("%s - Index %d, Type %s\n", ti[numtracks].fname, numtracks, ti[numtracks].type == CDDA ? "CDDA" : "DATA");
				}
				numtracks++;
				foundMP3 = 1;
				memset(ti[numtracks].fname, 0, MAXPATHLEN);
				sscanf(linebuf, " FILE \"%[^\"]\" OGG", ti[numtracks].fname);
			}
			else if (strstr(linebuf, "WAV") != NULL)
			{
				if (numtracks >= 1)
				{
					printf("%s - Index %d, Type %s\n", ti[numtracks].fname, numtracks, ti[numtracks].type == CDDA ? "CDDA" : "DATA");
				}
				numtracks++;
				foundMP3 = 1;
				memset(ti[numtracks].fname, 0, MAXPATHLEN);
				sscanf(linebuf, " FILE \"%[^\"]\" WAV", ti[numtracks].fname);
			}
			else if (strstr(linebuf, "BINARY") != NULL)
			{
				numtracks++;
				foundMP3 = 1;
				sscanf(linebuf, " FILE \"%[^\"]\" BINARY", ti[numtracks].fname);
			}
			else
				foundMP3 = 0;
		}
		else if (foundMP3) {
#endif
		if (!strcmp(token, "TRACK")){
#ifndef USE_MP3_CDDA
			numtracks++;
#endif

			if (strstr(linebuf, "AUDIO") != NULL) {
				ti[numtracks].type = CDDA;
#ifdef USE_MP3_CDDA
				numaudiotracks++;
				if (firstaudiotrack < 0)
					firstaudiotrack = numtracks;
#endif
			}
			else if (strstr(linebuf, "MODE1/2352") != NULL || strstr(linebuf, "MODE2/2352") != NULL || strstr(linebuf, "MODE1/2048") != NULL) {
				ti[numtracks].type = DATA;
			}
		}
		else if (!strcmp(token, "INDEX")) {
			tmp = strstr(linebuf, "INDEX");
			if (tmp != NULL) {
				tmp += strlen("INDEX") + 3; // 3 - space + numeric index
				while (*tmp == ' ') tmp++;
				if (*tmp != '\n') sscanf(tmp, "%8s", time);
			}

			tok2msf((char *)&time, (char *)&ti[numtracks].start);

			t = msf2sec(ti[numtracks].start) + 2 * 75;
			sec2msf(t, ti[numtracks].start);

			// If we've already seen another track, this is its end
			if (numtracks > 1) {
				t = msf2sec(ti[numtracks].start) - msf2sec(ti[numtracks - 1].start);
				sec2msf(t, ti[numtracks - 1].length);
			}
		}
#ifdef USE_MP3_CDDA
		}
#endif
	}

	fclose(fi);

	// Fill out the last track's end based on size
	if (numtracks >= 1) {
		fseek(cdHandle, 0, SEEK_END);
		t = ftell(cdHandle) / 2352 - msf2sec(ti[numtracks].start) + 2 * 75;
		sec2msf(t, ti[numtracks].length);
	}

	return 0;
}

// this function tries to get the .ccd file of the given .img
// the necessary data is put into the ti (trackinformation)-array
static int parseccd(const char *isofile) {
	char			ccdname[MAXPATHLEN];
	FILE			*fi;
	char			linebuf[256];
	unsigned int	t;

	numtracks = 0;

	// copy name of the iso and change extension from .img to .ccd
	strncpy(ccdname, isofile, sizeof(ccdname));
	ccdname[MAXPATHLEN - 1] = '\0';
	if (strlen(ccdname) >= 4) {
		strcpy(ccdname + strlen(ccdname) - 4, ".ccd");
	}
	else {
		return -1;
	}

	if ((fi = fopen(ccdname, "r")) == NULL) {
		return -1;
	}

	memset(&ti, 0, sizeof(ti));

	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		if (!strncmp(linebuf, "[TRACK", 6)){
			numtracks++;
		}
		else if (!strncmp(linebuf, "MODE=", 5)) {
			sscanf(linebuf, "MODE=%d", &t);
			ti[numtracks].type = ((t == 0) ? CDDA : DATA);
		}
		else if (!strncmp(linebuf, "INDEX 1=", 8)) {
			sscanf(linebuf, "INDEX 1=%d", &t);
			sec2msf(t + 2 * 75, ti[numtracks].start);

			// If we've already seen another track, this is its end
			if (numtracks > 1) {
				t = msf2sec(ti[numtracks].start) - msf2sec(ti[numtracks - 1].start);
				sec2msf(t, ti[numtracks - 1].length);
			}
		}
	}

	fclose(fi);

	// Fill out the last track's end based on size
	if (numtracks >= 1) {
		fseek(cdHandle, 0, SEEK_END);
		t = ftell(cdHandle) / 2352 - msf2sec(ti[numtracks].start) + 2 * 75;
		sec2msf(t, ti[numtracks].length);
	}

	return 0;
}

// this function tries to get the .mds file of the given .mdf
// the necessary data is put into the ti (trackinformation)-array
static int parsemds(const char *isofile) {
	char			mdsname[MAXPATHLEN];
	FILE			*fi;
	unsigned int	offset, extra_offset, l, i;
	unsigned short	s;

	numtracks = 0;

	// copy name of the iso and change extension from .mdf to .mds
	strncpy(mdsname, isofile, sizeof(mdsname));
	mdsname[MAXPATHLEN - 1] = '\0';
	if (strlen(mdsname) >= 4) {
		strcpy(mdsname + strlen(mdsname) - 4, ".mds");
	}
	else {
		return -1;
	}

	if ((fi = fopen(mdsname, "rb")) == NULL) {
		return -1;
	}

	memset(&ti, 0, sizeof(ti));

	// check if it's a valid mds file
	fread(&i, 1, sizeof(unsigned int), fi);
	i = SWAP32(i);
	if (i != 0x4944454D) {
		// not an valid mds file
		fclose(fi);
		return -1;
	}

	// get offset to session block
	fseek(fi, 0x50, SEEK_SET);
	fread(&offset, 1, sizeof(unsigned int), fi);
	offset = SWAP32(offset);

	// get total number of tracks
	offset += 14;
	fseek(fi, offset, SEEK_SET);
	fread(&s, 1, sizeof(unsigned short), fi);
	s = SWAP16(s);
	numtracks = s;

	// get offset to track blocks
	fseek(fi, 4, SEEK_CUR);
	fread(&offset, 1, sizeof(unsigned int), fi);
	offset = SWAP32(offset);

	// skip lead-in data
	while (1) {
		fseek(fi, offset + 4, SEEK_SET);
		if (fgetc(fi) < 0xA0) {
			break;
		}
		offset += 0x50;
	}

	// check if the image contains interleaved subchannel data
	fseek(fi, offset + 1, SEEK_SET);
	subChanInterleaved = fgetc(fi);

	// read track data
	for (i = 1; i <= numtracks; i++) {
		fseek(fi, offset, SEEK_SET);

		// get the track type
		ti[i].type = ((fgetc(fi) == 0xA9) ? CDDA : DATA);
		fseek(fi, 8, SEEK_CUR);

		// get the track starting point
		ti[i].start[0] = fgetc(fi);
		ti[i].start[1] = fgetc(fi);
		ti[i].start[2] = fgetc(fi);

		if (i > 1) {
			l = msf2sec(ti[i].start);
			sec2msf(l - 2 * 75, ti[i].start); // ???
		}

		// get the track length
		fread(&extra_offset, 1, sizeof(unsigned int), fi);
		extra_offset = SWAP32(extra_offset);

		fseek(fi, extra_offset + 4, SEEK_SET);
		fread(&l, 1, sizeof(unsigned int), fi);
		l = SWAP32(l);
		sec2msf(l, ti[i].length);

		offset += 0x50;
	}

	fclose(fi);
	return 0;
}

// this function tries to get the .sub file of the given .img
static int opensubfile(const char *isoname) {
	char		subname[MAXPATHLEN];

	// copy name of the iso and change extension from .img to .sub
	strncpy(subname, isoname, sizeof(subname));
	subname[MAXPATHLEN - 1] = '\0';
	if (strlen(subname) >= 4) {
		strcpy(subname + strlen(subname) - 4, ".sub");
	}
	else {
		return -1;
	}

	subHandle = fopen(subname, "rb");
	if (subHandle == NULL) {
		return -1;
	}

	return 0;
}

long CDR_shutdown(void) {
	stopCDDA();
	if (cdHandle != NULL) {
		fclose(cdHandle);
		cdHandle = NULL;
	}
	if (subHandle != NULL) {
		fclose(subHandle);
		subHandle = NULL;
	}
	return 0;
}

long CDR_init(void) {
	CDR_shutdown();
	numtracks = 0;
	return 0; // do nothing
}

long CDR_readTrack(unsigned char *time);
static __inline__ unsigned char * CDR_getBuffer(void);
static void mmssdd( char *b, char *p );
static int GetCdromFile(unsigned char *mdir, unsigned char *time, const char *filename, unsigned dirlen);

// This function is invoked by the front-end when opening an ISO
// file for playback
long CDR_open(void) {
	int i,j;

	if (cdHandle != NULL) {
		return 0; // it's already open
	}

	cdHandle = fopen(IsoFile, "rb");
	if (cdHandle == NULL) {
		return -1;
	}

	printf("Loaded CD Image: %s", IsoFile);

	cddaBigEndian = 0;
	subChanInterleaved = 0;

	if (parsetoc(IsoFile) == 0) {
		cddaBigEndian = 1; // cdrdao uses big-endian for CD Audio
		printf("[+toc]");
	}
	else if (parsecue(IsoFile) == 0) {
		printf("[+cue]");
	}
	else if (parseccd(IsoFile) == 0) {
		printf("[+ccd]");
	}
	else if (parsemds(IsoFile) == 0) {
		printf("[+mds]");
	}

	if (!subChanInterleaved && opensubfile(IsoFile) == 0) {
		printf("[+sub]");
	}

	memset((void *)&cdr_files[0],0,sizeof(cdr_files));

	for(i=0;i<(sizeof(cdr_modes)/4);i++) {
		cdr_mode=cdr_modes[i];
		for(j=0;j<(sizeof(cdr_frames)/4);j++) {
			struct iso_directory_record *dir;
			unsigned char time[4],*buf;
			unsigned char mdir[MAXDIRLEN];

			cdr_frame=cdr_frames[j];

			time[0] = itob(0); time[1] = itob(2); time[2] = itob(0x10);
			READTRACK();
			dir = (struct iso_directory_record *)&buf[CD_FRAMESIZE_PAD + 156];
			unsigned long long ulldirlen=*((unsigned long long *)(&dir->size[0]));
			unsigned dirlen=(unsigned)ulldirlen;
			if (dirlen>MAXDIRLEN) dirlen=MAXDIRLEN;
			mmssdd(dir->extent, (char*)time);
			READDIR(mdir);
			if (GetCdromFile(mdir, time, "IPL.TXT", dirlen)>0)
				i=j=12345678;
		}
	}
	printf(" (%i/MODE%i).\n",cdr_frame,cdr_mode/8);

	return 0;
}

long CDR_close(void) {
	if (cdHandle != NULL) {
		fclose(cdHandle);
		cdHandle = NULL;
	}
	if (subHandle != NULL) {
		fclose(subHandle);
		subHandle = NULL;
	}
	stopCDDA();
	return 0;
}

// return Starting and Ending Track
// buffer:
//  byte 0 - start track
//  byte 1 - end track
long CDR_getTN(unsigned char *buffer) {
	buffer[0] = 1;

	if (numtracks > 0) {
		buffer[1] = numtracks;
	}
	else {
		buffer[1] = 1;
	}

	return 0;
}

// return Track Time
// buffer:
//  byte 0 - frame
//  byte 1 - second
//  byte 2 - minute
long CDR_getTD(unsigned char track, unsigned char *buffer) {
	if (numtracks > 0 && track <= numtracks) {
		buffer[2] = ti[track].start[0];
		buffer[1] = ti[track].start[1];
		buffer[0] = ti[track].start[2];
	}
	else {
		buffer[2] = 0;
		buffer[1] = 2;
		buffer[0] = 0;
	}

	return 0;
}

// lookup table for crc calculation
static unsigned short crctab[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108,
	0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 0x1231, 0x0210,
	0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B,
	0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 0x2462, 0x3443, 0x0420, 0x1401,
	0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE,
	0xF5CF, 0xC5AC, 0xD58D, 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6,
	0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D,
	0xC7BC, 0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 0x5AF5,
	0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC,
	0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 0x6CA6, 0x7C87, 0x4CE4,
	0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD,
	0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13,
	0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A,
	0x9F59, 0x8F78, 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E,
	0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1,
	0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 0xB5EA, 0xA5CB,
	0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0,
	0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xA7DB, 0xB7FA, 0x8799, 0x97B8,
	0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657,
	0x7676, 0x4615, 0x5634, 0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9,
	0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882,
	0x28A3, 0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 0xFD2E,
	0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07,
	0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 0xEF1F, 0xFF3E, 0xCF5D,
	0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74,
	0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

static unsigned short calcCrc(unsigned char *d, int len) {
	unsigned short crc = 0;
	int i;

	for (i = 0; i < len; i++) {
		crc = crctab[(crc >> 8) ^ d[i]] ^ (crc << 8);
	}

	return ~crc;
}

// read track
// time: byte 0 - minute; byte 1 - second; byte 2 - frame
// uses bcd format
long CDR_readTrack(unsigned char *time) {
	if (cdHandle == NULL) {
		return -1;
	}

	if (subChanInterleaved) {
		fseek(cdHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * (CD_FRAMESIZE_RAW + SUB_FRAMESIZE) + CD_FRAMESIZE_PAD, SEEK_SET);
		fread(cdbuffer, 1, DATA_SIZE, cdHandle);
		fread(subbuffer, 1, SUB_FRAMESIZE, cdHandle);

		if ((((unsigned short)subbuffer[22 - 12 + CD_FRAMESIZE_PAD] << 8) | (unsigned short)subbuffer[23 -12 + CD_FRAMESIZE_PAD]) != calcCrc(&subbuffer[CD_FRAMESIZE_PAD], 10)) {
			memset(&subbuffer[15 - 12 + CD_FRAMESIZE_PAD], 0, 7); // CRC wrong, wipe out time data
		}
	}
	else {
		fseek(cdHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * CD_FRAMESIZE_RAW + CD_FRAMESIZE_PAD, SEEK_SET);
		fread(cdbuffer, 1, DATA_SIZE, cdHandle);

		if (subHandle != NULL) {
			fseek(subHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * SUB_FRAMESIZE, SEEK_SET);
			fread(subbuffer, 1, SUB_FRAMESIZE, subHandle);

			if ((((unsigned short)subbuffer[22 - 12 + CD_FRAMESIZE_PAD] << 8) | (unsigned short)subbuffer[23 - 12 + CD_FRAMESIZE_PAD]) != calcCrc(&subbuffer[CD_FRAMESIZE_PAD], 10)) {
				memset(&subbuffer[15 - 12 + CD_FRAMESIZE_PAD], 0, 7); // CRC wrong, wipe out time data
			}
		}
	}

	return 0;
}

// return readed track
static __inline__ unsigned char * CDR_getBuffer(void) {
	return cdbuffer;
}

// plays cdda audio
// sector: byte 0 - minute; byte 1 - second; byte 2 - frame
// does NOT uses bcd format
long CDR_play(unsigned char *time) {
#if 0
	if (SPU_playCDDAchannel != NULL) {
#endif
		if (subChanInterleaved) {
			startCDDA(MSF2SECT(time[0], time[1], time[2]) * (CD_FRAMESIZE_RAW + SUB_FRAMESIZE));
		}
		else {
			startCDDA(MSF2SECT(time[0], time[1], time[2]) * CD_FRAMESIZE_RAW);
		}
#if 0
	}
#endif
	return 0;
}

// stops cdda audio
long CDR_stop(void) {
	stopCDDA();
	return 0;
}

// gets subchannel data
unsigned char* CDR_getBufferSub(void) {
	if (subHandle != NULL || subChanInterleaved) {
		return subbuffer;
	}

	return NULL;
}

long CDR_getStatus(struct CdrStat *stat) {
	int sec;

	if (cdOpenCaseTime < 0 || cdOpenCaseTime > (long long)time(NULL))
		stat->Status = 0x10;
	else
		stat->Status = 0;

	if (playing) {
		stat->Type = 0x02;
		stat->Status |= 0x80;
		sec = cddaCurOffset / CD_FRAMESIZE_RAW;
		sec2msf(sec, (char *)stat->Time);
	}
	else {
		stat->Type = 0x01;
	}

	return 0;
}

int CDR_IsoActive(void) {
	return (cdHandle != NULL);
}

static void mmssdd( char *b, char *p )
 {
	int m, s, d;
	int block = (b[0]&0xff) | ((b[1]&0xff)<<8) | ((b[2]&0xff)<<16) | (b[3]<<24);

	block += 150;
	m = block / 4500;			// minuten
	block = block - m * 4500;	// minuten rest
	s = block / 75;				// sekunden
	d = block - s * 75;			// sekunden rest

	m = ( ( m / 10 ) << 4 ) | m % 10;
	s = ( ( s / 10 ) << 4 ) | s % 10;
	d = ( ( d / 10 ) << 4 ) | d % 10;

	p[0] = m;
	p[1] = s;
	p[2] = d;
}

static int GetCdromFile(unsigned char *mdir, unsigned char *time, const char *filename, unsigned dirln) {
	struct iso_directory_record *dir;
	unsigned char ddir[MAXDIRLEN];
	unsigned char *buf;
	int i;

	// only try to scan if a filename is given
	if (strlen(filename)==0) return -1;

	if (dirln>MAXDIRLEN) dirln=MAXDIRLEN;
	i = 0;
	while (i < dirln) {
		dir = (struct iso_directory_record*) &mdir[i];
		if (dir->length[0] <= 0) {
			if (i<(dirln-2048)) i=((i/2048)+1)*2048;
			else return -1;
		} else
		i += dir->length[0];

		if (dir->flags[0] & 0x2) { // it's a dir
			if (!strncasecmp((char *)&dir->name[0], filename, dir->name_len[0])) {
				if (filename[dir->name_len[0]] != '\\') continue;

				filename += dir->name_len[0] + 1;

				unsigned long long ulldirlen=*((unsigned long long *)(&dir->size[0]));
				unsigned dirlen=(unsigned)ulldirlen;
				if (dirlen>MAXDIRLEN) dirlen=MAXDIRLEN;
				mmssdd(dir->extent, (char *)time);
				READDIR(ddir);
				i = 0;
				mdir = (unsigned char*)ddir;
			}
		} else {
			if (!strncasecmp((char *)&dir->name[0], filename, strlen(filename))) {
				unsigned long long len=*((unsigned long long *)(&dir->size[0]));
				mmssdd(dir->extent, (char *)time);
				return (unsigned)(len&0xFFFFFFFFULL);
				break;
			}
		}
	}
	return -1;
}

int CDR_LoadCdromFile(const char *filename, unsigned char *addr, unsigned len, unsigned offset) {
	struct iso_directory_record *dir;
	unsigned char time[4],*buf;
	unsigned char mdir[MAXDIRLEN];
	unsigned size;
	int i,total_len,ret=0;

	time[0] = itob(0); time[1] = itob(2); time[2] = itob(0x10);

	READTRACK();

	// skip head and sub, and go to the root directory record
	dir = (struct iso_directory_record *)&buf[CD_FRAMESIZE_PAD + 156];

	unsigned long long ulldirlen=*((unsigned long long *)(&dir->size[0]));
	unsigned dirlen=(unsigned)ulldirlen;
	if (dirlen>MAXDIRLEN) dirlen=MAXDIRLEN;
	mmssdd(dir->extent, (char*)time);

	READDIR(mdir);

	if ((total_len=GetCdromFile(mdir, time, filename, dirlen)) <0 ) return -1;

	if (addr==NULL) return 0;

	for(i=0;i<(total_len)&&(len>0);i+=2048) {
		int diff=(i+2048)>total_len?total_len-i:2048;
		READTRACK(); incTime();
		if (i+diff>=offset) {
			int c=len>diff?diff:len;
			if (i>=offset) {
				memcpy(addr, buf + CD_FRAMESIZE_PAD, c);
			} else {
				int b=diff-(offset-i);
				c-=b;
				memcpy(addr, buf + CD_FRAMESIZE_PAD + b, c);
			}
			ret += c;
			addr += c;
			len -= c;
		}
	}

	return ret;
}

void *CDR_fopen(const char *filename, const char *filemode) {
	int i_cdr=0;
	struct iso_directory_record *dir;
	unsigned char time[4],*buf;
	unsigned char mdir[MAXDIRLEN];
	int total_len;

	if (!filemode ||!filename) return NULL;
	if (filemode[0]!='r' && filemode[0]!='R') return NULL;
	for (i_cdr=0;i_cdr<(sizeof(cdr_files)/sizeof(cdr_file));i_cdr++)
		if (!cdr_files[i_cdr].size) break;

	time[0] = itob(0); time[1] = itob(2); time[2] = itob(0x10);

	READTRACK_p();

	// skip head and sub, and go to the root directory record
	dir = (struct iso_directory_record *)&buf[CD_FRAMESIZE_PAD + 156];

	unsigned long long ulldirlen=*((unsigned long long *)(&dir->size[0]));
	unsigned dirlen=(unsigned)ulldirlen;
	if (dirlen>MAXDIRLEN) dirlen=MAXDIRLEN;
	mmssdd(dir->extent, (char*)time);

	READDIR_p(mdir);

	if ((total_len=GetCdromFile(mdir, time, filename, dirlen)) <0 ) return NULL;

	cdr_files[i_cdr].time[0]=time[0];
	cdr_files[i_cdr].time[1]=time[1];
	cdr_files[i_cdr].time[2]=time[2];
	cdr_files[i_cdr].time[3]=time[3];
	cdr_files[i_cdr].size=total_len;
	cdr_files[i_cdr].pos=0;

	return (void *)&cdr_files[i_cdr];

}

void CDR_fclose(void *fp) {
	if (((unsigned)fp >= (unsigned)&cdr_files[0])&&
	    ((unsigned)fp < (unsigned)&cdr_files[(sizeof(cdr_files)/sizeof(cdr_file))])) {
		cdr_file *cf=(cdr_file *)fp;
		cf->size=0;
	}
}

unsigned CDR_fread(void *addr_b, unsigned size_f, unsigned count_f, void *fp) {
	if (!addr_b || !fp || !size_f || !count_f) return 0;
	if (((unsigned)fp >= (unsigned)&cdr_files[0])&&
	    ((unsigned)fp < (unsigned)&cdr_files[(sizeof(cdr_files)/sizeof(cdr_file))])) {
		unsigned char *addr=(unsigned char *)addr_b;
		unsigned len=size_f*count_f;
		cdr_file *cf=(cdr_file *)fp;
		unsigned char time[4], *buf;
		unsigned i,ret=0;

		if (cf->pos>=cf->size) return 0;
		if (len>cf->size) len=cf->size;

		time[0]=cf->time[0];
		time[1]=cf->time[1];
		time[2]=cf->time[2];
		time[3]=cf->time[3];

		for(i=0;i<(cf->size)&&(len>0);i+=2048) {
			int diff=(i+2048)>cf->size?cf->size-i:2048;
			READTRACK_n(); incTime();
			if (i+diff>=cf->pos) {
				int c=len>diff?diff:len;
				if (i>=cf->pos) {
					memcpy(addr, buf + CD_FRAMESIZE_PAD, c);
				} else {
					int b=cf->pos-i;
					if ((b+c)>2048)
						c=2048-b;
					memcpy(addr, buf + CD_FRAMESIZE_PAD + b, c);
				}
				ret += c;
				addr += c;
				len -= c;
			}
		}
		if (cf->pos+ret>cf->size)
			ret=cf->size-cf->pos;
		cf->pos+=ret;
		return ret/size_f;
	}
	return 0;

}

int CDR_feof(void *fp){
	if (((unsigned)fp >= (unsigned)&cdr_files[0])&&
	    ((unsigned)fp < (unsigned)&cdr_files[(sizeof(cdr_files)/sizeof(cdr_file))])) {
		cdr_file *cf=(cdr_file *)fp;
		if (cf->size)
			return cf->pos>=cf->size;
	}
	return -1;
}

char *CDR_fgets(char *addr_b, int size_b, void *fp){
	unsigned i, readed, pos;
	if (((unsigned)fp >= (unsigned)&cdr_files[0])&&
	    ((unsigned)fp < (unsigned)&cdr_files[(sizeof(cdr_files)/sizeof(cdr_file))])) {
		cdr_file *cf=(cdr_file *)fp;
		pos=cf->pos;
		readed=CDR_fread(addr_b,1,size_b,fp);
		if (readed<=0) return NULL;
		for(i=0;(i<readed) && (i<(size_b-1));i++)
			if (addr_b[i]==10 || addr_b[i]==13) break;
		addr_b[i]=0;
		cf->pos=pos+i+1;
		if (addr_b[i+1]==10 || addr_b[i+1]==13)
			cf->pos++;
		return addr_b;
	}
	return NULL;
}
#endif
