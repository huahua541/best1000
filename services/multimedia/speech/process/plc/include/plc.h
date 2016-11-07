/////////////////////////////////////////////////////////////////////////
//Packets Loss Concealment                                              /
/////////////////////////////////////////////////////////////////////////
#ifndef _CVSD_PLC_H_
#define _CVSD_PLC_H_


//quality set
//#define FINE_PLC_PITCH_SEARCH


//for mac using int32 or int64
#define DATA_FOR_FIND_PITCH_SHIFT (3)


#define FIXED_Q (1<<15)
#define FIXED_SQRT_STEP 16

//basic set
/* minimum allowed pitch, 200 Hz */
#define PITCH_MIN 40 

/* maximum allowed pitch, 66 Hz */
#define PITCH_MAX 120 

#define PITCHDIFF (PITCH_MAX - PITCH_MIN)

/* maximum pitch OLA window */
#define POVERLAPMAX (PITCH_MAX >> 2)

/* history buffer length*/
#define HISTORYLEN (PITCH_MAX * 3 + POVERLAPMAX) 

/* 2:1 decimation */
#define NDEC 2 

/* 20 ms correlation length */
#define CORRLEN 160 

/* correlation buffer length */
#define CORRBUFLEN (CORRLEN + PITCH_MAX) 

/* minimum power */
#define CORRMINPOWER (250) 

/* end OLA increment per frame, 4 ms */
#define EOVERLAPINCR (32) 

/* 10 ms at 8 KHz */
#define FRAMESZ 80 

/* attenuation factor per 10 ms frame */
#define ATTENFAC ((short)(0.2*FIXED_Q)) 


/* attenuation per sample */
#define ATTENINCR (ATTENFAC/FRAMESZ)


struct PlcSt
{
	int LaseOverlen;

	int LaseOverTatollen;

	int LaseEraseCntSample;

	/* consecutive erased samples */
	int OverCntSample;

	/* consecutive erased samples */
	int EraseCntSample;

	/* overlap based on pitch */
	int poverlap;

	/* offset into pitch period */
	int poffset;

	/* pitch estimate */
	int pitch;

	/* current pitch buffer length */
	int pitchblen;

	/* end of pitch buffer */
	short *pitchbufend;

	/* start of pitch buffer */
	short *pitchbufstart;

	/* buffer for cycles of speech */
	short pitchbuf[HISTORYLEN];

	/* saved last quarter wavelength */
	short lastq[POVERLAPMAX];

	/* history buffer */
	short history[HISTORYLEN];

	/* tail of previous pitch estimate */
	short OverBuf[POVERLAPMAX];

	short LastOverbuf[FRAMESZ];
};





/* synthesize speech for erasure */
void Dofe(struct PlcSt *lc, short *out, int num);


/* add a good frame to history buffer */
void AddToHistory(struct PlcSt *lc, short *s, int num);

void ScaleSpeech(struct PlcSt *lc, short *out, int num);
void GetFeSpeech(struct PlcSt *lc, short *out, int sz);
void SaveSpeech(struct PlcSt *lc, short *s, int num);


int FindPitch(struct PlcSt *lc);

void FOverLapAdd(short *l, short *r, short *o, int cnt);
void SOverLapAdd(short *l, short *r, short *o, int totalcnt, int newcnt, int oldcnt);
void OverLapAddAtEnd(struct PlcSt *lc, short *s, short *f, int totalcnt, int newcnt, int oldcnt);

void CopySample(short *f, short *t, int cnt);
void Zeros(short *s, int cnt);
void PlcInit(struct PlcSt *lc);

struct PlcSt *speech_plc_init(void* (* speex_alloc_ext)(int));
int speech_plc(struct PlcSt *lc, short *InBuf, int len);

#endif












