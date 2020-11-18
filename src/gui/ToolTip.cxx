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
#include "DialogContainer.hxx"
#include "Font.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Widget.hxx"

#include "ToolTip.hxx"

// TODOs:
// - option to disable tips
// - multi line tips

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ToolTip::ToolTip(Dialog& dialog, const GUI::Font& font)
  : myDialog(dialog)
{
  myScale = myDialog.instance().frameBuffer().hidpiScaleFactor();

  setFont(font);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::setFont(const GUI::Font& font)
{
  myFont = &font;
  const int fontWidth = font.getMaxCharWidth(),
    fontHeight = font.getFontHeight();

  myTextXOfs = fontHeight < 24 ? 5 : 8;
  myTextYOfs = fontHeight < 24 ? 2 : 3;
  myWidth = fontWidth * MAX_LEN + myTextXOfs * 2;
  myHeight = fontHeight + myTextYOfs * 2;

  mySurface = myDialog.instance().frameBuffer().allocateSurface(myWidth, myHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::request()
{
  // Called each frame when a tooltip is wanted
  if(myFocusWidget && myTimer < DELAY_TIME * RELEASE_SPEED)
  {
    const string tip = myFocusWidget->getToolTip(myMousePos);

    if(!tip.empty())
    {
      myTipWidget = myFocusWidget;
      myTimer += RELEASE_SPEED;
      if(myTimer >= DELAY_TIME * RELEASE_SPEED)
        show(tip);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::update(const Widget* widget, const Common::Point& pos)
{
  // Called each mouse move
  myMousePos = pos;
  myFocusWidget = widget;

  if(myTipWidget != widget)
    release(false);

  if(!myTipShown)
    release(true);
  else
  {
    if(myTipWidget->changedToolTip(myTipPos, myMousePos))
    {
      const string tip = myTipWidget->getToolTip(myMousePos);

      if(!tip.empty())
        show(tip);
      else
        release(true);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::hide()
{
  if(myTipShown)
  {
    myTimer = 0;
    myTipWidget = myFocusWidget = nullptr;
    myTipShown = false;
    myDialog.instance().frameBuffer().setPendingRender();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::release(bool emptyTip)
{
  if(myTipShown)
  {
    myTipShown = false;
    myDialog.instance().frameBuffer().setPendingRender();
  }

  // After displaying a tip, slowly reset the timer to 0
  //  until a new tip is requested
  if((emptyTip || myTipWidget != myFocusWidget) && myTimer)
    myTimer--;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::show(const string& tip)
{
  myTipPos = myMousePos;

  const uInt32 V_GAP = 1;
  const uInt32 H_CURSOR = 18;
  uInt32 width = std::min(myWidth, myFont->getStringWidth(tip) + myTextXOfs * 2);
  // Note: The rects include HiDPI scaling
  const Common::Rect imageRect = myDialog.instance().frameBuffer().imageRect();
  const Common::Rect dialogRect = myDialog.surface().dstRect();
  // Limit position to app size and adjust accordingly
  const Int32 xAbs = myTipPos.x + dialogRect.x() / myScale;
  const uInt32 yAbs = myTipPos.y + dialogRect.y() / myScale;
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
  mySurface->drawString(*myFont, tip, myTextXOfs, myTextYOfs,
                        width - myTextXOfs * 2, myHeight - myTextYOfs * 2, kTextColor);

  myTipShown = true;
  myDialog.instance().frameBuffer().setPendingRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::render()
{
  if(myTipShown)
    mySurface->render(), cerr << "    render tooltip" << endl;
}
