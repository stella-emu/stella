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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "AbstractFrameManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFrameManager::AbstractFrameManager() :
  myIsRendering(false),
  myVsync(false),
  myVblank(false),
  myCurrentFrameFinalLines(0),
  myPreviousFrameFinalLines(0),
  myTotalFrames(0),
  myLayout(FrameLayout::ntsc),
  myFrameRate(0),
  myOnFrameComplete(0),
  myOnFrameStart(0),
  myOnRenderingStart(0)
{}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AbstractFrameManager::setHandlers(
  callback frameStartCallback,
  callback frameCompletionCallback,
  callback renderingStartCallback
) {
  myOnFrameStart = frameStartCallback;
  myOnFrameComplete = frameCompletionCallback;
  myOnRenderingStart = renderingStartCallback;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AbstractFrameManager::setVblank(bool vblank)
{
  if (vblank == myVblank) return;

  myVblank = vblank;

  onSetVblank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AbstractFrameManager::setVsync(bool vsync)
{
  if (vsync == myVsync) return;

  myVsync = vsync;

  onSetVsync();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AbstractFrameManager::notifyFrameStart()
{
  if (myOnFrameStart) myOnFrameStart();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AbstractFrameManager::notifyFrameComplete(uInt32 finalScanlines)
{
  myPreviousFrameFinalLines = myCurrentFrameFinalLines;
  myCurrentFrameFinalLines = finalScanlines;
  myTotalFrames++;

  if (myOnFrameComplete) myOnFrameComplete();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AbstractFrameManager::notifyRenderingStart()
{
  if (myOnRenderingStart) myOnRenderingStart();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AbstractFrameManager::save(Serializer& out) const
{
  try {
    out.putString(name());

    out.putBool(myIsRendering);
    out.putBool(myVsync);
    out.putBool(myVblank);
    out.putInt(myCurrentFrameFinalLines);
    out.putInt(myPreviousFrameFinalLines);
    out.putInt(myTotalFrames);
    out.putInt(uInt32(myLayout));
    out.putDouble(myFrameRate);

    return onSave(out);
  }
  catch(...)
  {
    cerr << "ERROR: AbstractFrameManager::save" << endl;
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AbstractFrameManager::load(Serializer& in)
{
  try {
    if (in.getString() != name()) return false;

    myIsRendering = in.getBool();
    myVsync = in.getBool();
    myVblank = in.getBool();
    myCurrentFrameFinalLines = in.getInt();
    myPreviousFrameFinalLines = in.getInt();
    myTotalFrames = in.getInt();
    myLayout = FrameLayout(in.getInt());
    myFrameRate = float(in.getDouble());

    return onLoad(in);
  }
  catch(...)
  {
    cerr << "ERROR: AbstractFrameManager::load" << endl;
    return false;
  }
}
