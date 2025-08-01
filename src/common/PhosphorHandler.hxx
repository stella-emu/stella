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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef PHOSPHOR_HANDLER_HXX
#define PHOSPHOR_HANDLER_HXX

#include "FrameBufferConstants.hxx"
#include "bspf.hxx"

class PhosphorHandler
{
  public:
    // Phosphor settings names
    static constexpr string_view SETTING_MODE = "tv.phosphor";
    static constexpr string_view SETTING_BLEND = "tv.phosblend";
    // Setting values of phosphor modes
    static constexpr string_view VALUE_BYROM = "byrom";
    static constexpr string_view VALUE_ALWAYS = "always";
    static constexpr string_view VALUE_AUTO_ON = "autoon";
    static constexpr string_view VALUE_AUTO = "auto";

    enum PhosphorMode: uInt8 {
      ByRom,
      Always,
      Auto_on,
      Auto,
      NumTypes
    };

    static constexpr string_view DEFAULT_BLEND = "50"; // align with myPhosphorPercent!

    PhosphorHandler() = default;
    ~PhosphorHandler() = default;

    bool initialize(bool enable, int blend);

    bool phosphorEnabled() const { return myUsePhosphor; }

    static PhosphorMode toPhosphorMode(string_view name);
    static string_view toPhosphorName(PhosphorMode type);

    /**
      Used to calculate an averaged color pixel for the 'phosphor' effect.

      @param c  RGB Color 1 (current frame)
      @param p  RGB Color 2 (previous frame)

      @return  Averaged value of the two RGB colors
    */
    static constexpr uInt32 getPixel(const uInt32 c, const uInt32 p)
    {
      // Mix current calculated frame with previous displayed frame
      const auto rc = static_cast<uInt8>(c),
                 gc = static_cast<uInt8>(c >> 8),
                 bc = static_cast<uInt8>(c >> 16),
                 rp = static_cast<uInt8>(p),
                 gp = static_cast<uInt8>(p >> 8),
                 bp = static_cast<uInt8>(p >> 16);

      return  ourPhosphorLUT[rc][rp] | (ourPhosphorLUT[gc][gp] << 8) |
              (ourPhosphorLUT[bc][bp] << 16);
    }

  private:
    // Use phosphor effect
    bool myUsePhosphor{false};

    // Amount to blend when using phosphor effect
    float myPhosphorPercent{0.50F};
    bool myLUTInitialized{false};

    // Precalculated averaged phosphor colors
    using PhosphorLUT = BSPF::array2D<uInt8, kColor, kColor>;
    static PhosphorLUT ourPhosphorLUT;

  private:
    PhosphorHandler(const PhosphorHandler&) = delete;
    PhosphorHandler(PhosphorHandler&&) = delete;
    PhosphorHandler& operator=(const PhosphorHandler&) = delete;
    PhosphorHandler& operator=(const PhosphorHandler&&) = delete;
};

#endif // PHOSPHOR_HANDLER_HXX
