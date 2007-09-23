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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: StateManager.cxx,v 1.1 2007-09-23 17:04:17 stephena Exp $
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "Serializer.hxx"
#include "Deserializer.hxx"
#include "Settings.hxx"
#include "Console.hxx"
#include "System.hxx"

#include "StateManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateManager::StateManager(OSystem* osystem)
  : myOSystem(osystem),
    myCurrentSlot(0)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateManager::~StateManager()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::loadState(int slot)
{
  if(&myOSystem->console())
  {
    if(slot < 0) slot = myCurrentSlot;

    const string& name = myOSystem->console().properties().get(Cartridge_Name);
    const string& md5  = myOSystem->console().properties().get(Cartridge_MD5);

    ostringstream buf;
    buf << myOSystem->stateDir() << BSPF_PATH_SEPARATOR
        << name << ".st" << slot;

    // Make sure the file can be opened for reading
    Deserializer in;
    if(!in.open(buf.str()))
    {
      buf.str("");
      buf << "Error loading state " << slot;
      myOSystem->frameBuffer().showMessage(buf.str());
      return;
    }

    // Do a state load using the System
    buf.str("");
    if(myOSystem->console().system().loadState(md5, in))
      buf << "State " << slot << " loaded";
    else
      buf << "Invalid state " << slot << " file";

    in.close();
    myOSystem->frameBuffer().showMessage(buf.str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::saveState(int slot)
{
  if(&myOSystem->console())
  {
    if(slot < 0) slot = myCurrentSlot;

    const string& name = myOSystem->console().properties().get(Cartridge_Name);
    const string& md5  = myOSystem->console().properties().get(Cartridge_MD5);

    ostringstream buf;
    buf << myOSystem->stateDir() << BSPF_PATH_SEPARATOR
        << name << ".st" << slot;

    // Make sure the file can be opened for writing
    Serializer out;
    if(!out.open(buf.str()))
    {
      myOSystem->frameBuffer().showMessage("Error saving state file");
      return;
    }

    // Do a state save using the System
    buf.str("");
//int start = myOSystem->getTicks();
    if(myOSystem->console().system().saveState(md5, out))
    {
//int end = myOSystem->getTicks();
//cerr << "ticks to save a state slot: " << (end - start) << endl;

      buf << "State " << slot << " saved";
      if(myOSystem->settings().getBool("autoslot"))
      {
        myCurrentSlot = (slot + 1) % 10;
        buf << ", switching to slot " << slot;
      }
    }
    else
      buf << "Error saving state " << slot;

    out.close();
    myOSystem->frameBuffer().showMessage(buf.str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::changeState()
{
  myCurrentSlot = (myCurrentSlot + 1) % 10;

  // Print appropriate message
  ostringstream buf;
  buf << "Changed to slot " << myCurrentSlot;
  myOSystem->frameBuffer().showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::reset()
{
  myCurrentSlot = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateManager::StateManager(const StateManager&)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateManager& StateManager::operator = (const StateManager&)
{
  assert(false);
  return *this;
}
