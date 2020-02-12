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
  : Dialog(osystem, parent, font, "High Scores")
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
  myVariation = new PopUpWidget(this, font, s->getRight(), ypos,
                                font.getStringWidth("256") - 4, lineHeight, items, "", 0, 0);

  ypos += lineHeight + VGAP * 4;

  int xposRank = HBORDER;
  int xposScore = xposRank + font.getStringWidth("Rank") + 16;
  int xposSpecial = xposScore + font.getStringWidth("Score") + 24;
  int xposName = xposSpecial + font.getStringWidth("Round") + 16;
  int xposDate = xposName + font.getStringWidth("Name") + 16;
  int nWidth = font.getStringWidth("ABC") + 4;

  new StaticTextWidget(this, font, xposRank, ypos + 1, "Rank");
  new StaticTextWidget(this, font, xposScore, ypos + 1, "Score");
  new StaticTextWidget(this, font, xposSpecial, ypos + 1, "Round");
  new StaticTextWidget(this, font, xposName - 2, ypos + 1, "Name");
  new StaticTextWidget(this, font, xposDate+16, ypos + 1, "Date   Time");

  ypos += lineHeight + VGAP;

  for (uInt32 p = 0; p < NUM_POSITIONS; ++p)
  {
    myPositions[p] = new StaticTextWidget(this, font, xposRank, ypos + 1,
                                          (p < 9 ? " " : "") + std::to_string(p + 1) + ". ");
    myScores[p] = new StaticTextWidget(this, font, xposScore, ypos + 1, "123456");
    mySpecials[p] = new StaticTextWidget(this, font, xposSpecial + 8, ypos + 1, "123");
    myEditNames[p] = new EditTextWidget(this, font, xposName, ypos - 1, nWidth, lineHeight, "JTZ");
    //myEditNames[p]->setEditable(false);
    myEditNames[p]->setFlags(EditTextWidget::FLAG_INVISIBLE);
    wid.push_back(myEditNames[p]);
    myNames[p] = new StaticTextWidget(this, font, xposName + 2, ypos + 1, "JTZ");

    //new StaticTextWidget(this, font, xposDate, ypos + 1, "12.02.20 17:15");
    //new StaticTextWidget(this, font, xposDate, ypos + 1, "02/12/20 12:30am");
    myDates[p] = new StaticTextWidget(this, font, xposDate, ypos + 1, "12-02-20 17:15");

    ypos += lineHeight + VGAP;
  }
  ypos += VGAP * 2;

  myMD5 = new StaticTextWidget(this, ifont, xpos, ypos + 1, "MD5: 9ad36e699ef6f45d9eb6c4cf90475c9f");

  wid.clear();
  addOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HighScoresDialog::~HighScoresDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::loadConfig()
{
  // Enable blending (only once is necessary)
  if(!surface().attributes().blending)
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
  myVariation->addItems(items);
  myVariation->setSelected(instance().highScores().variation());

  // mock data
  const string SCORES[NUM_POSITIONS] = {"999999", "250000", "100000", " 50000", " 20000",
                                        "  5000", "  2000", "   700", "   200", "     -"};
  const string SPECIALS[NUM_POSITIONS] = {"200", "150", " 90", " 70", " 45",
                                          " 30", " 25", " 10", "  7", "  -"};
  const string NAMES[NUM_POSITIONS] = {"RAM", "CDW", "AD ", "JTZ", "DOA",
                                       "ROM", "VCS", "N.S", "JWC", "  -"};
  const string DATES[NUM_POSITIONS] = {"19-12-24 21:00", "19-07-18 00:00", "20-01-01 12:00",
                                       "20-02-12 21:50", "20-02-11 14:16", "20-02-11 13:11",
                                       "20-02-10 19:45", "10-02-10 20:04", "05-02-09 22:32",
                                       "       -     -"};

  for (Int32 p = 0; p < NUM_POSITIONS; ++p)
  {
    myScores[p]->setLabel(SCORES[p]);
    mySpecials[p]->setLabel(SPECIALS[p]);
    myNames[p]->setLabel(NAMES[p]);
    myEditNames[p]->setText(NAMES[p]);
    myDates[p]->setLabel(DATES[p]);
  }

  //myEditNames[4]->setEditable(true);

  //myNames[3]->setHeight(1);
  //myNames[3]->setWidth(0);
  myEditNames[4]->clearFlags(EditTextWidget::FLAG_INVISIBLE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch (cmd)
  {
    case kOKCmd:
    case kCloseCmd:
      instance().eventHandler().leaveMenuMode();
      instance().eventHandler().handleEvent(Event::ExitMode);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
