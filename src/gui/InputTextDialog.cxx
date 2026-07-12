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
  WidgetArray wid;

  myWidthChars = widthChars;
  myMaxLen.resize(labels.size(), 0);

  // Create a label + editbox for each entry; layout() assigns all geometry.
  // A label takes its width from its own text (the auto-sizing ctor), because
  // the label column is sized from what the labels ask for -- one built at a
  // placeholder width would report that width and collapse the column
  for(const auto& label: labels)
  {
    myLabel.push_back(new StaticTextWidget(this, lfont, 0, 0, label));

    auto* w = new EditTextWidget(this, nfont, 0, 0, 1);
    wid.push_back(w);
    myInput.push_back(w);
  }

  myMessage = new StaticTextWidget(this, lfont, 0, 0);
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
  using GUI::stretchedItem;
  using GUI::GridLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  const int numRows = static_cast<int>(myInput.size());

  // A label + editbox per row.  The label column is as wide as the longest of
  // the labels (nobody measures one), and the fields line up beside it
  auto grid = std::make_unique<GridLayout>(2, numRows, fontWidth, VGAP);
  grid->columnAuto(0).columnStretch(1);
  for(int i = 0; i < numRows; ++i)
  {
    grid->rowAuto(i);
    grid->place(0, i, anchoredItem(myLabel[i]));

    // A field with a character limit is only as wide as that many characters;
    // one without fills the rest of the row
    if(myMaxLen[i] > 0)
    {
      auto field = std::make_unique<BoxLayout>(Dir::Horizontal);
      field->addFixed(stretchedItem(myInput[i]),
                      EditTextWidget::calcWidth(myInput[i]->font(), myMaxLen[i]));
      field->addStretchSpace();
      grid->place(1, i, std::move(field));
    }
    else
      grid->place(1, i, stretchedItem(myInput[i]));
  }

  // The rows, then the message line below them; the button group sits below
  // that, positioned by layoutButtonGroup()
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addAuto(std::move(grid));
  root->addSpace(VGAP * 2);
  root->addAuto(stretchedItem(myMessage));

  // This dialog states its width in characters, since what it has to show is
  // whatever text it is handed; only its height is derived from the content
  const Common::Size natural = root->naturalSize();

  _w = std::max(HBORDER * 2 + fontWidth * myWidthChars,
                Dialog::buttonGroupWidth());
  _h = _th + static_cast<int>(natural.h) + buttonHeight + VBORDER;

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
