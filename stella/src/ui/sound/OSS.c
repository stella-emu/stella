/*============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa  
//   SSSS     ttt  eeeee llll llll  aaaaa
//   
// Copyright (c) 1995-1998 by Bradford W. Mott
// 
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
// 
// $Id: OSS.c,v 1.1.1.1 2001-12-27 19:54:35 bwmott Exp $
//==========================================================================*/

/**
  This file implements the "stella-sound" process for the 
  Open Sound System (OSS) API.

  @author  Bradford W. Mott
  @version $Id: OSS.c,v 1.1.1.1 2001-12-27 19:54:35 bwmott Exp $
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __FreeBSD__
  #include <machine/soundcard.h>
#else
  #include <sys/soundcard.h>
#endif

#include "TIASound.h"

/**
  Compute Fragment size to use based on the sample rate 

  @param sampleRate The sample rate to compute the fragment size for
*/
unsigned long computeFragmentSize(int sampleRate);


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int main(int argc, char* argv[])
{
  int fd;
  int numberAndSizeOfFragments;
  int fragmentSize;
  unsigned char* fragmentBuffer;
  int sampleRate;
  int format;
  int stereo;
  int mute = 0;

  /* Open the sound device for writing */
  if((fd = open("/dev/dsp", O_WRONLY, 0)) == -1)
  {
    printf("stella-sound: Unable to open /dev/dsp device!\n");
    return 1;
  }

  /* Set the AUDIO DATA FORMAT */
  format = AFMT_U8;
  if(ioctl(fd, SNDCTL_DSP_SETFMT, &format) == -1)
  {
    printf("stella-sound: Unable to set 8-bit sample mode!\n");
    return 1;
  }

  if(format != AFMT_U8)
  {
    printf("stella-sound: Sound card doesn't support 8-bit sample mode!\n");
    return 1;
  }

  /* Set MONO MODE */
  stereo = 0;
  if(ioctl(fd, SNDCTL_DSP_STEREO, &stereo) == -1)
  {
    printf("stella-sound: Sound card doesn't support mono mode!\n");
    return 1;
  }
 
  if(stereo != 0)
  {
    printf("stella-sound: Sound card doesn't support mono mode!\n");
    return 1;
  }

  /* Set the SAMPLE RATE */
  sampleRate = 31400;
  if(ioctl(fd, SNDCTL_DSP_SPEED, &sampleRate) == -1)
  {
    printf("stella-sound: Unable to set sample rate for /dev/dsp!\n");
    return 1;
  }

  /* Set the NUMBER AND SIZE OF FRAGMENTS */
  numberAndSizeOfFragments = 0x00020000 | computeFragmentSize(sampleRate);
  if(ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &numberAndSizeOfFragments) == -1)
  {
    printf("stella-sound: Unable to set fragment size!\n");
    return 1;
  }
  
  /* Query for the actual fragment size */
  ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &fragmentSize);

  /* Allocate fragment buffer */
  fragmentBuffer = (unsigned char*)malloc(fragmentSize);


  /* Initialize the TIA Sound Library */
  Tia_sound_init(31400, sampleRate);


  /* Make sure STDIN is in nonblocking mode */
  if(fcntl(0, F_SETFL, O_NONBLOCK) == -1)
  {
    printf("stella-sound: Couldn't set non-blocking mode\n");
    return 1;
  }

  /* Loop reading commands from the emulator and playing sound fragments */
  for(;;)
  {
    int done = 0;

    while(!done)
    {
      int i;
      int n;
      unsigned char input[1024];

      /* Read as many commands as available */
      n = read(0, input, 1024);

      /* Process all of the commands we read */
      for(i = 0; i < n; ++i)
      {
        unsigned char value = input[i];

        switch((value >> 5) & 0x07)
        {
          case 0:    /* Set AUDC0 */
            Update_tia_sound(0x15, value);
            break;

          case 1:    /* Set AUDC1 */
            Update_tia_sound(0x16, value);
            break;

          case 2:    /* Set AUDF0 */
            Update_tia_sound(0x17, value);
            break;

          case 3:    /* Set AUDF1 */
            Update_tia_sound(0x18, value);
            break;

          case 4:    /* Set AUDV0 */
            Update_tia_sound(0x19, value);
            break;

          case 5:    /* Set AUDV1 */
            Update_tia_sound(0x1A, value);
            break;

          case 6:    /* Quit */
            close(fd);
            return 1;
            break;

          case 7:    /* Change mute command */
            mute = value & 0x01;
            break;
 
          default:
            break;
        }
      }
      done = (n != 1024);   
    } 

    /* If sound isn't muted then play something */
    if(!mute)
    {
      /* Create the next fragment to play */
      Tia_process(fragmentBuffer, fragmentSize);

      /* Write fragment to sound device */
      write(fd, fragmentBuffer, fragmentSize);
    }
    else
    {
      /* Sound is muted so let's sleep for a little while */
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 10000;
      select(0, 0, 0, 0, &timeout);
    }
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
unsigned long computeFragmentSize(int sampleRate)
{
  int t;

  for(t = 7; t < 24; ++t) 
  {
    if((1 << t) > (sampleRate / 60))
      return t - 1;
  }

  /* Default to 256 byte fragment size */
  return 8;
}

