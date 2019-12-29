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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "EditTextWidget.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "Widget.hxx"
#include "InputTextDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputTextDialog::InputTextDialog(GuiObject* boss, const GUI::Font& font,
                                 const StringList& labels, const string& title)
  : Dialog(boss->instance(), boss->parent(), font, title),
    CommandSender(boss)
{
  initialize(font, font, labels);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputTextDialog::InputTextDialog(GuiObject* boss, const GUI::Font& lfont,
                                 const GUI::Font& nfont,
                                 const StringList& labels, const string& title)
  : Dialog(boss->instance(), boss->parent(), lfont, title),
    CommandSender(boss),
    myEnableCenter(false),
    myErrorFlag(false),
    myXOrig(0),
    myYOrig(0)
{
  initialize(lfont, nfont, labels);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::initialize(const GUI::Font& lfont, const GUI::Font& nfont,
                                 const StringList& labels)
{
  const int fontWidth  = lfont.getMaxCharWidth(),
            fontHeight = lfont.getFontHeight(),
            lineHeight = lfont.getLineHeight();
  uInt32 xpos, ypos, i, lwidth = 0, maxIdx = 0;
  WidgetArray wid;

  // Calculate real dimensions
  _w = fontWidth * 41;
  _h = lineHeight * 4 + int(labels.size()) * (lineHeight + 5) + _th;

  // Determine longest label
  for(i = 0; i < labels.size(); ++i)
  {
    if(labels[i].length() > lwidth)
    {
      lwidth = int(labels[i].length());
      maxIdx = i;
    }
  }
  lwidth = lfont.getStringWidth(labels[maxIdx]);

  // Create editboxes for all labels
  ypos = lineHeight + _th;
  for(i = 0; i < labels.size(); ++i)
  {
    xpos = 10;
    new StaticTextWidget(this, lfont, xpos, ypos + 2,
                         lwidth, fontHeight,
                         labels[i], TextAlign::Left);

    xpos += lwidth + fontWidth;
    EditTextWidget* w = new EditTextWidget(this, nfont, xpos, ypos,
                                           _w - xpos - 10, lineHeight, "");
    wid.push_back(w);

    myInput.push_back(w);
    ypos += lineHeight + 5;
  }

  xpos = 10;
  myMessage = new StaticTextWidget(this, lfont, xpos, ypos, _w - 2*xpos, fontHeight,
                                 "", TextAlign::Left);
  myMessage->setTextColor(kTextColorEm);

  addToFocusList(wid);

  // Add OK and Cancel buttons
  wid.clear();
  addOKCancelBGroup(wid, lfont);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::show()
{
  myEnableCenter = true;
  open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::show(uInt32 x, uInt32 y, const Common::Rect& bossRect)
{
  uInt32 scale = instance().frameBuffer().hidpiScaleFactor();
  myXOrig = bossRect.x() + x * scale;
  myYOrig = bossRect.y() + y * scale;

  // Only show dialog if we're inside the visible area
  if(!bossRect.contains(myXOrig, myYOrig))
    return;

  myEnableCenter = false;
  open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::center()
{
  if(!myEnableCenter)
  {
    // First set position according to original coordinates
    surface().setDstPos(myXOrig, myYOrig);

    // Now make sure that the entire menu can fit inside the screen bounds
    // If not, we reset its position
    if(!instance().frameBuffer().screenRect().contains(
        myXOrig, myXOrig, surface().dstRect()))
      surface().setDstPos(myXOrig, myYOrig);
  }
  else
    Dialog::center();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setMessage(const string& title)
{
  myMessage->setLabel(title);
  myErrorFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& InputTextDialog::getResult(int idx)
{
  if(uInt32(idx) < myInput.size())
    return myInput[idx]->getText();
  else
    return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setText(const string& str, int idx)
{
  if(uInt32(idx) < myInput.size())
    myInput[idx]->setText(str);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setTextFilter(const EditableWidget::TextFilter& f, int idx)
{
  if(uInt32(idx) < myInput.size())
    myInput[idx]->setTextFilter(f);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setFocus(int idx)
{
  if(uInt32(idx) < myInput.size())
    Dialog::setFocus(getFocusList()[idx]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::handleCommand(CommandSender* sender, int cmd,
                                    int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
    case EditableWidget::kAcceptCmd:
    {
      // Send a signal to the calling class that a selection has been made
      // Since we aren't derived from a widget, we don't have a 'data' or 'id'
      if(myCmd)
        sendCommand(myCmd, 0, 0);

      // We don't close, but leave the parent to do it
      // If the data isn't valid, the parent may wait until it is
      break;
    }

    case EditableWidget::kChangedCmd:
      // Erase the invalid message once editing is restarted
      if(myErrorFlag)
      {
        myMessage->setLabel("");
        myErrorFlag = false;
      }
      break;

    case EditableWidget::kCancelCmd:
      Dialog::handleCommand(sender, GuiObject::kCloseCmd, data, id);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, id);
      break;
  }
}
