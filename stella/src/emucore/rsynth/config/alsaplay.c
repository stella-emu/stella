/*
    Copyright (c) 2003 Nick Ing-Simmons.
    All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA 02111-1307, USA

*/

#include "config.h"


#include "useconfig.h"
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <unistd.h>

// #define ALSA_PCM_OLD_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include "l2u.h"

char *pcm_name = "default";

#include "getargs.h"
#include "hplay.h"

#define SAMP_RATE 11025
long samp_rate = SAMP_RATE;

/* Audio Parameters */

static int linear_fd = -1;

char *prog = "hplay";
static snd_pcm_t *pcm;
static snd_pcm_hw_params_t *hwparams;
static snd_pcm_uframes_t chunk;

static int
audio_open(void)
{
 int err;
 if ((err = snd_pcm_open(&pcm,pcm_name,SND_PCM_STREAM_PLAYBACK,0)) < 0)
  {
   fprintf(stderr,"Cannot open %s:%s",pcm_name,snd_strerror(err));
   return 0;
  }
 else
  {
   unsigned int want = samp_rate;
   int dir = 0;
   snd_pcm_hw_params_malloc(&hwparams);
   snd_pcm_hw_params_any(pcm,hwparams);
   /* Check capabilities */
   if ((err = snd_pcm_hw_params_set_access(pcm, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
     fprintf(stderr,"Cannot set access %s:%s",pcm_name,snd_strerror(err));
     return 0;
    }
   /* Set sample format */
   if ((err=snd_pcm_hw_params_set_format(pcm, hwparams, SND_PCM_FORMAT_S16)) < 0)
    {
     fprintf(stderr,"Error setting format %s:%s",pcm_name,snd_strerror(err));
     return(0);
    }
#ifdef ALSA_PCM_OLD_HW_PARAMS_API
   want = snd_pcm_hw_params_set_rate_near(pcm, hwparams, want, &dir);
#else
   err = snd_pcm_hw_params_set_rate_near(pcm, hwparams, &want, &dir);
#endif
   if (dir != 0 || want != samp_rate)
    {
     fprintf(stderr,"Wanted %ldHz, got %uHz (%d)",samp_rate,want,dir);
     samp_rate = want;
    }
   if ((err=snd_pcm_hw_params_set_channels(pcm, hwparams, 1)) < 0)
    {
     fprintf(stderr,"Error setting channels %s:%s",pcm_name,snd_strerror(err));
     return(0);
    }
   /* Apply HW parameter settings to */
   /* PCM device and prepare device  */
   if ((err=snd_pcm_hw_params(pcm, hwparams)) < 0)
    {
     fprintf(stderr,"Error setting parameters %s:%s",pcm_name,snd_strerror(err));
     return(0);
    }
#ifdef ALSA_PCM_OLD_HW_PARAMS_API
   chunk = snd_pcm_hw_params_get_buffer_size (hwparams);
#else
   snd_pcm_hw_params_get_buffer_size (hwparams,&chunk);
#endif
   return 1;
  }
}

int
audio_init(int argc, char *argv[])
{
 int rate_set = 0;
 int use_audio = 1;

 prog = argv[0];

 argc = getargs("ALSA Sound driver",argc, argv,
                "r", "%d", &rate_set,    "Sample rate",
                "A", "default", &pcm_name, "Device",
                "a", NULL, &use_audio,   "Audio enable",
                NULL);

 if (help_only)
  return argc;

 if (rate_set)
  samp_rate = rate_set;

 if (use_audio)
  audio_open();

 return argc;
}

void
audio_term(void)
{
 /* Finish linear file */
 if (linear_fd >= 0)
  {
   ftruncate(linear_fd, lseek(linear_fd, 0L, SEEK_CUR));
   close(linear_fd);
   linear_fd = -1;
  }

 /* Close audio system  */
 if (pcm)
  {
   int err = snd_pcm_drain(pcm);
   if (err < 0)
    {
     fprintf(stderr,"%s:%s\n",pcm_name,snd_strerror(err));
    }
   if ((err = snd_pcm_close(pcm)) < 0)
    {
     fprintf(stderr,"%s:%s\n",pcm_name,snd_strerror(err));
    }
   pcm = 0;
  }

 if (hwparams)
  {
   snd_pcm_hw_params_free(hwparams);
   hwparams = 0;
  }
}

void
audio_play(int n, short *data)
{
 if (n > 0)
  {
   if (linear_fd >= 0)
    {
     if (write(linear_fd, data, n) != n)
      perror("write");
    }
   if (pcm)
    {
     snd_pcm_sframes_t ret;
     while (n > 0)
      {
       size_t amount = ((size_t) n > chunk) ? chunk : (size_t) n;
       while ((ret = snd_pcm_writei(pcm, data, amount)) < 0)
        {
         /* Pipe may well drain - but harmless as we write
            whole words to the pipe
          */
         if (ret != -EPIPE)
          {
           fprintf(stderr,"%s:%s\n",pcm_name,snd_strerror(ret));
          }
         snd_pcm_prepare(pcm);
        }
       n -= ret;
       data += ret;
      }
    }
  }
}
