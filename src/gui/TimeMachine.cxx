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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Dialog.hxx"
#include "FrameBuffer.hxx"
#include "TimeMachineDialog.hxx"
#include "TimeMachine.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeMachine::TimeMachine(OSystem& osystem)
  : DialogContainer(osystem),
    myWidth(FrameBuffer::kFBMinW)
{
  myBaseDialog = new TimeMachineDialog(myOSystem, *this, myWidth);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachine::requestResize()
{
  uInt32 w, h;
  myBaseDialog->getResizableBounds(w, h);

  // If dialog is too large for given area, we need to resize it
  // Otherwise, make it 80% of the allowable width
  int newWidth = myWidth;
  if(w < FrameBuffer::kFBMinW)
    newWidth = w;
  else if(myBaseDialog->getWidth() != 0.8 * w)
    newWidth = uInt32(0.8 * w);

  // Only re-create when absolutely necessary
  if(myWidth != newWidth)
  {
    myWidth = newWidth;
    Dialog* oldPtr = myBaseDialog;
    Int32 enterWinds = static_cast<TimeMachineDialog*>(myBaseDialog)->getEnterWinds();
    delete myBaseDialog;
    myBaseDialog = new TimeMachineDialog(myOSystem, *this, myWidth);
    setEnterWinds(enterWinds);
    Dialog* newPtr = myBaseDialog;

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
void TimeMachine::setEnterWinds(Int32 numWinds)
{
  static_cast<TimeMachineDialog*>(myBaseDialog)->setEnterWinds(numWinds);
}
