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
#include "Layout.hxx"
#include "InputTextDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputTextDialog::InputTextDialog(GuiObject* boss, const GUI::Font& font,
                                 const StringList& labels, string_view title)
  : Dialog(boss->instance(), boss->parent(), font, title),
    CommandSender(boss)
{
  initialize(font, font, labels);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputTextDialog::InputTextDialog(GuiObject* boss, const GUI::Font& lfont,
                                 const GUI::Font& nfont,
                                 const StringList& labels, string_view title)
  : Dialog(boss->instance(), boss->parent(), lfont, title),
    CommandSender(boss)
{
  initialize(lfont, nfont, labels);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputTextDialog::InputTextDialog(OSystem& osystem, DialogContainer& parent,
                                 const GUI::Font& font, const StringList& labels,
                                 string_view title, int widthChars)
  : Dialog(osystem, parent, font, title),
    CommandSender(nullptr)
{
  clear();
  initialize(font, font, labels, widthChars);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::initialize(const GUI::Font& lfont, const GUI::Font& nfont,
                                 const StringList& labels, int widthChars)
{
  const int lineHeight = Dialog::lineHeight(),
            fontHeight = Dialog::fontHeight();
  WidgetArray wid;

  myWidthChars = widthChars;
  myMaxLen.resize(labels.size(), 0);

  // Create a label + editbox for each entry; layout() assigns all geometry
  for(const auto& label: labels)
  {
    myLabel.push_back(new StaticTextWidget(this, lfont, 0, 0, 1, fontHeight, label));

    auto* w = new EditTextWidget(this, nfont, 0, 0, 1, lineHeight);
    wid.push_back(w);
    myInput.push_back(w);
  }

  myMessage = new StaticTextWidget(this, lfont, 0, 0, 1, fontHeight);
  myMessage->setTextColor(kTextColorEm);

  addToFocusList(wid);

  // Add OK and Cancel buttons
  wid.clear();
  addOKCancelBGroup(wid, lfont);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::widgetItem;
  using GUI::alignedItem;
  using GUI::HAlign;
  using GUI::VAlign;
  using Dir = BoxLayout::Dir;

  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  const int numRows = static_cast<int>(myInput.size());

  _w = HBORDER * 2 + fontWidth * myWidthChars;
  _h = buttonHeight + lineHeight + VGAP + numRows * (lineHeight + VGAP)
       + _th + VBORDER * 2;

  // The longest label defines the shared label column width
  int lwidth = 0;
  for(auto* l: myLabel)
    lwidth = std::max(lwidth, _font.getStringWidth(l->getLabel()));

  // A label + editbox per row, then a message line; the editbox fills the row
  // (or keeps a fixed width if a max length was set via setMaxLen)
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  for(int i = 0; i < numRows; ++i)
  {
    auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
    row->addFixed(alignedItem(myLabel[i], HAlign::Fill, VAlign::Center), lwidth);
    row->addSpace(fontWidth);
    if(myMaxLen[i] > 0)
      row->addFixed(alignedItem(myInput[i], HAlign::Fill, VAlign::Center),
                    (myMaxLen[i] + 1) * fontWidth);
    else
      row->addStretch(alignedItem(myInput[i], HAlign::Fill, VAlign::Center));
    root->addFixed(std::move(row), lineHeight);
    root->addSpace(VGAP);
  }
  root->addSpace(VGAP);
  root->addFixed(widgetItem(myMessage), Dialog::fontHeight());
  root->doLayout(0, _th, _w, _h - _th);

  // Standard OK/Cancel button group along the bottom edge
  layoutButtonGroup();
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
  const uInt32 scale = instance().frameBuffer().hidpiScaleFactor();
  myXOrig = bossRect.x() + x * scale;
  myYOrig = bossRect.y() + y * scale;

  // Only show dialog if we're inside the visible area
  if(!bossRect.contains(myXOrig, myYOrig))
    return;

  myEnableCenter = false;
  open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setPosition()
{
  if(!myEnableCenter)
  {
    // First set position according to original coordinates
    surface().setDstPos(myXOrig, myYOrig);

    // Now make sure that the entire menu can fit inside the screen bounds
    // If not, we reset its position
    if(!instance().frameBuffer().screenRect().adjustToFit(
        myXOrig, myXOrig, surface().dstRect()))
      surface().setDstPos(myXOrig, myYOrig);
  }
  else
    Dialog::setPosition();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setMessage(string_view title)
{
  myMessage->setLabel(title);
  myErrorFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& InputTextDialog::getResult(int idx)
{
  if(static_cast<uInt32>(idx) < myInput.size())
    return myInput[idx]->getText();
  else
    return EmptyString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setText(string_view str, int idx)
{
  if(static_cast<uInt32>(idx) < myInput.size())
    myInput[idx]->setText(str);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setTextFilter(const EditableWidget::TextFilter& f, int idx)
{
  if(static_cast<uInt32>(idx) < myInput.size())
    myInput[idx]->setTextFilter(f);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setMaxLen(int len, int idx)
{
  if(static_cast<uInt32>(idx) < myInput.size())
  {
    // Remember the limit so layout() keeps this input at its fixed width
    // instead of stretching it to fill the row
    myMaxLen[idx] = len;
    myInput[idx]->setMaxLen(len);
    myInput[idx]->setWidth((len + 1) * Dialog::fontWidth());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setToolTip(string_view str, int idx)
{
  if(static_cast<uInt32>(idx) < myLabel.size())
    myLabel[idx]->setToolTip(str);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setFocus(int idx)
{
  if(static_cast<uInt32>(idx) < myInput.size())
    Dialog::setFocus(getFocusList()[idx]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputTextDialog::setEditable(bool editable, int idx)
{
  myInput[idx]->setEditable(editable);
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

    default:
      Dialog::handleCommand(sender, cmd, data, id);
      break;
  }
}
