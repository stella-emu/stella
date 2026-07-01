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

#include "Command.hxx"
#include "Dialog.hxx"
#include "FBSurface.hxx"
#include "FileListWidget.hxx"
#include "Icons.hxx"

#include "NavigationWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavigationWidget::NavigationWidget(GuiObject* boss, const GUI::Font& font,
    int xpos, int ypos, int w, int h)
  : Widget(boss, font, xpos, ypos, w, h)
{
  // Add some buttons and a path field to show the current directory
  const int
    fontWidth    = _font.getMaxCharWidth(),
    BTN_GAP      = fontWidth / 4;
  const bool smallIcon = _font.getLineHeight() < 26;
  const GUI::Icon& homeIcon = smallIcon ? GUI::icon_home_small : GUI::icon_home_large;
  const GUI::Icon& prevIcon = smallIcon ? GUI::icon_prev_small : GUI::icon_prev_large;
  const GUI::Icon& nextIcon = smallIcon ? GUI::icon_next_small : GUI::icon_next_large;
  const GUI::Icon& upIcon = smallIcon ? GUI::icon_up_small : GUI::icon_up_large;
  const int iconWidth = homeIcon.width();
  const int buttonWidth = iconWidth + ((fontWidth + 1) & ~0b1) + 1; // round up to next odd
  const int buttonHeight = h;
#ifndef BSPF_MACOS
  const string altKey = "Alt";
#else
  const string altKey = "Cmd";
#endif

  myHomeButton = new ButtonWidget(boss, _font, xpos, ypos,
    buttonWidth, buttonHeight, homeIcon, FileListWidget::kHomeDirCmd);
  myHomeButton->setToolTip("Go back to initial directory. (" + altKey + "+Pos1)");
  boss->addFocusWidget(myHomeButton);
  xpos = myHomeButton->getRight() + BTN_GAP;

  myPrevButton = new ButtonWidget(boss, _font, xpos, ypos,
    buttonWidth, buttonHeight, prevIcon, FileListWidget::kPrevDirCmd);
  myPrevButton->setToolTip("Go back in directory history. (" + altKey + "+Left)");
  boss->addFocusWidget(myPrevButton);
  xpos = myPrevButton->getRight() + BTN_GAP;

  myNextButton = new ButtonWidget(boss, _font, xpos, ypos,
    buttonWidth, buttonHeight, nextIcon, FileListWidget::kNextDirCmd);
  myNextButton->setToolTip("Go forward in directory history. (" + altKey + "+Right)");
  boss->addFocusWidget(myNextButton);
  xpos = myNextButton->getRight() + BTN_GAP;

  myUpButton = new ButtonWidget(boss, _font, xpos, ypos,
    buttonWidth, buttonHeight, upIcon, ListWidget::kParentDirCmd);
  myUpButton->setToolTip("Go Up.", Event::UIPrevDir, EventMode::kMenuMode);
  boss->addFocusWidget(myUpButton);
  xpos = myUpButton->getRight() + BTN_GAP;

  myPath = new PathWidget(boss, this, _font, xpos, ypos, _w + _x - xpos, h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::layoutChildren()
{
  const int fontWidth = _font.getMaxCharWidth();
  const int BTN_GAP = fontWidth / 4;
  const bool smallIcon = _font.getLineHeight() < 26;
  const GUI::Icon& homeIcon = smallIcon ? GUI::icon_home_small : GUI::icon_home_large;
  const GUI::Icon& prevIcon = smallIcon ? GUI::icon_prev_small : GUI::icon_prev_large;
  const GUI::Icon& nextIcon = smallIcon ? GUI::icon_next_small : GUI::icon_next_large;
  const GUI::Icon& upIcon   = smallIcon ? GUI::icon_up_small   : GUI::icon_up_large;

  // Re-pick the icon variants (the font height may have crossed the threshold)
  myHomeButton->setIcon(homeIcon);
  myPrevButton->setIcon(prevIcon);
  myNextButton->setIcon(nextIcon);
  myUpButton->setIcon(upIcon);

  const int buttonWidth = homeIcon.width() + ((fontWidth + 1) & ~0b1) + 1;
  int xpos = _x;
  myHomeButton->setArea(xpos, _y, buttonWidth, _h); xpos += buttonWidth + BTN_GAP;
  myPrevButton->setArea(xpos, _y, buttonWidth, _h); xpos += buttonWidth + BTN_GAP;
  myNextButton->setArea(xpos, _y, buttonWidth, _h); xpos += buttonWidth + BTN_GAP;
  myUpButton->setArea(xpos, _y, buttonWidth, _h);   xpos += buttonWidth + BTN_GAP;
  myPath->setArea(xpos, _y, _w + _x - xpos, _h);
  // The path is unchanged but its folder-link widths depend on the font
  myPath->refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::setArea(int x, int y, int w, int h)
{
  setPos(x, y);
  Widget::setWidth(w);   // base setters; children are re-flowed below
  Widget::setHeight(h);
  layoutChildren();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::setList(FileListWidget* list)
{
  myList = list;

  // Let the FileListWidget handle the button commands
  myHomeButton->setTarget(myList);
  myPrevButton->setTarget(myList);
  myNextButton->setTarget(myList);
  myUpButton->setTarget(myList);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::setWidth(int w)
{
  // Adjust path display accordingly too
  myPath->setWidth(w - (myPath->getLeft() - _x));
  Widget::setWidth(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::setVisible(bool isVisible)
{
  if(isVisible)
  {
    this->clearFlags(FLAG_INVISIBLE);
    this->setEnabled(true);
    myHomeButton->clearFlags(FLAG_INVISIBLE);
    myHomeButton->setEnabled(true);
    myPrevButton->clearFlags(FLAG_INVISIBLE);
    myPrevButton->setEnabled(true);
    myNextButton->clearFlags(FLAG_INVISIBLE);
    myNextButton->setEnabled(true);
    myUpButton->clearFlags(FLAG_INVISIBLE);
    myUpButton->setEnabled(true);
    myPath->clearFlags(FLAG_INVISIBLE);
    myPath->setEnabled(true);
  }
  else
  {
    this->setFlags(FLAG_INVISIBLE);
    this->setEnabled(false);
    myHomeButton->setFlags(FLAG_INVISIBLE);
    myHomeButton->setEnabled(false);
    myPrevButton->setFlags(FLAG_INVISIBLE);
    myPrevButton->setEnabled(false);
    myNextButton->setFlags(FLAG_INVISIBLE);
    myNextButton->setEnabled(false);
    myUpButton->setFlags(FLAG_INVISIBLE);
    myUpButton->setEnabled(false);

    myPath->setFlags(FLAG_INVISIBLE);
    myPath->setEnabled(false);
    myPath->setPath("");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::updateUI()
{
  if(isVisible())
  {
    // Only enable the navigation buttons if function is available
    myHomeButton->setEnabled(myList->hasPrevHistory());
    myPrevButton->setEnabled(myList->hasPrevHistory());
    myNextButton->setEnabled(myList->hasNextHistory());
    myUpButton->setEnabled(myList->currentDir().hasParent());
    myPath->setPath(myList->currentDir().getShortPath());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::handleCommand(CommandSender* sender, int cmd, int data,
                                     int id)
{
  if(cmd == kFolderClicked)
  {
    const FSNode node(myPath->getPath(id));
    myList->selectDirectory(node);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavigationWidget::PathWidget::PathWidget(GuiObject* boss, CommandReceiver* target,
    const GUI::Font& font, int xpos, int ypos, int w, int h)
  : Widget(boss, font, xpos, ypos, w, h),
    myTarget{target}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::PathWidget::setPath(string_view path)
{
  if(path == myLastPath)
    return;

  myLastPath = path;

  const int fontWidth = _font.getMaxCharWidth();
  int x = _x + fontWidth;
  int w = _w;
  FSNode node(path);

  // Calculate how many path parts can be displayed
  std::vector<FSNode> nodes;
  bool cutFirst = false;
  while(node.hasParent() && w >= fontWidth)
  {
    string_view name = node.getName();
    int l = static_cast<int>(name.length() + 2);

    if(name.back() == FSNode::PATH_SEPARATOR)
      l--;
    if(node.getParent().hasParent())
      l++;

    w -= l * fontWidth;
    nodes.push_back(node);
    node = node.getParent();
  }
  if(w < 0 || node.hasParent())
    cutFirst = true;

  // Update/add widgets for path parts display
  size_t idx = 0;
  for(auto it = nodes.rbegin(); it != nodes.rend(); ++it, ++idx)
  {
    string name = it->getName();
    const string& curPath = it->getPath();

    if(it == nodes.rbegin() && cutFirst)
      name = ">";
    else
    {
      if(name.back() == FSNode::PATH_SEPARATOR)
        name.pop_back();
      if(it + 1 != nodes.rend())
        name += " >";
    }
    const int width = static_cast<int>(name.length() + 1) * fontWidth;

    if(myFolderList.size() > idx)
    {
      myFolderList[idx]->setPath(curPath);
      // Set the full geometry: after a font change the row height and vertical
      // position move too, not just the X/width, so the text stays centered
      myFolderList[idx]->setArea(x, _y, width, _h);
      myFolderList[idx]->setLabel(name);
    }
    else
    {
      // Add new widget to list
      auto* s = new FolderLinkWidget(_boss, _font, x, _y,
                                     width, _h, name, curPath);
      s->setID(static_cast<uInt32>(idx));
      s->setTarget(myTarget);
      myFolderList.push_back(s);
    }
    x += width;
  }
  // Hide any remaining widgets
  while(idx < size(myFolderList))
  {
    myFolderList[idx]->setWidth(0);
    ++idx;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::PathWidget::refresh()
{
  // setPath() early-returns when the path is unchanged; clear the cache so the
  // folder-link widths are recomputed for the current font
  const string path = myLastPath;
  myLastPath.clear();
  setPath(path);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& NavigationWidget::PathWidget::getPath(int idx) const
{
  assert(size_t(idx) < myFolderList.size());
  return myFolderList[idx]->getPath();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavigationWidget::PathWidget::FolderLinkWidget::FolderLinkWidget(
    GuiObject* boss, const GUI::Font& font,
    int x, int y, int w, int h, string_view text, string_view path)
  : ButtonWidget(boss, font, x, y, w, h, text, kFolderClicked),
    myPath{path}
{
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_CLEARBG;

  _bgcolor = kDlgColor;
  _bgcolorhi = kBtnColorHi;
  _textcolor = kTextColor;
  _align = TextAlign::Center;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::PathWidget::FolderLinkWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

  if(hilite)
    s.frameRect(_x, _y, _w, _h, kBtnBorderColorHi);
  s.drawString(_font, _label, _x + 1, _y + (_h - _font.getFontHeight()) / 2 , _w,
    hilite ? _textcolorhi : _textcolor, _align);
}
