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
// $Id: CheatManager.cxx,v 1.1 2005-11-11 21:44:18 stephena Exp $
//============================================================================

#include "OSystem.hxx"
#include "Cheat.hxx"
#include "CheetahCheat.hxx"
#include "BankRomCheat.hxx"
#include "RamCheat.hxx"

#include "CheatManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatManager::CheatManager(OSystem* osystem)
  : myOSystem(osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatManager::~CheatManager()
{
  // Don't delete the items from per-frame list, since it will be done in
  // the following loop
  myPerFrameList.clear();

  for(unsigned int i = 0; i < myCheatList.size(); i++)
  {
    cerr << myCheatList[i]->name() << ": " << myCheatList[i]->code() << endl;
    delete myCheatList[i];
  }
  myCheatList.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Cheat* CheatManager::add(const string& name, const string& code,
                               bool enable)
{
  for(unsigned int i = 0; i < code.size(); i++)
    if(!isxdigit(code[i]))
      return NULL;

  Cheat* cheat = (Cheat*) NULL;

  // Check if already present
  bool duplicate = false;
  for(unsigned int i = 0; i < myCheatList.size(); i++)
  {
    if(myCheatList[i]->code() == code)
    {
      cheat = myCheatList[i];
      duplicate = true;
cerr << "Duplicate found: " << cheat->name() << ":" << cheat->code() << endl;
      break;
    }
  }

  // Only create new cheat when necessary
  if(!duplicate)
  {
    switch(code.size())
    {
      case 4:
        cheat = new RamCheat(myOSystem, name, code);
        break;

      case 6:
        cheat = new CheetahCheat(myOSystem, name, code);
        break;

      case 8:
        cheat = new BankRomCheat(myOSystem, name, code);
        break;
    }

    // Add the cheat to the main cheat list
    if(cheat)
      myCheatList.push_back(cheat);
  }

  if(cheat)
  {
    // And enable/disable it (the cheat knows how to enable or disable itself)
    if(enable)
      cheat->enable();
    else
      cheat->disable();
  }

  return cheat;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::addPerFrame(Cheat* cheat, bool enable)
{
  if(!cheat)
    return;

  // Make sure there are no duplicates
  bool found = false;
  unsigned int i = 0;
  for(i = 0; i < myPerFrameList.size(); i++)
  {
    if(myPerFrameList[i]->code() == cheat->code())
    {
      found = true;
      break;
    }
  }

  if(enable)
    if(!found)
      myPerFrameList.push_back(cheat);
  else
    if(found)
      myPerFrameList.remove_at(i);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::parse(const string& cheats)
{
  // FIXME - deal with comma-separated cheats
  cerr << "parsing cheats: " << cheats << endl;

  add("TEST", cheats, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::enable(const string& code, bool enable)
{
  for(unsigned int i = 0; i < myCheatList.size(); i++)
  {
    if(myCheatList[i]->code() == code)
    {
      if(enable)
        myCheatList[i]->enable();
      else
        myCheatList[i]->disable();
      break;
    }
  }
}
