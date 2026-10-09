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

#ifndef MEMVIEW_PARAMS_HXX
#define MEMVIEW_PARAMS_HXX

#include "Cart.hxx"
#include "bspf.hxx"
#include <vector>

/**
  This class encapsulates all common used parameters for all the different
  data and heatmap layers and the MemViewWidget itself.

  @author Christian Hammers
*/

class MemViewParams
{

  public:

    static constexpr int SEPARATOR_WIDTH = 2;
    static constexpr int SEPARATOR_HEIGHT = 2;

    struct LayoutParams {
      LayoutParams() : bankWidth{0}, bankHeight{0}, hBanks{0}, vBanks{0}, minZoomLevel{0} { };
      uInt16 bankWidth;     // Width of one bank in bytes
      uInt16 bankHeight;    // Height of one bank in bytes
      int hBanks;           // Number of horizontal banks next to each other
      int vBanks;           // Number of vertical banks on top of each other
      int minZoomLevel;     // Minimum zoom level (1..max)
    };

  public:

    /**
      Constructor

      @param bankSize     The size of one bank (RAM or ROM) processed by this instance of MemViewWidget
      @param bankCount    The number of banks processed by this instance of MemViewWidget
      @param baseAddress  Base address within system's address range
      @param cartridge    Reference to running cartridge to be able to calculate addresses
      @param posX         X position of surface within the window
      @param posY         Y position of surface within the window
    */
    MemViewParams(uInt16 bankSize, uInt16 bankCount, uInt32 baseAddress, Cartridge &cartridge,
      int posX, int posY);
    MemViewParams(const MemViewParams&) = default;

    ~MemViewParams() = default;

    /**
      Sets the size and position of the expected access data counters within our whole data range.

      @param size     Number of access counters to take over
      @param offset   Offset inside our whole data range to put the new data to

      @returns true if valid
    */
    bool setAccessDataParams(uInt32 size, uInt32 offset);

    /**
      Sets layout parameters to be used and calculates all depending values.

      @param params     LayoutParams struct holding the layout values
      @param singleRow  Flag to have all banks in a single row
      @param separators Flag to draw separators between the banks or not
    */
    void setLayoutParameters(LayoutParams& params, bool singleRow, bool separators);

    /**
      Some calculations
    */
    void layoutPreCalc();
    void layoutPostCalc();
    void layoutRecalc() { layoutPreCalc(); layoutPostCalc(); }

    /**
      Sets the decay rate to be used and precalculate depending values (0..100%)

      @param percentage The value
    */
    void setDecayRate(int percantage);

    /**
      Limit X position of the whole visible widget (0..mySurfaceWidth-1)
      to a position within our data (0..myCurrentWidth-1)

      @param x    X position to limit
      @return     Corrected position
    */
    int limitPosX(int x) const;

    /**
      Limit Y position of the whole visible widget (0..mySurfaceHeight-1)
      to a position within our data (0..myCurrentHeight-1)

      @param y    Y position to limit
      @return     Corrected position
    */
    int limitPosY(int y) const;

    enum PositionType: uInt8 {
      WITHIN_DATA                   = 0,
      WITHIN_VERTICAL_SEPARATOR     = (1U << 0U),
      WITHIN_HORIZONTAL_SEPARATOR   = (1U << 1U),
      WITHIN_BOTH_SEPARATORS        = WITHIN_VERTICAL_SEPARATOR | WITHIN_HORIZONTAL_SEPARATOR
    };
    using PositionFlags = uInt8;

    /**
      Returns the data position infos about a specific X, Y position within the currently
      visible data (not on the shown grey boarder!) with respect to the current zoom
      level and switch on/off separators.

      @param x            Horizontal position (0..myCurrentWidth-1)
      @param y            Vertical position (0..myCurrentHeight-1)
      @param byteOffset   Pointer to receive the calculated byte offset where the position is in
      @param xFracPixels  The horizontal fractional pixel position within the data byte where
                          the position is in (0..8*myZoomLevel-1)
      @param yFracPixels  The vertical fractional pixel position within the data byte where
                          the position is in (0..myZoomLevel-1)
      @return A flagfield indicating what kind of position that is:
                          WITHIN_DATA - The position is on a data byte
                          WITHIN_VERTICAL_SEPARATOR - The position is on a vertical separator
                            In this case xFracPixels delivers the number of vertical pixels
                            until the start of the next adjacent data byte.
                          WITHIN_HORIZONTAL_SEPARATOR - The position is on a horizontal separator
                            In this case yFracPixels delivers the number of horizontal pixel
                            lines until the start of the next data byte below.
                          WITHIN_BOTH_SEPARATORS - A combination of both cases above.
    */
    PositionFlags getPosition(int x, int y, unsigned int* byteOffset,
      int* xFracPixels, int* yFracPixels) const;

    /**
      Converts a byte offset within the rearranged data back to it's linear offset within
      the original data.

      @param byteOffset   Offset like coming from getPosition()
      @return Linear offset within the data
    */
    unsigned int getLinearOffset(unsigned int byteOffset) const;

    /**
      Converts a linear offset within the original data to the rearranged counterpart.

      @param offset   Linear offset within the original data
      @return Rearranged offset within the data
    */
    unsigned int getRearrangedOffset(unsigned int offset) const;

    /**
      Get the address (and bank number) of one data cell specified by a screen position within the
      currently shown data.

      @param x      X position (0..mySurfaceWidth-1)
      @param y      Y position (0..mySurfaceHeight-1)
      @param bank   Pointer to get back the bank number (if interested - else nullptr)
      @param offset Pointer to get back the linear (original) offset (if interested - else nullptr)
      @return The read and write addresses plus a flag if valid
    */
    Common::RwAddress getAddress(int x, int y, int* bank = nullptr, unsigned int* offset = nullptr) const;

    /**
      Get a bank origin address for one of our shown banks.

      @param bank   Bank number (0..myBankCount-1)
      @param PC     If this query is regarding the current PC, pass it here to help searching
      @return The read and write addresses plus a flag if valid
    */
    Common::RwAddress getBankOrigin(int bank, uInt16 PC = 0) const;

  public:

    uInt16 myBankSize;        // Bytes
    uInt16 myBankCount;       // Number of banks
    uInt32 myDataSize;        // Data size in bytes
    uInt32 myBaseAddress;     // Byte-Address

    Cartridge &myCartridge;   // Reference to cartidge to retrieve addresses
    int mySurfaceWidth{0};    // Pixel
    int mySurfaceHeight{0};   // Pixel
    int mySurfacePosX{0};     // Pixel
    int mySurfacePosY{0};     // Pixel
    int myOffsetX{0};         // Pixel
    int myOffsetY{0};         // Pixel
    uInt16 myBankWidth{1};    // Bytes
    uInt16 myBankHeight{1};   // Bytes
    int myHBanks{1};          // Number of horizontally banks
    int myVBanks{1};          // Number of vertically banks

    int myMinZoom{1};         // Minimum zoom level
    static constexpr int myMaxZoom{16};
    int myZoomLevel{1};       // Current zoom level
    bool mySingleRow{false};  // Single row is selected
    bool mySeparators{false}; // Show separators or not

    // Precalulated positions of the separators (of any) for faster access
    // The pair-ones hold the position together with the number of following pixel
    // (0..SEPARATOR_WIDTH-1 of SEPARATOR_HEIGHT-1 accordingly)
    std::vector<int> myVSepPos;                     // (within the range of myTotalWidth)
    std::vector<std::pair<int, int>> myAbsVSepPos;  // (within the range of myCurrentWidth)
    std::vector<int> myHSepPos;                     // (within the range of myTotalHeight)
    std::vector<std::pair<int, int>> myAbsHSepPos;  // (within the range of myCurrentHeight)

    int myTotalWidth{1};          // Pixel
    int myTotalHeight{1};         // Pixel
    int myCurrentWidth{1};        // Pixel
    int myCurrentHeight{1};       // Pixel
    int myLeftBorderWidth{0};     // Pixel
    int myRightBorderWidth{0};    // Pixel
    int myTopBorderHeight{0};     // Pixel
    int myBottomBorderHeight{0};  // Pixel
    int myTotalBitsX{8};          // Data bits of one line
    int myTotalBitsY{1};          // Data bits of one column
    int myTotalSepsWidth{0};      // Pixel
    int myTotalSepsHeight{0};     // Pixel
    int myTotalBytesX{1};         // Bytes per line
    int myBankRowSize{1};         // Total bytes of one bank row

    int myYStart{0};              // Y position within surface where first data is
    int myXFirstPixelReps{0};     // First vertical pixel repetitions of the first data to show
    int myYFirstPixelReps{0};     // First horizontal line repetitions of the first data to show
    int mySkipXPixels{0};         // Number of pixel to skip on each line at the beginning
    int myFirstBitCount{0};       // First number of bits to show
    int myBitOffset{0};           // Bit offset within the first data byte to show

    // Precalculated value for heatmap data decrements based on the user's decay rate slider setting
    double myAccessDataDecrement{4.0};

    uInt32 myAccessDataSize{0};
    uInt32 myAccessDataOffset{0};

  private:
    // Following constructors and assignment operators not supported
    MemViewParams() = delete;
    MemViewParams(MemViewParams&&) = delete;
    MemViewParams& operator=(const MemViewParams&) = delete;
    MemViewParams& operator=(MemViewParams&&) = delete;
};

#endif  // MEMVIEW_PARAMS_HXX
