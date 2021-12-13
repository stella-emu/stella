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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Command.hxx"
#include "Dialog.hxx"
#include "EditTextWidget.hxx"
#include "FileListWidget.hxx"
#include "Icons.hxx"
#include "OSystem.hxx"

#include "NavigationWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavigationWidget::NavigationWidget(GuiObject* boss, const GUI::Font& font,
    int xpos, int ypos, int w, int h)
  : Widget(boss, font, xpos, ypos, w, h)
{
  // Add some buttons and textfield to show current directory
  const int lineHeight = _font.getLineHeight();
  const bool useMinimalUI = instance().settings().getBool("minimal_ui");

  if(!useMinimalUI)
  {
    const int
      fontHeight   = _font.getFontHeight(),
      fontWidth    = _font.getMaxCharWidth(),
      BTN_GAP      = fontWidth / 4;
    const bool smallIcon = lineHeight < 26;
    const GUI::Icon& homeIcon = smallIcon ? GUI::icon_home_small : GUI::icon_home_large;
    const GUI::Icon& prevIcon = smallIcon ? GUI::icon_prev_small : GUI::icon_prev_large;
    const GUI::Icon& nextIcon = smallIcon ? GUI::icon_next_small : GUI::icon_next_large;
    const GUI::Icon& upIcon = smallIcon ? GUI::icon_up_small : GUI::icon_up_large;
    const int iconWidth = homeIcon.width();
    const int buttonWidth = iconWidth + ((fontWidth + 1) & ~0b1) - 1; // round up to next odd
    const int buttonHeight = lineHeight + 2;

    myHomeButton = new ButtonWidget(boss, _font, xpos, ypos,
      buttonWidth, buttonHeight, homeIcon, FileListWidget::kHomeDirCmd);
    myHomeButton->setToolTip("Go back to initial directory.");
    boss->addFocusWidget(myHomeButton);
    xpos = myHomeButton->getRight() + BTN_GAP;

    myPrevButton = new ButtonWidget(boss, _font, xpos, ypos,
      buttonWidth, buttonHeight, prevIcon, FileListWidget::kPrevDirCmd);
    myPrevButton->setToolTip("Go back in directory history.");
    boss->addFocusWidget(myPrevButton);
    xpos = myPrevButton->getRight() + BTN_GAP;

    myNextButton = new ButtonWidget(boss, _font, xpos, ypos,
      buttonWidth, buttonHeight, nextIcon, FileListWidget::kNextDirCmd);
    myNextButton->setToolTip("Go forward in directory history.");
    boss->addFocusWidget(myNextButton);
    xpos = myNextButton->getRight() + BTN_GAP;

    myUpButton = new ButtonWidget(boss, _font, xpos, ypos,
      buttonWidth, buttonHeight, upIcon, ListWidget::kParentDirCmd);
    myUpButton->setToolTip("Go Up");
    boss->addFocusWidget(myUpButton);
    xpos = myUpButton->getRight() + BTN_GAP;
  }
  myDir = new EditTextWidget(boss, _font, xpos, ypos, _w + _x - xpos, lineHeight, "");
  myDir->setEditable(false, true);
  myDir->clearFlags(Widget::FLAG_RETAIN_FOCUS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::setList(FileListWidget* list)
{
  myList = list;

  // Let the FileListWidget handle the button commands
  if(myHomeButton)
    myHomeButton->setTarget(myList);
  if(myPrevButton)
    myPrevButton->setTarget(myList);
  if(myNextButton)
    myNextButton->setTarget(myList);
  if(myUpButton)
    myUpButton->setTarget(myList);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::setWidth(int w)
{
  // Adjust path display accordingly too
  myDir->setWidth(w - (myDir->getLeft() - _x));
  Widget::setWidth(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::updateUI()
{
  // Only enable the navigation buttons if function is available
  if(myHomeButton)
    myHomeButton->setEnabled(myList->hasPrevHistory());
  if(myPrevButton)
    myPrevButton->setEnabled(myList->hasPrevHistory());
  if(myNextButton)
    myNextButton->setEnabled(myList->hasNextHistory());
  if(myUpButton)
    myUpButton->setEnabled(myList->currentDir().hasParent());

  // Show current directory
  myDir->setText(myList->currentDir().getShortPath());
}
