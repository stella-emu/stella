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

#ifndef CARTRIDGE_DETECTOR_HXX
#define CARTRIDGE_DETECTOR_HXX

#include "Bankswitch.hxx"
#include "FSNode.hxx"
#include "bspf.hxx"

/**
  Auto-detect cart type based on various attributes (file size, signatures,
  filenames, etc)

  @author  Stephen Anthony
*/
class CartDetector
{
  public:
    /**
      Try to auto-detect the bankswitching type of the cartridge

      @param image  A pointer to the ROM image
      @param size   The size of the ROM image

      @return The "best guess" for the cartridge type
    */
    static Bankswitch::Type autodetectType(const ByteBuffer& image, size_t size);

    /**
      MVC cartridges are of arbitary large length
      Returns size of frame if stream is probably an MVC movie cartridge
    */
    static size_t isProbablyMVC(const FSNode& rom);

    /**
      Returns true if the image is probably a HSC PlusROM
    */
    static bool isProbablyPlusROM(const ByteBuffer& image, size_t size);

  private:
    /**
      Search the image for the specified byte signature

      @param image      The ROM image as a span
      @param signature  The byte sequence to search for as a span
      @param minhits    The minimum number of times a signature is to be found

      @return  True if the signature was found at least 'minhits' time, else false
    */
    static bool searchForBytes(ByteSpan image, ByteSpan signature, uInt32 minhits = 1);

    /**
      Returns true if the image is probably a SuperChip (128 bytes RAM)
      Note: should be called only on ROMs with size multiple of 4K
    */
    static bool isProbablySC(ByteSpan image);

    /**
      Returns true if the image probably contains ARM code in the first 1K
    */
    static bool isProbablyARM(ByteSpan image);

    /**
      Returns true if the image is probably a 03E0 bankswitching cartridge
    */
    static bool isProbably03E0(ByteSpan image);

    /**
      Returns true if the image is probably a 0840 bankswitching cartridge
    */
    static bool isProbably0840(ByteSpan image);

    /**
      Returns true if the image is probably a Brazilian 0FA0 bankswitching cartridge
    */
    static bool isProbably0FA0(ByteSpan image);

    /**
      Returns true if the image is probably a 3E bankswitching cartridge
    */
    static bool isProbably3E(ByteSpan image);

    /**
    Returns true if the image is probably a 3EX bankswitching cartridge
    */
    static bool isProbably3EX(ByteSpan image);

    /**
      Returns true if the image is probably a 3E+ bankswitching cartridge
    */
    static bool isProbably3EPlus(ByteSpan image);

    /**
      Returns true if the image is probably a 3F bankswitching cartridge
    */
    static bool isProbably3F(ByteSpan image);

    /**
      Returns true if the image is probably a 4A50 bankswitching cartridge
    */
    static bool isProbably4A50(ByteSpan image);

    /**
      Returns true if the image is probably a 4K SuperChip (128 bytes RAM)
    */
    static bool isProbably4KSC(ByteSpan image);

    /**
      Returns true if the image is probably a BF/BFSC bankswitching cartridge
    */
    static bool isProbablyBF(ByteSpan image, Bankswitch::Type& type);

    /**
      Returns true if the image is probably a BUS bankswitching cartridge
    */
    static bool isProbablyBUS(ByteSpan image);

    /**
      Returns true if the image is probably a CDF bankswitching cartridge
    */
    static bool isProbablyCDF(ByteSpan image);

    /**
      Returns true if the image is probably a CTY bankswitching cartridge
    */
    static bool isProbablyCTY(ByteSpan image);

    /**
      Returns true if the image is probably a CV bankswitching cartridge
    */
    static bool isProbablyCV(ByteSpan image);

    /**
      Returns true if the image is probably a DF/DFSC bankswitching cartridge
    */
    static bool isProbablyDF(ByteSpan image, Bankswitch::Type& type);

    /**
      Returns true if the image is probably a DPC+ bankswitching cartridge
    */
    static bool isProbablyDPCplus(ByteSpan image);

    /**
      Returns true if the image is probably a E0 bankswitching cartridge
    */
    static bool isProbablyE0(ByteSpan image);

    /**
      Returns true if the image is probably a E7 bankswitching cartridge
    */
    static bool isProbablyE7(ByteSpan image);

    /**
      Returns true if the image is probably a E78K bankswitching cartridge
    */
    static bool isProbablyE78K(ByteSpan image);

    /**
      Returns true (and sets "type") if the image is probably an EF/EFSC bankswitching cartridge
    */
    static bool isProbablyEF(ByteSpan image, Bankswitch::Type& type);

    /**
       Returns true if the image is probably an EFF (Grizzards)
       bankswitching+eeprom cartridge
     */
    static bool isProbablyEFF(ByteSpan image);

    /**
      Returns true if the image is probably an F6 bankswitching cartridge
    */
    //static bool isProbablyF6(ByteSpan image);

    /**
      Returns true if the image is probably an FA2 bankswitching cartridge
    */
    static bool isProbablyFA2(ByteSpan image);

    /**
      Returns true if the image is probably an FC bankswitching cartridge
    */
    static bool isProbablyFC(ByteSpan image);

    /**
      Returns true if the image is probably an FE bankswitching cartridge
    */
    static bool isProbablyFE(ByteSpan image);

    /**
      Returns true if the image is probably a JANE cartridge (Tarzan)
    */
    static bool isProbablyJANE(ByteSpan image);

    /**
      Returns true if the image is probably a GameLine cartridge
    */
    static bool isProbablyGL(ByteSpan image);

    /**
      Returns true if the image is probably a MDM bankswitching cartridge
    */
    static bool isProbablyMDM(ByteSpan image);

    /**
      Returns true if the image is probably an MVC movie cartridge
    */
    static bool isProbablyMVC(ByteSpan image);

    /**
      Returns true if the image is probably a SB bankswitching cartridge
    */
    static bool isProbablySB(ByteSpan image);

    /**
      Returns true if the image is probably a TV Boy bankswitching cartridge
    */
    static bool isProbablyTVBoy(ByteSpan image);

    /**
      Returns true if the image is probably a UA bankswitching cartridge
    */
    static bool isProbablyUA(ByteSpan image);

    /**
      Returns true if the image is probably a Wickstead Design bankswitching cartridge
    */
    static bool isProbablyWD(ByteSpan image);

    /**
      Returns true if the image is probably an X07 bankswitching cartridge
    */
    static bool isProbablyX07(ByteSpan image);

    /**
      Returns true if the image is probably an ELF cartridge
    */
    static bool isProbablyELF(ByteSpan image);

  private:
    // Following constructors and assignment operators not supported
    CartDetector() = delete;
    ~CartDetector() = delete;
    CartDetector(const CartDetector&) = delete;
    CartDetector(CartDetector&&) = delete;
    CartDetector& operator=(const CartDetector&) = delete;
    CartDetector& operator=(CartDetector&&) = delete;
};

#endif
