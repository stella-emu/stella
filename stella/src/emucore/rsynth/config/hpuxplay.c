#include <config.h>
/*****************************************************************/
/***                                                           ***/
/***    Play out a file on an HP                               ***/
/***                                                           ***/
/***    accesses audio device directly,                        ***/
/***    should use Aserver & Alib instead                      ***/
/***                                                           ***/
/***    Markus.Gyger@itr.ch  23 Jul 94                         ***/
/***                                                           ***/
/*****************************************************************/


#include <useconfig.h>
#include <stdio.h>
#include <fcntl.h>

/* Force this on as otherwise <sys/ioctl.h> may not
   expand enough for <sys/audio.h> to work as expected,
   and that is after all the *whole* point of this file.
*/
#define _INCLUDE_HPUX_SOURCE
#include <sys/ioctl.h>
#include <sys/audio.h>
#include <errno.h>
#include <limits.h>

#include "getargs.h"
#include "hplay.h"
#include "getargs.h"

#define AUDIO_DEVICE "/dev/audio"
#define SAMP_RATE    10000        /* desired, will be 11025 or 8000 */
long samp_rate = SAMP_RATE;
static int audioDescriptor = -1;

int
audio_init(argc, argv)
int argc;
char *argv[];
{
 int i,
     sr,
     headphone = 2,
     speaker = 2,
     waitidle = 0;
 struct audio_describe audioInfo;
 struct audio_gain audioGain;
 audioGain.cgain[0].transmit_gain = INT_MIN;	/* never used value */

 /* options g/h/s aren't needed if AudioCP or acontrol is used */
 argc = getargs("HP-UX Audio",argc, argv,
                "r", "%ld", &samp_rate,  "Sample rate",
		"g", "%d", &audioGain.cgain[0].transmit_gain, "Gain -128..127",
		"h", NULL, &headphone, "Headphones",
		"s", NULL, &speaker,   "Speaker",
		"W", NULL, &waitidle,  "Wait until idle (not implemented)",
		NULL);

 if (help_only)
  return argc;

 /* open audio device */
 audioDescriptor = open(AUDIO_DEVICE, O_WRONLY);
 if (audioDescriptor == -1)
  {
   perror("open");
   exit(1);
  }

 /* get list of available sample rates */
 if (ioctl(audioDescriptor, AUDIO_DESCRIBE, &audioInfo) == -1)
  {
   perror("AUDIO_DESCRIBE");
   exit(1);
  }

 /* set output gain */
 if (audioGain.cgain[0].transmit_gain != INT_MIN)
  {
   if (audioGain.cgain[0].transmit_gain < AUDIO_OFF_GAIN)
    audioGain.cgain[0].transmit_gain = AUDIO_OFF_GAIN;
   if (audioGain.cgain[0].transmit_gain > AUDIO_MAX_GAIN)
    audioGain.cgain[0].transmit_gain = AUDIO_MAX_GAIN;

   audioGain.channel_mask = AUDIO_CHANNEL_0;
   audioGain.cgain[0].receive_gain = AUDIO_OFF_GAIN;
   audioGain.cgain[0].monitor_gain = AUDIO_OFF_GAIN;
   if (ioctl(audioDescriptor, AUDIO_SET_GAINS, &audioGain) == -1)
    {
     perror("AUDIO_SET_GAINS");
     exit(1);
    }
  }

 /* select output */
 if (headphone != 2 || speaker != 2)
  {
   int outSel;

   if (ioctl(audioDescriptor, AUDIO_GET_OUTPUT, &outSel) == -1)
    {
     perror("AUDIO_GET_OUTPUT");
     exit(1);
    }

   if (headphone != 2)
    if (headphone)
     outSel |= AUDIO_OUT_EXTERNAL;
    else
     outSel &= ~AUDIO_OUT_EXTERNAL;

   if (speaker != 2)              /* internal and external speakers */
    if (speaker)
     outSel |= (AUDIO_OUT_INTERNAL | AUDIO_OUT_LINE);
    else
     outSel &= ~(AUDIO_OUT_INTERNAL | AUDIO_OUT_LINE);

   if (ioctl(audioDescriptor, AUDIO_SET_OUTPUT, outSel) == -1)
    {
     perror("AUDIO_SET_OUTPUT");
     exit(1);
    }
  }

 /* switch to 16 bit samples (710 converts to u-law) */
 if (ioctl(audioDescriptor, AUDIO_SET_DATA_FORMAT, AUDIO_FORMAT_LINEAR16BIT) == -1)
  {
   perror("AUDIO_SET_DATA_FORMAT");
   exit(1);
  }

 /* choose nearest available sample rate (710 has 8 kHz only) */
 sr = audioInfo.sample_rate[0];
 for (i = 1; i < audioInfo.nrates; i++)
  if (abs(audioInfo.sample_rate[i] - samp_rate) < abs(sr - samp_rate))
   sr = audioInfo.sample_rate[i];
 samp_rate = sr;

 /* set sampling rate */
 if (ioctl(audioDescriptor, AUDIO_SET_SAMPLE_RATE, (int) samp_rate) == -1)
  {
   perror("AUDIO_SET_SAMPLE_RATE");
   exit(1);
  }

 /* switch to mono */
 if (ioctl(audioDescriptor, AUDIO_SET_CHANNELS, 1) == -1)
  {
   perror("AUDIO_SET_CHANNELS");
   exit(1);
  }

 return argc;
}

void
audio_term()
{
 if (audioDescriptor >= 0)
  {
   if (ioctl(audioDescriptor, AUDIO_DRAIN) == -1)
    {
     perror("AUDIO_DRAIN");
     exit(1);
    }
   if (close(audioDescriptor) == -1)
    {
     perror("close");
     exit(1);
    }
   audioDescriptor = -1;
  }
}

void
audio_play(n, data)
int n;
short *data;
{
 if (n > 0)
  {
   unsigned size = n * sizeof(*data);
   if (write(audioDescriptor, data, size) != size)
    perror("write");
  }
}
