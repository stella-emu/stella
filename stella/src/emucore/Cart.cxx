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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Cart.cxx,v 1.5 2002-12-15 05:49:04 bwmott Exp $
//============================================================================

#include <assert.h>
#include <string.h>
#include "Cart.hxx"
#include "Cart2K.hxx"
#include "Cart3F.hxx"
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
#include "MD5.hxx"
#include "Props.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge* Cartridge::create(const uInt8* image, uInt32 size,
    const Properties& properties)
{
  Cartridge* cartridge = 0;

  // Get the type of the cartridge we're creating
  string type = properties.get("Cartridge.Type");

  // See if we should try to auto-detect the cartridge type
  if(type == "Auto-detect")
  {
    type = autodetectType(image, size);
  }

  // We should know the cart's type by now so let's create it
  if(type == "2K")
    cartridge = new Cartridge2K(image);
  else if(type == "3F")
    cartridge = new Cartridge3F(image, size);
  else if(type == "4K")
    cartridge = new Cartridge4K(image);
  else if(type == "AR")
    cartridge = new CartridgeAR(image, size);
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
    cartridge = new CartridgeF8(image);
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
  else
  {
    // TODO: At some point this should be handled in a better way...
    assert(false);
  }

  return cartridge;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::Cartridge()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::~Cartridge()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge::autodetectType(const uInt8* image, uInt32 size)
{
  // The following is a simple table mapping games to type's using MD5 values
  struct MD5ToType
  {
    const char* md5;
    const char* type;
  };

  static MD5ToType table[] = {
    {"5336f86f6b982cc925532f2e80aa1e17", "E0"},    // Death Star
    {"b311ab95e85bc0162308390728a7361d", "E0"},    // Gyruss
    {"c29f8db680990cb45ef7fef6ab57a2c2", "E0"},    // Super Cobra
    {"085322bae40d904f53bdcc56df0593fc", "E0"},    // Tutankamn
    {"c7f13ef38f61ee2367ada94fdcc6d206", "E0"},    // Popeye
    {"6339d28c9a7f92054e70029eb0375837", "E0"},    // Star Wars, Arcade
    {"27c6a2ca16ad7d814626ceea62fa8fb4", "E0"},    // Frogger II
    {"3347a6dd59049b15a38394aa2dafa585", "E0"},    // Montezuma's Revenge
    {"6dda84fb8e442ecf34241ac0d1d91d69", "F6SC"},  // Dig Dug
    {"57fa2d09c9e361de7bd2aa3a9575a760", "F8SC"},  // Stargate
    {"3a771876e4b61d42e3a3892ad885d889", "F8SC"},  // Defender ][
    {"efefc02bbc5258815457f7a5b8d8750a", "FASC"},  // Tunnel runner
    {"7e51a58de2c0db7d33715f518893b0db", "FASC"},  // Mountain King
    {"9947f1ebabb56fd075a96c6d37351efa", "FASC"},  // Omega Race
    {"0443cfa9872cdb49069186413275fa21", "E7"},    // Burger Timer
    {"76f53abbbf39a0063f24036d6ee0968a", "E7"},    // Bump-N-Jump
    {"3b76242691730b2dd22ec0ceab351bc6", "E7"},    // He-Man
    {"ac7c2260378975614192ca2bc3d20e0b", "FE"},    // Decathlon
    {"4f618c2429138e0280969193ed6c107e", "FE"},    // Robot Tank
    {"6d842c96d5a01967be9680080dd5be54", "DPC"},   // Pitfall II
    {(char*)0,                           (char*)0}
  };

  // Get the MD5 message-digest for the ROM image
  string md5 = MD5(image, size);

  // Take a closer look at the ROM image and try to figure out its type
  const char* type = 0;

  // First we'll see if it's type is listed in the table above
  for(MD5ToType* entry = table; (entry->md5 != 0); ++entry)
  {
    if(entry->md5 == md5)
    {
      type = entry->type;
      break;
    }
  }

  // If we didn't find the type in the table then guess it based on size
  if(type == 0)
  {
    if((size % 8448) == 0)
    {
      type = "AR";
    }
    else if((size == 2048) || (memcmp(image, image + 2048, 2048) == 0))
    {
      type = "2K";
    }
    else if((size == 4096) || (memcmp(image, image + 4096, 4096) == 0))
    {
      type = "4K";
    }
    else if((size == 8192) || (memcmp(image, image + 8192, 8192) == 0))
    {
      type = "F8";
    }
    else if((size == 10495) || (size == 10240))
    {
      type = "DPC";
    }
    else if(size == 12288)
    {
      type = "FASC";
    }
    else if(size == 32768)
    {
      // Assume this is a 32K super-cart then check to see if it is
      type = "F4SC";

      uInt8 first = image[0];
      for(uInt32 i = 0; i < 256; ++i)
      {
        if(image[i] != first)
        {
          // It's not a super cart (probably)
          type = "F4";
          break;
        }
      }
    }
    else if(size == 65536)
    {
      type = "MB";
    }
    else if(size == 131072)
    {
      type = "MC";
    }
    else
    {
      // Assume this is a 16K super-cart then check to see if it is
      type = "F6SC";

      uInt8 first = image[0];
      for(uInt32 i = 0; i < 256; ++i)
      {
        if(image[i] != first)
        {
          // It's not a super cart (probably)
          type = "F6";
          break;
        }
      }
    }
  }

  return type;
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

