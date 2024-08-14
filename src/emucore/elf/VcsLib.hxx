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
#include "Random.hxx"
#include "CortexM0.hxx"
#include "BusTransactionQueue.hxx"
#include "Serializable.hxx"

class Serializer;

class VcsLib: public CortexM0::BusTransactionDelegate, public Serializable {
  public:
    explicit VcsLib(BusTransactionQueue& transactionQueue);
    ~VcsLib() override = default;

    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

    void reset();

    CortexM0::err_t fetch16(uInt32 address, uInt16& value, uInt8& op, CortexM0& cortex) override;

    bool updateBus(uInt16 address, uInt8 value) {
      myCurrentAddress = address;
      myCurrentValue = value;

      return myIsWaitingForRead && myTransactionQueue.size() == 0 && (myWaitingForReadAddress == myCurrentAddress);
    }

    bool isSuspended() const {
      return
        myIsWaitingForRead && (myTransactionQueue.size() > 0 || (myWaitingForReadAddress != myCurrentAddress));
    }

    void vcsWrite5(uInt8 zpAddress, uInt8 value);
    void vcsCopyOverblankToRiotRam();
    void vcsStartOverblank();
    void vcsEndOverblank();
    void vcsNop2n(uInt16 n);
    void vcsLda2(uInt8 value);

  private:
    static CortexM0::err_t returnFromStub(uInt16& value, uInt8& op) {
      constexpr uInt16 BX_LR = 0x4770;

      value = BX_LR;
      op = CortexM0::decodeInstructionWord(BX_LR);

      return CortexM0::ERR_NONE;
    }

    CortexM0::err_t stackOperation(uInt16& value, uInt8& op, uInt8 opcode);

  private:
    BusTransactionQueue& myTransactionQueue;

    uInt8 myStuffMaskA{0x00};
    uInt8 myStuffMaskX{0x00};
    uInt8 myStuffMaskY{0x00};

    bool myIsWaitingForRead{false};
    uInt16 myWaitingForReadAddress{0};

    uInt16 myCurrentAddress{0};
    uInt8 myCurrentValue{0};

    Random myRand;

  private:
    VcsLib(const VcsLib&) = delete;
    VcsLib(VcsLib&&) = delete;
    const VcsLib& operator=(const VcsLib&) = delete;
    const VcsLib& operator=(VcsLib&&) = delete;
};

#endif // VCSLIB_H
