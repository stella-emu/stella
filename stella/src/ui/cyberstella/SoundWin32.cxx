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
// $Id: SoundWin32.cxx,v 1.5 2003-11-24 23:56:10 stephena Exp $
//============================================================================

#include <assert.h>

#include <dsound.h>

#include "bspf.hxx"
#include "MediaSrc.hxx"
#include "SoundWin32.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundWin32::SoundWin32()
    : myIsInitializedFlag(false),
      myBufferSize(4096),
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
  if( FAILED(hr = myDSDevice->SetCooperativeLevel(hWnd, DSSCL_EXCLUSIVE)) )
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
  dsbdesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_STATIC;
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
  if(myDSBuffer)
  {
    myDSBuffer->Stop();
    myDSBuffer->Release();
    myDSBuffer = NULL;
  }

  if(myDSDevice)
  {
    myDSDevice->Release();
    myDSDevice = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundWin32::getSampleRate() const
{
  return mySampleRate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundWin32::isSuccessfullyInitialized() const
{
  return myIsInitializedFlag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWin32::setVolume(Int32 percent)
{
  // By default, use the system volume
  if(percent < 0 || percent > 100)
    return;

// FIXME - let the percentage accurately represent decibel level
//         ie, so volume 50 is half as loud as volume 100

  // Scale the volume over the given range
  if(myDSBuffer)
  {
    long offset = (long)((DSBVOLUME_MAX - DSBVOLUME_MIN) * (percent/100.0));
    myDSBuffer->SetVolume(DSBVOLUME_MIN + offset);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWin32::update()
{
  if(myIsInitializedFlag)
  {
    HRESULT hr;
    uInt8 periodCount = 0;
    uInt8* buffer = new uInt8[myBufferSize];

    if(myPauseStatus)  // FIXME - don't stop the buffer so many times
    {
      myDSBuffer->Stop();
      return;
    }

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
        myDSBuffer->Play(0, 0, DSBPLAY_LOOPING);
        periodCount++;
      }
    }
    delete[] buffer;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWin32::SoundError(const char* message)
{
  string error = "Error:  ";
  error += message;
  OutputDebugString(error.c_str());

  myIsInitializedFlag = false;
}
