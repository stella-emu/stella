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
NavigationWidget::NavigationWidget(GuiObject* boss, const GUI::Font& font)
  : Widget(boss, font, 0, 0)
{
  // Add some buttons and a path field to show the current directory.  They are
  // created at a placeholder position; layoutChildren() is the ONE place that
  // positions them, and it runs again from setArea() whenever we are resized
  const bool largeIcon = _font.isLarge();
  const GUI::Icon& homeIcon = largeIcon ? GUI::icon_home_large : GUI::icon_home_small;
  const GUI::Icon& prevIcon = largeIcon ? GUI::icon_prev_large : GUI::icon_prev_small;
  const GUI::Icon& nextIcon = largeIcon ? GUI::icon_next_large : GUI::icon_next_small;
  const GUI::Icon& upIcon = largeIcon ? GUI::icon_up_large : GUI::icon_up_small;
#ifndef BSPF_MACOS
  const string altKey = "Alt";
#else
  const string altKey = "Cmd";
#endif

  myHomeButton = new ButtonWidget(boss, _font, homeIcon, FileListWidget::kHomeDirCmd);
  myHomeButton->setToolTip("Go back to initial directory. (" + altKey + "+Pos1)");
  boss->addFocusWidget(myHomeButton);

  myPrevButton = new ButtonWidget(boss, _font, prevIcon, FileListWidget::kPrevDirCmd);
  myPrevButton->setToolTip("Go back in directory history. (" + altKey + "+Left)");
  boss->addFocusWidget(myPrevButton);

  myNextButton = new ButtonWidget(boss, _font, nextIcon, FileListWidget::kNextDirCmd);
  myNextButton->setToolTip("Go forward in directory history. (" + altKey + "+Right)");
  boss->addFocusWidget(myNextButton);

  myUpButton = new ButtonWidget(boss, _font, upIcon, ListWidget::kParentDirCmd);
  myUpButton->setToolTip("Go Up.", Event::UIPrevDir, EventMode::kMenuMode);
  boss->addFocusWidget(myUpButton);

  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  myPath = new PathWidget(boss, this, _font);

  layoutChildren();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size NavigationWidget::naturalSize() const
{
  // My ctor is built at a placeholder, so I cannot know my own height any other
  // way.  Read it from the font directly (the same formula the icon buttons use
  // to size themselves) rather than from a button's current height: that height
  // is mutable and layoutChildren() -- called once from the ctor at height 0 --
  // would otherwise have already overwritten it with a stale 0 by the time
  // anyone asks
  return Common::Size(std::max(_w, 0), ButtonWidget::calcHeight(_font));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavigationWidget::layoutChildren()
{
  const int fontWidth = _font.getMaxCharWidth();
  const int BTN_GAP = fontWidth / 4;
  const bool largeIcon = _font.isLarge();
  const GUI::Icon& homeIcon = largeIcon ? GUI::icon_home_large : GUI::icon_home_small;
  const GUI::Icon& prevIcon = largeIcon ? GUI::icon_prev_large : GUI::icon_prev_small;
  const GUI::Icon& nextIcon = largeIcon ? GUI::icon_next_large : GUI::icon_next_small;
  const GUI::Icon& upIcon   = largeIcon ? GUI::icon_up_large   : GUI::icon_up_small;

  // Re-pick the icon variants (the font height may have crossed the threshold)
  myHomeButton->setIcon(homeIcon);
  myPrevButton->setIcon(prevIcon);
  myNextButton->setIcon(nextIcon);
  myUpButton->setIcon(upIcon);

  // setIcon() re-sized each button around its new bitmap, so it knows its width.
  // Its HEIGHT comes from the font, the same as naturalSize() reports -- not
  // from _h, which is 0 on the placeholder pass the ctor makes before anyone
  // has given me a real height, and would otherwise squash the buttons down to
  // that placeholder permanently (setArea() is the only place their height is
  // ever set again). Center the row within whatever height I was actually given.
  const int buttonWidth = myHomeButton->getWidth();
  const int buttonHeight = ButtonWidget::calcHeight(_font);
  const int ypos = _y + (_h - buttonHeight) / 2;

  int xpos = _x;
  myHomeButton->setArea(xpos, ypos, buttonWidth, buttonHeight); xpos += buttonWidth + BTN_GAP;
  myPrevButton->setArea(xpos, ypos, buttonWidth, buttonHeight); xpos += buttonWidth + BTN_GAP;
  myNextButton->setArea(xpos, ypos, buttonWidth, buttonHeight); xpos += buttonWidth + BTN_GAP;
  myUpButton->setArea(xpos, ypos, buttonWidth, buttonHeight);   xpos += buttonWidth + BTN_GAP;
  myPath->setArea(xpos, ypos, _w + _x - xpos, buttonHeight);
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
    const GUI::Font& font)
  : Widget(boss, font, 0, 0),
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
  auto idx = 0uz;
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
      auto* s = new FolderLinkWidget(_boss, _font, name, curPath);
      s->setArea(x, _y, width, _h);
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
    string_view text, string_view path)
  : ButtonWidget(boss, font, 0, 0, text, kFolderClicked),
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
  s.drawString(_font, _label, _x + 1, _y + firstTextY(), _w,
    hilite ? _textcolorhi : _textcolor, _align);
}
