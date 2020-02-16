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
#include "EventHandler.hxx"
#include "Font.hxx"
#include "FBSurface.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "MessageBox.hxx"
#include "HighScoresManager.hxx"


#include "HighScoresDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HighScoresDialog::HighScoresDialog(OSystem& osystem, DialogContainer& parent,
                                   const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "High Scores"),
  _max_w(max_w),
  _max_h(max_h),
  myInitials(""),
  myDirty(false)
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight = font.getLineHeight(),
    fontWidth = font.getMaxCharWidth();
  const int VBORDER = 8;
  const int HBORDER = 10;
  const int VGAP = 4;

  int xpos, ypos;
  WidgetArray wid;
  VariantList items;

  _w = std::min(max_w, 44 * fontWidth + HBORDER * 2);
  _h = std::min(max_h, 400);

  ypos = VBORDER + _th; xpos = HBORDER;


  //items.clear();

  StaticTextWidget* s = new StaticTextWidget(this, font, xpos, ypos + 1, "Variation ");
  myVariationWidget = new PopUpWidget(this, font, s->getRight(), ypos,
                                      font.getStringWidth("256") - 4, lineHeight, items, "", 0,
                                      kVariationChanged);
  wid.push_back(myVariationWidget);

  ypos += lineHeight + VGAP * 4;

  int xposRank = HBORDER;
  int xposScore = xposRank + font.getStringWidth("Rank") + 16;
  int xposSpecial = xposScore + font.getStringWidth("Score") + 24;
  int xposName = xposSpecial + font.getStringWidth("Round") + 16;
  int xposDate = xposName + font.getStringWidth("Name") + 16;
  int xposDelete = xposDate + font.getStringWidth("YY-MM-DD HH:MM") + 16;
  int nWidth = font.getStringWidth("ABC") + 4;

  new StaticTextWidget(this, font, xposRank, ypos + 1, "Rank");
  new StaticTextWidget(this, font, xposScore, ypos + 1, " Score");
  mySpecialLabelWidget = new StaticTextWidget(this, font, xposSpecial, ypos + 1, "Round");
  new StaticTextWidget(this, font, xposName - 2, ypos + 1, "Name");
  new StaticTextWidget(this, font, xposDate+16, ypos + 1, "Date   Time");

  ypos += lineHeight + VGAP;

  for (uInt32 p = 0; p < NUM_POSITIONS; ++p)
  {
    myPositionsWidget[p] = new StaticTextWidget(this, font, xposRank + 8, ypos + 1,
                                          (p < 9 ? " " : "") + std::to_string(p + 1));
    myScoresWidget[p] = new StaticTextWidget(this, font, xposScore, ypos + 1, "123456");
    mySpecialsWidget[p] = new StaticTextWidget(this, font, xposSpecial + 8, ypos + 1, "123");
    myNamesWidget[p] = new StaticTextWidget(this, font, xposName + 2, ypos + 1, "   ");
    myEditNamesWidget[p] = new EditTextWidget(this, font, xposName, ypos - 1, nWidth, lineHeight);
    myEditNamesWidget[p]->setFlags(EditTextWidget::FLAG_INVISIBLE);
    myEditNamesWidget[p]->setEnabled(false);
    wid.push_back(myEditNamesWidget[p]);
    myDatesWidget[p] = new StaticTextWidget(this, font, xposDate, ypos + 1, "12-02-20 17:15");
    myDeleteButtons[p] = new ButtonWidget(this, font, xposDelete, ypos + 1, 18, 18, "X",
                                         kDeleteSingle);
    myDeleteButtons[p]->setID(p);
    wid.push_back(myDeleteButtons[p]);

    ypos += lineHeight + VGAP;
  }
  ypos += VGAP * 2;

  myMD5Widget = new StaticTextWidget(this, ifont, xpos, ypos + 1, "MD5: 12345678901234567890123456789012");

  addDefaultsOKCancelBGroup(wid, font, "Save", "Cancel", " Reset ");
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
  if (!surface().attributes().blending)
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
  myVariationWidget->addItems(items);
  myVariationWidget->setSelected(instance().highScores().variation());
  myVariationWidget->setEnabled(instance().highScores().numVariations() > 1);

  string label = "   " + instance().highScores().specialLabel();
  if (label.length() > 5)
    label = label.substr(label.length() - 5);
  mySpecialLabelWidget->setLabel(label);

  myMD5 = instance().console().properties().get(PropType::Cart_MD5);
  myMD5Widget->setLabel("MD5: " + myMD5);

  myEditPos = myHighScorePos = -1;
  myNow = now();
  myDirty = false;
  handleVariation(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::saveConfig()
{
  // save initials and remember for the next time
  if (myHighScorePos != -1)
  {
    myInitials = myEditNamesWidget[myHighScorePos]->getText();
    myNames[myHighScorePos] = myInitials;
  }
  // save selected variation
  saveHighScores(myVariation);
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
      instance().eventHandler().leaveMenuMode();
      break;

    case kVariationChanged:
      handleVariation();
      break;

    case kDeleteSingle:
      deletePos(id);
      updateWidgets();
      break;

    case GuiObject::kDefaultsCmd:
      for (int p = NUM_POSITIONS - 1; p >= 0; --p)
        deletePos(p);
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
    myVariation = myVariationWidget->getSelectedTag().toInt();

    loadHighScores(myVariation);

    myEditPos = -1;

    if (myVariation == instance().highScores().variation())
      handlePlayedVariation();

    updateWidgets(init);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::updateWidgets(bool init)
{
  for (Int32 p = 0; p < NUM_POSITIONS; ++p)
  {
    ostringstream buf;

    if (myHighScores[p] > 0)
    {
      buf << std::setw(HSM::MAX_SCORE_DIGITS) << std::setfill(' ') << myHighScores[p];
      myPositionsWidget[p]->clearFlags(Widget::FLAG_INVISIBLE);
      myDeleteButtons[p]->clearFlags(Widget::FLAG_INVISIBLE);
      myDeleteButtons[p]->setEnabled(true);
    }
    else
    {
      myPositionsWidget[p]->setFlags(Widget::FLAG_INVISIBLE);
      myDeleteButtons[p]->setFlags(Widget::FLAG_INVISIBLE);
      myDeleteButtons[p]->setEnabled(false);
    }
    myScoresWidget[p]->setLabel(buf.str());

    buf.str("");
    if (mySpecials[p] > 0)
      buf << std::setw(HSM::MAX_SPECIAL_DIGITS) << std::setfill(' ') << mySpecials[p];
    mySpecialsWidget[p]->setLabel(buf.str());

    myNamesWidget[p]->setLabel(myNames[p]);
    myDatesWidget[p]->setLabel(myDates[p]);

    if (p == myEditPos)
    {
      myNamesWidget[p]->setFlags(EditTextWidget::FLAG_INVISIBLE);
      myEditNamesWidget[p]->clearFlags(EditTextWidget::FLAG_INVISIBLE);
      myEditNamesWidget[p]->setEnabled(true);
      myEditNamesWidget[p]->setEditable(true);
      if (init)
        myEditNamesWidget[p]->setText(myInitials);
    }
    else
    {
      myNamesWidget[p]->clearFlags(EditTextWidget::FLAG_INVISIBLE);
      myEditNamesWidget[p]->setFlags(EditTextWidget::FLAG_INVISIBLE);
      myEditNamesWidget[p]->setEnabled(false);
      myEditNamesWidget[p]->setEditable(false);
    }
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::handlePlayedVariation()
{
  Int32 newScore = instance().highScores().score();

  if (newScore > 0)
  {
    Int32 newSpecial = instance().highScores().special();
    bool scoreInvert = instance().highScores().scoreInvert();

    for (myHighScorePos = 0; myHighScorePos < NUM_POSITIONS; ++myHighScorePos)
    {
      if ((!scoreInvert && newScore > myHighScores[myHighScorePos]) ||
        ((scoreInvert && newScore < myHighScores[myHighScorePos]) || myHighScores[myHighScorePos] == 0))
        break;
      if (newScore == myHighScores[myHighScorePos] && newSpecial > mySpecials[myHighScorePos])
        break;
    }

    if (myHighScorePos < NUM_POSITIONS)
    {
      myEditPos = myHighScorePos;
      for (Int32 p = NUM_POSITIONS - 1; p > myHighScorePos; --p)
      {
        myHighScores[p] = myHighScores[p - 1];
        mySpecials[p] = mySpecials[p - 1];
        myNames[p] = myNames[p - 1];
        myDates[p] = myDates[p - 1];
      }
      myHighScores[myHighScorePos] = newScore;
      //myNames[myHighScorePos] = "";
      mySpecials[myHighScorePos] = newSpecial;
      myDates[myHighScorePos] = myNow;
      myDirty = true;
    }
    else
      myHighScorePos = -1;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::deletePos(int pos)
{
  for (Int32 p = pos; p < NUM_POSITIONS - 1; ++p)
  {
    myHighScores[p] = myHighScores[p + 1];
    mySpecials[p] = mySpecials[p + 1];
    myNames[p] = myNames[p + 1];
    myDates[p] = myDates[p + 1];
  }
  myHighScores[NUM_POSITIONS - 1] = 0;
  mySpecials[NUM_POSITIONS - 1] = 0;
  myNames[NUM_POSITIONS - 1] = "";
  myDates[NUM_POSITIONS - 1] = "";

  if (myEditPos == pos)
  {
    myHighScorePos = myEditPos = -1;
  }
  if (myEditPos > pos)
  {
    myHighScorePos--;
    myEditPos--;
    myEditNamesWidget[myEditPos]->setText(myEditNamesWidget[myEditPos + 1]->getText());
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

      msg.push_back("Do you want to save the changed");
      msg.push_back("high scores for this variation?");
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
void HighScoresDialog::saveHighScores(Int32 variation) const
{
  if(instance().hasConsole())
  {
    ostringstream buf;
    buf << instance().stateDir()
      << instance().console().properties().get(PropType::Cart_Name)
      << ".hs" << variation;

    // Make sure the file can be opened for writing
    Serializer out(buf.str());

    if(!out)
    {
      buf.str("");
      buf << "Can't open/save to high scores file for variation " << variation;
      instance().frameBuffer().showMessage(buf.str());
    }

    // Do a complete high scores save
    if (!save(out, variation))
    {
      buf.str("");
      buf << "Error saving high scores for variation" << variation;
      instance().frameBuffer().showMessage(buf.str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::loadHighScores(Int32 variation)
{
  for (Int32 p = 0; p < NUM_POSITIONS; ++p)
  {
    myHighScores[p] = 0;
    mySpecials[p] = 0;
    myNames[p] = "";
    myDates[p] = "";
  }

  if(instance().hasConsole())
  {
    ostringstream buf;
    buf << instance().stateDir()
      << instance().console().properties().get(PropType::Cart_Name)
      << ".hs" << variation;

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
    for (Int32 p = 0; p < NUM_POSITIONS; ++p)
    {
      out.putInt(myHighScores[p]);
      out.putInt(mySpecials[p]);
      out.putString(myNames[p]);
      out.putString(myDates[p]);
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

    for (Int32 p = 0; p < NUM_POSITIONS; ++p)
    {
      myHighScores[p] = in.getInt();
      mySpecials[p] = in.getInt();
      myNames[p] = in.getString();
      myDates[p] = in.getString();
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
