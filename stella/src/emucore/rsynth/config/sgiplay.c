#include <config.h>
#include <useconfig.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/signal.h>

#include <stropts.h>
#include <sys/ioctl.h>

#define TRUE 1
#define FALSE 0
#include <audio.h>
#include "getargs.h"
#include "hplay.h"

#define SAMP_RATE 11025
long samp_rate = SAMP_RATE;

/* Audio Parameters */

int Verbose = FALSE;              /* verbose messages */
int Immediate = FALSE;            /* Should we hang waiting for device ? */

static int async = TRUE;
char *Ifile;                      /* current filename */

static ALconfig alconf;
static ALport alprt;

/* sgi system doesn't need to be opened */

static int audio_open
(void)
{
 long pvbuf[2];
 long buflen;

 pvbuf[0] = AL_OUTPUT_RATE;
 pvbuf[1] = samp_rate;
 buflen = 2;
 ALsetparams(AL_DEFAULT_DEVICE, pvbuf, buflen);

 alconf = ALnewconfig();

 ALsetwidth(alconf, AL_SAMPLE_16);
 ALsetchannels(alconf, AL_MONO);
 alprt = ALopenport("say", "w", alconf);
 if (alprt == NULL)
  {
   fprintf(stderr, "cannot open audio port\n");
   ALfreeconfig(alconf);
   return 1;
  }
 return 0;
}

/* sgi system */
int
audio_init(int argc, char **argv)
{
 argc = getargs("Sun Audio",argc, argv,
                /* Really should support sample rate ... */
                NULL);
 if (help_only)
  return argc;
 audio_open();
 return argc;
}

void
audio_term(void)
{
 /* on sgi systems, wait for port to complete */
 while (ALgetfilled(alprt) != 0)
  {
   sleep(1);
  }
 ALcloseport(alprt);
}

void
audio_play(int n, short *data)
{
 ALwritesamps(alprt, (void *) data, (long) n);
}
