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

#include "Dialog.hxx"
#include "FrameBufferConstants.hxx"
#include "TimeMachineDialog.hxx"
#include "TimeMachine.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeMachine::TimeMachine(OSystem& osystem)
  : DialogContainer(osystem),
    myWidth{FBMinimum::Width}
{
  myBaseDialog = std::make_unique<TimeMachineDialog>(myOSystem, *this,
                                                     static_cast<int>(myWidth));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeMachine::~TimeMachine() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachine::requestResize()
{
  uInt32 w = 0, h = 0;
  myBaseDialog->getDynamicBounds(w, h);

  // Only re-create when absolutely necessary
  if(myWidth != w)
  {
    myWidth = w;
    const Dialog* oldPtr = myBaseDialog.get();
    const Int32 enterWinds = myBaseDialog->getEnterWinds();
    myBaseDialog = std::make_unique<TimeMachineDialog>(myOSystem, *this,
                                                       static_cast<int>(myWidth));
    setEnterWinds(enterWinds);
    Dialog* newPtr = myBaseDialog.get();

    // Update the container stack; it may contain a reference to the old pointer
    if(oldPtr != newPtr)
    {
      myDialogStack.applyAll([&oldPtr,&newPtr](Dialog*& d){
        if(d == oldPtr)
          d = newPtr;
        });
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog* TimeMachine::baseDialog()
{
  return myBaseDialog.get();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachine::setEnterWinds(Int32 numWinds)
{
  myBaseDialog->setEnterWinds(numWinds);
}
