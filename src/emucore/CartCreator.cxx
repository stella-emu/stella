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

#include "bspf.hxx"
#include "Cart.hxx"
#include "Cart03E0.hxx"
#include "Cart0840.hxx"
#include "Cart0FA0.hxx"
#include "Cart2K.hxx"
#include "Cart3E.hxx"
#include "Cart3EX.hxx"
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
#include "CartDF.hxx"
#include "CartDFSC.hxx"
#include "CartDPC.hxx"
#include "CartDPCPlus.hxx"
#include "CartE0.hxx"
#include "CartE7.hxx"
#include "CartEF.hxx"
#include "CartEFSC.hxx"
#include "CartEFF.hxx"
#include "CartF0.hxx"
#include "CartF4.hxx"
#include "CartF4SC.hxx"
#include "CartF6.hxx"
#include "CartF6SC.hxx"
#include "CartF8.hxx"
#include "CartF8SC.hxx"
#include "CartFA.hxx"
#include "CartFA2.hxx"
#include "CartFC.hxx"
#include "CartFE.hxx"
#include "CartGL.hxx"
#include "CartJANE.hxx"
#include "CartMDM.hxx"
#include "CartMVC.hxx"
#include "CartSB.hxx"
#include "CartTVBoy.hxx"
#include "CartUA.hxx"
#include "CartWD.hxx"
#include "CartWF8.hxx"
#include "CartX07.hxx"
#include "CartELF.hxx"
#include "MD5.hxx"
#include "Settings.hxx"

#include "CartDetector.hxx"
#include "CartCreator.hxx"

namespace  // anonymous namespace, to keep these functions private
{
  /**
    Create a cartridge from the entire image pointer.

    @param image    A pointer to the complete ROM image
    @param size     The size of the ROM image
    @param type     The bankswitch type of the ROM image
    @param md5      The md5sum for the ROM image
    @param settings The settings container

    @return  Pointer to the new cartridge object allocated on the heap
  */
  unique_ptr<Cartridge>
  createFromImage(const ByteBuffer& image, size_t size, Bankswitch::Type type,
                  string_view md5, Settings& settings)
  {
    // We should know the cart's type by now so let's create it
    switch(type)
    {
      using enum Bankswitch::Type;
      case _03E0: return std::make_unique<Cartridge03E0>(image, size, md5, settings);
      case _0840: return std::make_unique<Cartridge0840>(image, size, md5, settings);
      case _0FA0: return std::make_unique<Cartridge0FA0>(image, size, md5, settings);
      case _2K:   return std::make_unique<Cartridge2K>(image, size, md5, settings);
      case _3E:   return std::make_unique<Cartridge3E>(image, size, md5, settings);
      case _3EX:  return std::make_unique<Cartridge3EX>(image, size, md5, settings);
      case _3EP:  return std::make_unique<Cartridge3EPlus>(image, size, md5, settings);
      case _3F:   return std::make_unique<Cartridge3F>(image, size, md5, settings);
      case _4A50: return std::make_unique<Cartridge4A50>(image, size, md5, settings);
      case _4K:   return std::make_unique<Cartridge4K>(image, size, md5, settings);
      case _4KSC: return std::make_unique<Cartridge4KSC>(image, size, md5, settings);
      case AR:    return std::make_unique<CartridgeAR>(image, size, md5, settings);
      case BF:    return std::make_unique<CartridgeBF>(image, size, md5, settings);
      case BFSC:  return std::make_unique<CartridgeBFSC>(image, size, md5, settings);
      case BUS:   return std::make_unique<CartridgeBUS>(image, size, md5, settings);
      case CDF:   return std::make_unique<CartridgeCDF>(image, size, md5, settings);
      case CM:    return std::make_unique<CartridgeCM>(image, size, md5, settings);
      case CTY:   return std::make_unique<CartridgeCTY>(image, size, md5, settings);
      case CV:    return std::make_unique<CartridgeCV>(image, size, md5, settings);
      case DF:    return std::make_unique<CartridgeDF>(image, size, md5, settings);
      case DFSC:  return std::make_unique<CartridgeDFSC>(image, size, md5, settings);
      case DPC:   return std::make_unique<CartridgeDPC>(image, size, md5, settings);
      case DPCP:  return std::make_unique<CartridgeDPCPlus>(image, size, md5, settings);
      case E0:    return std::make_unique<CartridgeE0>(image, size, md5, settings);
      case E7:    return std::make_unique<CartridgeE7>(image, size, md5, settings);
      case EF:    return std::make_unique<CartridgeEF>(image, size, md5, settings);
      case EFF:   return std::make_unique<CartridgeEFF>(image, size, md5, settings);
      case EFSC:  return std::make_unique<CartridgeEFSC>(image, size, md5, settings);
      case F0:    return std::make_unique<CartridgeF0>(image, size, md5, settings);
      case F4:    return std::make_unique<CartridgeF4>(image, size, md5, settings);
      case F4SC:  return std::make_unique<CartridgeF4SC>(image, size, md5, settings);
      case F6:    return std::make_unique<CartridgeF6>(image, size, md5, settings);
      case F6SC:  return std::make_unique<CartridgeF6SC>(image, size, md5, settings);
      case F8:    return std::make_unique<CartridgeF8>(image, size, md5, settings);
      case F8SC:  return std::make_unique<CartridgeF8SC>(image, size, md5, settings);
      case FA:    return std::make_unique<CartridgeFA>(image, size, md5, settings);
      case FA2:   return std::make_unique<CartridgeFA2>(image, size, md5, settings);
      case FC:    return std::make_unique<CartridgeFC>(image, size, md5, settings);
      case FE:    return std::make_unique<CartridgeFE>(image, size, md5, settings);
      case GL:    return std::make_unique<CartridgeGL>(image, size, md5, settings);
      case JANE:  return std::make_unique<CartridgeJANE>(image, size, md5, settings);
      case MDM:   return std::make_unique<CartridgeMDM>(image, size, md5, settings);
      case UA:    return std::make_unique<CartridgeUA>(image, size, md5, settings);
      case UASW:  return std::make_unique<CartridgeUA>(image, size, md5, settings, true);
      case SB:    return std::make_unique<CartridgeSB>(image, size, md5, settings);
      case TVBOY: return std::make_unique<CartridgeTVBoy>(image, size, md5, settings);
      case WD:    [[fallthrough]];
      case WDSW:  return std::make_unique<CartridgeWD>(image, size, md5, settings);
      case WF8:   return std::make_unique<CartridgeWF8>(image, size, md5, settings);
      case X07:   return std::make_unique<CartridgeX07>(image, size, md5, settings);
      case ELF:   return std::make_unique<CartridgeELF>(image, size, md5, settings);
      default:    return nullptr;  // The remaining types have already been handled
    }
  }

  /**
    Create a cartridge from a multi-cart image pointer; internally this
    takes a slice of the ROM image ues that for the cartridge.

    @param image    A pointer to the complete ROM image
    @param size     The size of the ROM image slice
    @param numRoms  The number of ROMs in the multicart
    @param md5      The md5sum for the slice of the ROM image
    @param type     The detected type of the slice of the ROM image
    @param id       The ID for the slice of the ROM image
    @param settings The settings container

    @return  Pointer to the new cartridge object allocated on the heap
  */
  unique_ptr<Cartridge>
  createFromMultiCart(const ByteBuffer& image, size_t& size, uInt32 numRoms,
                      string& md5, Bankswitch::Type& type, string& id,
                      Settings& settings)
  {
    // Get a piece of the larger image
    uInt32 i = settings.getInt("romloadcount");

    // Move to the next game
    if(!settings.getBool("romloadprev"))
      i = (i + 1) % numRoms;
    else
      i = (i - 1) % numRoms;
    settings.setValue("romloadcount", i);

    size /= numRoms;
    const ByteBuffer slice = std::make_unique<uInt8[]>(size);
    std::copy_n(image.get() + i * size, size, slice.get());

    // We need a new md5 and name
    md5 = MD5::hash(slice, size);
    id = std::format(" [G{}]", i + 1);

    // TODO: allow using ROM properties instead of autodetect only
    if(size <= 2_KB)
      type = Bankswitch::Type::_2K;
    else if(size == 4_KB)
      type = Bankswitch::Type::_4K;
    else if(size == 8_KB || size == 16_KB || size == 32_KB || size == 64_KB || size == 128_KB)
      type = CartDetector::autodetectType(slice, size);
    else  /* default */
      type = Bankswitch::Type::_4K;

    return createFromImage(slice, size, type, md5, settings);
  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Cartridge> CartCreator::create(const FSNode& file,
    const ByteBuffer& image, size_t size, string& md5,
    string_view dtype, Settings& settings)
{
  unique_ptr<Cartridge> cartridge;
  Bankswitch::Type type = Bankswitch::nameToType(dtype), detectedType = type;
  string id;

  // First inspect the file extension itself
  // If a valid type is found, it will override the one passed into this method
  const Bankswitch::Type typeByName = Bankswitch::typeFromExtension(file);
  if(typeByName != Bankswitch::Type::AUTO)
    type = detectedType = typeByName;

  // See if we should try to auto-detect the cartridge type
  // If we ask for extended info, always do an autodetect
  bool autodetected = false;
  if(type == Bankswitch::Type::AUTO || settings.getBool("rominfo"))
  {
    detectedType = CartDetector::autodetectType(image, size);
    if(type != Bankswitch::Type::AUTO && type != detectedType)
      cerr << "Auto-detection not consistent: "
           << Bankswitch::typeToName(type) << ", "
           << Bankswitch::typeToName(detectedType) << '\n';

    type = detectedType;
    autodetected = true;
  }

  // Check for multicart first; if found, get the correct part of the image
  bool validMultiSize = true;
  uInt32 numMultiRoms = 0;
  switch(type)
  {
    using enum Bankswitch::Type;
    case _2IN1:
      numMultiRoms = 2;
      validMultiSize = (size == 2 * 2_KB || size == 2 * 4_KB || size == 2 * 8_KB || size == 2 * 16_KB || size == 2 * 32_KB);
      break;

    case _4IN1:
      numMultiRoms = 4;
      validMultiSize = (size == 4 * 2_KB || size == 4 * 4_KB || size == 4 * 8_KB || size == 4 * 16_KB);
      break;

    case _8IN1:
      numMultiRoms = 8;
      validMultiSize = (size == 8 * 2_KB || size == 8 * 4_KB || size == 8 * 8_KB);
      break;

    case _16IN1:
      numMultiRoms = 16;
      validMultiSize = (size == 16 * 2_KB || size == 16 * 4_KB || size == 16 * 8_KB);
      break;

    case _32IN1:
      numMultiRoms = 32;
      validMultiSize = (size == 32 * 2_KB || size == 32 * 4_KB);
      break;

    case _64IN1:
      numMultiRoms = 64;
      validMultiSize = (size == 64 * 2_KB || size == 64 * 4_KB);
      break;

    case _128IN1:
      numMultiRoms = 128;
      validMultiSize = (size == 128 * 2_KB || size == 128 * 4_KB);
      break;

    case MVC:
      cartridge = std::make_unique<CartridgeMVC>(file.getPath(), size, md5, settings);
      break;

    default:
      cartridge = createFromImage(image, size, detectedType, md5, settings);
      break;
  }

  if(numMultiRoms)
  {
    if(validMultiSize)
      cartridge = createFromMultiCart(image, size, numMultiRoms, md5, detectedType, id, settings);
    else
      throw std::runtime_error(std::format(
          "Invalid cart size for type '{}'", Bankswitch::typeToName(type)));
  }

  const string_view typeName = Bankswitch::typeToName(type);
  const string_view detectedTypeName = Bankswitch::typeToName(detectedType);

  const string sizeStr = (size < 1_KB)
      ? std::format("{}B", size)
      : std::format("{}K", size / 1_KB);

  const bool showDetectedType = numMultiRoms &&
             detectedType != Bankswitch::Type::_2K &&
             detectedType != Bankswitch::Type::_4K;

  const string detectedSuffix = showDetectedType
      ? std::format(" {}", detectedTypeName) : "";

  const string about = std::format("{}{} ({}{}) ",
      typeName, autodetected ? "*" : "", sizeStr, detectedSuffix);

  cartridge->setAbout(about, typeName, id);

  return cartridge;
}
