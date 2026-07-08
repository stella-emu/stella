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

#include "OSystem.hxx"
#include "Settings.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "Layout.hxx"
#include "RomListWidget.hxx"
#include "RomListSettings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomListSettings::RomListSettings(GuiObject* boss, const GUI::Font& font)
  : Dialog(boss->instance(), boss->parent(), font),
    CommandSender(boss)
{
  const int buttonWidth  = Dialog::buttonWidth("Disassemble @ current line"),
            buttonHeight = Dialog::buttonHeight();
  WidgetArray wid;

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // Action buttons
  mySetPC = new ButtonWidget(this, font, 0, 0, buttonWidth, buttonHeight,
                             "Set PC @ current line", RomListWidget::kSetPCCmd);
  wid.push_back(mySetPC);
  myRuntoPC = new ButtonWidget(this, font, 0, 0, buttonWidth, buttonHeight,
                               "RunTo PC @ current line", RomListWidget::kRuntoPCCmd);
  wid.push_back(myRuntoPC);
  mySetTimer = new ButtonWidget(this, font, 0, 0, buttonWidth, buttonHeight,
                                "Set timer @ current line", RomListWidget::kSetTimerCmd);
  wid.push_back(mySetTimer);
  myDisassemble = new ButtonWidget(this, font, 0, 0, buttonWidth, buttonHeight,
                                   "Disassemble @ current line", RomListWidget::kDisassembleCmd);
  wid.push_back(myDisassemble);

  // Settings for Distella
  myShowTentative = new CheckboxWidget(this, font, 0, 0,
                                       "Show tentative code", RomListWidget::kTentativeCodeCmd);
  myShowTentative->setToolTip("Check to differentiate between tentative code\n"
                              "vs. data sections via static code analysis.");
  wid.push_back(myShowTentative);
  myShowAddresses = new CheckboxWidget(this, font, 0, 0,
                                       "Show PC addresses", RomListWidget::kPCAddressesCmd);
  myShowAddresses->setToolTip("Check to show program counter addresses as labels.");
  wid.push_back(myShowAddresses);
  myShowGFXBinary = new CheckboxWidget(this, font, 0, 0,
                                       "Show GFX as binary", RomListWidget::kGfxAsBinaryCmd);
  myShowGFXBinary->setToolTip("Check to allow editing GFX sections in binary format.");
  wid.push_back(myShowGFXBinary);
  myUseRelocation = new CheckboxWidget(this, font, 0, 0,
                                       "Use address relocation", RomListWidget::kAddrRelocationCmd);
  myUseRelocation->setToolTip("Check to relocate calls out of address range.");
  wid.push_back(myUseRelocation);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  addToFocusList(wid);

  // We don't have a close/cancel button, but we still want the cancel
  // event to be processed
  processCancelWithoutWidget(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListSettings::layout()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::indentedItem;
  using Dir = BoxLayout::Dir;

  const int buttonWidth  = Dialog::buttonWidth("Disassemble @ current line"),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // The four action buttons share the widest label's width; refresh their
  // size from the live font (ButtonWidget::refreshFontMetrics doesn't resize).
  for(auto* b: {mySetPC, myRuntoPC, mySetTimer, myDisassemble})
  {
    b->setWidth(buttonWidth);
    b->setHeight(buttonHeight);
  }

  // No title bar (_th == 0), so content starts at the top margin
  _w = buttonWidth + HBORDER * 2;
  _h = VBORDER * 2 + 8 * buttonHeight + VGAP * 9;

  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addFixed(anchoredItem(mySetPC),       buttonHeight); root->addSpace(VGAP);
  root->addFixed(anchoredItem(myRuntoPC),     buttonHeight); root->addSpace(VGAP);
  root->addFixed(anchoredItem(mySetTimer),    buttonHeight); root->addSpace(VGAP);
  root->addFixed(anchoredItem(myDisassemble), buttonHeight);
  root->addSpace(VGAP * 3);
  root->addFixed(indentedItem(myShowTentative, indent()/2), buttonHeight); root->addSpace(VGAP);
  root->addFixed(indentedItem(myShowAddresses, indent()/2), buttonHeight); root->addSpace(VGAP);
  root->addFixed(indentedItem(myShowGFXBinary, indent()/2), buttonHeight); root->addSpace(VGAP);
  root->addFixed(indentedItem(myUseRelocation, indent()/2), buttonHeight);
  root->doLayout(0, _th, _w, _h - _th);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListSettings::show(uInt32 x, uInt32 y, const Common::Rect& bossRect, int data)
{
  const uInt32 scale = instance().frameBuffer().hidpiScaleFactor();
  _xorig = bossRect.x() + x * scale;
  _yorig = bossRect.y() + y * scale;

  // Only show if we're inside the visible area
  if(!bossRect.contains(_xorig, _yorig))
    return;

  _item = data;
  open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListSettings::setPosition()
{
  // First set position according to original coordinates
  surface().setDstPos(_xorig, _yorig);

  // Now make sure that the entire menu can fit inside the screen bounds
  // If not, we reset its position
  if(!instance().frameBuffer().screenRect().adjustToFit(
      _xorig, _yorig, surface().dstRect()))
    surface().setDstPos(_xorig, _yorig);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListSettings::loadConfig()
{
  myShowTentative->setState(instance().settings().getBool("dis.resolve"));
  myShowAddresses->setState(instance().settings().getBool("dis.showaddr"));
  myShowGFXBinary->setState(instance().settings().getString("dis.gfxformat") == "2");
  myUseRelocation->setState(instance().settings().getBool("dis.relocate"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListSettings::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  // Close dialog if mouse click is outside it (simulates a context menu)
  // Otherwise let the base dialog class process it
  if(x >= 0 && x < _w && y >= 0 && y < _h)
    Dialog::handleMouseDown(x, y, b, clickCount);
  else
    close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListSettings::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  // We remove the dialog when the user has selected an item
  // Make sure the dialog is removed before sending any commands,
  // since one consequence of sending a command may be to add another
  // dialog/menu
  close();

  switch(cmd)
  {
    case RomListWidget::kSetPCCmd:
    case RomListWidget::kRuntoPCCmd:
    case RomListWidget::kSetTimerCmd:
    case RomListWidget::kDisassembleCmd:
    {
      sendCommand(cmd, _item, -1);
      break;
    }
    case RomListWidget::kTentativeCodeCmd:
    {
      sendCommand(cmd, myShowTentative->getState(), -1);
      break;
    }
    case RomListWidget::kPCAddressesCmd:
    {
      sendCommand(cmd, myShowAddresses->getState(), -1);
      break;
    }
    case RomListWidget::kGfxAsBinaryCmd:
    {
      sendCommand(cmd, myShowGFXBinary->getState(), -1);
      break;
    }
    case RomListWidget::kAddrRelocationCmd:
    {
      sendCommand(cmd, myUseRelocation->getState(), -1);
      break;
    }
    default:
      break;
  }
}
