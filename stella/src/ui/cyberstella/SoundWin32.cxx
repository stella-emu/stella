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
// $Id: SoundWin32.cxx,v 1.3 2003-11-19 21:06:27 stephena Exp $
//============================================================================

#include <assert.h>

#include <dsound.h>

#include "bspf.hxx"
#include "MediaSrc.hxx"
#include "SoundWin32.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundWin32::SoundWin32()
    : myIsInitializedFlag(false),
      myBufferSize(512),
      mySampleRate(31400),
      myDSBuffer(NULL)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundWin32::~SoundWin32()
{
  closeDevice();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HRESULT SoundWin32::Initialize(HWND hWnd)
{
  HRESULT hr;

  // Create IDirectSound using the primary sound device
  if( FAILED( hr = DirectSoundCreate8( NULL, &myDSDevice, NULL ) ) )
  {
    SoundError("DirectSoundCreate8");
    return hr;
  }

  // Set DirectSound coop level 
  if( FAILED(hr = myDSDevice->SetCooperativeLevel(hWnd, DSSCL_PRIORITY)) )
  {
    SoundError("SetCooperativeLevel");
    return hr;
  }

  // Set up the static sound buffer
  WAVEFORMATEX wfx;
  DSBUFFERDESC dsbdesc;

  ZeroMemory(&wfx, sizeof(wfx));
  wfx.wFormatTag = WAVE_FORMAT_PCM;
  wfx.nChannels = 1;
  wfx.nSamplesPerSec = mySampleRate;
  wfx.wBitsPerSample = 8;
  wfx.nBlockAlign = 1;      //wfx.wBitsPerSample / 8 * wfx.nChannels;
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
   
  ZeroMemory(&dsbdesc, sizeof(dsbdesc));
  dsbdesc.dwSize = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_CTRLVOLUME;
  dsbdesc.dwBufferBytes = myBufferSize;
  dsbdesc.lpwfxFormat = &wfx;

  hr = myDSDevice->CreateSoundBuffer(&dsbdesc, &myDSBuffer, NULL);
  if(SUCCEEDED(hr)) 
    myIsInitializedFlag = true;

  return hr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWin32::closeDevice()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundWin32::getSampleRate() const
{
  return myIsInitializedFlag ? mySampleRate : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundWin32::isSuccessfullyInitialized() const
{
  return myIsInitializedFlag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWin32::setVolume(Int32 percent)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWin32::update()
{
  if(myIsInitializedFlag)
  {
    HRESULT hr;
    uInt8 periodCount = 0;
    uInt8* buffer = new uInt8[myBufferSize];

    // Dequeue samples as long as full fragments are available
    while(myMediaSource->numberOfAudioSamples() >= myBufferSize)
    {
      myMediaSource->dequeueAudioSamples(buffer, myBufferSize);

      LPVOID lpvWrite;
      DWORD  dwLength;
      hr = myDSBuffer->Lock(0, 0, &lpvWrite, &dwLength, NULL, NULL, DSBLOCK_ENTIREBUFFER);
      if(hr == DS_OK)
      {
        memcpy(lpvWrite, buffer, dwLength);
        myDSBuffer->Unlock(lpvWrite, dwLength, NULL, 0);
        myDSBuffer->SetCurrentPosition(0);
        myDSBuffer->Play(0, 0, 0);
        periodCount++;
      }
    }
    delete[] buffer;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWin32::SoundError(const char* message)
{
  cout << "ERROR in SOUND: " << message << endl;
  myIsInitializedFlag = false;
}

/*    // Fill any unused fragments with silence so that we have a lower
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
    }*/
