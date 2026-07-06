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

#include "Cheat.hxx"
#include "CheatManager.hxx"
#include "CheckListWidget.hxx"
#include "DialogContainer.hxx"
#include "Dialog.hxx"
#include "Font.hxx"
#include "InputTextDialog.hxx"
#include "Layout.hxx"
#include "OSystem.hxx"
#include "Widget.hxx"

#include "CheatCodeDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatCodeDialog::CheatCodeDialog(OSystem& osystem, DialogContainer& parent,
                                 const GUI::Font& font)
  : Dialog(osystem, parent, font, "Cheat codes")
{
  const int buttonHeight = Dialog::buttonHeight(),
            buttonWidth  = Dialog::buttonWidth("One shot ");
  WidgetArray wid;

  // Widgets are only created here (at placeholder geometry); layout() assigns
  // all geometry from the current font.

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // List of cheats, with checkboxes to enable/disable
  myCheatList = new CheckListWidget(this, font, 0, 0, 1, 1);
  myCheatList->setEditable(false);
  wid.push_back(myCheatList);

  myAddButton = new ButtonWidget(this, font, 0, 0, buttonWidth, buttonHeight,
                                 "Add" + ELLIPSIS, kAddCheatCmd);
  wid.push_back(myAddButton);

  myEditButton = new ButtonWidget(this, font, 0, 0, buttonWidth, buttonHeight,
                                  "Edit" + ELLIPSIS, kEditCheatCmd);
  wid.push_back(myEditButton);

  myRemoveButton = new ButtonWidget(this, font, 0, 0, buttonWidth, buttonHeight,
                                    "Remove", kRemCheatCmd);
  wid.push_back(myRemoveButton);

  myOneShotButton = new ButtonWidget(this, font, 0, 0, buttonWidth, buttonHeight,
                                     "One shot" + ELLIPSIS, kAddOneShotCmd);
  wid.push_back(myOneShotButton);

  // Inputbox which will pop up when adding/editing a cheat
  StringList labels;
  labels.emplace_back("Name       ");
  labels.emplace_back("Code (hex) ");
  myCheatInput = std::make_unique<InputTextDialog>(this, font, labels, "Cheat code");
  myCheatInput->setTarget(this);

  // Add filtering for each textfield
  const EditableWidget::TextFilter f0 = [](char c) {
    return isprint(c) && c != '\"' && c != ':';
  };
  myCheatInput->setTextFilter(f0, 0);

  const EditableWidget::TextFilter f1 = [](char c) {
    return (c >= 'a' && c <= 'f') || (c >= '0' && c <= '9');
  };
  myCheatInput->setTextFilter(f1, 1);
  myCheatInput->setToolTip("See Stella documentation for details.", 1);

  addToFocusList(wid);

  // Add OK and Cancel buttons
  wid.clear();
  addOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);

  setHelpAnchor("Cheats");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatCodeDialog::~CheatCodeDialog() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::widgetItem;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            buttonWidth  = Dialog::buttonWidth("One shot "),
            VGAP         = Dialog::vGap(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder();

  // Size the (fixed) dialog from the current font so it reflows on font change
  _w = 45 * fontWidth + HBORDER * 2;
  _h = _th + 11 * (lineHeight + 4) + VBORDER * 2;

  const int listH = _h - _th - buttonHeight - VBORDER * 3;

  // Right-hand column of action buttons.  Each button is buttonHeight tall but
  // the column advances on a lineHeight pitch (matching the original), so it is
  // anchored in a lineHeight cell and overflows downward; a larger gap sets
  // 'One shot' apart from the editing buttons.
  auto buttonCol = std::make_unique<BoxLayout>(Dir::Vertical, 0, 0, 0);
  buttonCol->addFixed(anchoredItem(myAddButton), lineHeight);
  buttonCol->addSpace(VGAP * 2);
  buttonCol->addFixed(anchoredItem(myEditButton), lineHeight);
  buttonCol->addSpace(VGAP * 2);
  buttonCol->addFixed(anchoredItem(myRemoveButton), lineHeight);
  buttonCol->addSpace(VGAP * 2 * 3);
  buttonCol->addFixed(anchoredItem(myOneShotButton), lineHeight);

  // The list fills the remaining width to the left of the button column
  auto mainRow = std::make_unique<BoxLayout>(Dir::Horizontal, 0, 0, 0);
  mainRow->addStretch(widgetItem(myCheatList));
  mainRow->addSpace(fontWidth);
  mainRow->addFixed(std::move(buttonCol), buttonWidth);

  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addFixed(std::move(mainRow), listH);
  root->doLayout(0, _th, _w, _h - _th);

  // OK/Cancel along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::loadConfig()
{
  // Load items from CheatManager
  // Note that the items are always in the same order/number as given in
  // the CheatManager, so the arrays will be one-to-one
  StringList l;
  BoolArray b;

  const CheatList& list = instance().cheat().list();
  for(const auto& c: list)
  {
    l.push_back(c->name());
    b.push_back(c->enabled());
  }
  myCheatList->setList(l, b);

  // Redraw the list, auto-selecting the first item if possible
  myCheatList->setSelected(!l.empty() ? 0 : -1);

  const bool enabled = !list.empty();
  myEditButton->setEnabled(enabled);
  myRemoveButton->setEnabled(enabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::saveConfig()
{
  // Inspect checkboxes for enable/disable codes
  const CheatList& list = instance().cheat().list();
  for(uInt32 i = 0; const auto& cheat : list)
  {
    const bool enabled = myCheatList->getState(i++);
    enabled ? cheat->enable() : cheat->disable();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::addCheat()
{
  myCheatInput->show();    // Center input dialog over entire screen
  myCheatInput->setText("", 0);
  myCheatInput->setText("", 1);
  myCheatInput->setMessage("");
  myCheatInput->setFocus(0);
  myCheatInput->setEmitSignal(kCheatAdded);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::editCheat()
{
  const int idx = myCheatList->getSelected();
  if(idx < 0)
    return;

  const CheatList& list = instance().cheat().list();
  const string& name = list[idx]->name();
  const string& code = list[idx]->code();

  myCheatInput->show();    // Center input dialog over entire screen
  myCheatInput->setText(name, 0);
  myCheatInput->setText(code, 1);
  myCheatInput->setMessage("");
  myCheatInput->setFocus(1);
  myCheatInput->setEmitSignal(kCheatEdited);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::removeCheat()
{
  instance().cheat().remove(myCheatList->getSelected());
  loadConfig();  // reload the cheat list
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::addOneShotCheat()
{
  myCheatInput->show();    // Center input dialog over entire screen
  myCheatInput->setText("One-shot cheat", 0);
  myCheatInput->setText("", 1);
  myCheatInput->setMessage("");
  myCheatInput->setFocus(1);
  myCheatInput->setEmitSignal(kOneShotCheatAdded);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::handleCommand(CommandSender* sender, int cmd,
                                    int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kCloseCmd:
      close();
      break;

    case ListWidget::kDoubleClickedCmd:
      editCheat();
      break;

    case kAddCheatCmd:
      addCheat();
      break;

    case kEditCheatCmd:
      editCheat();
      break;

    case kCheatAdded:
    {
      const string& name = myCheatInput->getResult(0);
      const string& code = myCheatInput->getResult(1);
      if(CheatManager::isValidCode(code))
      {
        myCheatInput->close();
        instance().cheat().add(name, code);
        loadConfig();  // show changes onscreen
      }
      else
        myCheatInput->setMessage("Invalid code");
      break;
    }

    case kCheatEdited:
    {
      const string& name = myCheatInput->getResult(0);
      const string& code = myCheatInput->getResult(1);
      const bool enable = myCheatList->getSelectedState();
      const int idx = myCheatList->getSelected();
      if(CheatManager::isValidCode(code))
      {
        myCheatInput->close();
        instance().cheat().add(name, code, enable, idx);
        loadConfig();  // show changes onscreen
      }
      else
        myCheatInput->setMessage("Invalid code");
      break;
    }

    case kRemCheatCmd:
      removeCheat();
      break;

    case kAddOneShotCmd:
      addOneShotCheat();
      break;

    case kOneShotCheatAdded:
    {
      const string& name = myCheatInput->getResult(0);
      const string& code = myCheatInput->getResult(1);
      if(CheatManager::isValidCode(code))
      {
        myCheatInput->close();
        instance().cheat().addOneShot(name, code);
      }
      else
        myCheatInput->setMessage("Invalid code");
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
