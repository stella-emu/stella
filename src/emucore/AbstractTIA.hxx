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

#ifndef ABSTRACT_TIA_HXX
#define ABSTRACT_TIA_HXX

#include "bspf.hxx"
#include "Device.hxx"
#include "Serializer.hxx"
#include "TIATypes.hxx"

class AbstractTIA: public Device
{
  public:

    virtual ~AbstractTIA() = default;

    virtual void frameReset() = 0;

    virtual bool saveDisplay(Serializer& out) const = 0;

    virtual bool loadDisplay(Serializer& in) = 0;

    virtual void update() = 0;

    virtual const uInt8* currenFrameBuffer() const = 0;

    virtual const uInt8 previousFrameBuffer() const = 0;

    virtual uInt32 width() const {
      return 160;
    }

    virtual uInt32 height() const = 0;

    virtual uInt32 ystart() const = 0;

    virtual void setHeight(uInt32 height) = 0;

    virtual void setWidth(uInt32 width) = 0;

    virtual void enableAutoFrame(bool enabled) = 0;

    virtual void enableColorLoss(bool enabled) = 0;

    virtual bool isPAL() const = 0;

    virtual uInt32 clocksThisLine() const = 0;

    virtual uInt32 scanlines() const = 0;

    virtual bool partialFrame() const = 0;

    virtual uInt32 startScanline() const = 0;

    virtual bool scanlinePos(uInt16& x, uInt16& y) = 0;

    virtual bool toggleBit(TIABit b, uInt8 mode = 2) = 0;

    virtual bool toggleBits() = 0;

    virtual bool toggleCollision(TIABit b, uInt8 mode = 2) = 0;

    virtual bool toogleCollisions() = 0;

    virtual bool toggleHMOVEBlank() = 0;

    virtual bool toggleFixedColor(uInt8 mode = 2) = 0;

    virtual bool driveUnusedPinsRandom(uInt8 mode = 2) = 0;

    virtual bool toggleJitter(uInt8 mode = 2) = 0;

    virtual void setJitterRecoveryFactor(Int32 f) = 0;

    #ifdef DEBUGGER_SUPPORT

      virtual void updateScanline() = 0;

      virtual void updateScanlineByStep() = 0;

      virtual void updateScanlineByTrace(int targetAddress) = 0; 

    #endif

  protected:

    AbstractTIA() {}

};

#endif