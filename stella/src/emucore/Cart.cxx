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
// $Id: Cart.cxx,v 1.28 2007-01-01 18:04:45 stephena Exp $
//============================================================================

#include <cassert>
#include <sstream>

#include "bspf.hxx"
#include "Cart.hxx"
#include "Cart2K.hxx"
#include "Cart3E.hxx"
#include "Cart3F.hxx"
#include "Cart4A50.hxx"
#include "Cart4K.hxx"
#include "CartAR.hxx"
#include "CartDPC.hxx"
#include "CartE0.hxx"
#include "CartE7.hxx"
#include "CartF4.hxx"
#include "CartF4SC.hxx"
#include "CartF6.hxx"
#include "CartF6SC.hxx"
#include "CartF8.hxx"
#include "CartF8SC.hxx"
#include "CartFASC.hxx"
#include "CartFE.hxx"
#include "CartMC.hxx"
#include "CartMB.hxx"
#include "CartCV.hxx"
#include "CartUA.hxx"
#include "MD5.hxx"
#include "Props.hxx"
#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge* Cartridge::create(const uInt8* image, uInt32 size,
    const Properties& properties, const Settings& settings)
{
  Cartridge* cartridge = 0;

  // Get the type of the cartridge we're creating
  const string& md5 = properties.get(Cartridge_MD5);
  string type = properties.get(Cartridge_Type);

  // First consider the ROMs that are special and don't have a properties entry
  // Hopefully this list will be very small
  if(md5 == "bc24440b59092559a1ec26055fd1270e" ||
     md5 == "75ee371ccfc4f43e7d9b8f24e1266b55")
  {
    // These two ROMs are normal 8K images, except they must be initialized
    // from the opposite bank compared to normal ones
    type = "F8 swapped";
  }

  // Collect some info about the ROM
  ostringstream buf;
  buf << "  Size of ROM: " << size << endl
      << "  Specified bankswitch type: " << type << endl;

  // See if we should try to auto-detect the cartridge type
  // If we ask for extended info, always do an autodetect
  if(type == "AUTO-DETECT" || settings.getBool("rominfo"))
  {
    string detected = autodetectType(image, size);
    buf << "  Auto-detected bankswitch type: " << detected << endl;
    if(type != "AUTO-DETECT" && type != detected)
      buf << "  ==> Bankswitch auto-detection not consistent" << endl;

    type = detected;
  }
  myAboutString = buf.str();

  // We should know the cart's type by now so let's create it
  if(type == "2K")
    cartridge = new Cartridge2K(image);
  else if(type == "3E")
    cartridge = new Cartridge3E(image, size);
  else if(type == "3F")
    cartridge = new Cartridge3F(image, size);
  else if(type == "4A50")
    cartridge = new Cartridge4A50(image);
  else if(type == "4K")
    cartridge = new Cartridge4K(image);
  else if(type == "AR")
    cartridge = new CartridgeAR(image, size, settings.getBool("fastscbios"));
  else if(type == "DPC")
    cartridge = new CartridgeDPC(image, size);
  else if(type == "E0")
    cartridge = new CartridgeE0(image);
  else if(type == "E7")
    cartridge = new CartridgeE7(image);
  else if(type == "F4")
    cartridge = new CartridgeF4(image);
  else if(type == "F4SC")
    cartridge = new CartridgeF4SC(image);
  else if(type == "F6")
    cartridge = new CartridgeF6(image);
  else if(type == "F6SC")
    cartridge = new CartridgeF6SC(image);
  else if(type == "F8")
    cartridge = new CartridgeF8(image, false);
  else if(type == "F8 swapped")
    cartridge = new CartridgeF8(image, true);
  else if(type == "F8SC")
    cartridge = new CartridgeF8SC(image);
  else if(type == "FASC")
    cartridge = new CartridgeFASC(image);
  else if(type == "FE")
    cartridge = new CartridgeFE(image);
  else if(type == "MC")
    cartridge = new CartridgeMC(image, size);
  else if(type == "MB")
    cartridge = new CartridgeMB(image);
  else if(type == "CV")
    cartridge = new CartridgeCV(image, size);
  else if(type == "UA")
    cartridge = new CartridgeUA(image);
  else
    cerr << "ERROR: Invalid cartridge type " << type << " ..." << endl;

  return cartridge;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::Cartridge()
{
  unlockBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::~Cartridge()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge::autodetectType(const uInt8* image, uInt32 size)
{
  // Guess type based on size
  const char* type = 0;

  if((size % 8448) == 0)
  {
    type = "AR";
  }
  else if((size == 2048) ||
          (size == 4096 && memcmp(image, image + 2048, 2048) == 0))
  {
    // TODO - autodetect CV
    type = "2K";
  }
  else if(size == 4096)
  {
    type = "4K";
  }
  else if(size == 8192)  // 8K
  {
    // TODO - autodetect FE and UA, probably not possible
    if(isProbablySC(image, size))
      type = "F8SC";
    else if(memcmp(image, image + 4096, 4096) == 0)
      type = "4K";
    else if(isProbablyE0(image, size))
      type = "E0";
    else if(isProbably3F(image, size))
      type = isProbably3E(image, size) ? "3E" : "3F";
    else
      type = "F8";
  }
  else if((size == 10495) || (size == 10496) || (size == 10240))  // 10K - Pitfall2
  {
    type = "DPC";
  }
  else if(size == 12288)  // 12K
  {
    // TODO - this should really be in a method that checks the first
    // 512 bytes of ROM and finds if either the lower 256 bytes or
    // higher 256 bytes are all the same.  For now, we assume that
    // all carts of 12K are CBS RAM Plus/FASC.
    type = "FASC";
  }
  else if(size == 16384)  // 16K
  {
    if(isProbablySC(image, size))
      type = "F6SC";
    else if(isProbablyE7(image, size))
      type = "E7";
    else if(isProbably3F(image, size))
      type = isProbably3E(image, size) ? "3E" : "3F";
    else
      type = "F6";
  }
  else if(size == 32768)  // 32K
  {
    if(isProbablySC(image, size))
      type = "F4SC";
    else if(isProbably3F(image, size))
      type = isProbably3E(image, size) ? "3E" : "3F";
    else
      type = "F4";
  }
  else if(size == 65536)  // 64K
  {
    // TODO - autodetect 4A50
    if(isProbably3F(image, size))
      type = isProbably3E(image, size) ? "3E" : "3F";
    else
      type = "MB";
  }
  else if(size == 131072)  // 128K
  {
    // TODO - autodetect 4A50
    if(isProbably3F(image, size))
      type = isProbably3E(image, size) ? "3E" : "3F";
    else
      type = "MC";
  }
  else  // what else can we do?
  {
    if(isProbably3F(image, size))
      type = isProbably3E(image, size) ? "3E" : "3F";
    else
      type = "4K";  // Most common bankswitching type
  }

  return type;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cartridge::searchForBytes(const uInt8* image, uInt32 size, uInt8 byte1, uInt8 byte2)
{
  uInt32 count = 0;
  for(uInt32 i = 0; i < size - 1; ++i)
  {
    if((image[i] == byte1) && (image[i + 1] == byte2))
    {
      ++count;
    }
  }

  return count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbablySC(const uInt8* image, uInt32 size)
{
  // We assume a Superchip cart contains the same bytes for its entire
  // RAM area; obviously this test will fail if it doesn't
  // The RAM area will be the first 256 bytes of each 4K bank
  uInt32 banks = size / 4096;
  for(uInt32 i = 0; i < banks; ++i)
  {
    uInt8 first = image[i*4096];
    for(uInt32 j = 0; j < 256; ++j)
    {
      if(image[i*4096+j] != first)
        return false;
    }
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbably3F(const uInt8* image, uInt32 size)
{
  return (searchForBytes(image, size, 0x85, 0x3F) > 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbably3E(const uInt8* image, uInt32 size)
{
  return (searchForBytes(image, size, 0x85, 0x3E) > 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbablyE0(const uInt8* image, uInt32 size)
{
  // E0 cart bankswitching is triggered by accessing addresses
  // $FE0 to $FF7 using absolute non-indexed addressing
  // So we search for the pattern 'LDA Fxxx' or 'STA Fxxx' in hex
  // using the regex (AD|8D, E0-F7, xF)
  // This must be present at least three times, since there are
  // three segments to be initialized (and a few more so that more
  // of the ROM is used)
  // Thanks to "stella@casperkitty.com" for this advice
  uInt32 count = 0;
  for(uInt32 i = 0; i < size - 2; ++i)
  {
    uInt8 b1 = image[i], b2 = image[i+1], b3 = image[i+2];
    if((b1 == 0xAD || b1 == 0x8D) &&
       (b2 >= 0xE0 && b2 <= 0xF7) &&
       (b3 & 0xF == 0xF))
    {
      if(++count > 4)  return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbablyE7(const uInt8* image, uInt32 size)
{
  // E7 carts map their second 1K block of RAM at addresses
  // $800 to $8FF.  However, since this occurs in the upper 2K address
  // space, and the last 2K in the cart always points to the last 2K of the
  // ROM image, the RAM area should fall in addresses $3800 to $38FF
  // Similar to the Superchip cart, we assume this RAM block contains
  // the same bytes for its entire area
  // Also, we want to distinguish between ROMs that have large blocks
  // of the same amount of (probably unused) data by making sure that
  // something differs in the previous 32 or next 32 bytes
  uInt8 first = image[0x3800];
  for(uInt32 i = 0x3800; i < 0x3A00; ++i)
  {
    if(first != image[i])
      return false;
  }

  // OK, now scan the surrounding 32 byte blocks
  uInt32 count1 = 0, count2 = 0;
  for(uInt32 i = 0x3800 - 32; i < 0x3800; ++i)
  {
    if(first != image[i])
      ++count1;
  }
  for(uInt32 i = 0x3A00; i < 0x3A00 + 32; ++i)
  {
    if(first != image[i])
      ++count2;
  }

  return (count1 > 0 || count2 > 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// default implementations of bankswitching-related methods.
// These are suitable to be inherited by a cart type that
// doesn't support bankswitching at all.

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::bank(uInt16 b)
{
  // do nothing.
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cartridge::bank()
{
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cartridge::bankCount()
{
  return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::patch(uInt16 address, uInt8 value)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::save(ofstream& out)
{
  int size = -1;

  uInt8* image = getImage(size);
  if(image == 0 || size <= 0)
  {
    cerr << "save not supported" << endl;
    return false;
  }

  for(int i=0; i<size; i++)
    out << image[i];

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* Cartridge::getImage(int& size)
{
  size = 0;
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::Cartridge(const Cartridge&)
{
  assert(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge& Cartridge::operator = (const Cartridge&)
{
  assert(false);
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge::myAboutString;
