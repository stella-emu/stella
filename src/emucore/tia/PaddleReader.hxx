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
#include "TvMode.hxx"

class PaddleReader
{
  public:

    PaddleReader();

  public:

    void reset(double timestamp);

    void vblank(uInt8 value, double timestamp);
    bool vblankDumped() const { return myIsDumped; }

    uInt8 inpt(double timestamp);

    void update(double value, double timestamp, TvMode tvMode);

  private:

    void setTvMode(TvMode mode);

    void updateCharge(double timestamp);

  private:

    double myUThresh;
    double myU;

    double myValue;
    double myTimestamp;

    TvMode myTvMode;
    double myClockFreq;

    bool myIsDumped;

  private:

    PaddleReader(const PaddleReader&) = delete;
    PaddleReader(PaddleReader&&) = delete;
    PaddleReader& operator=(const PaddleReader&) = delete;
    PaddleReader& operator=(PaddleReader&&) = delete;
};

#endif // TIA_PADDLE_READER
