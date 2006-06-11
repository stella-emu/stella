/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    
    win_audio.c
    
    Functions to play sound on the Win32 audio driver (Win 95 or Win NT).

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifdef __MINGW32__
#include "config/mmsystem.h"
#endif

#include "config.h"
#include "hplay.h"
#include "getargs.h"

typedef long int32;

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static void output_data(int32 *buf, int32 count);
static void flush_output(void);
static void purge_output(void);

/* export the playback mode */

#define dpm win32_play_mode

#define PE_MONO      1
#define PE_SIGNED    2
#define PE_16BIT     4
#define PE_ULAW      8
#define PE_BYTESWAP 16
#define DEFAULT_RATE 8000

typedef struct 
{
 int32 rate;
 int32 encoding;
 int32 extra_param[1];
} PlayMode;

long samp_rate = DEFAULT_RATE;

int
ftruncate(int fd,long size)
{
 return 0;
}

PlayMode dpm = {
  DEFAULT_RATE, PE_16BIT|PE_SIGNED|PE_MONO,
  {16},
};

/* Max audio blocks waiting to be played */

static LPHWAVEOUT dev;
static int nBlocks;

CRITICAL_SECTION critSect;

static void wait (void)
	{
	while (nBlocks)
		Sleep (0);
	}

static int play (void *mem, int len)
	{
	HGLOBAL hg;
	LPWAVEHDR wh;
	MMRESULT res;

	while (nBlocks >= dpm.extra_param[0])
		Sleep (0);

	hg = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (WAVEHDR));
	if (!hg)
		{
		fprintf(stderr, "GlobalAlloc failed!");
		return FALSE;
		}
	wh = GlobalLock (hg);
	wh->dwBufferLength = len;
	wh->lpData = mem;

	res = waveOutPrepareHeader (dev, wh, sizeof (WAVEHDR));
	if (res)
		{
		fprintf(stderr, "waveOutPrepareHeader: %d", res);
		GlobalUnlock (hg);
		GlobalFree (hg);
		return TRUE;
		}
	res = waveOutWrite (dev, wh, sizeof (WAVEHDR));
	if (res)
		{
		fprintf(stderr, "waveOutWrite: %d", res);
		GlobalUnlock (hg);
		GlobalFree (hg);
		return TRUE;
		}
	EnterCriticalSection (&critSect);
	nBlocks++;
	LeaveCriticalSection (&critSect);
	return FALSE;
	}

#pragma argsused
static void CALLBACK wave_callback (HWAVE hWave, UINT uMsg,
		DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
	{
	WAVEHDR *wh;
	HGLOBAL hg;

	if (uMsg == WOM_DONE)
		{
		EnterCriticalSection (&critSect);
		wh = (WAVEHDR *)dwParam1;
		waveOutUnprepareHeader (dev, wh, sizeof (WAVEHDR));
		hg = GlobalHandle (wh->lpData);
		GlobalUnlock (hg);
		GlobalFree (hg);
		hg = GlobalHandle (wh);
		GlobalUnlock (hg);
		GlobalFree (hg);
		nBlocks--;
		LeaveCriticalSection (&critSect);
		}
	}

static int open_output (void)
	{
	int i, j, mono, eight_bit, warnings = 0;
	PCMWAVEFORMAT pcm;
	MMRESULT res;

	/* Check if there is at least one audio device */
	if (!waveOutGetNumDevs ())
		{
		fprintf(stderr, "No audio devices present!");
		return -1;
		}

	/* They can't mean these */
	dpm.encoding &= ~(PE_ULAW|PE_BYTESWAP);

	if (dpm.encoding & PE_16BIT)
		dpm.encoding |= PE_SIGNED;
	else
		dpm.encoding &= ~PE_SIGNED;

	mono = (dpm.encoding & PE_MONO);
	eight_bit = !(dpm.encoding & PE_16BIT);

	pcm.wf.wFormatTag = WAVE_FORMAT_PCM;
	pcm.wf.nChannels = mono ? 1 : 2;
	pcm.wf.nSamplesPerSec = i = dpm.rate;
	j = 1;
	if (!mono)
		{
		i *= 2;
		j *= 2;
		}
	if (!eight_bit)
		{
		i *= 2;
		j *= 2;
		}
	pcm.wf.nAvgBytesPerSec = i;
	pcm.wf.nBlockAlign = j;
	pcm.wBitsPerSample = eight_bit ? 8 : 16;

	res = waveOutOpen (NULL, 0, (LPWAVEFORMAT)&pcm, NULL, 0, WAVE_FORMAT_QUERY);
	if (res)
		{
		fprintf(stderr, "Format not supported!\n");
		return -1;
		}
	res = waveOutOpen (&dev, 0, (LPWAVEFORMAT)&pcm, (DWORD)wave_callback, 0, CALLBACK_FUNCTION);
	if (res)
		{
		fprintf(stderr, "Can't open audio device");
		return -1;
		}
	nBlocks = 0;
	return warnings;
	}


void
conv8bit(short *lp, int c)
{
 unsigned char *cp = (unsigned char *) lp;
 short l;
 while (c--)
  {
   short l = (*lp++) >> (16-8);
   if (l > 127) l = 127;
   else if (l < -128) l = -128;
   *cp++ = 0x80 ^ ((unsigned char) l);
  }
}

void 
audio_play(int count, short *buf)
	{
	int len = count;
	HGLOBAL hg;
	void *b;

	if (!(dpm.encoding & PE_MONO)) /* Stereo sample */
		{
		count *= 2;
		len *= 2;
		}

	if (dpm.encoding & PE_16BIT)
		len *= 2;

	hg = GlobalAlloc (GMEM_MOVEABLE, len);
	if (!hg)
		{
		fprintf(stderr, "GlobalAlloc failed!");
		return;
		}
	b = GlobalLock (hg);

	if (!(dpm.encoding & PE_16BIT))
		/* Convert to 8-bit unsigned. */
		conv8bit(buf, count);

#ifdef __MINGW32__
        memcpy(b, buf, len);
#else
	CopyMemory(b, buf, len);
#endif
	if (play (b, len))
		{
		GlobalUnlock (hg);
		GlobalFree (hg);
		}
	}

static void close_output (void)
	{
	wait ();
	waveOutClose (dev);
	}

static void flush_output (void)
	{
	wait ();
	}

static void purge_output (void)
	{
	waveOutReset (dev);
	wait ();
	}           

int use_audio = 1;

int
audio_init(argc, argv)
int argc;
char *argv[];
{
 InitializeCriticalSection(&critSect);
 argc = getargs("Win32 waveOut",argc, argv,
                "a", NULL, &use_audio, "Use audio", 
#if 0
                "g", "%lg", &gain,     "Gain 0 .. 0.1",
                "r", "%d", &rate_set,  "Sample rate",
                "h", NULL, &headphone, "Headphones",
                "s", NULL, &speaker,   "Speaker",
                "W", NULL, &Wait,      "Wait till idle",
                "L", NULL, &use_linear,"Force linear",
#endif
                NULL);
 if (help_only)
  return argc;
 open_output();
 return argc;
}

void
audio_term()
{
 close_output();
 DeleteCriticalSection(&critSect);
}             


