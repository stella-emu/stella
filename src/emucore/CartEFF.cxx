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

#include "CartEFF.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeEFF::CartridgeEFF(ByteSpan image, string_view md5,
                           const Settings& settings, size_t bsSize)
  : CartridgeEF(image, md5, settings, bsSize)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEFF::checkSwitchBank(uInt16 address, uInt8)
{
  address &= ROM_MASK;

  // Switch banks if necessary
  if((address >= 0x0FE0) && (address <= 0x0FEF))
  {
    bank(address - 0x0FE0);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeEFF::peek(uInt16 address)
{
  const uInt16 romAddress = address & ROM_MASK;
  if((address & 0x1000) && (romAddress >= 0x0FF0) && (romAddress <= 0x0FF4))
  {
    switch(romAddress)
    {
      case 0x0FF0:
        setI2CClock(false);
        break;
      case 0x0FF1:
        setI2CClock(true);
        break;
      case 0x0FF2:
        setI2CData(false);
        break;
      case 0x0FF3:
        setI2CData(true);
        break;
      case 0x0FF4:
        return readI2C();
      default:
        break;
    }
  }
  return CartridgeEF::peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEFF::poke(uInt16 address, uInt8 value)
{
  const uInt16 romAddress = address & ROM_MASK;
  if((address & 0x1000) && (romAddress >= 0x0FF0) && (romAddress <= 0x0FF3))
  {
    switch(romAddress)
    {
      case 0x0FF0:
        setI2CClock(false);
        setI2CClock(false);
        return false;

      case 0x0FF1:
        setI2CClock(true);
        setI2CClock(true);
        return false;

      case 0x0FF2:
        setI2CData(false);
        setI2CData(false);
        return false;

      case 0x0FF3:
        setI2CData(true);
        setI2CData(true);
        return false;

      default:
        break;
    }
  }
  return CartridgeEF::poke(address, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEFF::setI2CClock(bool value)
{
  myI2CClock = value;
  if(myEEPROM)
    myEEPROM->writeSCL(myI2CClock);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEFF::setI2CData(bool value)
{
  myI2CData = value;
  if(myEEPROM)
    myEEPROM->writeSDA(myI2CData);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeEFF::readI2C()
{
  if(myEEPROM)
    return myImage[0x0FF4 + (myEEPROM->readSDA() ? 1 : 0)];

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEFF::setNVRamFile(string_view pathname)
{
  myEEPROMFile = string{pathname} + "_eeprom.dat";
  if(mySystem)
    myEEPROM = std::make_unique<MT24LC16B>(FSNode(myEEPROMFile), *mySystem, nullptr);
  else
    cerr << "ERROR asked to set up eeprom before cartridge installed in system\n";
}
