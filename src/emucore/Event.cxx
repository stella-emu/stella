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

#include "Serializer.hxx"
#include "Event.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Event::saveInputWindow(Serializer& out) const
{
  const std::scoped_lock lock(myMutex);

  // Digital transition schedule (post-finalize: every pos assigned, none pending)
  out.putInt(static_cast<uInt32>(myTransitions.size()));
  for(const auto& t: myTransitions)
  {
    out.putShort(t.type);
    out.putLong(t.pos);
    out.putInt(static_cast<uInt32>(t.value));
  }

  // Continuous inputs carry no transitions, so their whole-window value must be
  // captured separately
  for(uInt16 type = 0; type < LastType; ++type)
    if(isContinuous(static_cast<Type>(type)))
      out.putInt(static_cast<uInt32>(myValues[type]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Event::loadInputWindow(Serializer& in, uInt64 nowCycles)
{
  const std::scoped_lock lock(myMutex);

  myWindowStartCycle = nowCycles;
  myRecording = false;

  // Restore the transition schedule; each transition's value also becomes the
  // latched value seen by get(type).  Digital inputs with no transition this
  // window keep their prior latched value, which persists in myValues across
  // playback frames.
  const uInt32 count = in.getInt();
  myTransitions.clear();
  myTransitions.reserve(count);
  for(uInt32 i = 0; i < count; ++i)
  {
    Transition t;
    t.type    = static_cast<Type>(in.getShort());
    t.pos     = in.getLong();
    t.value   = static_cast<Int32>(in.getInt());
    t.pending = false;
    myTransitions.push_back(t);
    myValues[t.type] = t.value;
  }

  // Continuous inputs are fully defined by the recording each window
  for(uInt16 type = 0; type < LastType; ++type)
    if(isContinuous(static_cast<Type>(type)))
      myValues[type] = static_cast<Int32>(in.getInt());

  myHasTransitions = !myTransitions.empty();
}
