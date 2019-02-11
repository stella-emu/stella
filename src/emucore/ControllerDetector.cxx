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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Settings.hxx"
#include "OSystem.hxx"

#include "ControllerDetector.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string ControllerDetector::detect(const BytePtr& image, uInt32 size,
                                  const string& controller, const Controller::Jack port,
                                  const OSystem& osystem)
{
  string type(controller);
  //string type("AUTO"); // dirty hack for testing!!!

  if(type == "AUTO" || osystem.settings().getBool("rominfo"))
  {
    string detectedType = autodetectPort(image, size, port, osystem);

    if(type != "AUTO" && type != detectedType)
    {
      cerr << "Controller auto-detection not consistent: "
        << type << ", " << detectedType << endl;
    }
    type = detectedType;
  }

  return type;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string ControllerDetector::autodetectPort(const BytePtr& image, uInt32 size,
                                          Controller::Jack port, const OSystem& osystem)
{
  // default type joystick
  string type = "JOYSTICK"; // TODO: remove magic strings

  if(isProbablyAtariVox(image, size, port))
    type = "ATARIVOX";
  else if (isProbablySaveKey(image, size, port))
    type = "SAVEKEY";
  else if(usesJoystickButtons(image, size, port))
  {
    if(isProbablyTrakBall(image, size))
      type = "TRAKBALL";
    else if(isProbablyAtariMouse(image, size))
      type = "ATARIMOUSE";
    else if(isProbablyAmigaMouse(image, size))
      type = "AMIGAMOUSE";
    else if(usesPaddleButtons(image, size, port, osystem))
      type = "KEYBOARD"; // only keyboard uses joystick and paddle buttons
      // TODO: Big Bird, Brain Games, Cookie Monster Munch (right),
      //   Game of Concentration, Grover's Music Maker (right)
      //   Holey Moley, Hunt & Score
      //   Monster Cise, Oscar's Trash Race (different, Peek-A-Boo
  }
  else
  {
    if(usesPaddleButtons(image, size, port, osystem))
      type = "PADDLES";
  }
  // TODO: GENESIS, BOOSTERGRIP, DRIVING, MINDLINK, ATARIVOX, KIDVID
  // not detectable: PADDLES_IAXIS, PADDLES_IAXDR
  return type;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::searchForBytes(const uInt8* image, uInt32 imagesize,
                                        const uInt8* signature, uInt32 sigsize,
                                        uInt32 minhits)
{
  uInt32 count = 0;
  for(uInt32 i = 0; i < imagesize - sigsize; ++i)
  {
    uInt32 matches = 0;
    for(uInt32 j = 0; j < sigsize; ++j)
    {
      if(image[i + j] == signature[j])
        ++matches;
      else
        break;
    }
    if(matches == sigsize)
    {
      ++count;
      i += sigsize;  // skip past this signature 'window' entirely
    }
    if(count >= minhits)
      break;
  }

  return (count >= minhits);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::usesJoystickButtons(const BytePtr& image, uInt32 size, Controller::Jack port)
{
  if(port == Controller::Left)
  {
    const int NUM_SIGS_0 = 13;
    const int SIG_SIZE_0 = 3;
    uInt8 signature_0[NUM_SIGS_0][SIG_SIZE_0] = {
      { 0x24, 0x0c, 0x10 }, // bit INPT4; bpl (joystick games only)
      { 0x24, 0x0c, 0x30 }, // bit INPT4; bmi (joystick games only)
      { 0xa5, 0x0c, 0x10 }, // lda INPT4; bpl (joystick games only)
      { 0xa5, 0x0c, 0x30 }, // lda INPT4; bmi (joystick games only)
      { 0xb5, 0x0c, 0x10 }, // lda INPT4,x; bpl (joystick games only)
      { 0xb5, 0x0c, 0x30 }, // lda INPT4,x; bmi (joystick games only)
      { 0x24, 0x3c, 0x10 }, // bit INPT4|$30; bpl (joystick games + Compumate)
      { 0x24, 0x3c, 0x30 }, // bit INPT4|$30; bmi (joystick, keyboard and mindlink games)
      { 0xa5, 0x3c, 0x10 }, // lda INPT4|$30; bpl (joystick and keyboard games)
      { 0xa5, 0x3c, 0x30 }, // lda INPT4|$30; bmi (joystick, keyboard and mindlink games)
      { 0xb5, 0x3c, 0x10 }, // lda INPT4|$30,x; bpl (joystick, keyboard and driving games)
      { 0xb5, 0x3c, 0x30 }, // lda INPT4|$30,x; bmi (joystick and keyboard games)
      { 0xb4, 0x0c, 0x30 }  // ldy INPT4|$30,x; bmi (joystick games only)
    };
    const int NUM_SIGS_1 = 5;
    const int SIG_SIZE_1 = 5;
    uInt8 signature_1[NUM_SIGS_1][SIG_SIZE_1] = {
      { 0xa5, 0x0c, 0x25, 0x0d, 0x10 }, // lda INPT4; and INPT5; bpl (joystick games only)
      { 0xa5, 0x0c, 0x25, 0x0d, 0x30 }, // lda INPT4; and INPT5; bmi (joystick games only)
      { 0xa5, 0x3c, 0x25, 0x3d, 0x10 }, // lda INPT4|$30; and INPT5|$30; bpl (joystick games only)
      { 0xa5, 0x3c, 0x25, 0x3d, 0x30 }, // lda INPT4|$30; and INPT5|$30; bmi (joystick games only)
      { 0xb5, 0x38, 0x29, 0x80, 0xd0 }, // lda INPT0|$30,y; and #$80; bne (Basic Programming)
    };

    for(uInt32 i = 0; i < NUM_SIGS_0; ++i)
      if(searchForBytes(image.get(), size, signature_0[i], SIG_SIZE_0))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_1; ++i)
      if(searchForBytes(image.get(), size, signature_1[i], SIG_SIZE_1))
        return true;
  }
  else if(port == Controller::Right)
  {
    const int NUM_SIGS = 12;
    const int SIG_SIZE = 3;
    uInt8 signature[NUM_SIGS][SIG_SIZE] = {
      { 0x24, 0x0d, 0x10 }, // bit INPT5; bpl (joystick games only)
      { 0x24, 0x0d, 0x30 }, // bit INPT5; bmi (joystick games only)
      { 0xa5, 0x0d, 0x10 }, // lda INPT5; bpl (joystick games only)
      { 0xa5, 0x0d, 0x30 }, // lda INPT5; bmi (joystick games only)
      { 0xb5, 0x0c, 0x10 }, // lda INPT4,x; bpl (joystick games only)
      { 0xb5, 0x0c, 0x30 }, // lda INPT4,x; bmi (joystick games only)
      { 0x24, 0x3d, 0x10 }, // bit INPT5|$30; bpl (joystick games, Compumate)
      { 0x24, 0x3d, 0x30 }, // bit INPT5|$30; bmi (joystick and keyboard games)
      { 0xa5, 0x3d, 0x10 }, // lda INPT5|$30; bpl (joystick games only)
      { 0xa5, 0x3d, 0x30 }, // lda INPT5|$30; bmi (joystick and keyboard games)
      { 0xb5, 0x3c, 0x10 }, // lda INPT4|$30,x; bpl (joystick, keyboard and driving games)
      { 0xb5, 0x3c, 0x30 }  // lda INPT4|$30,x; bmi (joystick and keyboard games)
    };

    const int NUM_SIGS_1 = 1;
    const int SIG_SIZE_1 = 5;
    uInt8 signature_1[NUM_SIGS_1][SIG_SIZE_1] = {
      { 0xb5, 0x38, 0x29, 0x80, 0xd0 }, // lda INPT0|$30,y; and #$80; bne (Basic Programming)
    };

    for(uInt32 i = 0; i < NUM_SIGS; ++i)
      if(searchForBytes(image.get(), size, signature[i], SIG_SIZE))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_1; ++i)
      if(searchForBytes(image.get(), size, signature_1[i], SIG_SIZE_1))
        return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::usesPaddleButtons(const BytePtr& image, uInt32 size,
                                           Controller::Jack port, const OSystem& osystem)
{
  // TODO: swap paddles/ports (uses INPT1/INPT3)
  if(port == Controller::Left)
  {
    const int NUM_SIGS_0 = 13;
    const int SIG_SIZE_0 = 3;
    uInt8 signature_0[NUM_SIGS_0][SIG_SIZE_0] = {
      //{ 0x24, 0x08, 0x10 }, // bit INPT0; bpl (many joystick games too!)
      //{ 0x24, 0x08, 0x30 }, // bit INPT0; bmi (joystick games: Spike's Peak, Sweat, Turbo!)
      { 0xa5, 0x08, 0x10 }, // lda INPT0; bpl (no joystick games)
      { 0xa5, 0x08, 0x30 }, // lda INPT0; bmi (no joystick games)
      { 0xb5, 0x08, 0x10 }, // lda INPT0,x; bpl (Duck Attack!)
      { 0xb5, 0x08, 0x30 }, // lda INPT0,x; bmi (no joystick games)
      { 0x24, 0x38, 0x10 }, // bit INPT0|$30; bpl (no joystick games)
      { 0x24, 0x38, 0x30 }, // bit INPT0|$30; bmi (no joystick games)
      { 0xa5, 0x38, 0x10 }, // lda INPT0|$30; bpl (no joystick games)
      { 0xa5, 0x38, 0x30 }, // lda INPT0|$30; bmi (no joystick games)
      { 0xb5, 0x38, 0x10 }, // lda INPT0|$30,x; bpl (Circus Atari, old code!)
      { 0xb5, 0x38, 0x30 }, // lda INPT0|$30,x; bmi (no joystick games)
      { 0x68, 0x48, 0x10 }, // pla; pha; bpl (i.a. Bachelor Party)
      { 0xa5, 0x3b, 0x30 }, // lda INPT3|$30; bmi (only Tac Scan, ports and paddles swapped)
      { 0xa5, 0x09, 0x30 }, // lda INPT3|$30; bmi (only Backgammon)
    };
    const int NUM_SIGS_1 = 2;
    const int SIG_SIZE_1 = 4;
    uInt8 signature_1[NUM_SIGS_1][SIG_SIZE_1] = {
      { 0xb9, 0x08, 0x00, 0x30 }, // lda INPT0,y; bmi (i.a. Encounter at L-5)
      { 0xb9, 0x38, 0x00, 0x30 }, // lda INPT0|$30,y; bmi (i.a. SW-Jedi Arena, Video Olympics)
    };
    const int NUM_SIGS_2 = 4;
    const int SIG_SIZE_2 = 5;
    uInt8 signature_2[NUM_SIGS_2][SIG_SIZE_2] = {
      { 0xb5, 0x38, 0x29, 0x80, 0xd0 }, // lda INPT0|$30,x; and #$80; bne (Basic Programming)
      { 0x24, 0x38, 0x85, 0x08, 0x10 }, // bit INPT0|$30; sta COLUPF, bpl (Fireball)
      { 0xb5, 0x38, 0x49, 0xff, 0x0a }, // lda INPT0|$30,x; eor #$ff; asl (Blackjack)
      { 0xb1, 0xf2, 0x30, 0x02, 0xe6 }  // lda ($f2),y; bmi...; inc (Warplock)
    };

    for(uInt32 i = 0; i < NUM_SIGS_0; ++i)
      if(searchForBytes(image.get(), size, signature_0[i], SIG_SIZE_0))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_1; ++i)
      if(searchForBytes(image.get(), size, signature_1[i], SIG_SIZE_1))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_2; ++i)
      if(searchForBytes(image.get(), size, signature_2[i], SIG_SIZE_2))
        return true;
  }
  else if(port == Controller::Right)
  {
    const int NUM_SIGS_0 = 16;
    const int SIG_SIZE_0 = 3;
    uInt8 signature_0[NUM_SIGS_0][SIG_SIZE_0] = {
      { 0x24, 0x0a, 0x10 }, // bit INPT2; bpl
      { 0x24, 0x0a, 0x30 }, // bit INPT2; bmi
      { 0xa5, 0x0a, 0x10 }, // lda INPT2; bpl
      { 0xa5, 0x0a, 0x30 }, // lda INPT2; bmi
      { 0xb5, 0x0a, 0x10 }, // lda INPT2,x; bpl
      { 0xb5, 0x0a, 0x30 }, // lda INPT2,x; bmi
      { 0xb5, 0x08, 0x10 }, // lda INPT0,x; bpl (no joystick games)
      { 0xb5, 0x08, 0x30 }, // lda INPT0,x; bmi (no joystick games)
      { 0x24, 0x3a, 0x10 }, // bit INPT2|$30; bpl
      { 0x24, 0x3a, 0x30 }, // bit INPT2|$30; bmi
      { 0xa5, 0x3a, 0x10 }, // lda INPT2|$30; bpl
      { 0xa5, 0x3a, 0x30 }, // lda INPT2|$30; bmi
      { 0xb5, 0x3a, 0x10 }, // lda INPT2|$30,x; bpl
      { 0xb5, 0x3a, 0x30 }, // lda INPT2|$30,x; bmi
      { 0xb5, 0x38, 0x10 }, // lda INPT0|$30,x; bpl  (Circus Atari, old code!)
      { 0xb5, 0x38, 0x30 }, // lda INPT0|$30,x; bmi (no joystick games)
    };
    const int NUM_SIGS_1 = 1;
    const int SIG_SIZE_1 = 4;
    uInt8 signature_1[NUM_SIGS_1][SIG_SIZE_1] = {
      { 0xb9, 0x38, 0x00, 0x30 }, // lda INPT0|$30,y; bmi (Video Olympics)
    };
    const int NUM_SIGS_2 = 3;
    const int SIG_SIZE_2 = 5;
    uInt8 signature_2[NUM_SIGS_2][SIG_SIZE_2] = {
      { 0xb5, 0x38, 0x29, 0x80, 0xd0 }, // lda INPT0|$30,x; and #$80; bne (Basic Programming)
      { 0x24, 0x38, 0x85, 0x08, 0x10 }, // bit INPT2|$30; sta COLUPF, bpl (Fireball, patched at runtime!)
      { 0xb5, 0x38, 0x49, 0xff, 0x0a }  // lda INPT0|$30,x; eor #$ff; asl (Blackjack)
    };

    for(uInt32 i = 0; i < NUM_SIGS_0; ++i)
      if(searchForBytes(image.get(), size, signature_0[i], SIG_SIZE_0))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_1; ++i)
      if(searchForBytes(image.get(), size, signature_1[i], SIG_SIZE_1))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_2; ++i)
      if(searchForBytes(image.get(), size, signature_2[i], SIG_SIZE_2))
        return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::isProbablyTrakBall(const BytePtr& image, uInt32 size)
{
  const int NUM_SIGS = 1;
  const int SIG_SIZE = 8;
  uInt8 signature[SIG_SIZE] = {
    0b1010, 0b1000, 0b1000, 0b1010, 0b0010, 0b0000, 0b0000, 0b0010 // NextTrackTbl
    // TODO: Omegamatrix's signature (.MovementTab_1)
  };

  if(searchForBytes(image.get(), size, signature, SIG_SIZE))
    return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::isProbablyAtariMouse(const BytePtr& image, uInt32 size)
{
  const int SIG_SIZE = 8;
  uInt8 signature[SIG_SIZE] = {
     0b0101, 0b0111, 0b0100, 0b0110, 0b1101, 0b1111, 0b1100, 0b1110 // NextTrackTbl
     // TODO: Omegamatrix's signature (.MovementTab_1)
  };

  if(searchForBytes(image.get(), size, signature, SIG_SIZE))
    return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::isProbablyAmigaMouse(const BytePtr& image, uInt32 size)
{
  const int SIG_SIZE = 8;
  uInt8 signature[SIG_SIZE] = {
     0b1100, 0b1000, 0b0100, 0b0000, 0b1101, 0b1001, 0b0101, 0b0001 // NextTrackTbl
     // TODO: Omegamatrix's signature (.MovementTab_1)
  };

  if(searchForBytes(image.get(), size, signature, SIG_SIZE))
    return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::isProbablyAtariVox(const BytePtr& image, uInt32 size, Controller::Jack port)
{
  // TOOD: analyse example code
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::isProbablySaveKey(const BytePtr& image, uInt32 size, Controller::Jack port)
{
  // known SaveKey code only supports right port
  if(port == Controller::Right)
  {
    const int SIG_SIZE = 9;
    uInt8 signature[SIG_SIZE] = { // from I2C_TXNACK
       0xa9, 0x10,        // lda #I2C_SCL_MASK*2
       0x8d, 0x80, 0x02,  // sta SWCHA
       0x4a,              // lsr
       0x8d, 0x81, 0x02   // sta SWACNT
    };

    return (searchForBytes(image.get(), size, signature, SIG_SIZE));
  }

  return false;
}
