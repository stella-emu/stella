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

#include "bspf.hxx"
#include "Cart.hxx"
#include "Cart0840.hxx"
#include "Cart2K.hxx"
#include "Cart3E.hxx"
#include "Cart3EPlus.hxx"
#include "Cart3F.hxx"
#include "Cart4A50.hxx"
#include "Cart4K.hxx"
#include "Cart4KSC.hxx"
#include "CartAR.hxx"
#include "CartBF.hxx"
#include "CartBFSC.hxx"
#include "CartBUS.hxx"
#include "CartCDF.hxx"
#include "CartCM.hxx"
#include "CartCTY.hxx"
#include "CartCV.hxx"
#include "CartCVPlus.hxx"
#include "CartDASH.hxx"
#include "CartDF.hxx"
#include "CartDFSC.hxx"
#include "CartDPC.hxx"
#include "CartDPCPlus.hxx"
#include "CartE0.hxx"
#include "CartE7.hxx"
#include "CartE78K.hxx"
#include "CartEF.hxx"
#include "CartEFSC.hxx"
#include "CartF0.hxx"
#include "CartF4.hxx"
#include "CartF4SC.hxx"
#include "CartF6.hxx"
#include "CartF6SC.hxx"
#include "CartF8.hxx"
#include "CartF8SC.hxx"
#include "CartFA.hxx"
#include "CartFA2.hxx"
#include "CartFE.hxx"
#include "CartMDM.hxx"
#include "CartSB.hxx"
#include "CartUA.hxx"
#include "CartWD.hxx"
#include "CartX07.hxx"
#include "MD5.hxx"
#include "Props.hxx"

#include "CartDetector.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Cartridge> CartDetector::create(const FilesystemNode& file,
    const ByteBuffer& image, uInt32 size, string& md5,
    const string& propertiesType, Settings& settings)
{
  unique_ptr<Cartridge> cartridge;
  Bankswitch::Type type = Bankswitch::nameToType(propertiesType),
         detectedType = type;
  string id;

  // Collect some info about the ROM
  ostringstream buf;

  // First inspect the file extension itself
  // If a valid type is found, it will override the one passed into this method
  Bankswitch::Type typeByName = Bankswitch::typeFromExtension(file);
  if(typeByName != Bankswitch::Type::_AUTO)
    type = detectedType = typeByName;

  // See if we should try to auto-detect the cartridge type
  // If we ask for extended info, always do an autodetect
  if(type == Bankswitch::Type::_AUTO || settings.getBool("rominfo"))
  {
    detectedType = autodetectType(image, size);
    if(type != Bankswitch::Type::_AUTO && type != detectedType)
      cerr << "Auto-detection not consistent: "
           << Bankswitch::typeToName(type) << ", "
           << Bankswitch::typeToName(detectedType) << endl;

    type = detectedType;
    buf << Bankswitch::typeToName(type) << "*";
  }
  else
    buf << Bankswitch::typeToName(type);

  // Check for multicart first; if found, get the correct part of the image
  switch(type)
  {
    case Bankswitch::Type::_2IN1:
      // Make sure we have a valid sized image
      if(size == 2*2048 || size == 2*4096 || size == 2*8192 || size == 2*16384)
      {
        cartridge =
          createFromMultiCart(image, size, 2, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    case Bankswitch::Type::_4IN1:
      // Make sure we have a valid sized image
      if(size == 4*2048 || size == 4*4096 || size == 4*8192)
      {
        cartridge =
          createFromMultiCart(image, size, 4, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    case Bankswitch::Type::_8IN1:
      // Make sure we have a valid sized image
      if(size == 8*2048 || size == 8*4096 || size == 8*8192)
      {
        cartridge =
          createFromMultiCart(image, size, 8, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    case Bankswitch::Type::_16IN1:
      // Make sure we have a valid sized image
      if(size == 16*2048 || size == 16*4096 || size == 16*8192)
      {
        cartridge =
          createFromMultiCart(image, size, 16, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    case Bankswitch::Type::_32IN1:
      // Make sure we have a valid sized image
      if(size == 32*2048 || size == 32*4096)
      {
        cartridge =
          createFromMultiCart(image, size, 32, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    case Bankswitch::Type::_64IN1:
      // Make sure we have a valid sized image
      if(size == 64*2048 || size == 64*4096)
      {
        cartridge =
          createFromMultiCart(image, size, 64, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    case Bankswitch::Type::_128IN1:
      // Make sure we have a valid sized image
      if(size == 128*2048 || size == 128*4096)
      {
        cartridge =
          createFromMultiCart(image, size, 128, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    default:
      cartridge = createFromImage(image, size, detectedType, md5, settings);
      break;
  }

  if(size < 1024)
    buf << " (" << size << "B) ";
  else
    buf << " (" << (size/1024) << "K) ";

  cartridge->setAbout(buf.str(), Bankswitch::typeToName(type), id);

  return cartridge;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Cartridge>
CartDetector::createFromMultiCart(const ByteBuffer& image, uInt32& size,
    uInt32 numroms, string& md5, Bankswitch::Type type, string& id, Settings& settings)
{
  // Get a piece of the larger image
  uInt32 i = settings.getInt("romloadcount");
  size /= numroms;
  ByteBuffer slice = make_unique<uInt8[]>(size);
  memcpy(slice.get(), image.get()+i*size, size);

  // We need a new md5 and name
  md5 = MD5::hash(slice, size);
  ostringstream buf;
  buf << " [G" << (i+1) << "]";
  id = buf.str();

  // Move to the next game the next time this ROM is loaded
  settings.setValue("romloadcount", (i+1)%numroms);

  if(size <= 2048)       type = Bankswitch::Type::_2K;
  else if(size == 4096)  type = Bankswitch::Type::_4K;
  else if(size == 8192)  type = Bankswitch::Type::_F8;
  else  /* default */    type = Bankswitch::Type::_4K;

  return createFromImage(slice, size, type, md5, settings);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Cartridge>
CartDetector::createFromImage(const ByteBuffer& image, uInt32 size, Bankswitch::Type type,
                              const string& md5, Settings& settings)
{
  // We should know the cart's type by now so let's create it
  switch(type)
  {
    case Bankswitch::Type::_0840:
      return make_unique<Cartridge0840>(image, size, md5, settings);
    case Bankswitch::Type::_2K:
      return make_unique<Cartridge2K>(image, size, md5, settings);
    case Bankswitch::Type::_3E:
      return make_unique<Cartridge3E>(image, size, md5, settings);
    case Bankswitch::Type::_3EP:
      return make_unique<Cartridge3EPlus>(image, size, md5, settings);
    case Bankswitch::Type::_3F:
      return make_unique<Cartridge3F>(image, size, md5, settings);
    case Bankswitch::Type::_4A50:
      return make_unique<Cartridge4A50>(image, size, md5, settings);
    case Bankswitch::Type::_4K:
      return make_unique<Cartridge4K>(image, size, md5, settings);
    case Bankswitch::Type::_4KSC:
      return make_unique<Cartridge4KSC>(image, size, md5, settings);
    case Bankswitch::Type::_AR:
      return make_unique<CartridgeAR>(image, size, md5, settings);
    case Bankswitch::Type::_BF:
      return make_unique<CartridgeBF>(image, size, md5, settings);
    case Bankswitch::Type::_BFSC:
      return make_unique<CartridgeBFSC>(image, size, md5, settings);
    case Bankswitch::Type::_BUS:
      return make_unique<CartridgeBUS>(image, size, md5, settings);
    case Bankswitch::Type::_CDF:
      return make_unique<CartridgeCDF>(image, size, md5, settings);
    case Bankswitch::Type::_CM:
      return make_unique<CartridgeCM>(image, size, md5, settings);
    case Bankswitch::Type::_CTY:
      return make_unique<CartridgeCTY>(image, size, md5, settings);
    case Bankswitch::Type::_CV:
      return make_unique<CartridgeCV>(image, size, md5, settings);
    case Bankswitch::Type::_CVP:
      return make_unique<CartridgeCVPlus>(image, size, md5, settings);
    case Bankswitch::Type::_DASH:
      return make_unique<CartridgeDASH>(image, size, md5, settings);
    case Bankswitch::Type::_DF:
      return make_unique<CartridgeDF>(image, size, md5, settings);
    case Bankswitch::Type::_DFSC:
      return make_unique<CartridgeDFSC>(image, size, md5, settings);
    case Bankswitch::Type::_DPC:
      return make_unique<CartridgeDPC>(image, size, md5, settings);
    case Bankswitch::Type::_DPCP:
      return make_unique<CartridgeDPCPlus>(image, size, md5, settings);
    case Bankswitch::Type::_E0:
      return make_unique<CartridgeE0>(image, size, md5, settings);
    case Bankswitch::Type::_E7:
      return make_unique<CartridgeE7>(image, size, md5, settings);
    case Bankswitch::Type::_E78K:
      return make_unique<CartridgeE78K>(image, size, md5, settings);
    case Bankswitch::Type::_EF:
      return make_unique<CartridgeEF>(image, size, md5, settings);
    case Bankswitch::Type::_EFSC:
      return make_unique<CartridgeEFSC>(image, size, md5, settings);
    case Bankswitch::Type::_F0:
      return make_unique<CartridgeF0>(image, size, md5, settings);
    case Bankswitch::Type::_F4:
      return make_unique<CartridgeF4>(image, size, md5, settings);
    case Bankswitch::Type::_F4SC:
      return make_unique<CartridgeF4SC>(image, size, md5, settings);
    case Bankswitch::Type::_F6:
      return make_unique<CartridgeF6>(image, size, md5, settings);
    case Bankswitch::Type::_F6SC:
      return make_unique<CartridgeF6SC>(image, size, md5, settings);
    case Bankswitch::Type::_F8:
      return make_unique<CartridgeF8>(image, size, md5, settings);
    case Bankswitch::Type::_F8SC:
      return make_unique<CartridgeF8SC>(image, size, md5, settings);
    case Bankswitch::Type::_FA:
      return make_unique<CartridgeFA>(image, size, md5, settings);
    case Bankswitch::Type::_FA2:
      return make_unique<CartridgeFA2>(image, size, md5, settings);
    case Bankswitch::Type::_FE:
      return make_unique<CartridgeFE>(image, size, md5, settings);
    case Bankswitch::Type::_MDM:
      return make_unique<CartridgeMDM>(image, size, md5, settings);
    case Bankswitch::Type::_UA:
      return make_unique<CartridgeUA>(image, size, md5, settings);
    case Bankswitch::Type::_UASW:
      return make_unique<CartridgeUA>(image, size, md5, settings, true);
    case Bankswitch::Type::_SB:
      return make_unique<CartridgeSB>(image, size, md5, settings);
    case Bankswitch::Type::_WD:
      return make_unique<CartridgeWD>(image, size, md5, settings);
    case Bankswitch::Type::_X07:
      return make_unique<CartridgeX07>(image, size, md5, settings);
    default:
      return nullptr;  // The remaining types have already been handled
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Bankswitch::Type CartDetector::autodetectType(const ByteBuffer& image, uInt32 size)
{
  // Guess type based on size
  Bankswitch::Type type = Bankswitch::Type::_AUTO;

  if(isProbablyCVPlus(image, size))
  {
    type = Bankswitch::Type::_CVP;
  }
  else if((size % 8448) == 0 || size == 6144)
  {
    type = Bankswitch::Type::_AR;
  }
  else if(size < 2048)  // Sub2K images
  {
    type = Bankswitch::Type::_2K;
  }
  else if((size == 2048) ||
          (size == 4096 && memcmp(image.get(), image.get() + 2048, 2048) == 0))
  {
    type = isProbablyCV(image, size) ? Bankswitch::Type::_CV : Bankswitch::Type::_2K;
  }
  else if(size == 4096)
  {
    if(isProbablyCV(image, size))
      type = Bankswitch::Type::_CV;
    else if(isProbably4KSC(image, size))
      type = Bankswitch::Type::_4KSC;
    else
      type = Bankswitch::Type::_4K;
  }
  else if(size == 8*1024)  // 8K
  {
    // First check for *potential* F8
    uInt8 signature[2][3] = {
      { 0x8D, 0xF9, 0x1F },  // STA $1FF9
      { 0x8D, 0xF9, 0xFF }   // STA $FFF9
    };
    bool f8 = searchForBytes(image.get(), size, signature[0], 3, 2) ||
              searchForBytes(image.get(), size, signature[1], 3, 2);

    if(isProbablySC(image, size))
      type = Bankswitch::Type::_F8SC;
    else if(memcmp(image.get(), image.get() + 4096, 4096) == 0)
      type = Bankswitch::Type::_4K;
    else if(isProbablyE0(image, size))
      type = Bankswitch::Type::_E0;
    else if(isProbably3E(image, size))
      type = Bankswitch::Type::_3E;
    else if(isProbably3F(image, size))
      type = Bankswitch::Type::_3F;
    else if(isProbablyUA(image, size))
      type = Bankswitch::Type::_UA;
    else if(isProbablyFE(image, size) && !f8)
      type = Bankswitch::Type::_FE;
    else if(isProbably0840(image, size))
      type = Bankswitch::Type::_0840;
    else if(isProbablyE78K(image, size))
      type = Bankswitch::Type::_E78K;
    else
      type = Bankswitch::Type::_F8;
  }
  else if(size == 8*1024 + 3)  // 8195 bytes (Experimental)
  {
    type = Bankswitch::Type::_WD;
  }
  else if(size >= 10240 && size <= 10496)  // ~10K - Pitfall2
  {
    type = Bankswitch::Type::_DPC;
  }
  else if(size == 12*1024)  // 12K
  {
    type = Bankswitch::Type::_FA;
  }
  else if(size == 16*1024)  // 16K
  {
    if(isProbablySC(image, size))
      type = Bankswitch::Type::_F6SC;
    else if(isProbablyE7(image, size))
      type = Bankswitch::Type::_E7;
    else if(isProbably3E(image, size))
      type = Bankswitch::Type::_3E;
  /* no known 16K 3F ROMS
    else if(isProbably3F(image, size))
      type = Bankswitch::Type::_3F;
  */
    else
      type = Bankswitch::Type::_F6;
  }
  else if(size == 24*1024 || size == 28*1024)  // 24K & 28K
  {
    type = Bankswitch::Type::_FA2;
  }
  else if(size == 29*1024)  // 29K
  {
    if(isProbablyARM(image, size))
      type = Bankswitch::Type::_FA2;
    else /*if(isProbablyDPCplus(image, size))*/
      type = Bankswitch::Type::_DPCP;
  }
  else if(size == 32*1024)  // 32K
  {
    if (isProbablyCTY(image, size))
      type = Bankswitch::Type::_CTY;
    else if(isProbablySC(image, size))
      type = Bankswitch::Type::_F4SC;
    else if(isProbably3E(image, size))
      type = Bankswitch::Type::_3E;
    else if(isProbably3F(image, size))
      type = Bankswitch::Type::_3F;
    else if (isProbablyBUS(image, size))
      type = Bankswitch::Type::_BUS;
    else if (isProbablyCDF(image, size))
      type = Bankswitch::Type::_CDF;
    else if(isProbablyDPCplus(image, size))
      type = Bankswitch::Type::_DPCP;
    else if(isProbablyFA2(image, size))
      type = Bankswitch::Type::_FA2;
    else
      type = Bankswitch::Type::_F4;
  }
  else if(size == 60*1024)  // 60K
  {
    if(isProbablyCTY(image, size))
      type = Bankswitch::Type::_CTY;
    else
      type = Bankswitch::Type::_F4;
  }
  else if(size == 64*1024)  // 64K
  {
    if(isProbably3E(image, size))
      type = Bankswitch::Type::_3E;
    else if(isProbably3F(image, size))
      type = Bankswitch::Type::_3F;
    else if(isProbably4A50(image, size))
      type = Bankswitch::Type::_4A50;
    else if(isProbablyEF(image, size, type))
      ; // type has been set directly in the function
    else if(isProbablyX07(image, size))
      type = Bankswitch::Type::_X07;
    else
      type = Bankswitch::Type::_F0;
  }
  else if(size == 128*1024)  // 128K
  {
    if(isProbably3E(image, size))
      type = Bankswitch::Type::_3E;
    else if(isProbablyDF(image, size, type))
      ; // type has been set directly in the function
    else if(isProbably3F(image, size))
      type = Bankswitch::Type::_3F;
    else if(isProbably4A50(image, size))
      type = Bankswitch::Type::_4A50;
    else if(isProbablySB(image, size))
      type = Bankswitch::Type::_SB;
  }
  else if(size == 256*1024)  // 256K
  {
    if(isProbably3E(image, size))
      type = Bankswitch::Type::_3E;
    else if(isProbablyBF(image, size, type))
      ; // type has been set directly in the function
    else if(isProbably3F(image, size))
      type = Bankswitch::Type::_3F;
    else /*if(isProbablySB(image, size))*/
      type = Bankswitch::Type::_SB;
  }
  else  // what else can we do?
  {
    if(isProbably3E(image, size))
      type = Bankswitch::Type::_3E;
    else if(isProbably3F(image, size))
      type = Bankswitch::Type::_3F;
    else
      type = Bankswitch::Type::_4K;  // Most common bankswitching type
  }

  // Variable sized ROM formats are independent of image size and come last
  if(isProbablyDASH(image, size))
    type = Bankswitch::Type::_DASH;
  else if(isProbably3EPlus(image, size))
    type = Bankswitch::Type::_3EP;
  else if(isProbablyMDM(image, size))
    type = Bankswitch::Type::_MDM;

  return type;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::searchForBytes(const uInt8* image, uInt32 imagesize,
                                  const uInt8* signature, uInt32 sigsize,
                                  uInt32 minhits)
{
  uInt32 count = 0;
  for(uInt32 i = 0; i < imagesize - sigsize; ++i)
  {
    uInt32 matches = 0;
    for(uInt32 j = 0; j < sigsize; ++j)
    {
      if(image[i+j] == signature[j])
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
bool CartDetector::isProbablySC(const ByteBuffer& image, uInt32 size)
{
  // We assume a Superchip cart repeats the first 128 bytes for the second
  // 128 bytes in the RAM area, which is the first 256 bytes of each 4K bank
  const uInt8* ptr = image.get();
  while(size)
  {
    if(memcmp(ptr, ptr + 128, 128) != 0)
      return false;

    ptr  += 4096;
    size -= 4096;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyARM(const ByteBuffer& image, uInt32 size)
{
  // ARM code contains the following 'loader' patterns in the first 1K
  // Thanks to Thomas Jentzsch of AtariAge for this advice
  uInt8 signature[2][4] = {
    { 0xA0, 0xC1, 0x1F, 0xE0 },
    { 0x00, 0x80, 0x02, 0xE0 }
  };
  if(searchForBytes(image.get(), std::min(size, 1024u), signature[0], 4, 1))
    return true;
  else
    return searchForBytes(image.get(), std::min(size, 1024u), signature[1], 4, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably0840(const ByteBuffer& image, uInt32 size)
{
  // 0840 cart bankswitching is triggered by accessing addresses 0x0800
  // or 0x0840 at least twice
  uInt8 signature1[3][3] = {
    { 0xAD, 0x00, 0x08 },  // LDA $0800
    { 0xAD, 0x40, 0x08 },  // LDA $0840
    { 0x2C, 0x00, 0x08 }   // BIT $0800
  };
  for(uInt32 i = 0; i < 3; ++i)
    if(searchForBytes(image.get(), size, signature1[i], 3, 2))
      return true;

  uInt8 signature2[2][4] = {
    { 0x0C, 0x00, 0x08, 0x4C },  // NOP $0800; JMP ...
    { 0x0C, 0xFF, 0x0F, 0x4C }   // NOP $0FFF; JMP ...
  };
  for(uInt32 i = 0; i < 2; ++i)
    if(searchForBytes(image.get(), size, signature2[i], 4, 2))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably3E(const ByteBuffer& image, uInt32 size)
{
  // 3E cart bankswitching is triggered by storing the bank number
  // in address 3E using 'STA $3E', commonly followed by an
  // immediate mode LDA
  uInt8 signature[] = { 0x85, 0x3E, 0xA9, 0x00 };  // STA $3E; LDA #$00
  return searchForBytes(image.get(), size, signature, 4, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably3EPlus(const ByteBuffer& image, uInt32 size)
{
  // 3E+ cart is identified key 'TJ3E' in the ROM
  uInt8 tj3e[] = { 'T', 'J', '3', 'E' };
  return searchForBytes(image.get(), size, tj3e, 4, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably3F(const ByteBuffer& image, uInt32 size)
{
  // 3F cart bankswitching is triggered by storing the bank number
  // in address 3F using 'STA $3F'
  // We expect it will be present at least 2 times, since there are
  // at least two banks
  uInt8 signature[] = { 0x85, 0x3F };  // STA $3F
  return searchForBytes(image.get(), size, signature, 2, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably4A50(const ByteBuffer& image, uInt32 size)
{
  // 4A50 carts store address $4A50 at the NMI vector, which
  // in this scheme is always in the last page of ROM at
  // $1FFA - $1FFB (at least this is true in rev 1 of the format)
  if(image[size-6] == 0x50 && image[size-5] == 0x4A)
    return true;

  // Program starts at $1Fxx with NOP $6Exx or NOP $6Fxx?
  if(((image[0xfffd] & 0x1f) == 0x1f) &&
      (image[image[0xfffd] * 256 + image[0xfffc]] == 0x0c) &&
      ((image[image[0xfffd] * 256 + image[0xfffc] + 2] & 0xfe) == 0x6e))
    return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably4KSC(const ByteBuffer& image, uInt32 size)
{
  // We check if the first 256 bytes are identical *and* if there's
  // an "SC" signature for one of our larger SC types at 1FFA.

  uInt8 first = image[0];
  for(uInt32 i = 1; i < 256; ++i)
      if(image[i] != first)
        return false;

  if((image[size-6]=='S') && (image[size-5]=='C'))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyBF(const ByteBuffer& image, uInt32 size,
                                Bankswitch::Type& type)
{
  // BF carts store strings 'BFBF' and 'BFSC' starting at address $FFF8
  // This signature is attributed to "RevEng" of AtariAge
  uInt8 bf[]   = { 'B', 'F', 'B', 'F' };
  uInt8 bfsc[] = { 'B', 'F', 'S', 'C' };
  if(searchForBytes(image.get()+size-8, 8, bf, 4, 1))
  {
    type = Bankswitch::Type::_BF;
    return true;
  }
  else if(searchForBytes(image.get()+size-8, 8, bfsc, 4, 1))
  {
    type = Bankswitch::Type::_BFSC;
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyBUS(const ByteBuffer& image, uInt32 size)
{
  // BUS ARM code has 2 occurrences of the string BUS
  // Note: all Harmony/Melody custom drivers also contain the value
  // 0x10adab1e (LOADABLE) if needed for future improvement
  uInt8 bus[] = { 'B', 'U', 'S'};
  return searchForBytes(image.get(), size, bus, 3, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyCDF(const ByteBuffer& image, uInt32 size)
{
  // CDF ARM code has 3 occurrences of the string CDF
  // Note: all Harmony/Melody custom drivers also contain the value
  // 0x10adab1e (LOADABLE) if needed for future improvement
  uInt8 cdf[] = { 'C', 'D', 'F' };
  return searchForBytes(image.get(), size, cdf, 3, 3);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyCTY(const ByteBuffer& image, uInt32 size)
{
  uInt8 lenin[] = { 'L', 'E', 'N', 'I', 'N' };
  return searchForBytes(image.get(), size, lenin, 5, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyCV(const ByteBuffer& image, uInt32 size)
{
  // CV RAM access occurs at addresses $f3ff and $f400
  // These signatures are attributed to the MESS project
  uInt8 signature[2][3] = {
    { 0x9D, 0xFF, 0xF3 },  // STA $F3FF.X
    { 0x99, 0x00, 0xF4 }   // STA $F400.Y
  };
  if(searchForBytes(image.get(), size, signature[0], 3, 1))
    return true;
  else
    return searchForBytes(image.get(), size, signature[1], 3, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyCVPlus(const ByteBuffer& image, uInt32)
{
  // CV+ cart is identified key 'commavidplus' @ $04 in the ROM
  // We inspect only this area to speed up the search
  uInt8 cvp[12] = { 'c', 'o', 'm', 'm', 'a', 'v', 'i', 'd',
                    'p', 'l', 'u', 's' };
  return searchForBytes(image.get()+4, 24, cvp, 12, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyDASH(const ByteBuffer& image, uInt32 size)
{
  // DASH cart is identified key 'TJAD' in the ROM
  uInt8 tjad[] = { 'T', 'J', 'A', 'D' };
  return searchForBytes(image.get(), size, tjad, 4, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyDF(const ByteBuffer& image, uInt32 size,
                                Bankswitch::Type& type)
{

  // BF carts store strings 'DFDF' and 'DFSC' starting at address $FFF8
  // This signature is attributed to "RevEng" of AtariAge
  uInt8 df[]   = { 'D', 'F', 'D', 'F' };
  uInt8 dfsc[] = { 'D', 'F', 'S', 'C' };
  if(searchForBytes(image.get()+size-8, 8, df, 4, 1))
  {
    type = Bankswitch::Type::_DF;
    return true;
  }
  else if(searchForBytes(image.get()+size-8, 8, dfsc, 4, 1))
  {
    type = Bankswitch::Type::_DFSC;
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyDPCplus(const ByteBuffer& image, uInt32 size)
{
  // DPC+ ARM code has 2 occurrences of the string DPC+
  // Note: all Harmony/Melody custom drivers also contain the value
  // 0x10adab1e (LOADABLE) if needed for future improvement
  uInt8 dpcp[] = { 'D', 'P', 'C', '+' };
  return searchForBytes(image.get(), size, dpcp, 4, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyE0(const ByteBuffer& image, uInt32 size)
{
  // E0 cart bankswitching is triggered by accessing addresses
  // $FE0 to $FF9 using absolute non-indexed addressing
  // To eliminate false positives (and speed up processing), we
  // search for only certain known signatures
  // Thanks to "stella@casperkitty.com" for this advice
  // These signatures are attributed to the MESS project
  uInt8 signature[8][3] = {
    { 0x8D, 0xE0, 0x1F },  // STA $1FE0
    { 0x8D, 0xE0, 0x5F },  // STA $5FE0
    { 0x8D, 0xE9, 0xFF },  // STA $FFE9
    { 0x0C, 0xE0, 0x1F },  // NOP $1FE0
    { 0xAD, 0xE0, 0x1F },  // LDA $1FE0
    { 0xAD, 0xE9, 0xFF },  // LDA $FFE9
    { 0xAD, 0xED, 0xFF },  // LDA $FFED
    { 0xAD, 0xF3, 0xBF }   // LDA $BFF3
  };
  for(uInt32 i = 0; i < 8; ++i)
    if(searchForBytes(image.get(), size, signature[i], 3, 1))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyE7(const ByteBuffer& image, uInt32 size)
{
  // E7 cart bankswitching is triggered by accessing addresses
  // $FE0 to $FE6 using absolute non-indexed addressing
  // To eliminate false positives (and speed up processing), we
  // search for only certain known signatures
  // Thanks to "stella@casperkitty.com" for this advice
  // These signatures are attributed to the MESS project
  uInt8 signature[7][3] = {
    { 0xAD, 0xE2, 0xFF },  // LDA $FFE2
    { 0xAD, 0xE5, 0xFF },  // LDA $FFE5
    { 0xAD, 0xE5, 0x1F },  // LDA $1FE5
    { 0xAD, 0xE7, 0x1F },  // LDA $1FE7
    { 0x0C, 0xE7, 0x1F },  // NOP $1FE7
    { 0x8D, 0xE7, 0xFF },  // STA $FFE7
    { 0x8D, 0xE7, 0x1F }   // STA $1FE7
  };
  for(uInt32 i = 0; i < 7; ++i)
    if(searchForBytes(image.get(), size, signature[i], 3, 1))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyE78K(const ByteBuffer& image, uInt32 size)
{
  // E78K cart bankswitching is triggered by accessing addresses
  // $FE4 to $FE6 using absolute non-indexed addressing
  // To eliminate false positives (and speed up processing), we
  // search for only certain known signatures
  uInt8 signature[3][3] = {
    { 0xAD, 0xE4, 0xFF },  // LDA $FFE4
    { 0xAD, 0xE5, 0xFF },  // LDA $FFE5
    { 0xAD, 0xE6, 0xFF },  // LDA $FFE6
  };
  for(uInt32 i = 0; i < 3; ++i)
    if(searchForBytes(image.get(), size, signature[i], 3, 1))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyEF(const ByteBuffer& image, uInt32 size,
                                Bankswitch::Type& type)
{
  // Newer EF carts store strings 'EFEF' and 'EFSC' starting at address $FFF8
  // This signature is attributed to "RevEng" of AtariAge
  uInt8 efef[] = { 'E', 'F', 'E', 'F' };
  uInt8 efsc[] = { 'E', 'F', 'S', 'C' };
  if(searchForBytes(image.get()+size-8, 8, efef, 4, 1))
  {
    type = Bankswitch::Type::_EF;
    return true;
  }
  else if(searchForBytes(image.get()+size-8, 8, efsc, 4, 1))
  {
    type = Bankswitch::Type::_EFSC;
    return true;
  }

  // Otherwise, EF cart bankswitching switches banks by accessing addresses
  // 0xFE0 to 0xFEF, usually with either a NOP or LDA
  // It's likely that the code will switch to bank 0, so that's what is tested
  bool isEF = false;
  uInt8 signature[4][3] = {
    { 0x0C, 0xE0, 0xFF },  // NOP $FFE0
    { 0xAD, 0xE0, 0xFF },  // LDA $FFE0
    { 0x0C, 0xE0, 0x1F },  // NOP $1FE0
    { 0xAD, 0xE0, 0x1F }   // LDA $1FE0
  };
  for(uInt32 i = 0; i < 4; ++i)
  {
    if(searchForBytes(image.get(), size, signature[i], 3, 1))
    {
      isEF = true;
      break;
    }
  }

  // Now that we know that the ROM is EF, we need to check if it's
  // the SC variant
  if(isEF)
  {
    type = isProbablySC(image, size) ? Bankswitch::Type::_EFSC : Bankswitch::Type::_EF;
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyFA2(const ByteBuffer& image, uInt32)
{
  // This currently tests only the 32K version of FA2; the 24 and 28K
  // versions are easy, in that they're the only possibility with those
  // file sizes

  // 32K version has all zeros in 29K-32K area
  for(uInt32 i = 29*1024; i < 32*1024; ++i)
    if(image[i] != 0)
      return false;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyFE(const ByteBuffer& image, uInt32 size)
{
  // FE bankswitching is very weird, but always seems to include a
  // 'JSR $xxxx'
  // These signatures are attributed to the MESS project
  uInt8 signature[4][5] = {
    { 0x20, 0x00, 0xD0, 0xC6, 0xC5 },  // JSR $D000; DEC $C5
    { 0x20, 0xC3, 0xF8, 0xA5, 0x82 },  // JSR $F8C3; LDA $82
    { 0xD0, 0xFB, 0x20, 0x73, 0xFE },  // BNE $FB; JSR $FE73
    { 0x20, 0x00, 0xF0, 0x84, 0xD6 }   // JSR $F000; $84, $D6
  };
  for(uInt32 i = 0; i < 4; ++i)
    if(searchForBytes(image.get(), size, signature[i], 5, 1))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyMDM(const ByteBuffer& image, uInt32 size)
{
  // MDM cart is identified key 'MDMC' in the first 8K of ROM
  uInt8 mdmc[] = { 'M', 'D', 'M', 'C' };
  return searchForBytes(image.get(), std::min(size, 8192u), mdmc, 4, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablySB(const ByteBuffer& image, uInt32 size)
{
  // SB cart bankswitching switches banks by accessing address 0x0800
  uInt8 signature[2][3] = {
    { 0xBD, 0x00, 0x08 },  // LDA $0800,x
    { 0xAD, 0x00, 0x08 }   // LDA $0800
  };
  if(searchForBytes(image.get(), size, signature[0], 3, 1))
    return true;
  else
    return searchForBytes(image.get(), size, signature[1], 3, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyUA(const ByteBuffer& image, uInt32 size)
{
  // UA cart bankswitching switches to bank 1 by accessing address 0x240
  // using 'STA $240' or 'LDA $240'
  // Similar Brazilian cart bankswitching switches to bank 1 by accessing address 0x2C0
  // using 'BIT $2C0', 'STA $2C0' or 'LDA $2C0'
  uInt8 signature[6][3] = {
    { 0x8D, 0x40, 0x02 },  // STA $240
    { 0xAD, 0x40, 0x02 },  // LDA $240
    { 0xBD, 0x1F, 0x02 },  // LDA $21F,X
    { 0x2C, 0xC0, 0x02 },  // BIT $2C0
    { 0x8D, 0xC0, 0x02 },  // STA $2C0
    { 0xAD, 0xC0, 0x02 }   // LDA $2C0
  };
  for(uInt32 i = 0; i < 6; ++i)
    if(searchForBytes(image.get(), size, signature[i], 3, 1))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyX07(const ByteBuffer& image, uInt32 size)
{
  // X07 bankswitching switches to bank 0, 1, 2, etc by accessing address 0x08xd
  uInt8 signature[6][3] = {
    { 0xAD, 0x0D, 0x08 },  // LDA $080D
    { 0xAD, 0x1D, 0x08 },  // LDA $081D
    { 0xAD, 0x2D, 0x08 },  // LDA $082D
    { 0x0C, 0x0D, 0x08 },  // NOP $080D
    { 0x0C, 0x1D, 0x08 },  // NOP $081D
    { 0x0C, 0x2D, 0x08 }   // NOP $082D
  };
  for(uInt32 i = 0; i < 6; ++i)
    if(searchForBytes(image.get(), size, signature[i], 3, 1))
      return true;

  return false;
}
