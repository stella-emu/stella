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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventStreamer.cxx,v 1.1 2005-12-28 22:56:36 stephena Exp $
//============================================================================

#include "bspf.hxx"

#include "OSystem.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "EventStreamer.hxx"
#include "System.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventStreamer::EventStreamer(OSystem* osystem)
  : myOSystem(osystem),
    myEventRecordFlag(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventStreamer::~EventStreamer()
{
  stopRecording();

  myEventHistory.clear();
  myStreamReader.close();
  myStreamWriter.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventStreamer::startRecording()
{
  string eventfile = /*myOSystem->baseDir() + BSPF_PATH_SEPARATOR +*/ "test.inp";
  if(!myStreamWriter.open(eventfile))
    return false;

  // And save the current state to it
  string md5 = myOSystem->console().properties().get("Cartridge.MD5");
  myOSystem->console().system().saveState(md5, myStreamWriter);
  myEventHistory.clear();

  return myEventRecordFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventStreamer::stopRecording()
{
  if(!myStreamWriter.isOpen() || !myEventRecordFlag)
    return false;

  // Append the event history to the eventstream
  int size = myEventHistory.size();
  myStreamWriter.putString("EventStream");
  myStreamWriter.putInt(size);
  for(int i = 0; i < size; ++i)
    myStreamWriter.putInt(myEventHistory[i]);

  myStreamWriter.close();
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventStreamer::loadRecording()
{
cerr << "EventStreamer::loadRecording()\n";

  string eventfile = /*myOSystem->baseDir() + BSPF_PATH_SEPARATOR +*/ "test.inp";
  if(!myStreamReader.open(eventfile))
    return false;

  // Load ROM state
  string md5 = myOSystem->console().properties().get("Cartridge.MD5");
  myOSystem->console().system().loadState(md5, myStreamReader);

  if(myStreamReader.getString() != "EventStream")
    return false;

  // Now load the event stream
  myEventHistory.clear();
  int size = myStreamReader.getInt();
  for(int i = 0; i < size; ++i)
    myEventHistory.push_back(myStreamReader.getInt());

cerr << "event queue contains " << myEventHistory.size() << " items\n";

  return myEventRecordFlag = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventStreamer::addEvent(int type, int value)
{
  if(myEventRecordFlag)
  {
    myEventHistory.push_back(type);
    myEventHistory.push_back(value);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventStreamer::nextFrame()
{
  if(myEventRecordFlag)
  {
    int idx = myEventHistory.size() - 1;
    if(idx >= 0 && myEventHistory[idx] < 0)
      --myEventHistory[idx];
    else
      myEventHistory.push_back(-1);
  }
}
