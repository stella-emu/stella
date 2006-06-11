/*
    Copyright (c) 1994, 2002 Nick Ing-Simmons. All rights reserved.
 
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

#include <config.h>
#include <useconfig.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>

#include <fcntl.h>
#include <sys/file.h>
#include <sys/filio.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <signal.h>

#include <stropts.h>
#include <sys/ioctl.h>


#ifdef HAVE_SYS_IOCCOM_H
#include <sys/ioccom.h>
#endif
#ifdef HAVE_SYS_AUDIOIO_H
#include <sys/audioio.h>
#endif
#ifdef HAVE_SUN_AUDIOIO_H
#include <sun/audioio.h>
#endif
#include "l2u.h"
#include "getargs.h"
#include "hplay.h"

#define SAMP_RATE 8000
long samp_rate = SAMP_RATE;

/* Audio Parameters */

int Verbose = 0;   /* verbose messages */
int Wait = 1; /* Should we hang waiting for device ? */

static int async = 1;

static audio_info_t dev_info;     /* audio header for device */

#ifdef AUDIO_DEV_AMD
static int dev_kind = AUDIO_DEV_AMD;
#endif

static int dev_fd = -1;           /* file descriptor for audio device */
char *dev_file = "/dev/audio";

char *Ifile;                      /* current filename */

static RETSIGTYPE audio_catch(int);

static RETSIGTYPE
audio_catch(sig)
int sig;
{
 fprintf(stderr, "signal\n");
}

static int audio_open
(void)
{
 /* Try it quickly, first */
 dev_fd = open(dev_file, O_WRONLY | O_NDELAY);
 if ((dev_fd < 0) && (errno == EBUSY))
  {
   if (!Wait)
    {
     if (Verbose)
      fprintf(stderr, "%s: %s is busy\n", program, dev_file);
     exit(1);
    }
   if (Verbose)
    {
     fprintf(stderr, "%s: waiting for %s...", program, dev_file);
     (void) fflush(stderr);
    }

   /* Now hang until it's open */

   dev_fd = open(dev_file, O_WRONLY);
   if (Verbose)
    fprintf(stderr, "%s\n", (dev_fd < 0) ? "" : "open");
  }
 else if (dev_fd >= 0)
  {
   int flags = fcntl(dev_fd, F_GETFL, NULL);
   if (flags >= 0)
    fcntl(dev_fd, F_SETFL, flags & ~O_NDELAY);
   else
    perror("fcntl - F_GETFL");
  }
 if (dev_fd < 0)
  {
   fprintf(stderr, "%s: error opening ", program);
   perror(dev_file);
   return 0;
  }
 else
  {
#ifdef AUDIO_DEV_AMD
   /* Get the device output encoding configuration */
   if (ioctl(dev_fd, AUDIO_GETDEV, &dev_kind))
    {
     /* Old releases of SunOs don't support the ioctl,
        but can only be run on old machines which have AMD device...
      */
     dev_kind = AUDIO_DEV_AMD;
    }
#endif

   if (ioctl(dev_fd, AUDIO_GETINFO, &dev_info) != 0)
    {
     perror(dev_file);
     return 0;
    }
  }
 return 1;
}

int
audio_init(argc, argv)
int argc;
char *argv[];
{
 int rate_set = 0;
 int use_linear = 0;
 int use_audio = 1;
 double gain = -1.0;
 int headphone = 2;
 int speaker = 2;


 argc = getargs("Sun Audio",argc, argv,
                "g", "%lg", &gain,     "Gain 0 .. 0.1",
                "r", "%d", &rate_set,  "Sample rate",
                "h", NULL, &headphone, "Headphones",
                "s", NULL, &speaker,   "Speaker",
                "W", NULL, &Wait,      "Wait till idle",
                "a", NULL, &use_audio, "Use audio", 
                "L", NULL, &use_linear,"Force linear",
                NULL);

 if (help_only)
  return argc;

 if (rate_set)
  {
   samp_rate = rate_set;
  }

 if (use_audio && audio_open())
  {
   if (!rate_set)
    samp_rate = dev_info.play.sample_rate;

   if (rate_set || use_linear)
    {
     dev_info.play.sample_rate = samp_rate;
#ifdef AUDIO_ENCODING_LINEAR
     if (samp_rate > 8000 || use_linear)
      {
       dev_info.play.encoding = AUDIO_ENCODING_LINEAR;
       dev_info.play.precision = 16;
      }
#endif
     if (ioctl(dev_fd, AUDIO_SETINFO, &dev_info) != 0)
      {
       perror(dev_file);
       close(dev_fd);
       dev_fd = -1;
      }
    }
  }
 if (dev_fd >= 0)
  {
   if (gain >= 0.0)
    {
     dev_info.play.gain = (unsigned) (AUDIO_MAX_GAIN * gain);
     if (ioctl(dev_fd, AUDIO_SETINFO, &dev_info) != 0)
      perror("gain");
    }

   if (speaker != 2 || headphone != 2)
    {
     if (headphone != 2)
      {
       if (headphone)
        dev_info.play.port |= AUDIO_HEADPHONE;
       else
        dev_info.play.port &= ~AUDIO_HEADPHONE;
      }

     if (speaker != 2)
      {
       if (speaker)
        dev_info.play.port |= AUDIO_SPEAKER;
       else
        dev_info.play.port &= ~AUDIO_SPEAKER;
      }

     if (ioctl(dev_fd, AUDIO_SETINFO, &dev_info) != 0)
      perror("port");
    }

   if (async)
    {
     int flag = 1;
     /* May need to use streams calls to send a SIGPOLL when write
        buffer is no longer full and use non-blocking writes, and
        manipluate our own buffer of unwritten data.

        However, at present just use FIOASYNC which means write
        returns as soon as it can queue the data (I think).
      */
     signal(SIGIO, audio_catch);
     ioctl(dev_fd, FIOASYNC, &flag);
    }
  }
 return argc;
}

void
audio_term()
{
 /* Close audio system  */
 if (dev_fd >= 0)
  {
   ioctl(dev_fd, AUDIO_DRAIN, 0);
   close(dev_fd);
   dev_fd = -1;
   if (async)
    signal(SIGPOLL, SIG_DFL);
  }
}

void
audio_play(n, data)
int n;
short *data;
{
 if (n > 0 && dev_fd >= 0)
  {
#ifdef AUDIO_ENCODING_LINEAR
   if (dev_info.play.encoding == AUDIO_ENCODING_LINEAR)
    {
     unsigned size = n * sizeof(short);
     if (write(dev_fd, data, n * sizeof(short)) != size)
            perror("write");
    }
   else 
#endif
   if (dev_info.play.encoding == AUDIO_ENCODING_ULAW)
    {
     unsigned char *plabuf = (unsigned char *) malloc(n);
     if (plabuf)
      {
       int w;
       unsigned char *p = plabuf;
       unsigned char *e = p + n;
       while (p < e)
        {
         *p++ = short2ulaw(*data++);
        }
       p = plabuf;
       while ((w = write(dev_fd, p, n)) != n)
        {
         if (w == -1)
          {
           fprintf(stderr, "%d,%s:%d\n", errno, __FILE__, __LINE__);
           perror("audio");
           abort();
          }
         else
          {
           fprintf(stderr, "Writing %u, only wrote %u\n", n, w);
           p += w;
           n -= w;
          }
        }
       free(plabuf);
      }
     else
      {
       fprintf(stderr, "%s : No memory for ulaw data\n", program);
      }
    }
  }
}
