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

#ifndef TIA_PLAYFIELD
#define TIA_PLAYFIELD

#include "Serializable.hxx"
#include "bspf.hxx"
#include "TIAConstants.hxx"

class TIA;

class Playfield : public Serializable
{
  public:
    /**
      The collision mask is injected at construction
     */
    explicit Playfield(uInt32 collisionMask);

  public:

    /**
      Set the TIA instance
     */
    void setTIA(TIA* tia) { myTIA = tia; }

    /**
      Reset to initial state.
     */
    void reset();

    /**
      PF0 write.
     */
    void pf0(uInt8 value);

    /**
      PF1 write.
     */
    void pf1(uInt8 value);

    /**
      PF2 write.
     */
    void pf2(uInt8 value);

    /**
      CTRLPF write.
     */
    void ctrlpf(uInt8 value);

    /**
      Enable / disable PF display (debugging only, not used during normal emulation).
     */
    void toggleEnabled(bool enabled);

    /**
      Enable / disable PF collisions (debugging only, not used during normal emulation).
     */
    void toggleCollisions(bool enabled);

    /**
      Set color PF.
     */
    void setColor(uInt8 color);

    /**
      Set color P0.
    */
    void setColorP0(uInt8 color);

    /**
      Set color P1.
    */
    void setColorP1(uInt8 color);

    /**
      Set the color used in "debug colors" mode.
     */
    void setDebugColor(uInt8 color);

    /**
      Enable "debug colors" mode.
     */
    void enableDebugColors(bool enabled);

    /**
      Update internal state to use the color loss palette.
     */
    void applyColorLoss();

    /**
      Notify playfield of line change,
     */
    void nextLine();

    /**
      Is the playfield signal active? This is determined by looking at bit 8
      of the collision mask.
     */
    bool isOn() const { return (collision & 0x8000); }

    /**
      Get the current color.
     */
    uInt8 getColor() const;

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

    /**
      Tick one color clock. Inline for performance.
     */
    void tick(uInt32 x)
    {
      myX = x;

      // Reflected flag is updated only at x = 0 or x = 79
      if (myX == TIAConstants::H_PIXEL / 2 || myX == 0) myRefp = myReflected;

      if (x & 0x03) return;

      uInt32 currentPixel;

      if (myEffectivePattern == 0) {
          currentPixel = 0;
      } else if (x < TIAConstants::H_PIXEL / 2) {
          currentPixel = myEffectivePattern & (1 << (x >> 2));
      } else if (myRefp) {
          currentPixel = myEffectivePattern & (1 << (39 - (x >> 2)));
      } else {
          currentPixel = myEffectivePattern & (1 << ((x >> 2) - 20));
      }

      collision = currentPixel ? myCollisionMaskEnabled : myCollisionMaskDisabled;
    }

  public:

    /**
      16 bit Collision mask. Each sprite is represented by a single bit in the mask
      (1 = active, 0 = inactive). All other bits are always 1. The highest bit is
      abused to store the active / inactive state (as the actual collision bit will
      always be zero if collisions are disabled).
     */
    uInt32 collision;

  private:

    /**
      Playfield mode.
     */
    enum class ColorMode: uInt8 {normal, score};

  private:

    /**
      Recalculate playfield color based on COLUPF, debug colors, color loss, etc.
     */
    void applyColors();

    /**
      Recalculate the playfield pattern from PF0, PF1 and PF2.
     */
    void updatePattern();

  private:

    /**
      Collision mask values for active / inactive states. Disabling collisions
      will change those.
     */
    uInt32 myCollisionMaskDisabled;
    uInt32 myCollisionMaskEnabled;

    /**
      Enable / disable PF (debugging).
     */
    bool myIsSuppressed;

    /**
      Left / right PF colors. Derifed from P0 / P1 color, COLUPF and playfield mode.
     */
    uInt8 myColorLeft;
    uInt8 myColorRight;

    /**
      P0 / P1 colors
     */
    uInt8 myColorP0;
    uInt8 myColorP1;

    /**
      COLUPF and debug colors
     */
    uInt8 myObjectColor, myDebugColor;

    /**
      Debug colors enabled?
     */
    bool myDebugEnabled;

    /**
     * Plafield mode.
     */
    ColorMode myColorMode;

    /**
      Pattern derifed from PF0, PF1, PF2
     */
    uInt32 myPattern;

    /**
      "Effective pattern". Will be 0 if playfield is disabled (debug), otherwise the same as myPattern.
     */
    uInt32 myEffectivePattern;

    /**
      Reflected mode on / off.
     */
    bool myReflected;

    /**
     * Are we currently drawing the reflected PF?
     */
    bool myRefp;

    /**
      PF registers.
    */
    uInt8 myPf0;
    uInt8 myPf1;
    uInt8 myPf2;

    /**
      The current scanline position (0 .. 159).
     */
    uInt32 myX;

    /**
      TIA instance.
     */
    TIA* myTIA;

  private:
    Playfield() = delete;
    Playfield(const Playfield&) = delete;
    Playfield(Playfield&&) = delete;
    Playfield& operator=(const Playfield&) = delete;
    Playfield& operator=(Playfield&&) = delete;
};

#endif // TIA_PLAYFIELD
