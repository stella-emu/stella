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
//#include "StringParser.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "HighScoresManager.hxx"


#include "HighScoresDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HighScoresDialog::HighScoresDialog(OSystem& osystem, DialogContainer& parent,
                                   const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "High Scores"),
    myInitials("")
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight = font.getLineHeight(),
    fontWidth = font.getMaxCharWidth();
    //fontHeight = font.getFontHeight(),
    //buttonHeight = font.getLineHeight() + 4;
    //infoLineHeight = ifont.getLineHeight();
  const int VBORDER = 8;
  const int HBORDER = 10;
  const int VGAP = 4;

  int xpos, ypos;
  WidgetArray wid;
  VariantList items;

  _w = 40 * fontWidth + HBORDER * 2; // max_w - 20;
  _h = 400; // max_h - 20;

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

    ypos += lineHeight + VGAP;
  }
  ypos += VGAP * 2;

  myMD5Widget = new StaticTextWidget(this, ifont, xpos, ypos + 1, "MD5: 12345678901234567890123456789012");

  addOKCancelBGroup(wid, font);
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

  items.clear();
  for (Int32 i = 1; i <= instance().highScores().numVariations(); ++i)
  {
    ostringstream buf;
    buf << std::setw(3) << std::setfill(' ') << i;
    VarList::push_back(items, buf.str(), i);
  }
  myVariationWidget->addItems(items);
  myDisplayedVariation = instance().highScores().variation();
  myVariationWidget->setSelected(myDisplayedVariation);

  mySpecialLabelWidget->setLabel(instance().highScores().specialLabel());

  // TDOO: required when leaving with hot key
  for (Int32 p = 0; p < NUM_POSITIONS; ++p)
  {
    myNamesWidget[p]->clearFlags(EditTextWidget::FLAG_INVISIBLE);
    myEditNamesWidget[p]->setFlags(EditTextWidget::FLAG_INVISIBLE);
    myEditNamesWidget[p]->setEnabled(false);
  }

  myMD5 = instance().console().properties().get(PropType::Cart_MD5);
  myMD5Widget->setLabel("MD5: " + myMD5);


  myPlayedVariation = instance().highScores().variation();

  myEditPos = myHighScorePos = -1;
  myNow = now();
  handleVariation(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::saveConfig()
{
  if (myHighScorePos != -1)
  {
    if (myDisplayedVariation != myPlayedVariation)
    {
      loadHighScores(myPlayedVariation);
      handlePlayedVariation();
    }
    myInitials = myEditNamesWidget[myHighScorePos]->getText();
    myNames[myHighScorePos] = myInitials;
    saveHighScores();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch (cmd)
  {
    case kOKCmd:
      saveConfig();
      // falls through...
    case kCloseCmd:
      resetVisibility();
      instance().eventHandler().leaveMenuMode();
      break;

    case kVariationChanged:
      handleVariation();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::resetVisibility()
{
  if (myEditPos != -1)
  {
    // hide again
    myEditNamesWidget[myEditPos]->setFlags(EditTextWidget::FLAG_INVISIBLE);
    myEditNamesWidget[myEditPos]->setEnabled(false);
    myNamesWidget[myEditPos]->clearFlags(EditTextWidget::FLAG_INVISIBLE);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::handlePlayedVariation()
{
  Int32 newScore = instance().highScores().score();

  if (newScore > 0)
  {
    Int32 newSpecial = instance().highScores().special();
    for (myHighScorePos = 0; myHighScorePos < NUM_POSITIONS; ++myHighScorePos)
    {
      if (newScore > myHighScores[myHighScorePos] ||
        (newScore == myHighScores[myHighScorePos] && newSpecial > mySpecials[myHighScorePos]))
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
    }
    else
      myHighScorePos = -1;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::handleVariation(bool init)
{
  resetVisibility();

  myDisplayedVariation = myVariationWidget->getSelectedTag().toInt();

  loadHighScores(myDisplayedVariation);

  myEditPos = -1;

  if (myDisplayedVariation == myPlayedVariation)
  {
    handlePlayedVariation();

    if (myHighScorePos != -1)
    {
      myNamesWidget[myHighScorePos]->setFlags(EditTextWidget::FLAG_INVISIBLE);
      myEditNamesWidget[myHighScorePos]->clearFlags(EditTextWidget::FLAG_INVISIBLE);
      myEditNamesWidget[myHighScorePos]->setEnabled(true);
      myEditNamesWidget[myHighScorePos]->setEditable(true);
      if (init)
        myEditNamesWidget[myHighScorePos]->setText(myInitials);
    }
  }

  for (Int32 p = 0; p < NUM_POSITIONS; ++p)
  {
    ostringstream buf;

    if (myHighScores[p] > 0)
      buf << std::setw(HSM::MAX_SCORE_DIGITS) << std::setfill(' ') << myHighScores[p];
    myScoresWidget[p]->setLabel(buf.str());

    buf.str("");
    if (mySpecials[p] > 0)
      buf << std::setw(HSM::MAX_SPECIAL_DIGITS) << std::setfill(' ') << mySpecials[p];
    mySpecialsWidget[p]->setLabel(buf.str());

    myNamesWidget[p]->setLabel(myNames[p]);
    myDatesWidget[p]->setLabel(myDates[p]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::saveHighScores() const
{
  if(instance().hasConsole())
  {
    ostringstream buf;
    buf << instance().stateDir()
      << instance().console().properties().get(PropType::Cart_Name)
      << ".hs" << myPlayedVariation;

    // Make sure the file can be opened for writing
    Serializer out(buf.str());

    if(!out)
    {
      buf.str("");
      buf << "Can't open/save to high scores file for variation " << myPlayedVariation;
      instance().frameBuffer().showMessage(buf.str());
      return;
    }

    // Do a complete high scores save
    if (!save(out))
    {
      buf.str("");
      buf << "Error saving high scores for variation" << myPlayedVariation;
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

  /////////////////////////////////////////////////////////////////////////////////////////////////
  /*
  // mock data
  const Int32 SCORES[NUM_POSITIONS] = {999999, 250000, 100000,  50000,  20000,
    5000,   2000,    200,     20,      0};
  const Int32 SPECIALS[NUM_POSITIONS] = {200, 150,  90,  70,  45,
  30,  25,  10,   7,   0};
  const string NAMES[NUM_POSITIONS] = {"RAM", "CDW", "AD ", "JTZ", "DOA",
  "ROM", "VCS", "N.S", "JWC", "  -"};
  const string DATES[NUM_POSITIONS] = {"19-12-24 21:00", "19-07-18 00:00", "20-01-01 12:00",
  "20-02-12 21:50", "20-02-11 14:16", "20-02-11 13:11",
  "20-02-10 19:45", "10-02-10 20:04", "05-02-09 22:32",
  "       -     -"};

  for (Int32 p = 0; p < NUM_POSITIONS; ++p)
  {
    myHighScores[p] = SCORES[p];
    mySpecials[p] = SPECIALS[p];
    myNames[p] = NAMES[p];
    myDates[p] = DATES[p];
  }
  */
  /////////////////////////////////////////////////////////////////////////////////////////////////

  if(instance().hasConsole())
  {
    ostringstream buf;
    buf << instance().stateDir()
      << instance().console().properties().get(PropType::Cart_Name)
      << ".hs" << variation;

    // Make sure the file can be opened in read-only mode
    Serializer in(buf.str(), Serializer::Mode::ReadOnly);
    if(!in)
    {
      //buf.str("");
      //buf << "No high scores file for variation " << variation << " found.";
      //instance().frameBuffer().showMessage(buf.str());
      return;
    }

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
          //buf << "High scores for variation  " << variation << " loaded";
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
bool HighScoresDialog::save(Serializer& out) const
{
  try
  {
    // Add header so that if the high score format changes in the future,
    // we'll know right away, without having to parse the rest of the file
    out.putString(HIGHSCORE_HEADER);

    out.putString(myMD5);
    out.putInt(myPlayedVariation);
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
    {
      return false;
    }
    if (Int32(in.getInt()) != variation)
    {
      return false;
    }
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
