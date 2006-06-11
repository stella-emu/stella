/*****************************************************************/
/*****************************************************************/
/***                                                           ***/
/***                                                           ***/
/***    Play out a  file on the NeXT                           ***/
/***                                                           ***/
/***                                                           ***/
/***                B. Stuyts 21-feb-94                        ***/
/***                                                           ***/
/***                                                           ***/
/*****************************************************************/
/*****************************************************************/

#include <sound/sound.h>
#include <stdio.h>
#include <libc.h>
#include "getargs.h"
#include "hplay.h"

#undef DEBUG

#define SAMP_RATE SND_RATE_CODEC
long samp_rate = SAMP_RATE;

SNDSoundStruct *sound = NULL;

int
audio_init(int argc, char *argv[])
{
 int err;
 int rate_set = 0;

#ifdef DEBUG
 int i;

 printf("audio_init: %d\n", argc);
 for (i = 0; i < argc; i++)
  printf("audio_init arg %d = %s\n", i, argv[i]);
#endif

 argc = getargs("NeXT audio",argc, argv,
                "r", "%d", &rate_set, "Sample rate",
                NULL);
 if (help_only)
  return argc;

 if (rate_set)
  samp_rate = rate_set;

 err = SNDAlloc(&sound, 1000000, SND_FORMAT_LINEAR_16, samp_rate, 1, 0);
 if (err)
  {
   fprintf(stderr, "audio_init: %s\n", SNDSoundError(err));
   exit(1);
  }
 return argc;
}

void
audio_play(int n, short *data)
{
 int err;

#ifdef DEBUG
 printf("audio_play: %d words\n", n);
#if 0
 printf("audio_play: sound = %ld\n", sound);
 printf("audio_play: dataLocation = %ld\n", sound->dataLocation);
 printf("audio_play: sum = %ld\n", (char *) sound + sound->dataLocation);
#endif
#endif

 if (n > 0)
  {
   /* Wait for previous sound to finish before changing
      fields in sound
   */
   err = SNDWait(0);
   if (err)
    {
     fprintf(stderr, "SNDWait: %s\n", SNDSoundError(err));
     exit(1);
    }
   sound->dataSize = n * sizeof(short);
   /* Patch from  benstn@olivetti.nl (Ben Stuyts)
      Thanks to ugubser@avalon.unizh.ch for finding out why the NEXTSTEP
      version of rsynth didn't work on Intel systems. As suspected, it was a
      byte-order   problem. 
    */
#if i386
   swab((char *) data, (char *) sound + sound->dataLocation, n * sizeof(short));
#else /* i386 */
   bcopy(data, (char *) sound + sound->dataLocation, n * sizeof(short));
#endif

   err = SNDStartPlaying(sound, 1, 5, 0, 0, 0);
   if (err)
    {
     fprintf(stderr, "audio_play: %s\n", SNDSoundError(err));
     exit(1);
    }
  }
}

void
audio_term()
{
 int err;

#ifdef DEBUG
 printf("audio_term\n");
#endif

 if(!sound)
   return;

 err = SNDWait(0);
 if (err)
  {
   fprintf(stderr, "audio_play: %s\n", SNDSoundError(err));
   exit(1);
  }

 err = SNDFree(sound);
 if (err)
  {
   fprintf(stderr, "audio_term: %s\n", SNDSoundError(err));
   exit(1);
  }
}
