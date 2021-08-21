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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CARTRIDGE_CDF_HXX
#define CARTRIDGE_CDF_HXX

class System;

#include "bspf.hxx"
#include "CartARM.hxx"

/**
  Cartridge class used for CDF/CDFJ/CDFJ+/CDFJ+MAX.

  CDFJ bankswitching for Atari games using ARM/C code.
  There are two variants supported:
  1) CDF/CDFJ - initial scheme with 32K ROM and 8K RAM
  2) CDFJ+/CDFJ+MAX - support for larger ROM sizes (64/128/256/512K) and RAM sizes (16/32K)

  Features:
  32 fast fetchers
  2 fast jump queues
  1 parameter queue
  3 channel digital audio/1 channel sampled sound (4 channel sampled audio for CDFJ+MAX)
  7 banks (4K) of atari code
  4K display data (16K and 32K available with CDFJ+)

  Note that for CDFJ+ and CDFJ+MAX, the same driver is used for all RAM/RAM combinations.
  It is left to the programmer to ensure that only the available RAM/ROM on the target device is used.

  Bankswitching Note:
  CDF/CDFJ uses $FFF5 through $FFFB (initial bank 6)
  CDFJ+/CDFJ+MAX uses $FFF4 through $FFFA (initial bank 0)

  The letters CDFJ stand for Chris, Darrell, Fred, and John.

  @authors: Darrell Spice Jr, Chris Walton, Fred Quimby, John Champeau
            Thomas Jentzsch, Stephen Anthony, Bradford W. Mott
*/
class CartridgeCDF : public CartridgeARM
{
  friend class CartridgeCDFWidget;
  friend class CartridgeCDFInfoWidget;
  friend class CartridgeRamCDFWidget;

  public:

    enum class CDFSubtype {
      CDF0,
      CDF1,
      CDFJ,
      CDFJplus,
      CDFJmax
    };

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeCDF(const ByteBuffer& image, size_t size, const string& md5,
                 const Settings& settings);
    ~CartridgeCDF() override = default;

  public:
    /**
      Reset device to its power-on state
    */
    void reset() override;

    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

     @param system The system the device should install itself in
     */
    void install(System& system) override;

    /**
      Install pages for the specified bank in the system.

      @param bank     The bank that should be installed in the system
      @param segment  The segment the bank should be using

      @return  true, if bank has changed
    */
    bool bank(uInt16 bank, uInt16 segment = 0) override;

    /**
      Get the current bank.

      @param address The address to use when querying the bank
    */
    uInt16 getBank(uInt16 address = 0) const override;

    /**
      Query the number of banks supported by the cartridge.
    */
    uInt16 romBankCount() const override;

    /**
      Patch the cartridge ROM.

      @param address  The ROM address to patch
      @param value    The value to place into the address
      @return    Success or failure of the patch operation
    */
    bool patch(uInt16 address, uInt8 value) override;

    /**
      Access the internal ROM image for this cartridge.

      @param size  Set to the size of the internal ROM image data
      @return  A reference to the internal ROM image data
    */
    const ByteBuffer& getImage(size_t& size) const override;

    /**
      Save the current state of this cart to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const override;

    /**
      Load the current state of this cart from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in) override;

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override;

    /**
      Used for Thumbulator to pass values back to the cartridge
    */
    uInt32 thumbCallback(uInt8 function, uInt32 value1, uInt32 value2) override;

    /**
      Query the internal RAM size of the cart.

      @return The internal RAM size
    */
    uInt32 internalRamSize() const override { return uInt32(myRAM.size()); }

    /**
      Read a byte from cart internal RAM.

      @return The value of the interal RAM byte
    */
    uInt8 internalRamGetValue(uInt16 addr) const override;

    /**
      Set if we are using CDFJ+ and CDFJ+MAX bankswitching
     */
    bool isCDFJplus() const;

    /**
      Size of SRAM (RAM) area in cart
     */
    uInt32 ramSize() const;

    /**
      Size of Flash memory (ROM) area in cart
     */
    uInt32 romSize() const;

#ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
                                 const GUI::Font& nfont, int x, int y, int w, int h) override;
    CartDebugWidget* infoWidget(GuiObject* boss, const GUI::Font& lfont,
                                const GUI::Font& nfont, int x, int y, int w, int h) override;
#endif

  public:
    /**
      Get the byte at the specified address.

      @return The byte at the specified address
    */
    uInt8 peek(uInt16 address) override;

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
      @return  True if the poke changed the device address space, else false
    */
    bool poke(uInt16 address, uInt8 value) override;

  private:
    /**
      Checks if startup bank randomization is enabled.  For this scheme,
      randomization is not supported, since the ARM code is always in a
      pre-defined bank, and we *must* start from there.
    */
    bool randomStartBank() const override { return false; }

    /**
      Sets the initial state of the DPC pointers and RAM
    */
    void setInitialState() override;

    /**
      Updates any data fetchers in music mode based on the number of
      CPU cycles which have passed since the last update.
    */
    void updateMusicModeDataFetchers();

    /**
      Call Special Functions
    */
    void callFunction(uInt8 value);

    uInt32 getDatastreamPointer(uInt8 index) const;
    void setDatastreamPointer(uInt8 index, uInt32 value);

    uInt32 getDatastreamIncrement(uInt8 index) const;

    uInt8 readFromDatastream(uInt8 index);

    uInt32 getWaveform(uInt8 index) const;
    uInt32 getFrequency(uInt8 index) const;
    uInt32 getWaveformSize(uInt8 index) const;
    uInt32 getSample(uInt8 index) const;
    uInt32 getSampleValue(uInt32 sampleaddress) const;

    void setupVersion();

  private:
    // The ROM image of the cartridge
    ByteBuffer myImage{nullptr};

    // The size of the ROM image
    size_t mySize{0};

    // Pointer to the program ROM image of the cartridge
    uInt8* myProgramImage{nullptr};

    // Pointer to the display ROM image of the cartridge
    uInt8* myDisplayImage{nullptr};

    // Pointer to the driver image in RAM
    uInt8* myDriverImage{nullptr};

    // The CDFJ 8K RAM image, used as:
    //   $0000 - 2K Driver
    //   $0800 - 4K Display Data
    //   $1800 - 2K C Variable & Stack
    // For CDFJ+ & CDFJ+MAX, used as:
    //   $0000 - 2K Driver
    //   $0800 - Display Data, C Variables & Stack
    std::array<uInt8, 32_KB> myRAM;

    // Indicates the offset into the ROM image (aligns to current bank)
    uInt16 myBankOffset{0};

    // System cycle count from when the last update to music data fetchers occurred
    uInt64 myAudioCycles{0};

    // ARM cycle count from when the last callFunction() occurred
    uInt64 myARMCycles{0};

    // The audio routines in the driver run in 32-bit mode and take advantage
    // of the FIQ Shadow Registers which are not accessible to 16-bit thumb
    // code.  As such, Thumbulator does not support them.  The driver supplies a
    // few 16-bit subroutines used to pass values from 16-bit to 32-bit.  The
    // Thumbulator will trap these calls and pass the appropriate information to
    // the Cartridge Class via callFunction() so it can emulate the 32 bit audio routines.

    /* Register usage for audio (DPC+/CDF/CDFJ+):
      r8  = channel0 accumulator
      r9  = channel1 accumulator
      r10 = channel2 accumulator
      r11 = channel0 frequency
      r12 = channel1 frequency
      r13 = channel2 frequency
      r14 = timer base  */

    // The music counters, ARM FIQ shadow registers r8, r9, r10
    std::array<uInt32, 4> myMusicCounters{0};

    // The music frequency, ARM FIQ shadow registers r11, r12, r13
    std::array<uInt32, 4> myMusicFrequencies{0};

    // The music waveform sizes
    std::array<uInt8, 4> myMusicWaveformSize{0};

    // Fractional CDF music, OSC clocks unused during the last update
    double myFractionalClocks{0.0};

    // Controls mode, lower nybble sets Fast Fetch, upper nybble sets audio
    // -0 = Fast Fetch ON
    // -F = Fast Fetch OFF
    // 0- = Packed Digital Sample
    // F- = 3 Voice Music
    uInt8 myMode{0xFF};

    // set to address of #value if last byte peeked was A9 (LDA #)
    uInt16 myLDAimmediateOperandAddress{0};

    // set to address of the JMP operand if last byte peeked was 4C
    // *and* the next two bytes in ROM are 00 00
    uInt16 myJMPoperandAddress{0};

    uInt8 myFastJumpActive{0};

    // Pointer to the array of datastream pointers
    uInt16 myDatastreamBase{0};

    // Pointer to the array of datastream increments
    uInt16 myDatastreamIncrementBase{0};

    // Pointer to the beginning of the waveform data block
    uInt16 myWaveformBase{0};

    // Pointer to the beginning of the frequency data block (CDFJ+MAX)
    uInt16 myFrequencyBase{0};

    // Amplitude stream index
    uInt8 myAmplitudeStream{0};

    // Mask for determining the index of the datastream during fastjump
    uInt8 myFastjumpStreamIndexMask{0};

    // The currently selected fastjump stream
    uInt8 myFastJumpStream{0};

    // CDF subtype
    CDFSubtype myCDFSubtype{CDFSubtype::CDF0};

  private:
    // Following constructors and assignment operators not supported
    CartridgeCDF() = delete;
    CartridgeCDF(const CartridgeCDF&) = delete;
    CartridgeCDF(CartridgeCDF&&) = delete;
    CartridgeCDF& operator=(const CartridgeCDF&) = delete;
    CartridgeCDF& operator=(CartridgeCDF&&) = delete;
};

#endif
