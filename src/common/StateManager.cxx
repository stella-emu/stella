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

#include "OSystem.hxx"
#include "Settings.hxx"
#include "Console.hxx"
#include "Cart.hxx"
#include "Control.hxx"
#include "Switches.hxx"
#include "System.hxx"
#include "Event.hxx"
#include "RewindManager.hxx"

#include "StateManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateManager::StateManager(OSystem& osystem)
  : myOSystem{osystem},
    myRewindManager{std::make_unique<RewindManager>(osystem, *this)}
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateManager::~StateManager() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string StateManager::movieFile() const
{
  return std::format("{}{}.inp",
    myOSystem.stateDir().getPath(),
    myOSystem.console().properties().get(PropType::Cart_Name));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateManager::Mode StateManager::defaultMode() const
{
  const bool devSettings = myOSystem.settings().getBool("dev.settings");
  const bool timeMachine = myOSystem.settings().getBool(
    devSettings ? "dev.timemachine" : "plr.timemachine");

  return timeMachine ? Mode::TimeMachine : Mode::Off;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::toggleRecordMode()
{
  if(myActiveMode != Mode::MovieRecord)
    return startRecording();

  stopRecording();
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::togglePlaybackMode()
{
  if(myActiveMode != Mode::MoviePlayback)
    return startPlayback();

  stopPlayback();
  myOSystem.frameBuffer().showTextMessage("Movie playback stopped");
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::startRecording()
{
  if(!myOSystem.hasConsole())
    return false;

  // A movie is exclusive with any other state mode
  if(myActiveMode == Mode::MoviePlayback)
    stopPlayback();

  myMovie = std::make_unique<Serializer>(movieFile(),
                                         Serializer::FileMode::ReadWriteTrunc);
  if(!*myMovie)
  {
    myMovie.reset();
    myOSystem.frameBuffer().showTextMessage("Can't open movie file for recording");
    return false;
  }

  try
  {
    // The format id, ROM md5 and controller names tie a movie to a specific
    // machine setup; the initial state lets playback start from this point
    myMovie->putString(MOVIE_HEADER);
    myMovie->putString(myOSystem.console().properties().get(PropType::Cart_MD5));
    myMovie->putString(myOSystem.console().leftController().name());
    myMovie->putString(myOSystem.console().rightController().name());

    if(!saveState(*myMovie))
    {
      myMovie.reset();
      myOSystem.frameBuffer().showTextMessage("Error writing movie header");
      return false;
    }
  }
  catch(...)
  {
    myMovie.reset();
    myOSystem.frameBuffer().showTextMessage("Error writing movie header");
    return false;
  }

  myMovieFrames = 0;
  myActiveMode = Mode::MovieRecord;
  myOSystem.frameBuffer().showTextMessage("Movie recording started");
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::stopRecording()
{
  if(myMovie)
  {
    // End-of-movie marker, then flush and close
    try { myMovie->putBool(false); }
    catch(...) { }  // NOLINT(bugprone-empty-catch)
    myMovie.reset();
  }

  myActiveMode = defaultMode();
  myOSystem.frameBuffer().showTextMessage(
    std::format("Movie recording stopped ({} frames)", myMovieFrames));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::recordFrame(const Event& event)
{
  if(myActiveMode != Mode::MovieRecord || !myMovie)
    return;

  try
  {
    myMovie->putBool(true);  // a frame follows
    // Per-frame sync check: deterministic emulation reproduces the exact cycle
    // count, so a mismatch on playback pinpoints a desync
    myMovie->putLong(myOSystem.console().system().cycles());
    event.saveInputWindow(*myMovie);
    ++myMovieFrames;
  }
  catch(...)
  {
    myOSystem.frameBuffer().showTextMessage("Error writing movie, recording stopped");
    stopRecording();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::startPlayback()
{
  if(!myOSystem.hasConsole())
    return false;

  if(myActiveMode == Mode::MovieRecord)
    stopRecording();

  myMovie = std::make_unique<Serializer>(movieFile(),
                                         Serializer::FileMode::ReadOnly);
  if(!*myMovie)
  {
    myMovie.reset();
    myOSystem.frameBuffer().showTextMessage("No movie file to play back");
    return false;
  }

  try
  {
    if(myMovie->getString() != MOVIE_HEADER)
    {
      myMovie.reset();
      myOSystem.frameBuffer().showTextMessage("Incompatible movie file");
      return false;
    }
    if(myMovie->getString() !=
       myOSystem.console().properties().get(PropType::Cart_MD5))
    {
      myMovie.reset();
      myOSystem.frameBuffer().showTextMessage("Movie is for a different ROM");
      return false;
    }
    const string left  = myMovie->getString();
    const string right = myMovie->getString();
    if(left  != myOSystem.console().leftController().name() ||
       right != myOSystem.console().rightController().name())
    {
      myMovie.reset();
      myOSystem.frameBuffer().showTextMessage("Movie controller mismatch");
      return false;
    }
    if(!loadState(*myMovie))
    {
      myMovie.reset();
      myOSystem.frameBuffer().showTextMessage("Invalid movie state data");
      return false;
    }
  }
  catch(...)
  {
    myMovie.reset();
    myOSystem.frameBuffer().showTextMessage("Error reading movie file");
    return false;
  }

  myMovieFrames = 0;
  myActiveMode = Mode::MoviePlayback;
  myOSystem.frameBuffer().showTextMessage("Movie playback started");
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::stopPlayback()
{
  myMovie.reset();
  myActiveMode = defaultMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::playbackFrame(Event& event, uInt64 nowCycles)
{
  if(myActiveMode != Mode::MoviePlayback || !myMovie)
    return false;

  try
  {
    if(!myMovie->getBool())  // end-of-movie marker
    {
      const uInt32 frames = myMovieFrames;
      stopPlayback();
      myOSystem.frameBuffer().showTextMessage(
        std::format("Movie playback ended ({} frames)", frames));
      return false;
    }

    const uInt64 recCycles = myMovie->getLong();
    if(recCycles != nowCycles)
      myOSystem.frameBuffer().showTextMessage(
        std::format("Movie desync at frame {}", myMovieFrames));

    event.loadInputWindow(*myMovie, nowCycles);
    ++myMovieFrames;
    return true;
  }
  catch(...)
  {
    myOSystem.frameBuffer().showTextMessage("Error reading movie, playback stopped");
    stopPlayback();
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::toggleTimeMachine()
{
  const bool devSettings = myOSystem.settings().getBool("dev.settings");

  myActiveMode = (myActiveMode == Mode::TimeMachine)
    ? Mode::Off
    : Mode::TimeMachine;
  const bool enabled = (myActiveMode == Mode::TimeMachine);

  myOSystem.frameBuffer().showTextMessage(
    enabled ? "Time Machine enabled" : "Time Machine disabled");

  myOSystem.settings().setValue(
    devSettings ? "dev.timemachine" : "plr.timemachine", enabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::addExtraState(string_view message)
{
  if(myActiveMode == Mode::TimeMachine)
    return myRewindManager->addState(message);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::rewindStates(uInt32 numStates)
{
  return myRewindManager->rewindStates(numStates);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::unwindStates(uInt32 numStates)
{
  return myRewindManager->unwindStates(numStates);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::windStates(uInt32 numStates, bool unwind)
{
  return myRewindManager->windStates(numStates, unwind);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::update()
{
  // Movie record/playback is driven per input window from EventHandler::poll;
  // only the rewind facility needs servicing here
  if(myActiveMode == Mode::TimeMachine)
    myRewindManager->addState("Time Machine", true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::loadState(int slot)
{
  if(!myOSystem.hasConsole())
    return;

  if(slot < 0)
    slot = myCurrentSlot;

  const auto path = std::format("{}{}.st{}",
    myOSystem.stateDir().getPath(),
    myOSystem.console().properties().get(PropType::Cart_Name),
    slot);

  Serializer in(path, Serializer::FileMode::ReadOnly);
  if(!in)
  {
    myOSystem.frameBuffer().showTextMessage(
      std::format("Can't open/load from state file {}", slot));
    return;
  }

  try
  {
    // First test if we have a valid header
    // If so, do a complete state load using the Console
    const auto header = in.getString();
    if(header != STATE_HEADER)
      myOSystem.frameBuffer().showTextMessage(
        std::format("Incompatible state {} file", slot));
    else if(myOSystem.console().load(in))
      myOSystem.frameBuffer().showTextMessage(
        std::format("State {} loaded", slot));
    else
      myOSystem.frameBuffer().showTextMessage(
        std::format("Invalid data in state {} file", slot));
  }
  catch(...)
  {
    myOSystem.frameBuffer().showTextMessage(
      std::format("Invalid data in state {} file", slot));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::saveState(int slot)
{
  if(!myOSystem.hasConsole())
    return;

  if(slot < 0)
    slot = myCurrentSlot;

  const auto path = std::format("{}{}.st{}",
    myOSystem.stateDir().getPath(),
    myOSystem.console().properties().get(PropType::Cart_Name),
    slot);

  Serializer out(path, Serializer::FileMode::ReadWriteTrunc);
  if(!out)
  {
    myOSystem.frameBuffer().showTextMessage(
      std::format("Can't open/save to state file {}", slot));
    return;
  }

  try
  {
    // Add header so that if the state format changes in the future,
    // we'll know right away, without having to parse the rest of the file
    out.putString(STATE_HEADER);
  }
  catch(...)
  {
    myOSystem.frameBuffer().showTextMessage(
      std::format("Error saving state {}", slot));
    return;
  }

  // Do a complete state save using the Console
  if(myOSystem.console().save(out))
  {
    if(myOSystem.settings().getBool("autoslot"))
    {
      myCurrentSlot = (slot + 1) % 10;
      myOSystem.frameBuffer().showTextMessage(
        std::format("State {} saved, switching to slot {}", slot, myCurrentSlot));
    }
    else
    {
      myOSystem.frameBuffer().showTextMessage(
        std::format("State {} saved", slot));
    }
  }
  else
  {
    myOSystem.frameBuffer().showTextMessage(
      std::format("Error saving state {}", slot));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::changeState(int direction)
{
  myCurrentSlot = BSPF::clampw(myCurrentSlot + direction, 0, 9);

  myOSystem.frameBuffer().showTextMessage(
    direction != 0
      ? std::format("Changed to state slot {}", myCurrentSlot)
      : std::format("State slot {}", myCurrentSlot));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::toggleAutoSlot()
{
  const bool autoSlot = !myOSystem.settings().getBool("autoslot");

  myOSystem.frameBuffer().showTextMessage(
    std::format("Automatic slot change {}", autoSlot ? "enabled" : "disabled"));

  myOSystem.settings().setValue("autoslot", autoSlot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::loadState(Serializer& in)
{
  if(!myOSystem.hasConsole() || !in)
    return false;

  try
  {
    // First test if we have a valid header
    // If so, do a complete state load using the Console
    return in.getString() == STATE_HEADER && myOSystem.console().load(in);
  }
  catch(...)
  {
    cerr << "ERROR: StateManager::loadState(Serializer&)\n";
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateManager::saveState(Serializer& out)
{
  if(!myOSystem.hasConsole() || !out)
    return false;

  try
  {
    // Add header so that if the state format changes in the future,
    // we'll know right away, without having to parse the rest of the file
    out.putString(STATE_HEADER);

    // Do a complete state save using the Console
    return myOSystem.console().save(out);
  }
  catch(...)
  {
    cerr << "ERROR: StateManager::saveState(Serializer&)\n";
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateManager::reset()
{
  myCurrentSlot = 0;
  myRewindManager->clear();

  // Abandon any in-progress movie (e.g. on console reset/reload)
  myMovie.reset();
  myMovieFrames = 0;

  myActiveMode = defaultMode();
}
