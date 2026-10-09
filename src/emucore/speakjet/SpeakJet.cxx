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

#include "SpeakJet.hxx"

namespace {
  // Mnemonic and documented duration for MSA codes 128-254, from Table E;
  // the mnemonic also names the sample file
  struct CodeInfo { string_view name; uInt16 ms; };

  constexpr std::array<CodeInfo, 127> ourCodes = {{
  { "IY", 70 }, { "IH", 70 }, { "EY", 70 }, { "EH", 70 }, { "AY", 70 }, { "AX", 70 },
  { "UX", 70 }, { "OH", 70 }, { "AW", 70 }, { "OW", 70 }, { "UH", 70 }, { "UW", 70 },
  { "MM", 70 }, { "NE", 70 }, { "NO", 70 }, { "NGE", 70 }, { "NGO", 70 }, { "LE", 70 },
  { "LO", 70 }, { "WW", 70 }, { "RR", 70 }, { "IYRR", 200 }, { "EYRR", 200 },
  { "AXRR", 190 }, { "AWRR", 200 }, { "OWRR", 185 }, { "EYIY", 165 }, { "OHIY", 200 },
  { "OWIY", 225 }, { "OHIH", 185 }, { "IYEH", 170 }, { "EHLL", 140 }, { "IYUW", 180 },
  { "AXUW", 170 }, { "IHWW", 170 }, { "AYWW", 200 }, { "OWWW", 131 }, { "JH", 70 },
  { "VV", 70 }, { "ZZ", 70 }, { "ZH", 70 }, { "DH", 70 }, { "BE", 45 }, { "BO", 45 },
  { "EB", 10 }, { "OB", 10 }, { "DE", 45 }, { "DO", 45 }, { "ED", 10 }, { "OD", 10 },
  { "GE", 55 }, { "GO", 55 }, { "EG", 55 }, { "OG", 55 }, { "CH", 70 }, { "HE", 70 },
  { "HO", 70 }, { "WH", 70 }, { "FF", 70 }, { "SE", 40 }, { "SO", 40 }, { "SH", 50 },
  { "TH", 40 }, { "TT", 50 }, { "TU", 70 }, { "TS", 170 }, { "KE", 55 }, { "KO", 55 },
  { "EK", 55 }, { "OK", 45 }, { "PE", 99 }, { "PO", 99 }, { "R0", 80 }, { "R1", 80 },
  { "R2", 80 }, { "R3", 80 }, { "R4", 80 }, { "R5", 80 }, { "R6", 80 }, { "R7", 80 },
  { "R8", 80 }, { "R9", 80 }, { "A0", 300 }, { "A1", 101 }, { "A2", 102 }, { "A3", 540 },
  { "A4", 530 }, { "A5", 500 }, { "A6", 135 }, { "A7", 600 }, { "A8", 300 }, { "A9", 250 },
  { "B0", 200 }, { "B1", 270 }, { "B2", 280 }, { "B3", 260 }, { "B4", 300 }, { "B5", 100 },
  { "B6", 104 }, { "B7", 100 }, { "B8", 270 }, { "B9", 262 }, { "C0", 160 }, { "C1", 300 },
  { "C2", 182 }, { "C3", 120 }, { "C4", 175 }, { "C5", 350 }, { "C6", 160 }, { "C7", 260 },
  { "C8", 95 }, { "C9", 75 }, { "D0", 95 }, { "D1", 95 }, { "D2", 95 }, { "D3", 95 },
  { "D4", 95 }, { "D5", 95 }, { "D6", 95 }, { "D7", 95 }, { "D8", 95 }, { "D9", 95 },
  { "D10", 95 }, { "D11", 95 }, { "M0", 125 }, { "M1", 250 }, { "M2", 530 }
  }};

  // What pause codes 0-6 add to a phrase, in ms; a stop's closure after one
  // is added separately
  constexpr std::array<uInt32, 7> ourPauseMs = { 0, 90, 190, 642, 20, 47, 75 };

  // What Fast and Slow ask of the part of a sound Speed stretches, in percent
  constexpr uInt32 FAST_PCT = 65;
  constexpr uInt32 SLOW_PCT = 134;

  // The last of the continuants; the fricatives and stops follow
  constexpr uInt8 LAST_CONTINUANT = 164;
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJet::SpeakJet(Sink& sink)
  : mySink{sink}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string_view SpeakJet::nameOf(uInt8 code)
{
  if(code < FIRST_SOUND || code > LAST_SOUND)
    return {};

  return ourCodes[code - FIRST_SOUND].name;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJet::isVoiced(uInt8 code)
{
  // Table E's voiced groups run contiguously, from the vowels to the voiced
  // stops; everything from CH on is voiceless, and effects are not speech
  return code >= FIRST_SOUND && code <= 181;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJet::isContinuant(uInt8 code)
{
  return code >= FIRST_SOUND && code <= LAST_CONTINUANT;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJet::reset()
{
  myPending = Pending::None;
  myStress = myRelax = false;
  myNextScale = 100;
  myRepeat = 1;

  mySink.setParam(Param::Volume, DEFAULT_VOLUME);
  mySink.setParam(Param::Speed, DEFAULT_SPEED);
  mySink.setParam(Param::Pitch, DEFAULT_PITCH);
  mySink.setParam(Param::Bend, DEFAULT_BEND);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJet::write(uInt8 code)
{
  // The previous code took a value; this byte is it
  if(myPending != Pending::None)
  {
    switch(myPending)
    {
      case Pending::Volume:  mySink.setParam(Param::Volume, code);  break;
      case Pending::Speed:   mySink.setParam(Param::Speed, code);   break;
      case Pending::Pitch:   mySink.setParam(Param::Pitch, code);   break;
      case Pending::Bend:    mySink.setParam(Param::Bend, code);    break;
      case Pending::Repeat:  myRepeat = std::max<uInt8>(code, 1);     break;
      case Pending::Delay:   mySink.delay(U32(code) * 10);            break;
      default: break;
    }
    myPending = Pending::None;
    return;
  }

  // Allophones and sound effects
  if(code >= FIRST_SOUND && code <= LAST_SOUND)
  {
    const Utterance utterance{
      .code = code,
      .name = ourCodes[code - FIRST_SOUND].name,
      .durationMs = ourCodes[code - FIRST_SOUND].ms * myNextScale / 100,
      .scalePct = myNextScale,
      .stress = myStress,
      .relax = myRelax
    };

    // Repeat plays the sound several times over, just as writing it out would
    for(uInt8 i = 0; i < myRepeat; ++i)
      mySink.play(utterance);

    myStress = myRelax = false;
    myNextScale = 100;
    myRepeat = 1;
    return;
  }

  switch(code)
  {
    // Pauses
    case 0: case 1: case 2: case 3: case 4: case 5: case 6:
      mySink.pause(ourPauseMs[code]);
      break;

    // Fast and Slow, for the next sound only
    case 7:  myNextScale = FAST_PCT;  break;
    case 8:  myNextScale = SLOW_PCT;  break;

    // Stress and Relax apply to the next sound only
    case 14: myStress = true;  break;
    case 15: myRelax = true;   break;

    // Wait: the real chip holds off until a Start arrives.  Not modelled,
    // since nothing here can issue the Start
    case 16: break;

    case 20: myPending = Pending::Volume;  break;
    case 21: myPending = Pending::Speed;   break;
    case 22: myPending = Pending::Pitch;   break;
    case 23: myPending = Pending::Bend;    break;

    case 26: myPending = Pending::Repeat;  break;

    // Delay is silence in 10ms units, which Speed leaves alone
    case 30: myPending = Pending::Delay;   break;

    // PortCtr, Port, Call and Goto take a value byte we have no use for, but
    // it must still be swallowed
    case 24: case 25: case 28: case 29:
      myPending = Pending::Ignored;
      break;

    case 31: reset();  break;

    case 255: mySink.endOfPhrase();  break;

    // Codes 9-13, 17-19 and 32-127 are undocumented or reserved, but ROMs do
    // send them, so ignore them rather than choke
    default: break;
  }
}
