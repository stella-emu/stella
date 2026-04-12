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

#include "Console.hxx"
#include "Event.hxx"
#include "FSNode.hxx"
#include "OSystem.hxx"
#include "Sound.hxx"
#include "Switches.hxx"

#include "KidVid.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KidVid::KidVid(Jack jack, const Event& event, const OSystem& osystem,
               const System& system, string_view romMd5,
               const onMessageCallbackForced& callback)
  : Controller(jack, event, system, Controller::Type::KidVid),
    myOSystem{osystem},
    myCallback{callback},
    myEnabled{myJack == Jack::Right}
{
  // Right now, there are only two games that use the KidVid
  if(romMd5 == "ee6665683ebdb539e89ba620981cb0f6")
    myGame = Game::BBears;    // Berenstain Bears
  else if(romMd5 == "a204cd4fb1944c86e800120706512a64")
    myGame = Game::Smurfs;    // Smurfs Save the Day
  else
    myEnabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KidVid::write(DigitalPin pin, bool value)
{
  // Pin 1: Signal tape running or stopped; all other pins ignored
  if(pin == DigitalPin::One)
    setPin(DigitalPin::One, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KidVid::update()
{
  if(!myEnabled)
    return;

  if(myContinueSong)
  {
    // Continue playing song after state load
    const uInt8 temp = ourSongPositions[mySongPointer - 1] & 0x7f;
    assert(std::cmp_less(temp + 1, ourSongStart.size()));
    const uInt32 songLength = ourSongStart[temp + 1] - ourSongStart[temp] - (262 * ClickFrames);

    // Play the remaining WAV file
    const string& fileName = myOSystem.baseDir().getPath() +
      ((temp < 10) ? "KVSHARED.WAV": getFileName());
    myOSystem.sound().playWav(fileName, ourSongStart[temp] + (songLength - mySongLength), mySongLength);

    myContinueSong = false;
  }

  if(myGame == Game::Smurfs && myEvent.get(Event::ConsoleReset)) // Reset does not work with BBears!
  {
    myTape = 0; // rewind Kid Vid tape
    myFilesFound = mySongPlaying = false;
    myOSystem.sound().stopWav();
  }
  else if(myEvent.get(Event::RightKeyboard6) || myEvent.get(Event::ConsoleSelect) ||
          (myOSystem.hasConsole() && !myOSystem.console().switches().tvColor()))
  {
    // Some first songs trigger a sequence of timed actions, they cannot be skipped
    if(mySongPointer &&
        ourSongPositions[mySongPointer - 1] != 0 && // First song of all BBears games
        ourSongPositions[mySongPointer - 1] != 11)  // First song of Harmony Smurf
      myOSystem.sound().stopWav();
  }

  if(!myTape)
  {
    if(myEvent.get(Event::RightKeyboard1))
      myTape = 2;
    else if(myEvent.get(Event::RightKeyboard2))
      myTape = 3;
    else if(myEvent.get(Event::RightKeyboard3))
      myTape = myGame == Game::BBears ? 4 : 1; // Berenstain Bears or Smurfs Save The Day?
    // If no Keyboard controller is available, use SELECT and the same
    // switches settings as for playing Smurfs without a Kid Vid.
    else if(myEvent.get(Event::ConsoleSelect) && myOSystem.hasConsole())
    {
      // Harmony (2) : A B
      // Handy (3)   : B A
      // Greedy (1/4): B B
      myTape = 1
        + (myOSystem.console().switches().leftDifficultyA() ? 1 : 0)
        + (myOSystem.console().switches().rightDifficultyA() ? 2 : 0);
      if(myTape == 4) // ignore A A
        myTape = 0;
      else if(myTape == 1 && myGame == Game::BBears)
        myTape = 4;
    }

    if(myTape)
    {
      static constexpr std::array<string_view, 6> gameName = {
        "Harmony Smurf", "Handy Smurf", "Greedy Smurf",
        "Big Number Hunt", "Great Letter Roundup", "Spooky Spelling Bee"
      };

      myIdx = myGame == Game::BBears ? NumBlockBits : 0; // KVData48/KVData44
      myBlockIdx = NumBlockBits;
      myBlock = 0;
      openSampleFiles();

      myCallback(std::format("Game #{} - \"{}\"",
        gameNumber(), gameName[tapeIndex()]), true);
    }
  }

  // Is the tape running?
  if(myTape && getPin(DigitalPin::One) && !myTapeBusy)
  {
    setPin(DigitalPin::Four,
           static_cast<uInt8>(ourData[myIdx >> 3] << (myIdx & 0x07)) & 0x80U);

  #ifdef DEBUG_BUILD
    cerr << (static_cast<uInt8>(ourData[myIdx >> 3] << (myIdx & 0x07)) & 0x80U ? "X" : ".");
  #endif

    // increase to next bit
    ++myIdx;
    --myBlockIdx;

    // increase to next block (byte)
    if(!myBlockIdx)
    {
      if(!myBlock)
        myIdx = ((myTape * 6) + 12 - NumBlocks) * 8; // KVData00 - ourData = 12 (2 * 6)
      else
      {
        if(myBlock >= ourBlocks[tapeIndex()])
          myIdx = KVData80Offset;
        else
        {
          myIdx = KVPauseOffset;
          setNextSong();
        }
      }
      ++myBlock;
      myBlockIdx = NumBlockBits;
    }
  }

  if(myFilesFound)
  {
    if(mySongPlaying)
    {
      mySongLength = myOSystem.sound().wavSize();
      myTapeBusy = (mySongLength > 262 * TapeFrames) || !myBeep;
      // Check for end of played sample
      if(mySongLength == 0)
      {
        mySongPlaying = false;
        myTapeBusy = !myBeep;
        if(!myBeep)
          setNextSong();
      }
    }
  }
  else
  {
    if(mySongLength)
    {
      --mySongLength;
      myTapeBusy = (mySongLength > TapeFrames);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool KidVid::save(Serializer& out) const
{
  // Save WAV player state
  out.putInt(myTape);
  out.putBool(myFilesFound);
  out.putBool(myTapeBusy);
  out.putBool(myBeep);
  out.putBool(mySongPlaying);
  out.putInt(mySongPointer);
  out.putInt(mySongLength);
  // Save tape input simulation state
  out.putInt(myIdx);
  out.putInt(myBlockIdx);
  out.putInt(myBlock);

  return Controller::save(out);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool KidVid::load(Serializer& in)
{
  // Load WAV player state
  myTape = in.getInt();
  myFilesFound = in.getBool();
  myTapeBusy = in.getBool();
  myBeep = in.getBool();
  mySongPlaying = in.getBool();
  mySongPointer = in.getInt();
  mySongLength = in.getInt();
  // Load tape input simulation state
  myIdx = in.getInt();
  myBlockIdx = in.getInt();
  myBlock = in.getInt();

  myContinueSong = myFilesFound && mySongPlaying;

  return Controller::load(in);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KidVid::getFileName() const
{
  static constexpr std::array<string_view, 6> fileNames = {
    "KVS3.WAV", "KVS1.WAV", "KVS2.WAV",
    "KVB3.WAV", "KVB1.WAV", "KVB2.WAV"
  };

  return string{fileNames[tapeIndex()]};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 KidVid::tapeIndex() const
{
  assert(myTape >= 1 && myTape <= 4);
  if(myTape == 4) return 3;
  return myGame == Game::Smurfs ? myTape - 1 : myTape + 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 KidVid::gameNumber() const
{
  // Tape selection order is 2,3,1 (keyboard) or 1-4 (select button)
  // but games are numbered 1,2,3 on the cassette labels
  assert(myTape >= 1 && myTape <= 4);
  static constexpr std::array<uInt32, 4> mapping = { 3, 1, 2, 3 };
  return mapping[myTape - 1];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KidVid::openSampleFiles()
{
#ifdef SOUND_SUPPORT
  static constexpr std::array<uInt32, 6> firstSongPointer = {
    44 + 38,
    0,
    44,
    44 + 38 + 42 + 62 + 80,
    44 + 38 + 42,
    44 + 38 + 42 + 62
  };

  if(!myFilesFound)
  {
    myFilesFound = FSNode(myOSystem.baseDir().getPath() + getFileName()).exists() &&
                   FSNode(myOSystem.baseDir().getPath() + "KVSHARED.WAV").exists();

  #ifdef DEBUG_BUILD
    if(myFilesFound)
      cerr << "\nfound file: " << getFileName() << '\n'
           << "found file: " << "KVSHARED.WAV\n";
  #endif

    mySongLength = 0;
    mySongPointer = firstSongPointer[tapeIndex()];
  }
  myTapeBusy = false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KidVid::setNextSong()
{
  if(myFilesFound)
  {
    myBeep = (ourSongPositions[mySongPointer] & 0x80) == 0;

    const uInt8 temp = ourSongPositions[mySongPointer] & 0x7f;
    assert(std::cmp_less(temp + 1, ourSongStart.size()));
    mySongLength = ourSongStart[temp + 1] - ourSongStart[temp] - (262 * ClickFrames);

    // Play the WAV file
    const string& fileName = (temp < 10) ? "KVSHARED.WAV" : getFileName();
    myOSystem.sound().playWav(myOSystem.baseDir().getPath() + fileName,
                              ourSongStart[temp], mySongLength);
    myCallback(std::format("Read song #{} ({})",
                           mySongPointer, fileName), false);

  #ifdef DEBUG_BUILD
    cerr << fileName << ": " << (ourSongPositions[mySongPointer] & 0x7f) << '\n';
  #endif

    mySongPlaying = myTapeBusy = true;
    ++mySongPointer;
  }
  else
  {
    myBeep = true;
    myTapeBusy = true;
    mySongLength = TapeFrames + 32;   /* delay needed for Harmony without tape */
  }
}
