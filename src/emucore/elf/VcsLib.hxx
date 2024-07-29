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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef VCSLIB_H
#define VCSLIB_H

#include "bspf.hxx"
#include "CortexM0.hxx"

class BusTransactionQueue;

class VcsLib: public CortexM0::BusTransactionDelegate {
  public:
    explicit VcsLib(BusTransactionQueue& transactionQueue);

    void reset();

    CortexM0::err_t fetch16(uInt32 address, uInt16& value, uInt8& op, CortexM0& cortex) override;

    void vcsWrite5(uInt8 zpAddress, uInt8 value);
    void vcsCopyOverblankToRiotRam();
    void vcsStartOverblank();
    void vcsEndOverblank();
    void vcsNop2n(uInt16 n);
    void vcsLda2(uInt8 value);

  private:
    CortexM0::err_t returnFromStub(uInt16& value, uInt8& op);

  private:
    BusTransactionQueue& myTransactionQueue;

    uInt8 myStuffMaskA{0x00};
    uInt8 myStuffMaskX{0x00};
    uInt8 myStuffMaskY{0x00};

  private:
    VcsLib(const VcsLib&) = delete;
    VcsLib(VcsLib&&) = delete;
    const VcsLib& operator=(const VcsLib&) = delete;
    const VcsLib& operator=(VcsLib&&) = delete;
};

#endif // VCSLIB_H
