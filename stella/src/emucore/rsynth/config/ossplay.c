/*
    Copyright (c) 1991, H.F. Silverman, A. Smith, Rob W. W. Hooft.
    Copyright (c) 1994,1999,2001-2002 Nick Ing-Simmons. 
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

/******************************************************************
***
***    Play out a file on Linux
***
***                H.F. Silverman 1/4/91
***    Modified:   H.F. Silverman 1/16/91 for amax parameter
***    Modified:   A. Smith 2/14/91 for 8kHz for klatt synth
***    Modified:   Rob W. W. Hooft (hooft@EMBL-Heidelberg.DE)
***                adapted for linux soundpackage Version 2.0
***    Merged FreeBSD version - 11/11/94 NIS
***    Re-worked for OSS now I have one myself - NIS Sept. 1999
***    Re-worked again to use linear if we can figure out that
***    device supports it. Also turn off O_NONBLOCK and O_NDELAY
***    after open
***
*******************************************************************/

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

#include <sys/ioctl.h>

#ifdef HAVE_SYS_SOUNDCARD_H      /* linux style */
#include <sys/soundcard.h>
#endif

#ifdef HAVE_MACHINE_SOUNDCARD_H  /* FreeBSD style */
#include <machine/soundcard.h>
#endif

#ifndef AFMT_S16_LE
#define AFMT_S16_LE	16
#endif

#ifndef AFMT_U8
#define AFMT_U8		8
#endif

#ifndef AFMT_MU_LAW
#define AFMT_MU_LAW	1
#endif

/* file descriptor for audio device */

#include "l2u.h"

#if defined(HAVE_DEV_DSP)
 char *dev_file = "/dev/dsp";
 static int dev_fmt = AFMT_U8;
#else /* DSP */
 #if defined(HAVE_DEV_AUDIO)
  char *dev_file = "/dev/audio";
  static int dev_fmt = AFMT_MU_LAW;
 #else
  #if defined(HAVE_DEV_DSPW) || !defined(HAVE_DEV_SBDSP)
   char *dev_file = "/dev/dspW";
   static int dev_fmt = AFMT_S16_LE;
  #else
   #if defined(HAVE_DEV_SBDSP)
    char *dev_file = "/dev/sbdsp";
    static int dev_fmt = AFMT_U8;
   #endif  /* SBDSP */
  #endif /* DSPW */
 #endif /* AUDIO */
#endif /* DSP */

#include "getargs.h"
#include "hplay.h"

#define SAMP_RATE 11025
long samp_rate = SAMP_RATE;

/* Audio Parameters */

static int dev_fd = -1;
static int linear_fd = -1;

char *prog = "hplay";

static const short endian = 0x1234;

static int
audio_open(void)
{
 int attempt;
 for (attempt = 0; attempt < 50; attempt++)
  {
   dev_fd = open(dev_file, O_WRONLY | O_NDELAY | O_EXCL);
   if (dev_fd >= 0 || errno != EBUSY)
    break;
   usleep(10000);  
  } 
 if (dev_fd >= 0)
  {
   /* Modern /dev/dsp honours O_NONBLOCK and O_NDELAY for write which
      leads to data being dropped if we try and write and device isn't ready
      we would either have to retry or we can just turn it off ...
    */
   int fl = fcntl(dev_fd,F_GETFL,0);
   if (fl != -1)
    {
     if (fcntl(dev_fd,F_SETFL,fl & ~(O_NONBLOCK|O_NDELAY)) == 0)
      {
#ifdef SNDCTL_DSP_GETFMTS
       int fmts;
       if (ioctl(dev_fd,SNDCTL_DSP_GETFMTS,&fl) == 0)
        {
         fmts = fl;
         if (*((const char *)(&endian)) == 0x34)
          {
           if ((fl = fmts & AFMT_S16_LE) && ioctl(dev_fd,SNDCTL_DSP_SETFMT,&fl) == 0)
            {
             dev_fmt = fl;
             return 1;
            }
           }
         else
          {
           if ((fl = fmts & AFMT_S16_BE) && ioctl(dev_fd,SNDCTL_DSP_SETFMT,&fl) == 0)
            {
             dev_fmt = fl;
             return 1;
            }
          }
         if ((fl = fmts & AFMT_MU_LAW) && ioctl(dev_fd,SNDCTL_DSP_SETFMT,&fl) == 0)
          {
           dev_fmt = fl;
           return 1;
          }
        }
 #endif
       fprintf(stderr,"Using %s on %d fl=%X\n",dev_file,dev_fd,fl);
       return 1;
      }
    }
  }
 perror(dev_file);
 return 0;
}

int
audio_init(int argc, char *argv[])
{
 int rate_set = 0;
 int use_audio = 1;

 prog = argv[0];

 argc = getargs("Sound driver",argc, argv,
                "r", "%d", &rate_set,    "Sample rate",
                "a", NULL, &use_audio,   "Audio enable",
                NULL);

 if (help_only)
  return argc;

 if (use_audio)
  audio_open();

 if (rate_set)
  samp_rate = rate_set;

 if (dev_fd > 0)
  {
   int want = samp_rate;
   ioctl(dev_fd, SNDCTL_DSP_SPEED, &samp_rate);
   if (samp_rate != want)
    printf("Actual sample rate: %ld\n", samp_rate);
  }

 return argc;
}

void
audio_term(void)
{
 int dummy;

 /* Close audio system  */
 if (dev_fd >= 0)
  {
   ioctl(dev_fd, SNDCTL_DSP_SYNC, &dummy);
   close(dev_fd);
   dev_fd = -1;
  }

 /* Finish linear file */
 if (linear_fd >= 0)
  {
   ftruncate(linear_fd, lseek(linear_fd, 0L, SEEK_CUR));
   close(linear_fd);
   linear_fd = -1;
  }
}

void
audio_play(int n, short *data)
{
 if (n > 0)
  {
   if (dev_fmt == AFMT_S16_LE || dev_fmt == AFMT_S16_BE)
    {
     n *= 2;
     if (dev_fd >= 0)
      {
       if (write(dev_fd, data, n) != n)
        perror("write");
      }
     if (linear_fd >= 0)
      {
       if (write(linear_fd, data, n) != n)
        perror("write");
      }
    }
   else if (dev_fmt == AFMT_U8)
    {
     unsigned char *converted = (unsigned char *) malloc(n);
     int i;

     if (converted == NULL)
      {
       fprintf(stderr, "Could not allocate memory for conversion\n");
       exit(3);
      }

     for (i = 0; i < n; i++)
      converted[i] = (data[i] - 32767) / 256;

     if (linear_fd >= 0)
      {
       if (write(linear_fd, converted, n) != n)
        perror("write");
      }

     if (dev_fd >= 0)
      {
       if (write(dev_fd, converted, n) != n)
        perror("write");
      }
     free(converted);
    }
   else if (dev_fmt == AFMT_MU_LAW)
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
         if (w == -1 && errno != EINTR)
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
   else
    {
     fprintf(stderr, "%s : unknown audio format\n", program);
    }
  }
}
