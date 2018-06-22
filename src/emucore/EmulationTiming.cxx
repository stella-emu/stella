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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "EmulationTiming.hxx"

namespace {
  constexpr uInt32 AUDIO_HALF_FRAMES_PER_FRAGMENT = 1;

  uInt32 discreteDivCeil(uInt32 n, uInt32 d)
  {
    return n / d + ((n % d == 0) ? 0 : 1);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EmulationTiming::EmulationTiming(FrameLayout frameLayout) :
  myFrameLayout(frameLayout),
  myPlaybackRate(44100),
  myPlaybackPeriod(512),
  myAudioQueueExtraFragments(1),
  myAudioQueueHeadroom(2)
{}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EmulationTiming& EmulationTiming::updateFrameLayout(FrameLayout frameLayout)
{
  myFrameLayout = frameLayout;
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EmulationTiming& EmulationTiming::updatePlaybackRate(uInt32 playbackRate)
{
  myPlaybackRate = playbackRate;
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EmulationTiming& EmulationTiming::updatePlaybackPeriod(uInt32 playbackPeriod)
{
  myPlaybackPeriod = playbackPeriod;
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EmulationTiming& EmulationTiming::updateAudioQueueExtraFragments(uInt32 audioQueueExtraFragments)
{
  myAudioQueueExtraFragments = audioQueueExtraFragments;
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EmulationTiming& EmulationTiming::updateAudioQueueHeadroom(uInt32 audioQueueHeadroom)
{
  myAudioQueueHeadroom = audioQueueHeadroom;
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EmulationTiming::maxCyclesPerTimeslice() const
{
  return 2 * cyclesPerFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EmulationTiming::minCyclesPerTimeslice() const
{
  return cyclesPerFrame() / 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EmulationTiming::linesPerFrame() const
{
  switch (myFrameLayout) {
    case FrameLayout::ntsc:
      return 262;

    case FrameLayout::pal:
      return 312;

    default:
      throw runtime_error("invalid frame layout");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EmulationTiming::cyclesPerFrame() const
{
  return 76 * linesPerFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EmulationTiming::framesPerSecond() const
{
  switch (myFrameLayout) {
    case FrameLayout::ntsc:
      return 60;

    case FrameLayout::pal:
      return 50;

    default:
      throw runtime_error("invalid frame layout");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EmulationTiming::cyclesPerSecond() const
{
  return cyclesPerFrame() * framesPerSecond();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EmulationTiming::audioFragmentSize() const
{
  return AUDIO_HALF_FRAMES_PER_FRAGMENT * linesPerFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EmulationTiming::audioSampleRate() const
{
  return 2 * linesPerFrame() * framesPerSecond();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EmulationTiming::audioQueueCapacity() const
{
  uInt32 minCapacity = discreteDivCeil(maxCyclesPerTimeslice() * audioSampleRate(), audioFragmentSize() * cyclesPerSecond());

  return std::max(prebufferFragmentCount(), minCapacity) + myAudioQueueExtraFragments;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EmulationTiming::prebufferFragmentCount() const
{
  return discreteDivCeil(myPlaybackPeriod * audioSampleRate(), audioFragmentSize() * myPlaybackRate) + myAudioQueueHeadroom;
}
