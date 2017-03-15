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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIA_PADDLE_READER
#define TIA_PADDLE_READER

#include "bspf.hxx"
#include "FrameLayout.hxx"
#include "Serializable.hxx"

class PaddleReader : public Serializable
{
  public:

    PaddleReader();

  public:

    void reset(double timestamp);

    void vblank(uInt8 value, double timestamp);
    bool vblankDumped() const { return myIsDumped; }

    uInt8 inpt(double timestamp);

    void update(double value, double timestamp, FrameLayout layout);

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override { return "TIA_PaddleReader"; }

  private:

    void setLayout(FrameLayout layout);

    void updateCharge(double timestamp);

  private:

    double myUThresh;
    double myU;

    double myValue;
    double myTimestamp;

    FrameLayout myLayout;
    double myClockFreq;

    bool myIsDumped;

  private:
    PaddleReader(const PaddleReader&) = delete;
    PaddleReader(PaddleReader&&) = delete;
    PaddleReader& operator=(const PaddleReader&) = delete;
    PaddleReader& operator=(PaddleReader&&) = delete;
};

#endif // TIA_PADDLE_READER
