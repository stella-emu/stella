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

#ifndef PHOSPHOR_HANDLER_HXX
#define PHOSPHOR_HANDLER_HXX

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

    // Align with myPhosphorBlend!
    static constexpr string_view DEFAULT_BLEND = "50";

    PhosphorHandler() = default;
    ~PhosphorHandler() = default;

    bool initialize(bool enable, int blend);

    bool phosphorEnabled() const { return myUsePhosphor; }

    static PhosphorMode toPhosphorMode(string_view name);
    static string_view toPhosphorName(PhosphorMode type);

    /**
      Blend one scanline of the current frame with the persistent phosphor
      buffer, for the 'phosphor' effect.  Each colour channel becomes
      max(current, previous * blend%); both lines receive the result (the
      current line for display, the phosphor line as next frame's state).
      All four bytes of each pixel are blended; the unused/alpha byte is
      zero in all TV pipeline output and ignored by the TIA surface.

      @param curr   Current frame line (also receives the blended result)
      @param prev   Persistent phosphor line (also receives the blended result)
      @param width  Width of the lines, in pixels
    */
    void blendLine(uInt32* FORCE_RESTRICT curr, uInt32* FORCE_RESTRICT prev,
                   uInt32 width) const;

  private:
    // Use phosphor effect
    bool myUsePhosphor{false};

    // Percentage of the previous frame retained when blending (0 - 100)
    uInt32 myPhosphorBlend{50};

  private:
    PhosphorHandler(const PhosphorHandler&) = delete;
    PhosphorHandler(PhosphorHandler&&) = delete;
    PhosphorHandler& operator=(const PhosphorHandler&) = delete;
    PhosphorHandler& operator=(const PhosphorHandler&&) = delete;
};

#endif  // PHOSPHOR_HANDLER_HXX
