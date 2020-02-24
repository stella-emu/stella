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
#include "Console.hxx"
#include "Launcher.hxx"
#include "EventHandler.hxx"
#include "Font.hxx"
#include "PropsSet.hxx"
#include "FBSurface.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "MessageBox.hxx"
#include "HighScoresManager.hxx"

#include "HighScoresDialog.hxx"

static constexpr int BUTTON_GFX_W = 10, BUTTON_GFX_H = 10;

static constexpr std::array<uInt32, BUTTON_GFX_H> PREV_GFX = {
  0b0000110000,
  0b0000110000,
  0b0001111000,
  0b0001111000,
  0b0011001100,
  0b0011001100,
  0b0110000110,
  0b0110000110,
  0b1100000011,
  0b1100000011,
};

static constexpr std::array<uInt32, BUTTON_GFX_H> NEXT_GFX = {
  0b1100000011,
  0b1100000011,
  0b0110000110,
  0b0110000110,
  0b0011001100,
  0b0011001100,
  0b0001111000,
  0b0001111000,
  0b0000110000,
  0b0000110000,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HighScoresDialog::HighScoresDialog(OSystem& osystem, DialogContainer& parent,
                                   int max_w, int max_h,
                                   Menu::AppMode mode)
  : Dialog(osystem, parent, osystem.frameBuffer().font(), "High Scores"),
    myMode(mode),
    _max_w(max_w),
    _max_h(max_h),
    myVariation(HSM::DEFAULT_VARIATION),
    myInitials(""),
    myDirty(false),
    myHighScoreSaved(false)
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight = _font.getLineHeight(),
    fontWidth = _font.getMaxCharWidth(),
    infoLineHeight = ifont.getLineHeight();
  const int VBORDER = 10;
  const int HBORDER = 10;
  const int VGAP = 4;

  int xpos, ypos;
  WidgetArray wid;
  VariantList items;

  ypos = VBORDER + _th; xpos = HBORDER;
  ypos += lineHeight + VGAP * 2; // space for game name

  StaticTextWidget* s = new StaticTextWidget(this, _font, xpos, ypos + 1, "Variation ");
  myVariationPopup = new PopUpWidget(this, _font, s->getRight(), ypos,
                                      _font.getStringWidth("256") - 4, lineHeight, items, "", 0,
                                      kVariationChanged);
  wid.push_back(myVariationPopup);
  myPrevVarButton = new ButtonWidget(this, _font, myVariationPopup->getRight() + 157, ypos - 1,
                                     48, myVariationPopup->getHeight(),
                                     PREV_GFX.data(), BUTTON_GFX_W, BUTTON_GFX_H, kPrevVariation);
  wid.push_back(myPrevVarButton);
  myNextVarButton = new ButtonWidget(this, _font, myPrevVarButton->getRight() + 8, ypos - 1,
                                     48, myVariationPopup->getHeight(),
                                     NEXT_GFX.data(), BUTTON_GFX_W, BUTTON_GFX_H, kNextVariation);
  wid.push_back(myNextVarButton);

  ypos += lineHeight + VGAP * 4;

  int xposRank = HBORDER;
  int xposScore = xposRank + _font.getStringWidth("Rank");
  int xposSpecial = xposScore + _font.getStringWidth("   Score") + 16;
  int xposName = xposSpecial + _font.getStringWidth("Round") + 16;
  int xposDate = xposName + _font.getStringWidth("Name") + 16;
  int xposDelete = xposDate + _font.getStringWidth("YY-MM-DD HH:MM") + 16;
  int nWidth = _font.getStringWidth("ABC") + 4;

  new StaticTextWidget(this, _font, xposRank, ypos + 1, "Rank");
  new StaticTextWidget(this, _font, xposScore, ypos + 1, "   Score");
  mySpecialLabelWidget = new StaticTextWidget(this, _font, xposSpecial, ypos + 1, "Round");
  new StaticTextWidget(this, _font, xposName - 2, ypos + 1, "Name");
  new StaticTextWidget(this, _font, xposDate+16, ypos + 1, "Date   Time");

  ypos += lineHeight + VGAP;

  for (uInt32 r = 0; r < NUM_RANKS; ++r)
  {
    myRankWidgets[r] = new StaticTextWidget(this, _font, xposRank + 8, ypos + 1,
                                          (r < 9 ? " " : "") + std::to_string(r + 1));
    myScoreWidgets[r] = new StaticTextWidget(this, _font, xposScore, ypos + 1, "12345678");
    mySpecialWidgets[r] = new StaticTextWidget(this, _font, xposSpecial + 8, ypos + 1, "123");
    myNameWidgets[r] = new StaticTextWidget(this, _font, xposName + 2, ypos + 1, "   ");
    myEditNameWidgets[r] = new EditTextWidget(this, _font, xposName, ypos - 1, nWidth, lineHeight);
    myEditNameWidgets[r]->setFlags(EditTextWidget::FLAG_INVISIBLE);
    myEditNameWidgets[r]->setEnabled(false);
    wid.push_back(myEditNameWidgets[r]);
    myDateWidgets[r] = new StaticTextWidget(this, _font, xposDate, ypos + 1, "YY-MM-DD HH:MM");
    myDeleteButtons[r] = new ButtonWidget(this, _font, xposDelete, ypos + 1, 18, 18, "X",
                                          kDeleteSingle);
    myDeleteButtons[r]->setID(r);
    wid.push_back(myDeleteButtons[r]);

    ypos += lineHeight + VGAP;
  }
  ypos += VGAP;

  _w = myDeleteButtons[0]->getRight() + HBORDER;
  myNotesWidget = new StaticTextWidget(this, ifont, xpos, ypos + 1, _w - HBORDER * 2,
                                       infoLineHeight, "Note: ");

  ypos += infoLineHeight + VGAP;

  myMD5Widget = new StaticTextWidget(this, ifont, xpos, ypos + 1, "MD5:  12345678901234567890123456789012");

  _h = myMD5Widget->getBottom() + VBORDER + buttonHeight(_font) + VBORDER;

  myGameNameWidget = new StaticTextWidget(this, _font, HBORDER, VBORDER + _th + 1,
                                          _w - HBORDER * 2, lineHeight);

  addDefaultsOKCancelBGroup(wid, _font, "Save", "Cancel", " Reset ");
  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HighScoresDialog::~HighScoresDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::loadConfig()
{
  // Enable blending (only once is necessary)
  if (myMode == Menu::AppMode::emulator && !surface().attributes().blending)
  {
    surface().attributes().blending = true;
    surface().attributes().blendalpha = 90;
    surface().applyAttributes();
  }

  VariantList items;

  // fill drown down with all variation numbers of current game
  items.clear();
  for (Int32 i = 1; i <= instance().highScores().numVariations(); ++i)
  {
    ostringstream buf;
    buf << std::setw(3) << std::setfill(' ') << i;
    VarList::push_back(items, buf.str(), i);
  }
  myVariationPopup->addItems(items);

  Int32 variation;
  if(instance().highScores().numVariations() == 1)
    variation = HSM::DEFAULT_VARIATION;
  else
    variation = instance().highScores().variation();
  if(variation != HSM::NO_VALUE)
  {
    myVariationPopup->setSelected(variation);
    myUserDefVar = false;
  }
  else
  {
    // use last selected variation
    myVariationPopup->setSelected(myVariation);
    myUserDefVar = true;
  }

  myVariationPopup->setEnabled(instance().highScores().numVariations() > 1);

  string label = "   " + instance().highScores().specialLabel();
  if (label.length() > 5)
    label = label.substr(label.length() - 5);
  mySpecialLabelWidget->setLabel(label);

  myNotesWidget->setLabel("Note: " + instance().highScores().notes());

  if (instance().hasConsole())
    myMD5 = instance().console().properties().get(PropType::Cart_MD5);
  else
    myMD5 = instance().launcher().selectedRomMD5();

  myMD5Widget->setLabel("MD5:  " + myMD5);

  // requires the current MD5
  myGameNameWidget->setLabel(cartName());

  myEditRank = myHighScoreRank = -1;
  myNow = now();
  myDirty = myHighScoreSaved = false;
  handleVariation(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::saveConfig()
{
  // save initials and remember for the next time
  if (myHighScoreRank != -1)
  {
    myInitials = myEditNameWidgets[myHighScoreRank]->getText();
    myNames[myHighScoreRank] = myInitials;
  }
  // save selected variation
  saveHighScores(myVariation);
  if(myVariation == instance().highScores().variation() || myUserDefVar)
    myHighScoreSaved = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch (cmd)
  {
    case kOKCmd:
      saveConfig();
      [[fallthrough]];
    case kCloseCmd:
      if(myMode != Menu::AppMode::emulator)
        close();
      else
        instance().eventHandler().leaveMenuMode();
      break;

    case kVariationChanged:
      handleVariation();
      break;
    case kPrevVariation:
      myVariationPopup->setSelected(myVariation - 1);
      handleVariation();
      break;

    case kNextVariation:
      myVariationPopup->setSelected(myVariation + 1);
      handleVariation();
      break;

    case kDeleteSingle:
      deleteRank(id);
      updateWidgets();
      break;

    case GuiObject::kDefaultsCmd: // "Reset" button
      for (int r = NUM_RANKS - 1; r >= 0; --r)
        deleteRank(r);
      updateWidgets();
      break;

    case kConfirmSave:
      saveConfig();
      [[fallthrough]];
    case kCancelSave:
      myDirty = false;
      handleVariation();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::handleVariation(bool init)
{
  if (handleDirty())
  {
    myVariation = myVariationPopup->getSelectedTag().toInt();

    loadHighScores(myVariation);

    myEditRank = -1;

    if (myVariation == instance().highScores().variation() || myUserDefVar)
      handlePlayedVariation();

    updateWidgets(init);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::updateWidgets(bool init)
{
  myPrevVarButton->setEnabled(myVariation > 1);
  myNextVarButton->setEnabled(myVariation < instance().highScores().numVariations());

  for (Int32 r = 0; r < NUM_RANKS; ++r)
  {
    ostringstream buf;

    if (myHighScores[r] > 0)
    {
      myRankWidgets[r]->clearFlags(Widget::FLAG_INVISIBLE);
      myDeleteButtons[r]->clearFlags(Widget::FLAG_INVISIBLE);
      myDeleteButtons[r]->setEnabled(true);
    }
    else
    {
      myRankWidgets[r]->setFlags(Widget::FLAG_INVISIBLE);
      myDeleteButtons[r]->setFlags(Widget::FLAG_INVISIBLE);
      myDeleteButtons[r]->setEnabled(false);
    }
    myScoreWidgets[r]->setLabel(instance().highScores().formattedScore(myHighScores[r],
                                HSM::MAX_SCORE_DIGITS));

    if (mySpecials[r] > 0)
      buf << std::setw(HSM::MAX_SPECIAL_DIGITS) << std::setfill(' ') << mySpecials[r];
    mySpecialWidgets[r]->setLabel(buf.str());

    myNameWidgets[r]->setLabel(myNames[r]);
    myDateWidgets[r]->setLabel(myDates[r]);

    if (r == myEditRank)
    {
      myNameWidgets[r]->setFlags(EditTextWidget::FLAG_INVISIBLE);
      myEditNameWidgets[r]->clearFlags(EditTextWidget::FLAG_INVISIBLE);
      myEditNameWidgets[r]->setEnabled(true);
      myEditNameWidgets[r]->setEditable(true);
      if (init)
        myEditNameWidgets[r]->setText(myInitials);
    }
    else
    {
      myNameWidgets[r]->clearFlags(EditTextWidget::FLAG_INVISIBLE);
      myEditNameWidgets[r]->setFlags(EditTextWidget::FLAG_INVISIBLE);
      myEditNameWidgets[r]->setEnabled(false);
      myEditNameWidgets[r]->setEditable(false);
    }
  }
  _defaultWidget->setEnabled(myHighScores[0] > 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::handlePlayedVariation()
{
  Int32 newScore = instance().highScores().score();

  if (!myHighScoreSaved && newScore > 0)
  {
    Int32 newSpecial = instance().highScores().special();
    bool scoreInvert = instance().highScores().scoreInvert();

    for (myHighScoreRank = 0; myHighScoreRank < NUM_RANKS; ++myHighScoreRank)
    {
      if ((!scoreInvert && newScore > myHighScores[myHighScoreRank]) ||
        ((scoreInvert && newScore < myHighScores[myHighScoreRank]) || myHighScores[myHighScoreRank] == 0))
        break;
      if (newScore == myHighScores[myHighScoreRank] && newSpecial > mySpecials[myHighScoreRank])
        break;
    }

    if (myHighScoreRank < NUM_RANKS)
    {
      myEditRank = myHighScoreRank;
      for (Int32 r = NUM_RANKS - 1; r > myHighScoreRank; --r)
      {
        myHighScores[r] = myHighScores[r - 1];
        mySpecials[r] = mySpecials[r - 1];
        myNames[r] = myNames[r - 1];
        myDates[r] = myDates[r - 1];
      }
      myHighScores[myHighScoreRank] = newScore;
      mySpecials[myHighScoreRank] = newSpecial;
      myDates[myHighScoreRank] = myNow;
      myDirty |= !myUserDefVar; // only ask when the variation was read by defintion
    }
    else
      myHighScoreRank = -1;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::deleteRank(int rank)
{
  for (Int32 r = rank; r < NUM_RANKS - 1; ++r)
  {
    myHighScores[r] = myHighScores[r + 1];
    mySpecials[r] = mySpecials[r + 1];
    myNames[r] = myNames[r + 1];
    myDates[r] = myDates[r + 1];
  }
  myHighScores[NUM_RANKS - 1] = 0;
  mySpecials[NUM_RANKS - 1] = 0;
  myNames[NUM_RANKS - 1] = "";
  myDates[NUM_RANKS - 1] = "";

  if (myEditRank == rank)
  {
    myHighScoreRank = myEditRank = -1;
  }
  if (myEditRank > rank)
  {
    myHighScoreRank--;
    myEditRank--;
    myEditNameWidgets[myEditRank]->setText(myEditNameWidgets[myEditRank + 1]->getText());
  }
  myDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresDialog::handleDirty()
{
  if (myDirty)
  {
    if (!myConfirmMsg)
    {
      StringList msg;

      msg.push_back("Do you want to save the changes");
      msg.push_back("for this variation?");
      msg.push_back("");
      myConfirmMsg = make_unique<GUI::MessageBox>
        (this, _font, msg, _max_w, _max_h, kConfirmSave, kCancelSave,
         "Yes", "No", "Save High Scores", false);
    }
    myConfirmMsg->show();
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string HighScoresDialog::cartName() const
{
  if(instance().hasConsole())
    return instance().console().properties().get(PropType::Cart_Name);
  else
  {
    Properties props;

    instance().propSet().getMD5(myMD5, props);
    if(props.get(PropType::Cart_Name).empty())
      return instance().launcher().currentNode().getNameWithExt("");
    else
      return props.get(PropType::Cart_Name);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::saveHighScores(Int32 variation) const
{
  ostringstream buf;

  buf << instance().stateDir() << cartName() << ".hs" << variation;

  // Make sure the file can be opened for writing
  Serializer out(buf.str());

  if(!out)
  {
    buf.str("");
    buf << "Can't open/save to high scores file for variation " << variation;
    instance().frameBuffer().showMessage(buf.str());
  }

  // Do a complete high scores save
  if(!save(out, variation))
  {
    buf.str("");
    buf << "Error saving high scores for variation" << variation;
    instance().frameBuffer().showMessage(buf.str());
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::loadHighScores(Int32 variation)
{
  for (Int32 r = 0; r < NUM_RANKS; ++r)
  {
    myHighScores[r] = 0;
    mySpecials[r] = 0;
    myNames[r] = "";
    myDates[r] = "";
  }

  ostringstream buf;

  buf << instance().stateDir() << cartName() << ".hs" << variation;

  // Make sure the file can be opened in read-only mode
  Serializer in(buf.str(), Serializer::Mode::ReadOnly);

  if(!in)
    return;

  // First test if we have a valid header
  // If so, do a complete high scores load
  buf.str("");
  try
  {
    if (in.getString() != HIGHSCORE_HEADER)
      buf << "Incompatible high scores for variation " << variation << " file";
    else
    {
      if (load(in, variation))
        return;
      else
        buf << "Invalid data in high scores for variation " << variation << " file";
    }
  }
  catch(...)
  {
    buf << "Invalid data in high scores for variation " << variation << " file";
  }

  instance().frameBuffer().showMessage(buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresDialog::save(Serializer& out, Int32 variation) const
{
  try
  {
    // Add header so that if the high score format changes in the future,
    // we'll know right away, without having to parse the rest of the file
    out.putString(HIGHSCORE_HEADER);

    out.putString(myMD5);
    out.putInt(variation);
    for (Int32 r = 0; r < NUM_RANKS; ++r)
    {
      out.putInt(myHighScores[r]);
      out.putInt(mySpecials[r]);
      out.putString(myNames[r]);
      out.putString(myDates[r]);
    }
  }
  catch(...)
  {
    cerr << "ERROR: HighScoresDialog::save() exception\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HighScoresDialog::load(Serializer& in, Int32 variation)
{
  try
  {
    if (in.getString() != myMD5)
      return false;
    if (Int32(in.getInt()) != variation)
      return false;

    for (Int32 r = 0; r < NUM_RANKS; ++r)
    {
      myHighScores[r] = in.getInt();
      mySpecials[r] = in.getInt();
      myNames[r] = in.getString();
      myDates[r] = in.getString();
    }
  }
  catch(...)
  {
    cerr << "ERROR: HighScoresDialog::load() exception\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string HighScoresDialog::now() const
{
  std::tm now = BSPF::localTime();
  ostringstream ss;

  ss << std::setfill('0') << std::right
    << std::setw(2) << (now.tm_year - 100) << '-'
    << std::setw(2) << (now.tm_mon + 1) << '-'
    << std::setw(2) << now.tm_mday << " "
    << std::setw(2) << now.tm_hour << ":"
    << std::setw(2) << now.tm_min;

  return ss.str();
}
