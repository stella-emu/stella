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

#ifndef PLAYER_HXX
#define PLAYER_HXX

class TIA;

#include "bspf.hxx"
#include "Serializable.hxx"
#include "TIAConstants.hxx"

/**
  TIA player sprite object (P0 or P1). Emulates the horizontal counter,
  draw counter, graphics shift register, HMOVE movement, and all
  associated registers (GRP, HMP, NUSIZ, RESP, REFP, VDELP), producing
  per-clock collision and color output.

  @author  Christian Speckner (DirtyHairy)
*/
class Player : public Serializable
{
  public:
    explicit Player(uInt32 collisionMask);
    ~Player() override = default;

    /**
      Set the TIA instance.
     */
    void setTIA(TIA* tia) { myTIA = tia; }

    /**
      Reset to initial state.
     */
    void reset();

    /**
      GRP0/1 write: set the player graphics pattern.
     */
    void grp(uInt8 pattern);

    /**
      HMP0/1 write: set horizontal motion (bits 7-4).
     */
    void hmp(uInt8 value);

    /**
      NUSIZ0/1 write: update player copy count and size.
     */
    void nusiz(uInt8 value, bool hblank);

    /**
      RESP0/1 write: reset the horizontal position counter.
     */
    void resp(uInt8 counter);

    /**
      REFP0/1 write: bit 3 reflects the player graphics horizontally.
     */
    void refp(uInt8 value);

    /**
      VDELP0/1 write: bit 0 enables vertical delay (display the old pattern).
     */
    void vdelp(uInt8 value);

    /**
      Enable/disable player display (debugging only).
     */
    void toggleEnabled(bool enabled);

    /**
      Enable/disable collision detection (debugging only).
     */
    void toggleCollisions(bool enabled);

    /**
      Set the player color from COLUP0/1.
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
      Called when HMOVE is strobed: arm the movement counter.
     */
    void startMovement();

    /**
      Advance to the next scanline.
     */
    void nextLine();

    /**
      Get the current horizontal position counter value.
     */
    uInt8 getClock() const { return myCounter; }

    /**
      Is the player currently visible? Determined from bit 15 of the collision mask.
     */
    bool isOn() const { return (collision & 0x8000); }

    /**
      Get the current player color.
     */
    uInt8 getColor() const;

    /**
      Swap the old and new graphics patterns. Called when GRP1 is written
      (with a one-cycle delay) to implement VDELP.
     */
    void shufflePatterns();

    /**
      Get the counter value at the time of the last RESP write. Used by the debugger only.
     */
    uInt8 getRespClock() const;

    /**
      Get/set the sprite position derived from the counter. Used by the debugger only.
     */
    uInt8 getPosition() const;
    void setPosition(uInt8 newPosition);

    /**
      Get the old and new graphics patterns. Used by the debugger only.
     */
    uInt8 getGRPOld() const { return myPatternOld; }
    uInt8 getGRPNew() const { return myPatternNew; }

    /**
      Directly set the old graphics pattern. Used by the debugger only.
     */
    void setGRPOld(uInt8 pattern);

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

    /**
      Process one HMOVE clock. Inline for performance (implementation below).
     */
    FORCE_INLINE void movementTick(uInt32 clock, uInt32 hclock, bool hblank);

    /**
      Tick one color clock. Inline for performance (implementation below).
     */
    FORCE_INLINE void tick();

  public:
    // 16-bit collision mask; bit 15 encodes current visibility
    uInt32 collision{0};

    // True while HMOVE movement clocks are being propagated
    bool isMoving{false};

  private:
    /**
      Recalculate myPattern from myPatternOld/myPatternNew, VDEL, and debug state.
     */
    void updatePattern();

    /**
      Recalculate myColor from COLUP0/1, debug colors, and color loss.
     */
    void applyColors();

    /**
      Apply a pending size divider change and update myRenderCounterTripPoint.
     */
    void setDivider(uInt8 divider);

  private:
    enum Count: Int8 {
      // Render counter start value; display begins when it reaches 0
      renderCounterOffset = -5,
    };

  private:
    // Collision mask value when the player is invisible
    uInt32 myCollisionMaskDisabled{0};
    // Collision mask value when the player is visible
    uInt32 myCollisionMaskEnabled{0xFFFF};

    // Current computed color (output of applyColors())
    uInt8 myColor{0};
    // Color from COLUP0/1
    uInt8 myObjectColor{0};
    // Color override in "debug colors" mode
    uInt8 myDebugColor{0};
    // Whether "debug colors" mode is active
    bool myDebugEnabled{false};

    // Suppressed by the debugger
    bool myIsSuppressed{false};

    // Number of HMOVE clocks from HMP register
    uInt8 myHmmClocks{0};
    // Horizontal position counter (0-159)
    uInt8 myCounter{0};

    // Render latch; set when the counter hits a decode value
    bool myIsRendering{false};
    // Counts pixels from rendering start
    Int8 myRenderCounter{0};
    // Threshold at which the first output pixel is produced
    Int8 myRenderCounterTripPoint{0};
    // Which copy triggered the current rendering pass
    Int8 myCopy{1};
    // Current pixel clock divider (1, 2, or 4 for single/double/quad size)
    uInt8 myDivider{0};
    // Divider value to apply after the current render
    uInt8 myDividerPending{0};
    // Bit index into the graphics pattern (0-7)
    uInt8 mySampleCounter{0};
    // Countdown to apply the pending divider change
    Int8 myDividerChangeCounter{-1};

    // Pointer into the DrawCounterDecodes table for current NUSIZ
    const uInt8* myDecodes{nullptr};
    // Index of myDecodes in the table (needed for state saving)
    uInt8 myDecodesOffset{0};

    // Previous GRP value (displayed when VDEL is active)
    uInt8 myPatternOld{0};
    // Most recently written GRP value
    uInt8 myPatternNew{0};
    // Effective pattern (myPatternOld or myPatternNew based on VDEL)
    uInt8 myPattern{0};

    // REFP bit; mirrors the pattern horizontally when set
    bool myIsReflected{false};
    // VDEL bit; when set, display myPatternOld instead of myPatternNew
    bool myIsDelaying{false};
    // A movement tick outside HBLANK is pending (inverted phase mode)
    bool myInvertedPhaseClock{false};
    // Whether the inverted movement clock phase quirk is active
    bool myUseInvertedPhaseClock{false};
    // Whether the short late HMOVE quirk is active
    bool myUseShortLateHMove{false};

    // Required for flushing the line cache and requesting collision updates
    TIA* myTIA{nullptr};

  private:
    // Following constructors and assignment operators not supported
    Player(const Player&) = delete;
    Player(Player&&) = delete;
    Player& operator=(const Player&) = delete;
    Player& operator=(Player&&) = delete;
};

// ############################################################################
// Implementation
// ############################################################################

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::movementTick(uInt32 clock, uInt32 hclock, bool hblank)
{
  if(isMoving)
  {
    // Stop movement once the number of clocks according to HMPx is reached
    if (clock == myHmmClocks)
      isMoving = false;
    else if (!myUseShortLateHMove || hclock != 0)
    {
      // Process the tick if we are in hblank. Otherwise, the tick is either masked
      // by an ordinary tick or merges two consecutive ticks into a single tick (inverted
      // movement clock phase mode).
      if(hblank)
        tick();

      // Track a tick outside hblank for later processing
      myInvertedPhaseClock = !hblank;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::tick()
{
  // If we are in inverted movement clock phase mode and a movement tick
  // occurred, it will supress the tick.
  if(myUseInvertedPhaseClock && myInvertedPhaseClock)
  {
    myInvertedPhaseClock = false;
    return;
  }

  if (!myIsRendering || myRenderCounter < myRenderCounterTripPoint)
    collision = myCollisionMaskDisabled;
  else
    collision = (myPattern & (1 << mySampleCounter)) ? myCollisionMaskEnabled : myCollisionMaskDisabled;

  if (myDecodes[myCounter]) {
    myIsRendering = true;
    mySampleCounter = 0;
    myRenderCounter = renderCounterOffset;
    myCopy = myDecodes[myCounter];
  } else if (myIsRendering) {
    ++myRenderCounter;

    switch (myDivider) {
      case 1:
        if (myRenderCounter > 0)
          ++mySampleCounter;

        if (myRenderCounter >= 0 && myDividerChangeCounter >= 0 && myDividerChangeCounter-- == 0)  // NOLINT(bugprone-inc-dec-in-conditions)
          setDivider(myDividerPending);

        break;

      default:
        if (myRenderCounter > 1 && (((myRenderCounter - 1) % myDivider) == 0))
          ++mySampleCounter;

        if (myRenderCounter > 0 && myDividerChangeCounter >= 0 && myDividerChangeCounter-- == 0)  // NOLINT(bugprone-inc-dec-in-conditions)
          setDivider(myDividerPending);

        break;
    }

    if (mySampleCounter > 7) myIsRendering = false;
  }

  if (++myCounter >= TIAConstants::H_PIXEL) myCounter = 0;
}

#endif  // PLAYER_HXX
