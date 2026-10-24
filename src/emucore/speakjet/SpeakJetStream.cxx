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

#include <cmath>

#include "Console.hxx"
#include "EmulationTiming.hxx"
#include "OSystem.hxx"
#include "Serializer.hxx"
#include "Sound.hxx"
#include "System.hxx"
#include "SpeakJetSynth.hxx"
#include "SpeakJetStream.hxx"

namespace {
  // The chip's input buffer, and the fill above which its Buffer Half Full
  // line drops READY
  constexpr size_t BUFFER_SIZE = 64;
  constexpr size_t BUFFER_HALF = 32;

  // Silence queued ahead of speech after silence: the queue dips by up to
  // about 110ms as the emulation and the device take turns, and must not empty
  constexpr uInt32 PRIME_MS = 150;

  // Running faster than real time (e.g. turbo mode) would queue speech
  // without limit; past this much, what is made is dropped instead
  constexpr uInt32 MAX_QUEUED_MS = 500;

  // At most this much is made in one update, should the cycle count jump
  constexpr size_t MAX_RENDER = SpeakJetSynth::RATE;

  // The CPU clock to pace by before the console's timing is available
  constexpr uInt64 NTSC_CPU_HZ = 1193182;

  // Above this fraction of full scale, peaks ease into it rather than clip
  constexpr double LIMIT_KNEE = 0.8;

  // A sample as 16 bits, easing peaks above the knee into full scale
  Int16 limit(double v)
  {
    constexpr double FS = 32767.0;
    constexpr double knee = LIMIT_KNEE * FS;
    const double mag = std::abs(v);
    if(mag <= knee)
      return I16(v);

    const double room = FS - knee;
    return I16(std::copysign(knee + room * std::tanh((mag - knee) / room), v));
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetStream::SpeakJetStream(const OSystem& osystem, const System& system,
                               string_view deadPort)
  : myOSystem{osystem},
    mySystem{system},
    myMSA{[this]() { return lookahead(); }},
    myDecoder{myMSA},
    myLastCycle{system.cycles()}
{
  myAboutString = " (software SpeakJet)";

  // Say why, when this is a fallback rather than a choice
  if(!deadPort.empty())
    myAboutString += " [\'" + string{deadPort} + "\' unavailable]";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetStream::~SpeakJetStream()
{
  // Queued speech would otherwise play on into whatever runs next
  myOSystem.sound().stopSpeech();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 SpeakJetStream::sizeOf(uInt8 code)
{
  switch(code)
  {
    case 20: case 21: case 22: case 23: case 24: case 25: case 26:
    case 28: case 29: case 30:
      return 2;
    default:
      return 1;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetStream::write(uInt8 code)
{
  // A byte arriving at an idle chip starts it now, not at the cycle count
  // of the last update, which may be long past
  if(idle())
  {
    myLastCycle = mySystem.cycles();
    mySampleFraction = 0.0;
  }

  Command command{code, 0, 1};
  if(myHalfCode >= 0)
  {
    command = {U8(myHalfCode), code, 2};
    myHalfCode = -1;
  }
  else if(sizeOf(code) == 2)
  {
    myHalfCode = code;
    return;
  }

  // A command the buffer has no room for is lost, as on the chip
  if(myBuffered + command.size > BUFFER_SIZE)
    return;

  myQueue.push_back(command);
  myBuffered += command.size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetStream::ready()
{
  // The AtariVox wires the chip's Buffer Half Full line to READY
  return myBuffered <= BUFFER_HALF;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetStream::reset()
{
  silence();
  myDecoder.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetStream::silence()
{
  myOSystem.sound().stopSpeech();
  myMSA.silence();
  myQueue.clear();
  myBuffered = myExecuting = 0;
  myHalfCode = -1;
  myLastCycle = mySystem.cycles();
  mySampleFraction = 0.0;
  myStarting = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetStream::idle() const
{
  return !myMSA.busy() && myQueue.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetStream::execute()
{
  while(!myMSA.busy())
  {
    // The command carried out last leaves the buffer once it is done
    myBuffered -= myExecuting;
    myExecuting = 0;
    if(myQueue.empty())
      return;

    const Command command = myQueue.front();
    myQueue.pop_front();
    myExecuting = command.size;
    myDecoder.write(command.code);
    if(command.size == 2)
      myDecoder.write(command.data);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetMSA::Next SpeakJetStream::lookahead() const
{
  const SpeakJet::State state = myDecoder.state();
  SpeakJetMSA::Next next{-1, state.params[std::to_underlying(SpeakJet::Param::Speed)],
                         state.nextScale};

  for(const Command& command: myQueue)
  {
    const uInt8 code = command.code;
    if(code >= SpeakJet::FIRST_SOUND && code <= SpeakJet::LAST_SOUND)
    {
      next.code = code;
      return next;
    }
    // A pause or a delay comes first
    if(code <= 6 || code == 30)
    {
      next.pause = true;
      return next;
    }

    switch(code)
    {
      case 7:
        if(next.scalePct != SpeakJet::SLOW_PCT)
          next.scalePct = SpeakJet::FAST_PCT;
        break;
      case 8:   next.scalePct = SpeakJet::SLOW_PCT;  break;
      case 21:  next.speed = command.data;           break;
      case 31:
        next.speed = SpeakJet::INITIAL_SPEED;
        next.scalePct = 100;
        break;
      default:  break;
    }
  }
  return {};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 SpeakJetStream::cyclesPerSecond() const
{
  // The controllers are made while the console is still being built, before
  // OSystem has it, so until then the NTSC CPU clock stands in
  return myOSystem.hasConsole()
      ? myOSystem.console().emulationTiming().cyclesPerSecond() : NTSC_CPU_HZ;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetStream::update()
{
  const uInt64 cycle = mySystem.cycles();

  // The cycle count goes back on a reset or a state load
  if(cycle < myLastCycle)
    myLastCycle = cycle;

  Sound& sound = myOSystem.sound();
  if(idle())
  {
    // Time passes unrendered while there is nothing to say, and what was
    // said plays out now rather than when speech resumes
    if(!myStarting)
      sound.flushSpeech();
    myMSA.rest();
    myLastCycle = cycle;
    mySampleFraction = 0.0;
    myStarting = true;
    return;
  }

  // As many samples as the CPU cycles since the last update are worth
  const uInt64 perSecond = cyclesPerSecond();
  if(perSecond == 0)
    return;

  const double owed = mySampleFraction +
      DBL(cycle - myLastCycle) * SpeakJetSynth::RATE / DBL(perSecond);
  const auto count = std::min(SZT(owed), MAX_RENDER);
  mySampleFraction = count < MAX_RENDER ? owed - DBL(count) : 0.0;
  myLastCycle = cycle;
  if(count == 0)
    return;

  // Speech is made level with the TIA, so at loud Volumes its peaks ease
  // into full scale rather than clip
  myPCM.resize(count);
  for(Int16& pcm: myPCM)
  {
    execute();
    pcm = limit(myMSA.next() * 32768.0);
  }

  // Too far ahead of real time, so this is dropped rather than queued
  if(sound.speechQueued() > MAX_QUEUED_MS)
    return;

  // Speech after silence gets its cushion back first; the chip said nothing
  // meanwhile, so this only delays it
  const uInt32 queued = sound.speechQueued();
  if(myStarting && queued < PRIME_MS)
  {
    const vector<Int16> lead(SpeakJetSynth::RATE * (PRIME_MS - queued) / 1000, 0);
    sound.queueSpeech(lead, SpeakJetSynth::RATE);
  }
  myStarting = false;

  sound.queueSpeech(myPCM, SpeakJetSynth::RATE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetStream::save(Serializer& out) const
{
  // The decoder has carried out exactly the commands no longer in the buffer
  ByteArray bytes;
  for(const Command& command: myQueue)
  {
    bytes.push_back(command.code);
    if(command.size == 2)
      bytes.push_back(command.data);
  }
  if(myHalfCode >= 0)
    bytes.push_back(U8(myHalfCode));

  try
  {
    // The serial port's flow control state, as it reads with no device attached
    out.putBool(true);
    out.putBool(false);

    if(!SpeakJet::putState(out, myDecoder.state()))
      return false;

    out.putByte(U8(bytes.size()));
    out.putByteArray(bytes);
  }
  catch(...)
  {
    cerr << "ERROR: SpeakJetStream::save\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetStream::load(Serializer& in)
{
  // Whatever was speaking belongs to the moment the loaded state replaces
  silence();

  SpeakJet::State state;
  ByteArray bytes;
  try
  {
    // The serial port's flow control state, which means nothing here
    in.getBool();
    in.getBool();

    if(!SpeakJet::getState(in, state))
      return false;

    bytes.resize(in.getByte());
    in.getByteArray(bytes);
  }
  catch(...)
  {
    cerr << "ERROR: SpeakJetStream::load\n";
    return false;
  }

  // What the chip had yet to say is said from here
  myDecoder.setState(state);
  for(const uInt8 code: bytes)
    write(code);

  return true;
}
