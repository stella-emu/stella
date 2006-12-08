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

#ifndef SOUNDWINCE_HXX
#define SOUNDWINCE_HXX

#ifdef SOUND_SUPPORT

class OSystem;

#include "Sound.hxx"
#include "bspf.hxx"
#include "MediaSrc.hxx"
#include "TIASnd.hxx"

struct RegWrite
{
	uInt16 addr;
	uInt8 value;
    double delta;
};


class SoundWinCE : public Sound
{
  public:
    SoundWinCE(OSystem* osystem);
    virtual ~SoundWinCE();
    void setEnabled(bool state);
    void adjustCycleCounter(Int32 amount);
    void setChannels(uInt32 channels);
    void setFrameRate(uInt32 framerate);
    void initialize();
    void close();
    bool isSuccessfullyInitialized() const;
    void mute(bool state);
    void reset();
    void set(uInt16 addr, uInt8 value, Int32 cycle);
    void setVolume(Int32 percent);
    void adjustVolume(Int8 direction);
    bool load(Deserializer& in);
    bool save(Serializer& out);
	void update(void);

  protected:
    void processFragment(uInt8* stream, Int32 length);
    // Struct to hold information regarding a TIA sound register write
    class RegWriteQueue
    {
      public:
        RegWriteQueue(uInt32 capacity = 512);
        virtual ~RegWriteQueue();
        void clear();
        void dequeue();
        float duration();
        void enqueue(const RegWrite& info);
        RegWrite& front();
        uInt32 size() const;

      private:
        void grow();

      private:
        uInt32 myCapacity;
        RegWrite* myBuffer;
        uInt32 mySize;
        uInt32 myHead;
        uInt32 myTail;
    };

  private:
    TIASound myTIASound;
    bool myIsEnabled;
    bool myIsInitializedFlag;
    Int32 myLastRegisterSetCycle;
    uInt32 myDisplayFrameRate;
    uInt32 myNumChannels;
    double myFragmentSizeLogBase2;
    bool myIsMuted;
    uInt32 myVolume;
    RegWriteQueue myRegWriteQueue;
	//static void CALLBACK SoundWinCE::waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
	int myLatency, myMixRate, myBuffnum;
	WAVEHDR *myBuffers;
	HWAVEOUT myWout;
};

#endif  // SOUND_SUPPORT
#endif
