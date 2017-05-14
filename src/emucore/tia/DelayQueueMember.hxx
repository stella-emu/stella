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

template<int capacity>
class DelayQueueMember : public Serializable {

  public:
    struct Entry {
      uInt8 address;
      uInt8 value;
    };

  public:
    DelayQueueMember();

  public:
    void push(uInt8 address, uInt8 value);

    void remove(uInt8 address);

    void clear();

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override;

  public:
    Entry myEntries[capacity];
    uInt8 mySize;

  private:

    DelayQueueMember(const DelayQueueMember<capacity>&) = delete;
    DelayQueueMember(DelayQueueMember<capacity>&&) = delete;
    DelayQueueMember<capacity>& operator=(const DelayQueueMember<capacity>&) = delete;
    DelayQueueMember<capacity>& operator=(DelayQueueMember<capacity>&&) = delete;

};

// ############################################################################
// Implementation
// ############################################################################

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<int capacity>
DelayQueueMember<capacity>::DelayQueueMember()
  : mySize(0)
{}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<int capacity>
void DelayQueueMember<capacity>::push(uInt8 address, uInt8 value)
{
  if (mySize == capacity) throw runtime_error("delay queue overflow");

  myEntries[mySize].address = address;
  myEntries[mySize++].value = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<int capacity>
void DelayQueueMember<capacity>::remove(uInt8 address)
{
  uInt8 index;

  for (index = 0; index < mySize; index++) {
    if (myEntries[index].address == address) break;
  }

  if (index < mySize) {
    for (uInt8 i = index + 1; i < mySize; i++) {
      myEntries[i-1] = myEntries[i];
    }

    mySize--;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<int capacity>
void DelayQueueMember<capacity>::clear()
{
  mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<int capacity>
bool DelayQueueMember<capacity>::save(Serializer& out) const
{
    try
  {
    out.putInt(mySize);
    for(uInt8 i = 0; i < mySize; ++i)
    {
      const Entry& e = myEntries[i];
      out.putByte(e.address);
      out.putByte(e.value);
    }
  }
  catch(...)
  {
    cerr << "ERROR: TIA_DelayQueueMember::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<int capacity>
bool DelayQueueMember<capacity>::load(Serializer& in)
{
  try
  {
    mySize = in.getInt();
    if (mySize > capacity) throw new runtime_error("invalid delay queue size");
    for(uInt32 i = 0; i < mySize; ++i)
    {
      Entry& e = myEntries[i];
      e.address = in.getByte();
      e.value = in.getByte();
    }
  }
  catch(...)
  {
    cerr << "ERROR: TIA_DelayQueueMember::load" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<int capacity>
string DelayQueueMember<capacity>::name() const
{
  stringstream ss;

  ss << "TIA_DelayQueueMember<" << capacity << ">";

  return ss.str();
}

#endif // TIA_DELAY_QUEUE_MEMBER