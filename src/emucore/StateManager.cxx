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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "Settings.hxx"
#include "Console.hxx"
#include "Control.hxx"
#include "Switches.hxx"
#include "System.hxx"
#include "Serializable.hxx"

#include "StateManager.hxx"

#define STATE_HEADER "02060000state"
#define MOVIE_HEADER "02060000movie"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateManager::StateManager(OSystem* osystem)
  : myOSystem(osystem),
    myCurrentSlot(0),
    myActiveMode(kOffMode),
    myFrameCounter(0)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateManager::~StateManager()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::isActive()
{
  return myActiveMode != kOffMode;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::toggleRecordMode()
{
#if 0
  if(myActiveMode != kMovieRecordMode)  // Turn on movie record mode
  {
    myActiveMode = kOffMode;

    string moviefile = /*myOSystem->baseDir() + BSPF_PATH_SEPARATOR +*/ "test.inp";
    if(myMovieWriter.isOpen())
      myMovieWriter.close();
    if(!myMovieWriter.open(moviefile))
      return false;

    // Prepend the ROM md5 so this state file only works with that ROM
    myMovieWriter.putString(myOSystem->console().properties().get(Cartridge_MD5));

    if(!myOSystem->console().save(myMovieWriter))
      return false;

    // Save controller types for this ROM
    // We need to check this, since some controllers save more state than
    // normal, and those states files wouldn't be compatible with normal
    // controllers.
    myMovieWriter.putString(
      myOSystem->console().controller(Controller::Left).name());
    myMovieWriter.putString(
      myOSystem->console().controller(Controller::Right).name());

    // If we get this far, we're really in movie record mode
    myActiveMode = kMovieRecordMode;
  }
  else  // Turn off movie record mode
  {
    myActiveMode = kOffMode;
    myMovieWriter.close();
    return false;
  }

  return myActiveMode == kMovieRecordMode;
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::toggleRewindMode()
{
  // FIXME - For now, I'm going to use this to activate movie playback
#if 0
  // Close the writer, since we're about to re-open in read mode
  myMovieWriter.close();

  if(myActiveMode != kMoviePlaybackMode)  // Turn on movie playback mode
  {
    myActiveMode = kOffMode;

    string moviefile = /*myOSystem->baseDir() + BSPF_PATH_SEPARATOR +*/ "test.inp";
    if(myMovieReader.isOpen())
      myMovieReader.close();
    if(!myMovieReader.open(moviefile))
      return false;

    // Check the ROM md5
    if(myMovieReader.getString() !=
       myOSystem->console().properties().get(Cartridge_MD5))
      return false;

    if(!myOSystem->console().load(myMovieReader))
      return false;

    // Check controller types
    const string& left  = myMovieReader.getString();
    const string& right = myMovieReader.getString();

    if(left != myOSystem->console().controller(Controller::Left).name() ||
       right != myOSystem->console().controller(Controller::Right).name())
      return false;

    // If we get this far, we're really in movie record mode
    myActiveMode = kMoviePlaybackMode;
  }
  else  // Turn off movie playback mode
  {
    myActiveMode = kOffMode;
    myMovieReader.close();
    return false;
  }

  return myActiveMode == kMoviePlaybackMode;
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::update()
{
#if 0
  switch(myActiveMode)
  {
    case kMovieRecordMode:
      myOSystem->console().controller(Controller::Left).save(myMovieWriter);
      myOSystem->console().controller(Controller::Right).save(myMovieWriter);
      myOSystem->console().switches().save(myMovieWriter);
      break;

    case kMoviePlaybackMode:
      myOSystem->console().controller(Controller::Left).load(myMovieReader);
      myOSystem->console().controller(Controller::Right).load(myMovieReader);
      myOSystem->console().switches().load(myMovieReader);
      break;

    default:
      break;
  }
#endif
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
    Serializer in(buf.str());
    if(!in.isValid())
    {
      buf.str("");
      buf << "Error loading state " << slot;
      myOSystem->frameBuffer().showMessage(buf.str());
      return;
    }

    buf.str("");

    // First test if we have a valid header
    // If so, do a complete state load using the Console
    if(in.getString() != STATE_HEADER)
      buf << "Incompatible state " << slot << " file";
    else if(in.getString() == md5 && myOSystem->console().load(in))
      buf << "State " << slot << " loaded";
    else
      buf << "Invalid data in state " << slot << " file";

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
    Serializer out(buf.str());
    if(!out.isValid())
    {
      myOSystem->frameBuffer().showMessage("Error saving state file");
      return;
    }

    // Add header so that if the state format changes in the future,
    // we'll know right away, without having to parse the rest of the file
    out.putString(STATE_HEADER);

    // Prepend the ROM md5 so this state file only works with that ROM
    out.putString(md5);

    // Do a complete state save using the Console
    buf.str("");
    if(myOSystem->console().save(out))
    {
      buf << "State " << slot << " saved";
      if(myOSystem->settings().getBool("autoslot"))
      {
        myCurrentSlot = (slot + 1) % 10;
        buf << ", switching to slot " << slot;
      }
    }
    else
      buf << "Error saving state " << slot;

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
bool StateManager::loadState(Serializer& in)
{
  if(&myOSystem->console())
  {
    // Make sure the file can be opened for reading
    if(in.isValid())
    {
      // First test if we have a valid header
      // If so, do a complete state load using the Console
      const string& md5  = myOSystem->console().properties().get(Cartridge_MD5);
      if(in.getString() == STATE_HEADER && in.getString() == md5 &&
         myOSystem->console().load(in))
        return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::saveState(Serializer& out)
{
  if(&myOSystem->console())
  {
    // Make sure the file can be opened for writing
    if(out.isValid())
    {
      // Add header so that if the state format changes in the future,
      // we'll know right away, without having to parse the rest of the file
      out.putString(STATE_HEADER);

      // Prepend the ROM md5 so this state file only works with that ROM
      out.putString(myOSystem->console().properties().get(Cartridge_MD5));

      // Do a complete state save using the Console
      if(myOSystem->console().save(out))
        return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::reset()
{
#if 0
  myCurrentSlot = 0;

  switch(myActiveMode)
  {
    case kMovieRecordMode:
      myMovieWriter.close();
      break;

    case kMoviePlaybackMode:
      myMovieReader.close();
      break;

    default:
      break;
  }
  myActiveMode = kOffMode;
#endif
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
