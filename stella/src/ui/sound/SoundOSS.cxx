//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SoundOSS.cxx,v 1.1 2002-11-13 16:19:21 stephena Exp $
//============================================================================

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#ifdef __FreeBSD__
  #include <machine/soundcard.h>
#else
  #include <sys/soundcard.h>
#endif

#define DSP_DEVICE "/dev/dsp"
#define MIXER_DEVICE "/dev/mixer"

#include "SoundOSS.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundOSS::SoundOSS()
    : myIsInitializedFlag(false),
      myDspFd(-1),
      myMixerFd(-1),
      myOriginalVolume(-1),
      mySampleRate(0)
{
  // Open the sound device for writing
  if((myDspFd = open(DSP_DEVICE, O_WRONLY, 0)) == -1)
  {
    perror(DSP_DEVICE);
    return;
  }

  // Set the number and size of fragments
  int numberAndSizeOfFragments = 0x00020009;
  if(ioctl(myDspFd, SNDCTL_DSP_SETFRAGMENT, &numberAndSizeOfFragments) == -1)
  {
    perror(DSP_DEVICE);
    close(myDspFd);
    myDspFd = -1;
    return;
  }

  // Set the audio data format
  int format = AFMT_U8;
  if(ioctl(myDspFd, SNDCTL_DSP_SETFMT, &format) == -1)
  {
    perror(DSP_DEVICE);
    close(myDspFd);
    myDspFd = -1;
    return;
  }

  // Make sure the U8 format was selected
  if(format != AFMT_U8)
  {
    cerr << DSP_DEVICE << ": Doesn't support 8-bit sample format!" << endl;
    close(myDspFd);
    myDspFd = -1;
    return;
  }

  // Set the number of audio channels to 1 (mono mode)
  int channels = 1;
  if(ioctl(myDspFd, SNDCTL_DSP_CHANNELS, &channels) == -1)
  {
    perror(DSP_DEVICE);
    close(myDspFd);
    myDspFd = -1;
    return;
  }
 
  // Make sure mono mode was selected
  if(channels != 1)
  {
    cerr << DSP_DEVICE << ": Doesn't support mono mode!" << endl;
    close(myDspFd);
    myDspFd = -1;
    return;
  }

  // Set the audio sample rate
  mySampleRate = 31400;
  if(ioctl(myDspFd, SNDCTL_DSP_SPEED, &mySampleRate) == -1)
  {
    perror(DSP_DEVICE);
    close(myDspFd);
    myDspFd = -1;
    mySampleRate = 0;
    return;
  }

  // Now, open the mixer so we'll be able to change the volume 
  if((myMixerFd = open(MIXER_DEVICE, O_RDWR, 0)) == -1)
  {
    perror(MIXER_DEVICE);
  }
  else
  {
    if(ioctl(myMixerFd, MIXER_READ(SOUND_MIXER_PCM), &myOriginalVolume) == -1)
    {
      myOriginalVolume = -1;
    }
  } 

  // Indicate that the sound system is fully initialized
  myIsInitializedFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundOSS::~SoundOSS()
{
  closeDevice();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundOSS::closeDevice()
{
  if(myIsInitializedFlag)
  {
    if(myMixerFd != -1)
    {
      if(myOriginalVolume != -1)
      {
        if(ioctl(myMixerFd, MIXER_WRITE(SOUND_MIXER_PCM), 
            &myOriginalVolume) == -1)
        {
          perror(MIXER_DEVICE);
        }
      }

      close(myMixerFd);
    }

    if(myDspFd != -1)
    {
      close(myDspFd);
    }

    myIsInitializedFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundOSS::getSampleRate() const
{
  return myIsInitializedFlag ? mySampleRate : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundOSS::isSuccessfullyInitialized() const
{
  return myIsInitializedFlag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundOSS::setSoundVolume(uInt32 volume)
{
  if(myIsInitializedFlag && (myMixerFd != -1))
  {
    if(volume < 0)
    {
      volume = 0;
    }
    if(volume > 100)
    {
      volume = 100;
    }

    int v = volume | (volume << 8);
    if(ioctl(myMixerFd, MIXER_WRITE(SOUND_MIXER_PCM), &v) == -1)
    {
      perror(MIXER_DEVICE);
      close(myMixerFd);
      myMixerFd = -1;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundOSS::updateSound(MediaSource& mediaSource)
{
  if(myIsInitializedFlag)
  {
    // Get audio buffer information
    audio_buf_info info;
    if(ioctl(myDspFd, SNDCTL_DSP_GETOSPACE, &info) == -1)
    {
      return; 
    }

    // Dequeue samples as long as full fragments are available
    while(mediaSource.numberOfAudioSamples() >= (uInt32)info.fragsize)
    {
      uInt8 buffer[info.fragsize];
      mediaSource.dequeueAudioSamples(buffer, (uInt32)info.fragsize);
      write(myDspFd, buffer, info.fragsize);
    }

    // Fill any unused fragments with silence so that we have a lower
    // risk of having playback underruns
    for(;;)
    {
      // Get audio buffer information
      if(ioctl(myDspFd, SNDCTL_DSP_GETOSPACE, &info) == -1)
      {
        return; 
      }

      if(info.fragments > 0)
      {
        uInt8 buffer[info.fragsize];
        memset((void*)buffer, 0, info.fragsize);
        write(myDspFd, buffer, info.fragsize);
      }
      else
      {
        break;
      }
    }
  }
}
