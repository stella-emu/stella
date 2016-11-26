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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef TIA_6502TS_CORE_BALL
#define TIA_6502TS_CORE_BALL

#include "Serializable.hxx"
#include "bspf.hxx"

namespace TIA6502tsCore {

class Ball : public Serializable
{
  public:

    Ball(uInt32 collisionMask);

  public:

    void reset();

    void enabl(uInt8 value);

    void hmbl(uInt8 value);

    void resbl(bool hblank);

    void ctrlpf(uInt8 value);

    void vdelbl(uInt8 value);

    void setColor(uInt8 color);

    void startMovement();

    bool movementTick(uInt32 clock, bool apply);

    void render();

    void tick();

    uInt8 getPixel(uInt8 colorIn) const {
      return collision > 0 ? colorIn : myColor;
    }

    void shuffleStatus();

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override { return "TIA_Ball"; }

  public:

    uInt32 collision;

  private:

    void updateEnabled();

  private:

    uInt32 myCollisionMask;

    uInt8 myColor;

    bool myEnabledOld;
    bool myEnabledNew;
    bool myEnabled;
    bool myIsDelaying;

    uInt8 myHmmClocks;
    uInt8 myCounter;
    bool myIsMoving;
    uInt8 myWidth;

    bool myIsRendering;
    Int8 myRenderCounter;

  private:

    Ball() = delete;
    Ball(const Ball&) = delete;
    Ball(Ball&&) = delete;
    Ball& operator=(const Ball&) = delete;
    Ball& operator=(Ball&&);
};

} // namespace TIA6502tsCore

#endif // TIA_6502TS_CORE_BALL
