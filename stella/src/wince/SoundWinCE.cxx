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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// Windows CE Port by Kostas Nakos
//============================================================================

#ifdef SOUND_SUPPORT

#include "TIASnd.hxx"
#include "FrameBuffer.hxx"
#include "Serializer.hxx"
#include "Deserializer.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "OSystem.hxx"
#include "TIASnd.hxx"

#include "SoundWinCE.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundWinCE::SoundWinCE(OSystem* osystem)
    : Sound(osystem),
      myIsEnabled(osystem->settings().getBool("sound")),
      myIsInitializedFlag(false),
      myLastRegisterSetCycle(0),
      myDisplayFrameRate(60),
      myNumChannels(1),
      myFragmentSizeLogBase2(0),
      myIsMuted(false),
      myVolume(100),
	  myLatency(100), myMixRate(22050), myBuffnum(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundWinCE::~SoundWinCE()
{
  close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::setEnabled(bool state)
{
  myIsEnabled = state;
  myOSystem->settings().setBool("sound", state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::initialize()
{
  if(!myIsEnabled)
  {
    close();
    return;
  }

  myRegWriteQueue.clear();
  myTIASound.reset();

  // init wave out
  WAVEFORMATEX wf;
  MMRESULT err;
  memset(&wf, 0, sizeof(wf));
  wf.wFormatTag = WAVE_FORMAT_PCM;
  wf.nChannels = 1;
  wf.nSamplesPerSec = myMixRate;
  wf.nAvgBytesPerSec = myMixRate;
  wf.nBlockAlign = 2;
  wf.wBitsPerSample = 8;
  wf.cbSize = 0;
  err = waveOutOpen(&myWout, WAVE_MAPPER, &wf, NULL, NULL, CALLBACK_NULL);
  if (err != MMSYSERR_NOERROR)
      return;

  myBuffnum = ((wf.nAvgBytesPerSec * myLatency / 1000) >> 9) + 1;
  myBuffers = (WAVEHDR *) malloc(myBuffnum * sizeof(*myBuffers));
  for (int i = 0; i < myBuffnum; i++)
  {
    memset(&myBuffers[i], 0, sizeof (myBuffers[i]));
    if (!(myBuffers[i].lpData = (LPSTR) malloc(512)))
  	  return;
	memset(myBuffers[i].lpData, 128, 512);
    myBuffers[i].dwBufferLength = 512;
    err = waveOutPrepareHeader(myWout, &myBuffers[i], sizeof(myBuffers[i]));
    if (err != MMSYSERR_NOERROR)
	  return;
    myBuffers[i].dwFlags |= WHDR_DONE;
  }

  myIsInitializedFlag = true;
  myIsMuted = false;

  myTIASound.outputFrequency(myMixRate);
  myTIASound.channels(1);

  for (i=0; i<myBuffnum; i++)
	  waveOutWrite(myWout, &myBuffers[i], sizeof(WAVEHDR));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::close()
{
  if(myIsInitializedFlag)
  {
	int flag;
	do 
	{
		flag = true;
		for (int i=0; i<myBuffnum; i++)
		{
			if (!(myBuffers[i].dwFlags & WHDR_DONE))
			{
				flag = false;
				Sleep(myLatency);
			}
		}
	} while (flag = false);

	waveOutReset(myWout);

	for (int i=0; i<myBuffnum; i++)
	{
		if (waveOutUnprepareHeader(myWout, &myBuffers[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
			return;
		free(myBuffers[i].lpData);
	}
	free(myBuffers);
	waveOutClose(myWout);

    myIsInitializedFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundWinCE::isSuccessfullyInitialized() const
{
  return myIsInitializedFlag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::mute(bool state)
{
  if(myIsInitializedFlag)
  {
    if(myIsMuted == state)
      return;
    myIsMuted = state;
    myRegWriteQueue.clear();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::reset()
{
  if(myIsInitializedFlag)
  {
    myIsMuted = false;
    myLastRegisterSetCycle = 0;
    myRegWriteQueue.clear();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::setVolume(Int32 percent)
{
  if(myIsInitializedFlag)
  {
    if((percent >= 0) && (percent <= 100))
    {
      myOSystem->settings().setInt("volume", percent);
      myVolume = percent;
      myTIASound.volume(percent);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::adjustVolume(Int8 direction)
{
  Int32 percent = myVolume;

  if(direction == -1)
    percent -= 2;
  else if(direction == 1)
    percent += 2;

  if((percent < 0) || (percent > 100))
    return;

  setVolume(percent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::adjustCycleCounter(Int32 amount)
{
  myLastRegisterSetCycle += amount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::setChannels(uInt32 channels)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::setFrameRate(uInt32 framerate)
{
  myDisplayFrameRate = framerate;
  myLastRegisterSetCycle = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::set(uInt16 addr, uInt8 value, Int32 cycle)
{
  float delta = (((double)(cycle - myLastRegisterSetCycle)) / (1193191.66666667));
  delta = delta * (myDisplayFrameRate / (double)myOSystem->frameRate());
  RegWrite info;
  info.addr = addr;
  info.value = value;
  info.delta = delta;
  myRegWriteQueue.enqueue(info);
  myLastRegisterSetCycle = cycle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::processFragment(uInt8* stream, Int32 length)
{
  if(!myIsInitializedFlag)
    return;

  float position = 0.0;
  float remaining = length;

  if(myRegWriteQueue.duration() > 
      (9.0 / myDisplayFrameRate))
  {
    float removed = 0.0;
    while(removed < ((9.0 - 1) / myDisplayFrameRate))
    {
      RegWrite& info = myRegWriteQueue.front();
      removed += info.delta;
      myTIASound.set(info.addr, info.value);
      myRegWriteQueue.dequeue();
    }
//    cout << "Removed Items from RegWriteQueue!" << endl;
  }

  while(remaining > 0.0)
  {
    if(myRegWriteQueue.size() == 0)
    {
      myTIASound.process(stream + ((uInt32)position),length - (uInt32)position);
      myLastRegisterSetCycle = 0;
      break;
    }
    else
    {
      RegWrite& info = myRegWriteQueue.front();
      float duration = remaining / (float) myMixRate;
      if(info.delta <= duration)
      {
        if(info.delta > 0.0)
        {
          float samples = (myMixRate * info.delta);
          myTIASound.process(stream + ((uInt32)position),
              (uInt32)samples + (uInt32)(position + samples) - 
              ((uInt32)position + (uInt32)samples));
          position += samples;
          remaining -= samples;
        }
        myTIASound.set(info.addr, info.value);
        myRegWriteQueue.dequeue();
      }
      else
      {
        myTIASound.process(stream + ((uInt32)position), length - (uInt32)position);
        info.delta -= duration;
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*void CALLBACK SoundWinCE::waveOutProc(HWAVEOUT hwo,	UINT uMsg, DWORD dwInstance,
												DWORD dwParam1,	DWORD dwParam2)
{
	if (uMsg == WOM_DONE)
	{
		SoundWinCE *sound = (SoundWinCE *) dwInstance;
		sound->processFragment( (uInt8 *) ((WAVEHDR *) dwParam2)->lpData, 512);
		waveOutWrite(sound->myWout, (WAVEHDR *) dwParam2, sizeof(WAVEHDR));
	}
}
*/
void SoundWinCE::update(void)
{
	if (myIsMuted) return;
	for (int i=0; i<myBuffnum; i++)
		if (myBuffers[i].dwFlags & WHDR_DONE)
		{
			processFragment((uInt8 *) myBuffers[i].lpData, 512);
			waveOutWrite(myWout, &myBuffers[i], sizeof(WAVEHDR));
		}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundWinCE::load(Deserializer& in)
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundWinCE::save(Serializer& out)
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundWinCE::RegWriteQueue::RegWriteQueue(uInt32 capacity)
    : myCapacity(capacity),
      myBuffer(0),
      mySize(0),
      myHead(0),
      myTail(0)
{
  myBuffer = new RegWrite[myCapacity];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundWinCE::RegWriteQueue::~RegWriteQueue()
{
  delete[] myBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::RegWriteQueue::clear()
{
  myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::RegWriteQueue::dequeue()
{
  if(mySize > 0)
  {
    myHead = (myHead + 1) % myCapacity;
    --mySize;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float SoundWinCE::RegWriteQueue::duration()
{
  float duration = 0.0;
  for(uInt32 i = 0; i < mySize; ++i)
  {
    duration += myBuffer[(myHead + i) % myCapacity].delta;
  }
  return duration;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::RegWriteQueue::enqueue(const RegWrite& info)
{
  if(mySize == myCapacity)
    grow();
  myBuffer[myTail] = info;
  myTail = (myTail + 1) % myCapacity;
  ++mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RegWrite& SoundWinCE::RegWriteQueue::front()
{
  assert(mySize != 0);
  return myBuffer[myHead];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundWinCE::RegWriteQueue::size() const
{
  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundWinCE::RegWriteQueue::grow()
{
  RegWrite* buffer = new RegWrite[myCapacity * 2];
  for(uInt32 i = 0; i < mySize; ++i)
  {
    buffer[i] = myBuffer[(myHead + i) % myCapacity];
  }
  myHead = 0;
  myTail = mySize;
  myCapacity = myCapacity * 2;
  delete[] myBuffer;
  myBuffer = buffer;
}

#endif
