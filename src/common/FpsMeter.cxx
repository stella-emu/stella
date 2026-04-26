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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "FpsMeter.hxx"

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FpsMeter::FpsMeter(uInt32 queueSize)
  : myQueue{queueSize}
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FpsMeter::reset(uInt32 garbageFrameLimit)
{
  myQueue.clear();
  myQueueOffset = 0;
  myFrameCount = 0;
  myFps = 0.F;
  myGarbageFrameCounter = 0;
  myGarbageFrameLimit = garbageFrameLimit;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FpsMeter::render(uInt32 frameCount)
{
  if (myGarbageFrameCounter < myGarbageFrameLimit) {
    myGarbageFrameCounter += frameCount;
    return;
  }

  const size_t queueSize = myQueue.capacity();
  const Entry e_last{frameCount, high_resolution_clock::now()};

  if (myQueue.size() < queueSize) {
    myQueue.push_back(e_last);
    myFrameCount += frameCount;
  } else {
    myFrameCount = myFrameCount - myQueue[myQueueOffset].frames + frameCount;
    myQueue[myQueueOffset] = e_last;

    if(++myQueueOffset == queueSize) myQueueOffset = 0;
  }
  const Entry& e_first = myQueue[myQueueOffset];

  const float timeInterval = duration_cast<duration<float>>
    (e_last.timestamp - e_first.timestamp).count();

  if (timeInterval > 0)
    myFps = (myFrameCount - e_first.frames) / timeInterval;
}
