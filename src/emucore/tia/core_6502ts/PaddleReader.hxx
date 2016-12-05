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

#ifndef TIA_6502TS_CORE_PADDLE_READER
#define TIA_6502TS_CORE_PADDLE_READER

#include "bspf.hxx"
#include "Types.hxx"

namespace TIA6502tsCore {

class PaddleReader
{
  public:

    PaddleReader();

  public:

    void reset(double timestamp);

    void vblank(uInt8 value, double timestamp);

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

} // namespace TIA6502tsCore

#endif // TIA_6502TS_CORE_PADDLE_READER
