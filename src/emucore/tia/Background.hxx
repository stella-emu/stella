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

#ifndef BACKGROUND_HXX
#define BACKGROUND_HXX

class TIA;

#include "Serializable.hxx"
#include "bspf.hxx"

/**
  TIA background object. Holds the COLUBK color register and supplies the
  background color pixel at every clock where no higher-priority sprite or
  playfield object is visible.

  @author  Christian Speckner (DirtyHairy)
*/
class Background : public Serializable
{
  public:
    Background() = default;
    ~Background() override = default;

    /**
      Set the TIA instance.
     */
    void setTIA(TIA* tia) { myTIA = tia; }

    /**
      Reset to initial state.
     */
    void reset();

    /**
      COLUBK write: set the background color index.
     */
    void setColor(uInt8 color);

    /**
      Set the color used in "debug colors" mode.
     */
    void setDebugColor(uInt8 color);

    /**
      Enable/disable "debug colors" mode.
     */
    void enableDebugColors(bool enabled);

    /**
      Update internal state to reflect PAL color loss.
     */
    void applyColorLoss();

    /**
      Get the current background color index.
     */
    uInt8 getColor() const { return myColor; }

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

  private:
    /**
      Recalculate myColor from COLUBK, debug colors, and color loss.
     */
    void applyColors();

  private:
    // Current computed color (output of applyColors())
    uInt8 myColor{0};
    // COLUBK register value
    uInt8 myObjectColor{0};
    // Color override in "debug colors" mode
    uInt8 myDebugColor{0};
    // Whether "debug colors" mode is active
    bool myDebugEnabled{false};
    // Required for flushing the line cache
    TIA* myTIA{nullptr};

  private:
    // Following constructors and assignment operators not supported
    Background(const Background&) = delete;
    Background(Background&&) = delete;
    Background& operator=(const Background&) = delete;
    Background& operator=(Background&&) = delete;
};

#endif  // BACKGROUND_HXX
