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
// $Id: Event.cxx,v 1.4 2005-12-09 01:16:13 stephena Exp $
//============================================================================

#include "Event.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Event()
  : myNumberOfTypes(Event::LastType),
    myEventRecordFlag(true)
{
  // Set all of the events to 0 / false to start with
  clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::~Event()
{
  int events = 0, waits = 0, totalwaits = 0;
  cerr << "Event history contains " << myEventHistory.size()/2 << " events\n";
  for(unsigned int i = 0; i < myEventHistory.size(); ++i)
  {
    int tmp = myEventHistory[i];
    if(tmp < 0)
    {
      ++waits;
      totalwaits += -tmp;
    }
    else
      ++events;

    cerr << tmp << " ";
  }
  cerr << endl
       << "events pairs = " << events/2
       << ", frame waits = " << waits
       << ", total frame waits = " << totalwaits
       << endl;

  myEventHistory.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 Event::get(Type type) const
{
  return myValues[type];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Event::set(Type type, Int32 value)
{
  myValues[type] = value;

  // Add to history if we're in recording mode
  if(myEventRecordFlag)
  {
    myEventHistory.push_back(type);
    myEventHistory.push_back(value);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Event::clear()
{
  for(int i = 0; i < myNumberOfTypes; ++i)
  {
    myValues[i] = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Event::record(bool enable)
{
  if(myEventRecordFlag == enable)
    return;
  else
    myEventRecordFlag = enable;

  if(myEventRecordFlag)
    myEventHistory.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Event::nextFrame()
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
