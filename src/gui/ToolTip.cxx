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

// TODO:
// - disable when in edit mode
// - option to disable tips
// - multi line tips
// - nicer formating of DataDridWidget tip
// - allow reversing ToogleWidget (TooglePixelWidget)


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ToolTip::ToolTip(Dialog& dialog, const GUI::Font& font)
  : myDialog(dialog),
    myFont(font)
{
  const int fontWidth = font.getMaxCharWidth(),
    fontHeight = font.getFontHeight();

  myTextXOfs = fontHeight < 24 ? 5 : 8; // 3 : 5;
  myTextYOfs = fontHeight < 24 ? 2 : 3;
  myWidth = fontWidth * MAX_LEN + myTextXOfs * 2;
  myHeight = fontHeight + myTextYOfs * 2;

  mySurface = myDialog.instance().frameBuffer().allocateSurface(myWidth, myHeight);
  myScale = myDialog.instance().frameBuffer().hidpiScaleFactor();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::request(const Widget* widget)
{
  if(myWidget != widget)
    release();

  if(myTimer == DELAY_TIME)
  {
    myWidget = widget;

    const uInt32 VGAP = 1;
    const uInt32 hCursor = 18;
    string text = widget->getToolTip(myPos.x, myPos.y);
    uInt32 width = std::min(myWidth, myFont.getStringWidth(text) + myTextXOfs * 2);
    // Note: These include HiDPI scaling:
    const Common::Rect imageRect = myDialog.instance().frameBuffer().imageRect();
    const Common::Rect dialogRect = myDialog.surface().dstRect();
    // Limit to app or screen size and adjust position
    const Int32 xAbs = myPos.x + dialogRect.x() / myScale;
    const uInt32 yAbs = myPos.y + dialogRect.y() / myScale;
    Int32 x = std::min(xAbs, Int32(imageRect.w() / myScale - width));
    const uInt32 y = (yAbs + myHeight + hCursor > imageRect.h() / myScale)
      ? yAbs - myHeight - VGAP
      : yAbs + hCursor / myScale + VGAP;

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
    myDialog.setDirtyChain();
  }
  myTimer++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::release()
{
  if(myWidget != nullptr)
  {
    myTimer = 0;
    myWidget = nullptr;
    myDialog.setDirtyChain();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::render()
{
  if(myWidget != nullptr)
    mySurface->render();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToolTip::update(int x, int y)
{
  if(myWidget != nullptr && x != myPos.x || y != myPos.y)
    release();
  myPos.x = x;
  myPos.y = y;
}
