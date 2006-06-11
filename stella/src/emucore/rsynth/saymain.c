/*
    Copyright (c) 1994,2001-2004 Nick Ing-Simmons. All rights reserved.

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#include "config.h"
extern char *Revision;
#include <stdio.h>
#include <ctype.h>
#include "useconfig.h"
#include <math.h>
#ifdef OS2
#include <signal.h>
#include <setjmp.h>
#include <float.h>
#endif				/* OS2 */
#include "darray.h"
#include "rsynth.h"
#include "hplay.h"
#include "dict.h"
#include "lang.h"
#include "text.h"
#include "getargs.h"
#include "phones.h"
#include "aufile.h"
#include "charset.h"		/* national differences ,jms */
#include "english.h"
#include "deutsch.h"
#include "say.h"

char *program = "say";

int verbose = 0;

#if 1
lang_t *lang = &English;
#else
lang_t *lang = &Deutsch;
#endif

long clip_max;
float peak;

static short
clip(long *clip_max, float input, float *peak)
{
    long temp = (long) input;
    float isq = input * input;
#ifdef PEAK
    if (isq > *peak)
	*peak = isq;
#else
    *peak += isq;
#endif
    if (-temp > *clip_max)
	*clip_max = -temp;
    if (temp > *clip_max)
	*clip_max = temp;
    if (temp < -32767) {
	temp = -32767;
    }
    else if (temp > 32767) {
	temp = 32767;
    }
    return (temp);
}

static void *
save_sample(void *user_data, float sample, unsigned nsamp,
	    rsynth_t * rsynth)
{
    darray_t *buf = (darray_t *) user_data;
    darray_short(buf, clip(&clip_max, sample, &peak));	/* Convert back to integer */
    return (void *) buf;
}

static void *
flush_samples(void *user_data, unsigned nsamp, rsynth_t * rsynth)
{
    darray_t *buf = (darray_t *) user_data;
    if (buf->items) {
	short *samp = (short *) darray_find(buf, 0);
	audio_play(buf->items, samp);
	if (file_write)
	    (*file_write) (buf->items, samp);
	buf->items = 0;
    }
    return (void *) buf;
}

static char *
concat_args(int argc, char **argv)
{
    int len = 0;
    int i;
    char *buf;
    for (i = 1; i < argc; i++)
	len += strlen(argv[i]) + 1;
    buf = (char *) malloc(len);
    if (buf) {
	char *d = buf;
	for (i = 1; i < argc;) {
	    char *s = argv[i++];
	    while (*s)
		*d++ = *s++;
	    if (i < argc)
		*d++ = ' ';
	    else
		*d = '\0';
	}
    }
    return buf;
}

int
main(int argc, char *argv[])
{
    char *file_name = NULL;
    char *voice_file = NULL;
    char *par_name = NULL;
    char *input_phones = "sampa";
    double mSec_per_frame = 10;
    double F0Hz = 133.0;
    double speed = 1.0;
    double frac = 0.5;
    long gain = 57;
    int monotone = 0;
    int etrace = 0;
    int f0trace = 0;

    int dodur = 1;
#ifndef OS2
    program = argv[0];
#else				/* OS2 */

    _control87(EM_INVALID | EM_DENORMAL | EM_ZERODIVIDE | EM_OVERFLOW |
	       EM_UNDERFLOW | EM_INEXACT, MCW_EM);

    _ctype = ctable + 1;	/* jms: proper character handling for toupper, tolower... */

/* if (NULL == setlocale(LC_ALL, "GERM") ) */
/* if (NULL == setlocale(LC_ALL, "DE_DE") ) */
/* if (NULL == setlocale(LC_ALL, "") ) */
    /*printf("setlocale failed.\n") */ ;
    /* jms */

#endif				/* OS2 */

    init_locale();

    argc = getargs("Synth", argc, argv,
		   "d", "", &dict_path, "Which dictionary [b|a|g]",
		   "v", NULL, &verbose, "Verbose, show phonetic form",
		   "I", "", &input_phones,  "Phoneset of input (for festival)",
		   "L", "",  &voice_file, "Log file of voice data",
		   "D", NULL,  &dodur, "Honour durations in .pho file",
		   "m", "%lg",  &mSec_per_frame, "mSec per frame",
		   "F", "%lg", &F0Hz,  "F0 Frequency",
		   "f", "", &file_name, "File to process",
		   "p", "", &par_name, "Parameter file for plot",
		   "S", "%lg", &speed, "Speed (1.0 is 'normal')",
		   "K", "%lg",  &frac, "Parameter filter 'fraction'",
		   "M", NULL,  &monotone, "Hold F0 fixed",
		   "E", NULL, &etrace, "Trace element sequence",
		   "G", "%ld", &gain, "Overall Gain",
		   NULL);

    /* audio_init sets the rate */
    argc = audio_init(argc, argv);
    /* so must come before file init which writes rate to header */
    argc = file_init(argc, argv);


    if (help_only) {
	fprintf(stderr, "Usage: %s [options as above] [words to say]\n",
		program);
	fprintf(stderr, "or     %s [options as above] < file-to-say\n",
		program);
	fprintf(stderr, "(%s)\n", Revision);
    }
    else {
	darray_t samples;
	rsynth_t *rsynth;
	darray_init(&samples, sizeof(short), 2048);	/* chg jms */
	if (dict_path && *dict_path)
	    dict_init(dict_path);
	rsynth = rsynth_init(samp_rate, mSec_per_frame,
			     rsynth_speaker(F0Hz, gain, Elements),
			     save_sample, flush_samples, &samples);
	if (par_name) {
	    rsynth->parm_file = fopen(par_name, "w");
	    if (!rsynth->parm_file)
		perror(par_name);
	}
	rsynth->smooth = frac;
	rsynth->speed = speed;
	if (monotone)
	    rsynth->flags |= RSYNTH_MONOTONE;
	if (etrace)
	    rsynth->flags |= RSYNTH_ETRACE;
	if (verbose)
	    rsynth->flags |= RSYNTH_VERBOSE;
	if (f0trace)
	    rsynth->flags |= RSYNTH_F0TRACE;

	if (!file_name && argc < 2) {
	    say_file(rsynth, stdin);
	}
	else {
	    if (file_name) {
		rsynth_pho(rsynth, file_name, dodur, input_phones);
	    }
	    if (argc > 1) {
		char *s = concat_args(argc, argv);
		if (s) {
		    say_string(rsynth, s);
		    free(s);
		}
	    }
	}
	if (file_term)
	    (*file_term) ();
	audio_term();
	/* warn if we have clipped (or got close) converting to 16-bit */
	if (verbose || clip_max > 30000) {
	    float v = clip_max * 2.5 / 32767;
	    fprintf(stderr, "Clipng @ %lu %.4g %.3g %.4gW\n",
		    clip_max, clip_max / 32767.0,
		    20 * log10(clip_max / 32767.0), v * v / 8);

	}
	if (have_dict)
	    dict_term();
	rsynth_term(rsynth);
	darray_free(&samples);
    }
    return (0);
}
