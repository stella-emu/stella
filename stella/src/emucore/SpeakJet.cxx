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
// $Id: SpeakJet.cxx,v 1.1 2006-06-11 07:13:19 urchlay Exp $
//============================================================================

#include "SpeakJet.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJet::SpeakJet()
{
  spawnThread();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJet::~SpeakJet()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJet::spawnThread()
{
  ourThread = SDL_CreateThread(thread, 0);
  ourInputSemaphore = SDL_CreateSemaphore(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SpeakJet::thread(void *data) {
  cerr << "rsynth thread spawned" << endl;
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJet::write(uInt8 code)
{
  const char *rsynthPhones = xlatePhoneme(code);
  cerr << "rsynth: \"" << rsynthPhones << "\"" << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char *SpeakJet::xlatePhoneme(uInt8 code)
{
  if(code <= 6)
    return " ";

  if(code >= 128 && code <= 199)
    return ourPhonemeTable[ code - 128 ];

  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 *SpeakJet::getSamples(int *count) {
  *count = 0;
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJet::chipReady()
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
Table of rsynth phonemes, indexed by SpeakJet phoneme code number.
Table is offset by 128 bytes (ourPhonemeTable[0] is code #128)
see rsynth/phones.def for definitions of rsynth phonemes.
We prefix a "'" to the rsynth phoneme for stress or a ","
for relax.
FIXME: This will need a lot of tweaking, once I get a real
SpeakJet to test with.
*/

const char *SpeakJet::ourPhonemeTable[] = {
//"rsynth phoneme(s) (phones.def)",   // SJ phonemes (p. 16 in SJ manual)
  "i:",   // 128 IY   See, Even, Feed
  "I",    // 129 IH   Sit, Fix, Pin
  "eI",   // 130 EY   Hair, Gate, Beige
  "E",    // 131 EH   Met, Check, Red
  "{",    // 132 AY   Hat, Fast, Fan
  "A:",   // 133 AX   Cotten // maybe "@" instead?
  "V",    // 134 UX   Luck, Up, Uncle
  "Q",    // 135 OH   Hot, Clock, Fox
  "A:",   // 136 AW   Father, Fall
  "oU",   // 137 OW   Comb, Over, Hold
  "U",    // 138 UH   Book, Could, Should
  "u:",   // 139 UW   Food, June
  "m",    // 140 MM   Milk, Famous,
  "n",    // 141 NE   Nip, Danger, Thin
  "n",    // 142 NO   No, Snow, On
  "N",    // 143 NGE  Think, Ping
  "N",    // 144 NGO  Hung, Song
  "l",    // 145 LE   Lake, Alarm, Lapel
  "l",    // 146 LO   Clock, Plus, Hello
  "w",    // 147 WW   Wool, Sweat
  "r",    // 148 RR   Ray, Brain, Over
  "I@",   // 149 IYRR Clear, Hear, Year
  "e@",   // 150 EYRR Hair, Stair, Repair
  "3:",   // 151 AXRR Fir, Bird, Burn
  "A:",   // 152 AWRR Part, Farm, Yarn
  "Qr",   // 153 OWRR Corn, Four, Your [*]
  "eI",   // 154 EYIY Gate, Ate, Ray
  "aI",   // 155 OHIY Mice, Fight, White
  "OI",   // 156 OWIY Boy, Toy, Voice
  "aI",   // 157 OHIH Sky, Five, I
  "j",    // 158 IYEH Yes, Yarn, Million
  "el",   // 159 EHLL Saddle, Angle, Spell [*]
  "U@",   // 160 IYUW Cute, Few // maybe u
  "aU",   // 161 AXUW Brown, Clown, Thousan
  "U@",   // 162 IHWW Two, New, Zoo
  "aU",   // 163 AYWW Our, Ouch, Owl
  "@U",   // 164 OWWW Go, Hello, Snow // maybe "oU"?
  "dZ",   // 165 JH   Dodge, Jet, Savage
  "v",    // 166 VV   Vest, Even,
  "z",    // 167 ZZ   Zoo, Zap
  "Z",    // 168 ZH   Azure, Treasure
  "D",    // 169 DH   There, That, This
  "b",    // 170 BE   Bear, Bird, Beed   ???
  "b",    // 171 BO   Bone, Book Brown ???
  "b",    // 172 EB   Cab, Crib, Web   ???
  "b",    // 173 OB   Bob, Sub, Tub   ???
  "d",    // 174 DE   Deep, Date, Divide   ???
  "d",    // 175 DO   Do, Dust, Dog   ???
  "d",    // 176 ED   Could, Bird   ???
  "d",    // 177 OD   Bud, Food   ???
  "g",    // 178 GE   Get, Gate, Guest,   ???
  "g",    // 179 GO   Got, Glue, Goo   ???
  "g",    // 180 EG   Peg, Wig   ???
  "g",    // 181 OG   Dog, Peg   ???
  "tS",   // 182 CH   Church, Feature, March
  "h",    // 183 HE   Help, Hand, Hair
  "h",    // 184 HO   Hoe, Hot, Hug
  "hw",   // 185 WH   Who, Whale, White [*]
  "f",    // 186 FF   Food, Effort, Off
  "s",    // 187 SE   See, Vest, Plus
  "s",    // 188 SO   So, Sweat      ???
  "S",    // 189 SH   Ship, Fiction, Leash
  "T",    // 190 TH   Thin, month
  "t",    // 191 TT   Part, Little, Sit
  "t",    // 192 TU   To, Talk, Ten
  "ts",   // 193 TS   Parts, Costs, Robots
  "k",    // 194 KE   Can't, Clown, Key
  "k",    // 195 KO   Comb, Quick, Fox ???
  "k",    // 196 EK   Speak, Task ???
  "k",    // 197 OK   Book, Took, October ???
  "p",    // 198 PE   People, Computer
  "p"     // 199 PO   Paw, Copy ???
};

