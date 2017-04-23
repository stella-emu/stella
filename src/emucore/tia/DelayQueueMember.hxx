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

#ifndef TIA_DELAY_QUEUE_MEMBER
#define TIA_DELAY_QUEUE_MEMBER

#include "Serializable.hxx"
#include "bspf.hxx"

class DelayQueueMember : public Serializable
{
  public:
    struct Entry {
      uInt8 address;
      uInt8 value;
    };

  public:
    DelayQueueMember(uInt8 size = 0);

    DelayQueueMember(DelayQueueMember&&) = default;
    DelayQueueMember& operator=(DelayQueueMember&&) = default;

  public:

    typedef vector<Entry>::const_iterator iterator;

  public:

    void push(uInt8 address, uInt8 value);

    void remove(uInt8 address);

    iterator begin() const {
      return myEntries.begin();
    }

    iterator end() const {
      return (mySize < myEntries.size()) ? (myEntries.begin() + mySize) : myEntries.end();
    }

    void clear() {
      mySize = 0;
    }

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override { return "TIA_DelayQueueMember"; }

  private:
    vector<Entry> myEntries;
    uInt32 mySize;

  private:
    DelayQueueMember(const DelayQueueMember&) = delete;
    DelayQueueMember& operator=(const DelayQueueMember&) = delete;
};

#endif //  TIA_DELAY_QUEUE_MEMBER
