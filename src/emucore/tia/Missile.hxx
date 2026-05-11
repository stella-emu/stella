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

#ifndef MISSILE_HXX
#define MISSILE_HXX

class TIA;
class Player;

#include "Serializable.hxx"
#include "bspf.hxx"
#include "TIAConstants.hxx"

/**
  TIA missile sprite object (M0 or M1). Emulates the horizontal counter,
  draw counter, HMOVE movement, and all associated registers (ENAM, HMM,
  RESM, RESMP, NUSIZ), producing a per-clock collision mask and color
  output.

  @author  Christian Speckner (DirtyHairy)
*/
class Missile : public Serializable
{
  public:
    explicit Missile(uInt32 collisionMask);
    ~Missile() override = default;

    /**
      Set the TIA instance.
     */
    void setTIA(TIA* tia) { myTIA = tia; }

    /**
      Reset to initial state.
     */
    void reset();

    /**
      ENAM0/1 write: bit 1 enables the missile.
     */
    void enam(uInt8 value);

    /**
      HMM0/1 write: set horizontal motion (bits 7-4).
     */
    void hmm(uInt8 value);

    /**
      RESM0/1 write: reset the horizontal position counter.
     */
    void resm(uInt8 counter, bool hblank);

    /**
      RESMP0/1 write: when bit 1 is set, lock the missile position to its
      associated player.
     */
    void resmp(uInt8 value, const Player& player);

    /**
      NUSIZ0/1 write: update missile size and copy count.
     */
    void nusiz(uInt8 value);

    /**
      Called when HMOVE is strobed: arm the movement counter.
     */
    void startMovement();

    /**
      Advance to the next scanline.
     */
    void nextLine();

    /**
      Set the missile color from COLUP0/1.
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
      Enable/disable the "inverted movement clock phase" quirk. This emulates
      a phase difference between movement and ordinary clock pulses found in
      some TIA revisions (e.g. the Kool Aid Man bug on Jr. models).
     */
    void setInvertedPhaseClock(bool enable);

    /**
      Enable/disable the "short late HMOVE" quirk.
     */
    void setShortLateHMove(bool enable);

    /**
      Enable/disable collision detection (debugging only).
     */
    void toggleCollisions(bool enabled);

    /**
      Enable/disable missile display (debugging only).
     */
    void toggleEnabled(bool enabled);

    /**
      Is the missile currently visible? Determined from bit 15 of the collision mask.
     */
    bool isOn() const { return (collision & 0x8000); }

    /**
      Get the current missile color.
     */
    uInt8 getColor() const;

    /**
      Get/set the sprite position derived from the counter. Used by the debugger only.
     */
    uInt8 getPosition() const;
    void setPosition(uInt8 newPosition);

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

    /**
      Process one HMOVE clock. Inline for performance (implementation below).
     */
    FORCE_INLINE void movementTick(uInt8 clock, uInt8 hclock, bool hblank);

    /**
      Tick one color clock. Inline for performance (implementation below).
     */
    FORCE_INLINE void tick(uInt8 hclock, bool isReceivingMclock = true);

  public:
    // 16-bit collision mask; bit 15 encodes current visibility
    uInt32 collision{0};
    // True while HMOVE movement clocks are being propagated
    bool isMoving{false};

  private:
    /**
      Recalculate the effective enabled state from myEnam, myResmp, and myIsSuppressed.
     */
    void updateEnabled();

    /**
      Recalculate myColor from COLUP0/1, debug colors, and color loss.
     */
    void applyColors();

  private:
    enum Count: Int8 {
      // Render counter start value; display begins when it reaches 0
      renderCounterOffset = -4
    };

  private:
    // Collision mask value when the missile is invisible
    uInt32 myCollisionMaskDisabled{0};
    // Collision mask value when the missile is visible
    uInt32 myCollisionMaskEnabled{0xFFFF};

    // Computed enabled state (from myEnam, myResmp, and myIsSuppressed)
    bool myIsEnabled{false};
    // Suppressed by the debugger
    bool myIsSuppressed{false};
    // ENAM register bit (bit 1)
    bool myEnam{false};
    // RESMP register bit; when set, missile is locked to its player
    uInt8 myResmp{0};

    // Number of HMOVE clocks from HMM register
    uInt8 myHmmClocks{0};
    // Horizontal position counter (0-159)
    uInt8 myCounter{0};

    // Pixel width as configured by NUSIZ
    uInt8 myWidth{1};
    // Actual width this tick; may differ from myWidth in starfield mode
    uInt8 myEffectiveWidth{1};

    // Render latch; set when the counter hits a decode value
    bool myIsRendering{false};
    // Whether the missile signal is active this clock
    bool myIsVisible{false};
    // Counts pixels from rendering start; display begins at 0
    Int8 myRenderCounter{0};
    // Which copy triggered the current rendering pass
    Int8 myCopy{1};

    // Pointer into the DrawCounterDecodes table for current NUSIZ
    const uInt8* myDecodes{nullptr};
    // Index of myDecodes in the table (needed for state saving)
    uInt8 myDecodesOffset{0};

    // Current computed color (output of applyColors())
    uInt8 myColor{0};
    // Color from COLUP0/1
    uInt8 myObjectColor{0};
    // Color override in "debug colors" mode
    uInt8 myDebugColor{0};
    // Whether "debug colors" mode is active
    bool myDebugEnabled{false};

    // A movement tick outside HBLANK is pending (inverted phase mode)
    bool myInvertedPhaseClock{false};
    // Whether the inverted movement clock phase quirk is active
    bool myUseInvertedPhaseClock{false};
    // Whether the short late HMOVE quirk is active
    bool myUseShortLateHMove{false};

    // Required for flushing the line cache and requesting collision updates
    TIA *myTIA{nullptr};

  private:
    // Following constructors and assignment operators not supported
    Missile(const Missile&) = delete;
    Missile(Missile&&) = delete;
    Missile& operator=(const Missile&) = delete;
    Missile& operator=(Missile&&) = delete;
};

// ############################################################################
// Implementation
// ############################################################################

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::movementTick(uInt8 clock, uInt8 hclock, bool hblank)
{
  if(isMoving)
  {
    // Stop movement once the number of clocks according to HMMx is reached
    if(clock == myHmmClocks)
      isMoving = false;
    else if (!myUseShortLateHMove || hclock != 0)
    {
      // Process the tick if we are in hblank. Otherwise, the tick is either
      // masked by an ordinary tick or merges two consecutive ticks into a
      // single tick (inverted movement clock phase mode).
      if(hblank)
        tick(hclock, false);

      // Track a tick outside hblank for later processing
      myInvertedPhaseClock = !hblank;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::tick(uInt8 hclock, bool isReceivingMclock)
{
  // If we are in inverted movement clock phase mode and a movement tick
  // occurred, it will supress the tick.
  if(myUseInvertedPhaseClock && myInvertedPhaseClock)
  {
    myInvertedPhaseClock = false;
    return;
  }

  myIsVisible =
    myIsRendering &&
    (myRenderCounter >= 0 || (isMoving && isReceivingMclock && myRenderCounter == -1 && myWidth < 4 && ((hclock + 1) % 4 == 3)));

  // Consider enabled status and the signal to determine visibility
  // (as represented by the collision mask)
  collision = (myIsVisible && myIsEnabled)
    ? myCollisionMaskEnabled
    : myCollisionMaskDisabled;

  if (myDecodes[myCounter] && !myResmp) {
    myIsRendering = true;
    myRenderCounter = renderCounterOffset;
    myCopy = myDecodes[myCounter];
  } else if (myIsRendering) {

      if (myRenderCounter == -1) {
        // Regular clock pulse during movement -> starfield mode
        if (isMoving && isReceivingMclock) {
          switch ((hclock + 1) % 4) {
            case 3:
              myEffectiveWidth = myWidth == 1 ? 2 : myWidth;
              if (myWidth < 4) ++myRenderCounter;
              break;

            case 2:
              myEffectiveWidth = 0;
              break;

            default:
              myEffectiveWidth = myWidth;
              break;
          }
        } else {
          myEffectiveWidth = myWidth;
        }
      }

      if (std::cmp_greater_equal(++myRenderCounter,
                                 isMoving ? myEffectiveWidth : myWidth))
        myIsRendering = false;
  }

  if (++myCounter >= TIAConstants::H_PIXEL)
    myCounter = 0;
}

#endif  // MISSILE_HXX
