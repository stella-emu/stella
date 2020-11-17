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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "Font.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Widget.hxx"

#include "ToolTip.hxx"

// TODOs:
// + keep enabled when mouse moves over same widget
//   + static position and text for normal widgets
//   + moving position and text over e.g. DataGridWidget
// + reenable tip faster
// + disable when in edit mode
// - option to disable tips
// - multi line tips
// - nicer formating of DataDridWidget tip
// - allow reversing ToogleWidget (TooglePixelWidget)
// - shift checkbox bits
// - RomListWidget (hex codes, maybe disassembly operands)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ToolTip::ToolTip(Dialog& dialog, const GUI::Font& font)
  : myDialog(dialog),
    myFont(font)
{
  const int fontWidth = font.getMaxCharWidth(),
    fontHeight = font.getFontHeight();

  myTextXOfs = fontHeight < 24 ? 5 : 8;
  myTextYOfs = fontHeight < 24 ? 2 : 3;
  myWidth = fontWidth * MAX_LEN + myTextXOfs * 2;
  myHeight = fontHeight + myTextYOfs * 2;

  mySurface = myDialog.instance().frameBuffer().allocateSurface(myWidth, myHeight);
  myScale = myDialog.instance().frameBuffer().hidpiScaleFactor();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::update(const Widget* widget, Common::Point pos)
{
  if(myTipWidget != widget)
  {
    myFocusWidget = widget;
    release();
  }
  if(myTipShown && myTipWidget->changedToolTip(myPos, pos))
    myPos = pos, show();
  else
    myPos = pos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::hide()
{
  if(myTipShown)
  {
    myTimer = 0;
    myTipWidget = myFocusWidget = nullptr;

    myTipShown = false;
    myDialog.setDirtyChain();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::release()
{
  if(myTipShown)
  {
    myTimer = DELAY_TIME;

    myTipShown = false;
    myDialog.setDirtyChain();
  }

  // After displaying a tip, slowly reset the timer to 0
  //  until a new tip is requested
  if(myTipWidget != myFocusWidget && myTimer)
    myTimer--;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::request()
{
  myTipWidget = myFocusWidget;

  if(myTimer == DELAY_TIME)
    show();

  myTimer++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::show()
{
  if(myTipWidget == nullptr)
    return;

  const uInt32 V_GAP = 1;
  const uInt32 H_CURSOR = 18;
  string text = myTipWidget->getToolTip(myPos);
  uInt32 width = std::min(myWidth, myFont.getStringWidth(text) + myTextXOfs * 2);
  // Note: The rects include HiDPI scaling
  const Common::Rect imageRect = myDialog.instance().frameBuffer().imageRect();
  const Common::Rect dialogRect = myDialog.surface().dstRect();
  // Limit position to app size and adjust accordingly
  const Int32 xAbs = myPos.x + dialogRect.x() / myScale;
  const uInt32 yAbs = myPos.y + dialogRect.y() / myScale;
  Int32 x = std::min(xAbs, Int32(imageRect.w() / myScale - width));
  const uInt32 y = (yAbs + myHeight + H_CURSOR > imageRect.h() / myScale)
    ? yAbs - myHeight - V_GAP
    : yAbs + H_CURSOR / myScale + V_GAP;

  if(x < 0)
  {
    x = 0;
    width = imageRect.w() / myScale;
  }

  mySurface->setSrcSize(width, myHeight);
  mySurface->setDstSize(width * myScale, myHeight * myScale);
  mySurface->setDstPos(x * myScale, y * myScale);

  mySurface->frameRect(0, 0, width, myHeight, kColor);
  mySurface->fillRect(1, 1, width - 2, myHeight - 2, kWidColor);
  mySurface->drawString(myFont, text, myTextXOfs, myTextYOfs,
                        width - myTextXOfs * 2, myHeight - myTextYOfs * 2, kTextColor);

  myTipShown = true;
  myDialog.setDirtyChain();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::render()
{
  if(myTipShown)
    mySurface->render(), cerr << "    render tooltip" << endl;
}
