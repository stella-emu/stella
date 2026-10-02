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

#ifndef MEMVIEW_DATA_LAYER_HXX
#define MEMVIEW_DATA_LAYER_HXX

#include "MemViewLayer.hxx"

/**
  This is the specific class for representing a visible layer for holding and
  drawing RAM/ROM memory data bytes for the Memory View functionality.

  @author Christian Hammers
*/

class MemViewDataLayer : public MemViewLayer
{

  public:

    // (The alpha values set to full are needed for Direct3D 11, which sometimes seems
    // to ignore the blend setting of the surface)

    static constexpr uInt32 DATA_COLOR_DEFAULT     = 0xFF626262;
    static constexpr uInt32 DATA_COLOR_FADED       = 0xFF777777;

    static constexpr uInt32 RAM_DATA_COLOR_DEFAULT = 0xFFC0C0C0;
    static constexpr uInt32 RAM_DATA_COLOR_FADED   = RAM_DATA_COLOR_DEFAULT;

    static constexpr uInt32 DATA_COLOR_FADE_VALUE  = 0x00060606;

  public:

    /**
      Constructor

      @param dialog       Boss dialog we are living in
      @param params       Reference to the parameters common for everything within this view
      @param defaultColor Default color to use when byte fade is switched off
      @param fadeColor    The start color to use when byte fade is active
    */
    MemViewDataLayer(Dialog& dialog, MemViewParams &params, uInt32 defaultColor, uInt32 fadeColor);

    ~MemViewDataLayer() override;

    /**
      Draws content if layer is visible.
    */
    void draw() override;

    /**
      Directely draw the layer to a given address using custom parameters

      @param params   Reference to a parameter struct (must not be the original
                      one of the object)
      @param destAddr Pointer where to draw the layer to
    */
    void drawDirect(MemViewParams& params, uInt32* destAddr);

    /**
      Renders content if layer is visible.
    */
    void render() override;

    /**
      Set the two visual parameters "byte inverted" and "byte faded" and update internal
      structs accordingly.

      @param inverted   Select if to show the data bytes inverted or not
      @param byteFade   Show a light fade in color from MSB to LSB
    */
    void setVisualParameters(bool inverted, bool byteFade);

    /**
      Updates the data to be shown

      @param data   Reference to a ByteArray - will be copied over in our internal members
    */
    void updateData(const ByteArray& data);

  public:

    bool myInverted{false};
    bool myByteFade{false};
    uInt32 myDefaultColor;
    uInt32 myFadedColor;

  private:

    static constexpr uInt32 BACKGROUND_COLOR = 0xFF202020;
    static constexpr uInt32 SEPARATOR_COLOR = 0xFFBD8632;

    // A precalulated table with all the bits in the correct color for each of
    // the possible 256 values
    std::array<uInt32, 256UZ * 8> myBitDataTab{};   // ARGB

    // Internal rearranged version of the data to be shown
    ByteArray myData;

    // Dummy with black bits in case the data is set not to be shown
    uIntArray myEmptyFields;

  private:

    void calcBitDataTab();

    void dataToFields();

  private:
    // Following constructors and assignment operators not supported
    MemViewDataLayer() = delete;
    MemViewDataLayer(const MemViewDataLayer&) = delete;
    MemViewDataLayer(MemViewDataLayer&&) = delete;
    MemViewDataLayer& operator=(const MemViewDataLayer&) = delete;
    MemViewDataLayer& operator=(MemViewDataLayer&&) = delete;

};

#endif  // MEMVIEW_DATA_LAYER_HXX
