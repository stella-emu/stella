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
// $Id: SoundALSA.cxx,v 1.4 2003-11-06 22:22:32 stephena Exp $
//============================================================================

#include <alsa/asoundlib.h>
#include <stdio.h>

#include "SoundALSA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundALSA::SoundALSA()
    : myIsInitializedFlag(false),
      myPcmHandle(0),
      myMixerHandle(0),
      myMixerElem(0),
      myOriginalVolumeLeft(-1),
      myOriginalVolumeRight(-1),
      myBufferSize(0),
      mySampleRate(0),
      myPauseStatus(false)
{
  Int32 err;
  char pcmName[]   = "plughw:0,0";
  char mixerName[] = "PCM";
  char mixerCard[] = "default";

  snd_pcm_hw_params_t* hwparams;

  // Allocate the snd_pcm_hw_params_t structure on the stack
  snd_pcm_hw_params_alloca(&hwparams);

  // Open the PCM device for writing
  if((err = snd_pcm_open(&myPcmHandle, pcmName, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
  {
    alsaError(err);
    return;
  }

  // Init hwparams with full configuration space
  if((err = snd_pcm_hw_params_any(myPcmHandle, hwparams)) < 0)
  {
    alsaError(err);
    return;
  }

  // Set interleaved access
  if((err = snd_pcm_hw_params_set_access(myPcmHandle, hwparams,
    SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
  {
    alsaError(err);
    return;
  }

  // Set the audio data format
  if((err = snd_pcm_hw_params_set_format(myPcmHandle, hwparams, SND_PCM_FORMAT_U8)) < 0)
  {
    alsaError(err);
    return;
  }
 
  // Set the number of audio channels to 1 (mono mode)
  if((err = snd_pcm_hw_params_set_channels(myPcmHandle, hwparams, 1)) < 0)
  {
    alsaError(err);
    return;
  }

  // Set the audio sample rate. If the exact rate is not supported by
  // the hardware, use nearest possible rate
  mySampleRate = 31400;
  if((err = snd_pcm_hw_params_set_rate_near(myPcmHandle, hwparams, mySampleRate, 0)) < 0)
  {
    alsaError(err);
    return;
  }

  // Set number of fragments to 2
  if((err = snd_pcm_hw_params_set_periods(myPcmHandle, hwparams, 2, 0)) < 0)
  {
    alsaError(err);
    return;
  }

  // Set size of fragments to 512 bytes
  myBufferSize = 512;
  if((err = snd_pcm_hw_params_set_period_size(myPcmHandle, hwparams,
    myBufferSize, 0)) < 0)
  {
    alsaError(err);
    return;
  }

  // Apply HW parameter settings to PCM device
  if((err = snd_pcm_hw_params(myPcmHandle, hwparams)) < 0)
  {
    alsaError(err);
    return;
  }
 
  ////////////////////////////////////////////////////////////
  // Now, open the mixer so we'll be able to change the volume
  ////////////////////////////////////////////////////////////

  snd_mixer_selem_id_t* mixerID;

  // Allocate simple mixer ID
  snd_mixer_selem_id_alloca(&mixerID);

  // Sets simple mixer ID and name
  snd_mixer_selem_id_set_index(mixerID, 0);
  snd_mixer_selem_id_set_name(mixerID, mixerName);

  // Open the mixer device
  if((err = snd_mixer_open(&myMixerHandle, 0)) < 0)
  {
    alsaError(err);
    return;
  }

  // Attach the mixer to the default sound card
  if((err = snd_mixer_attach(myMixerHandle, mixerCard)) < 0)
  {
    alsaError(err);
    return;
  }

  // Register the mixer with the sound system
  if((err = snd_mixer_selem_register(myMixerHandle, NULL, NULL)) < 0)
  {
    alsaError(err);
    return;
  }

  if((err = snd_mixer_load(myMixerHandle)) < 0)
  {
    alsaError(err);
    return;
  }

  // Get the mixer element that will be used to control volume
  if((myMixerElem = snd_mixer_find_selem(myMixerHandle, mixerID)) == 0)
  {
    alsaError(err);
    return;
  }

  // Save the original volume so we can restore it on exit
  snd_mixer_selem_get_playback_volume(myMixerElem, (_snd_mixer_selem_channel_id) 0,
    &myOriginalVolumeLeft);
  snd_mixer_selem_get_playback_volume(myMixerElem, (_snd_mixer_selem_channel_id) 1,
    &myOriginalVolumeRight);

  // Prepare the audio device for playback
  snd_pcm_prepare(myPcmHandle);

  // Indicate that the sound system is fully initialized
  myIsInitializedFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundALSA::~SoundALSA()
{
  closeDevice();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundALSA::closeDevice()
{
  if(myIsInitializedFlag)
  {
    // Restore original volume
    if(myMixerHandle)
    {
      if((myOriginalVolumeLeft != -1) && (myOriginalVolumeRight != -1))
      {
        snd_mixer_selem_set_playback_volume(myMixerElem, (_snd_mixer_selem_channel_id) 0,
          myOriginalVolumeLeft);
        snd_mixer_selem_set_playback_volume(myMixerElem, (_snd_mixer_selem_channel_id) 1,
          myOriginalVolumeRight);
      }

      snd_mixer_close(myMixerHandle);
    }

    if(myPcmHandle)
      snd_pcm_close(myPcmHandle);
  }

  myIsInitializedFlag = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundALSA::getSampleRate() const
{
  return myIsInitializedFlag ? mySampleRate : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundALSA::isSuccessfullyInitialized() const
{
  return myIsInitializedFlag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundALSA::setSoundVolume(Int32 percent)
{
  if(myIsInitializedFlag && myMixerElem)
  {
    if((percent >= 0) && (percent <= 100))
    {
      long int lowerBound, upperBound, newVolume;
      snd_mixer_selem_get_playback_volume_range(myMixerElem, &lowerBound, &upperBound);

      newVolume = (long int) (((upperBound - lowerBound) * percent / 100.0) + lowerBound);
      snd_mixer_selem_set_playback_volume(myMixerElem, (_snd_mixer_selem_channel_id) 0,
        newVolume);
      snd_mixer_selem_set_playback_volume(myMixerElem, (_snd_mixer_selem_channel_id) 1,
        newVolume);
    }
    else if(percent == -1)   // If -1 has been specified, play sound at default volume
    {
      myOriginalVolumeRight = myOriginalVolumeRight = -1;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundALSA::updateSound(MediaSource& mediaSource)
{
  if(myIsInitializedFlag)
  {
    if(myPauseStatus)
      return;

    snd_pcm_sframes_t frames;
    uInt8 periodCount = 0;

    // Dequeue samples as long as full fragments are available
    while(mediaSource.numberOfAudioSamples() >= myBufferSize)
    {
      uInt8 buffer[myBufferSize];
      mediaSource.dequeueAudioSamples(buffer, myBufferSize);

      if((frames = snd_pcm_writei(myPcmHandle, buffer, myBufferSize)) == -EPIPE)
      {
        snd_pcm_prepare(myPcmHandle);
        break;
      }
      periodCount++;
    }

    // Fill any unused fragments with silence so that we have a lower
    // risk of having playback underruns
    for(int i = 0; i < 1-periodCount; ++i)
    {
      frames = snd_pcm_avail_update(myPcmHandle);
      if (frames > 0)
      {
        uInt8 buffer[frames];
        memset((void*)buffer, 0, frames);
        snd_pcm_writei(myPcmHandle, buffer, frames);
      }
      else if(frames == -EPIPE)   // this should never happen
      {
        cerr << "EPIPE after write\n";
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundALSA::alsaError(Int32 error)
{
  cerr << "SoundALSA:  " << snd_strerror(error) << endl;

  if(myMixerHandle)
    snd_mixer_close(myMixerHandle);
  if(myPcmHandle)
    snd_pcm_close(myPcmHandle);

  mySampleRate = 0;
}
