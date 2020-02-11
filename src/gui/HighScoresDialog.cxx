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


#include "HighScoresDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HighScoresDialog::HighScoresDialog(OSystem& osystem, DialogContainer& parent,
                             const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "High Scores")
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight = font.getLineHeight(),
    fontWidth = font.getMaxCharWidth(),
    fontHeight = font.getFontHeight(),
    buttonHeight = font.getLineHeight() + 4;
    //infoLineHeight = ifont.getLineHeight();
  const int VBORDER = 8;
  const int HBORDER = 10;
  const int VGAP = 4;

  int xpos, ypos, lwidth, fwidth, pwidth, tabID;
  WidgetArray wid;
  VariantList items;

  _w = 400; // max_w - 20;
  _h = 400; // max_h - 20;

  ypos = VBORDER + _th; xpos = HBORDER;


  //items.clear();

  StaticTextWidget* s = new StaticTextWidget(this, font, xpos, ypos + 1, "Variation ");
  myVariation = new PopUpWidget(this, font, s->getRight(), ypos,
                                font.getStringWidth("256"), lineHeight, items, "", 0, 0);

  ypos += lineHeight + VGAP * 2;

  int xposRank = HBORDER;
  int xposScore = xposRank + font.getStringWidth("Rank") + 16;
  int xposName = xposScore + font.getStringWidth("123456") + 24;
  int xposExtra = xposName + font.getStringWidth("Date") + 16;
  int xposDate = xposExtra + font.getStringWidth("Round") + 16;
  int nWidth = font.getStringWidth("ABC") + 4;

  new StaticTextWidget(this, font, xposRank, ypos + 1, "Rank");
  new StaticTextWidget(this, font, xposScore, ypos + 1, "Score");
  new StaticTextWidget(this, font, xposName - 2, ypos + 1, "Name");
  new StaticTextWidget(this, font, xposExtra, ypos + 1, "Round");
  new StaticTextWidget(this, font, xposDate+16, ypos + 1, "Date   Time");

  ypos += lineHeight + VGAP;

  for (uInt32 p = 0; p < NUM_POSITIONS; ++p)
  {
    myPositions[p] = new StaticTextWidget(this, font, xposRank, ypos + 1,
                                          (p < 9 ? " " : "") + std::to_string(p + 1) + ". ");

    myScores[p] = new StaticTextWidget(this, font, xposScore, ypos + 1, "123456");
    myNames[p] = new EditTextWidget(this, font, xposName, ypos + 1, nWidth, lineHeight, "JTZ");
    myNames[p]->setEditable(false);
    wid.push_back(myNames[p]);

    new StaticTextWidget(this, font, xposExtra, ypos + 1, " 123");

    //new StaticTextWidget(this, font, xposDate, ypos + 1, "12.02.20 17:15");
    //new StaticTextWidget(this, font, xposDate, ypos + 1, "02/12/20 12:30am");
    new StaticTextWidget(this, font, xposDate, ypos + 1, "12-02-20 17:15");

    ypos += lineHeight + VGAP;
  }
  ypos += VGAP;
  myMD5 = new StaticTextWidget(this, ifont, xpos, ypos + 1, "MD5: 9ad36e699ef6f45d9eb6c4cf90475c9f");
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

  for (Int32 i = 1; i <= 10; ++i)
    VarList::push_back(items, i, i);
  myVariation->addItems(items);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch (cmd)
  {
    case kOKCmd:
    case kCloseCmd:
      instance().eventHandler().handleEvent(Event::ExitMode);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
