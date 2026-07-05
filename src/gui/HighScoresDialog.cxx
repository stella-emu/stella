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
#include "Console.hxx"
#include "Launcher.hxx"
#include "EventHandler.hxx"
#include "Font.hxx"
#include "PropsSet.hxx"
#include "FBSurface.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "MessageBox.hxx"
#include "Layout.hxx"

#include "HighScoresDialog.hxx"

static constexpr int BUTTON_GFX_H = 10;
static constexpr int BUTTON_GFX_H_LARGE = 16;

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

static constexpr std::array<uInt32, BUTTON_GFX_H_LARGE> PREV_GFX_LARGE = {
  0b0000000110000000,
  0b0000000110000000,
  0b0000001111000000,
  0b0000001111000000,
  0b0000011111100000,
  0b0000011111100000,
  0b0000111001110000,
  0b0000111001110000,
  0b0001110000111000,
  0b0001110000111000,
  0b0011100000011100,
  0b0011100000011100,
  0b0111000000001110,
  0b0111000000001110,
  0b1110000000000111,
  0b1110000000000111,
};

static constexpr std::array<uInt32, BUTTON_GFX_H_LARGE> NEXT_GFX_LARGE = {
  0b1110000000000111,
  0b1110000000000111,
  0b0111000000001110,
  0b0111000000001110,
  0b0011100000011100,
  0b0011100000011100,
  0b0001110000111000,
  0b0001110000111000,
  0b0000111001110000,
  0b0000111001110000,
  0b0000011111100000,
  0b0000011111100000,
  0b0000001111000000,
  0b0000001111000000,
  0b0000000110000000,
  0b0000000110000000,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HighScoresDialog::HighScoresDialog(OSystem& osystem, DialogContainer& parent,
                                   int max_w, int max_h,
                                   AppMode mode)
  : Dialog(osystem, parent, osystem.frameBuffer().font(), "High Scores"),
    _max_w{max_w},
    _max_h{max_h},
    myMode{mode}
{
  myScores.variation = HSM::DEFAULT_VARIATION;

  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight = Dialog::lineHeight(),
            fontWidth  = Dialog::fontWidth();
  const int nWidth = _font.getStringWidth("ABC") + fontWidth * 0.75;
  const int bWidth = fontWidth * 5;
  const bool smallFont = _font.getFontHeight() < 24;
  const int buttonSize = smallFont ? BUTTON_GFX_H : BUTTON_GFX_H_LARGE;
  const int numRanks = static_cast<int>(NUM_RANKS);
  const VariantList items;
  WidgetArray wid;

  // Widgets are only created here (at placeholder geometry); layout() assigns
  // all geometry from the current font, so the dialog reflows on font change.
  myGameNameWidget = new StaticTextWidget(this, _font, 0, 0, "");

  myVariationLabel = new StaticTextWidget(this, _font, 0, 0, "Variation ");
  myVariationPopup = new PopUpWidget(this, _font, 0, 0,
      _font.getStringWidth("256"), lineHeight, items, "", 0, kVariationChanged);
  wid.push_back(myVariationPopup);
  myPrevVarButton = new ButtonWidget(this, _font, 0, 0, bWidth, myVariationPopup->getHeight(),
      smallFont ? PREV_GFX.data() : PREV_GFX_LARGE.data(),
      buttonSize, buttonSize, kPrevVariation);
  wid.push_back(myPrevVarButton);
  myNextVarButton = new ButtonWidget(this, _font, 0, 0, bWidth, myVariationPopup->getHeight(),
      smallFont ? NEXT_GFX.data() : NEXT_GFX_LARGE.data(),
      buttonSize, buttonSize, kNextVariation);
  wid.push_back(myNextVarButton);

  // Score-table column headers
  myRankLabel          = new StaticTextWidget(this, _font, 0, 0, "Rank");
  myScoreLabel         = new StaticTextWidget(this, _font, 0, 0, "Score");
  mySpecialLabelWidget = new StaticTextWidget(this, _font, 0, 0, "Round");
  myNameLabel          = new StaticTextWidget(this, _font, 0, 0, "Name");
  myDateLabel          = new StaticTextWidget(this, _font, 0, 0, "Date   Time");

  // Score-table data rows
  for(int r = 0; r < numRanks; ++r)
  {
    myRankWidgets[r] = new StaticTextWidget(this, _font, 0, 0,
                          (r < 9 ? " " : "") + std::to_string(r + 1));
    myScoreWidgets[r] = new StaticTextWidget(this, _font, 0, 0, "12345678");
    mySpecialWidgets[r] = new StaticTextWidget(this, _font, 0, 0, "123");
    myNameWidgets[r] = new StaticTextWidget(this, _font, 0, 0, "   ");
    myEditNameWidgets[r] = new EditTextWidget(this, _font, 0, 0, nWidth, lineHeight);
    myEditNameWidgets[r]->setFlags(EditTextWidget::FLAG_INVISIBLE);
    myEditNameWidgets[r]->setEnabled(false);
    myDateWidgets[r] = new StaticTextWidget(this, _font, 0, 0, "YY-MM-DD HH:MM");
    myDeleteButtons[r] = new ButtonWidget(this, _font, 0, 0, fontWidth * 2,
                                          Dialog::fontHeight(), "X", kDeleteSingle);
    myDeleteButtons[r]->setID(r);
    myDeleteButtons[r]->setToolTip("Click to delete this high score.");

    wid.push_back(myEditNameWidgets[r]);
    wid.push_back(myDeleteButtons[r]);
  }

  myNotesWidget = new StaticTextWidget(this, ifont, 0, 0, "Note: ");
  // Note: Only display the first 16 md5 chars + "..."
  myMD5Widget = new StaticTextWidget(this, ifont, 0, 0, "MD5: 1234567890123456.");
  myCheckSumWidget = new StaticTextWidget(this, ifont, 0, 0, "Props: 1234567890123456.");

  addDefaultsOKCancelBGroup(wid, _font, "Save", "Cancel", " Reset ");
  _defaultWidget->setToolTip("Click to reset all high scores of this variation.");
  addToFocusList(wid);

  _focusedWidget = _okWidget; // start with focus on 'Save' button

  setHelpAnchor("Highscores");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HighScoresDialog::~HighScoresDialog() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::layout()
{
  using GUI::GridLayout;
  using GUI::anchoredItem;

  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int infoLineHeight = ifont.getLineHeight();
  const int lineHeight   = Dialog::lineHeight(),
            fontHeight   = Dialog::fontHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            BUTTON_GAP   = Dialog::buttonGap(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  const int numRanks = static_cast<int>(NUM_RANKS);

  // Score-table columns.  Each column width is the width of a representative
  // (padded) string, so the columns line up; the table also drives the dialog
  // width.  Cells are anchored (see below) so wide data can't be clipped.
  const int xposRank = HBORDER;
  const int wRank    = _font.getStringWidth("Rank");
  const int wScore   = _font.getStringWidth("   Score  ");
  const int wSpecial = _font.getStringWidth("Round  ");
  const int wName    = _font.getStringWidth("Name  ");
  const int wDate    = _font.getStringWidth("YY-MM-DD HH:MM  ");
  const int wDelete  = fontWidth * 2;
  const int tableWidth = wRank + wScore + wSpecial + wName + wDate + wDelete;
  const int xposDelete = xposRank + wRank + wScore + wSpecial + wName + wDate;

  // Dialog width = widest of the table, the info text, and the button row
  const int buttonWidth = std::max({Dialog::buttonWidth(" Reset "),
                                    Dialog::buttonWidth("Save"),
                                    Dialog::buttonWidth("Cancel"),
                                    Dialog::buttonWidth("Defaults")});
  _w = std::max({HBORDER * 2 + tableWidth,
                 HBORDER * 2 + ifont.getMaxCharWidth() * (5 + 17 + 2 + 7 + 17),
                 HBORDER * 2 + buttonWidth * 2 + BUTTON_GAP});

  // Game name (top)
  myGameNameWidget->setArea(HBORDER, VBORDER + _th + 1, _w - HBORDER * 2, lineHeight);

  int ypos = VBORDER + _th + lineHeight + VGAP * 2;

  // Variation row: label + popup, with the prev/next buttons at the right
  myVariationLabel->setPos(HBORDER, ypos + 1);
  myVariationPopup->setPos(myVariationLabel->getRight(), ypos);
  const int bWidth = fontWidth * 5;
  myPrevVarButton->setPos(xposDelete + fontWidth * 2 - bWidth * 2 - BUTTON_GAP, ypos - 1);
  myNextVarButton->setPos(xposDelete + fontHeight - bWidth, ypos - 1);

  ypos += lineHeight + VGAP * 4;

  // Score table (header row + NUM_RANKS data rows) laid out as a grid so the
  // columns line up.  Cells are anchored (positioned, not stretched) so each
  // label keeps its natural width, matching the old loose packing.
  const int tableBase = ypos;
  auto grid = std::make_unique<GridLayout>(6, 1 + numRanks, 0, VGAP, 0, 0);
  grid->columnFixed(0, wRank).columnFixed(1, wScore).columnFixed(2, wSpecial)
      .columnFixed(3, wName).columnFixed(4, wDate).columnFixed(5, wDelete);
  for(int gr = 0; gr < 1 + numRanks; ++gr)
    grid->rowFixed(gr, lineHeight);

  grid->place(0, 0, anchoredItem(myRankLabel));
  grid->place(1, 0, anchoredItem(myScoreLabel));
  grid->place(2, 0, anchoredItem(mySpecialLabelWidget));
  grid->place(3, 0, anchoredItem(myNameLabel));
  grid->place(4, 0, anchoredItem(myDateLabel));
  for(int r = 0; r < numRanks; ++r)
  {
    const int gr = r + 1;
    grid->place(0, gr, anchoredItem(myRankWidgets[r]));
    grid->place(1, gr, anchoredItem(myScoreWidgets[r]));
    grid->place(2, gr, anchoredItem(mySpecialWidgets[r]));
    grid->place(3, gr, anchoredItem(myNameWidgets[r]));
    grid->place(4, gr, anchoredItem(myDateWidgets[r]));
    grid->place(5, gr, anchoredItem(myDeleteButtons[r]));
  }
  // The +1 preserves the previous per-row vertical nudge
  grid->doLayout(xposRank, tableBase + 1, tableWidth,
                 (1 + numRanks) * lineHeight + numRanks * VGAP);

  // Editable-name overlays sit on their name cells (not part of the grid flow)
  for(int r = 0; r < numRanks; ++r)
    myEditNameWidgets[r]->setPos(myNameWidgets[r]->getLeft(),
                                 myNameWidgets[r]->getTop() - 2);

  ypos = tableBase + (1 + numRanks) * (lineHeight + VGAP) + VGAP;

  // Notes, then MD5 (left) + properties checksum (right)
  myNotesWidget->setArea(HBORDER, ypos + 1, _w - HBORDER * 2, infoLineHeight);
  ypos += infoLineHeight + VGAP;
  myMD5Widget->setPos(HBORDER, ypos + 1);
  myCheckSumWidget->setPos(
      _w - HBORDER - ifont.getStringWidth("Props: 1234567890123456."), ypos + 1);

  _h = myMD5Widget->getBottom() + VBORDER + buttonHeight + VBORDER;

  // Standard button group (Reset / Save / Cancel) along the bottom
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::loadConfig()
{
  // Enable blending (only once is necessary)
  if (myMode == AppMode::emulator/* && surface().blendLevel() ... */) {
    surface().setBlendLevel(90);
    surface().enableBlend(true);
  }

  VariantList items;

  // fill drown down with all variation numbers of current game
  items.clear();
  for (Int32 i = 1; i <= instance().highScores().numVariations(); ++i)
    VarList::push_back(items, std::format("{:3}", i), i);
  myVariationPopup->addItems(items);

  Int32 variation = 0;
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
    myVariationPopup->setSelected(myScores.variation);
    myUserDefVar = true;
  }

  myVariationPopup->setEnabled(instance().highScores().numVariations() > 1);

  if(myInitials.empty())
    // load initials from last session
    myInitials = instance().settings().getString("initials");

  string label = "   " + instance().highScores().specialLabel();
  if (label.length() > 5)
    label = label.substr(label.length() - 5);
  mySpecialLabelWidget->setLabel(label);

  if (!instance().highScores().notes().empty())
    myNotesWidget->setLabel(std::format("Note: {}", instance().highScores().notes()));
  else
    myNotesWidget->setLabel("");

  if (instance().hasConsole())
    myScores.md5 = instance().console().properties().get(PropType::Cart_MD5);
  else
    myScores.md5 = instance().launcher().selectedRomMD5();

  myMD5Widget->setLabel(std::format("MD5: {}", myScores.md5));
  myCheckSumWidget->setLabel(std::format("Props: {}",
                                         instance().highScores().md5Props()));

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
    myScores.scores[myHighScoreRank].name = myInitials;
    // remember initials for next session
    instance().settings().setValue("initials", myInitials);
  }
  // save selected variation
  instance().highScores().saveHighScores(myScores);
  if(myScores.variation == instance().highScores().variation() || myUserDefVar)
    myHighScoreSaved = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kOKCmd:
      saveConfig();
      [[fallthrough]];
    case kCloseCmd:
      if(myMode != AppMode::emulator)
        close();
      else
        instance().eventHandler().leaveMenuMode();
      break;

    case kVariationChanged:
      handleVariation();
      break;
    case kPrevVariation:
      myVariationPopup->setSelected(myScores.variation - 1);
      handleVariation();
      break;

    case kNextVariation:
      myVariationPopup->setSelected(myScores.variation + 1);
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
    myScores.variation = myVariationPopup->getSelectedTag().toInt();

    instance().highScores().loadHighScores(myScores);

    myEditRank = -1;

    if (myScores.variation == instance().highScores().variation() || myUserDefVar)
      handlePlayedVariation();

    updateWidgets(init);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::updateWidgets(bool init)
{
  myPrevVarButton->setEnabled(myScores.variation > 1);
  myNextVarButton->setEnabled(myScores.variation < instance().highScores().numVariations());

  for (uInt32 r = 0; r < NUM_RANKS; ++r)
  {
    if(myScores.scores[r].score > 0)
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
    myScoreWidgets[r]->setLabel(instance().highScores().formattedScore(myScores.scores[r].score,
                                HSM::MAX_SCORE_DIGITS));

    mySpecialWidgets[r]->setLabel(
      myScores.scores[r].special > 0
        ? std::format("{:{}}", myScores.scores[r].special, HSM::MAX_SPECIAL_DIGITS)
        : "");

    myNameWidgets[r]->setLabel(myScores.scores[r].name);
    myDateWidgets[r]->setLabel(myScores.scores[r].date);

    if (std::cmp_equal(r, myEditRank))
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
  _defaultWidget->setEnabled(myScores.scores[0].score > 0);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::handlePlayedVariation()
{
  const Int32 newScore = instance().highScores().score();

  if (!myHighScoreSaved && newScore > 0)
  {
    const Int32 newSpecial = instance().highScores().special();
    const bool scoreInvert = instance().highScores().scoreInvert();

    for (myHighScoreRank = 0; std::cmp_less(myHighScoreRank, NUM_RANKS); ++myHighScoreRank)
    {
      const Int32 highScore = myScores.scores[myHighScoreRank].score;

      if ((!scoreInvert && newScore > highScore) ||
          ((scoreInvert && newScore < highScore) ||
          highScore == 0))
        break;
      if (newScore == highScore && newSpecial > myScores.scores[myHighScoreRank].special)
        break;
    }

    if (std::cmp_less(myHighScoreRank, NUM_RANKS))
    {
      myEditRank = myHighScoreRank;
      for(uInt32 r = NUM_RANKS - 1; std::cmp_greater(r, myHighScoreRank); --r)
        myScores.scores[r] = myScores.scores[r - 1];
      myScores.scores[myHighScoreRank].score = newScore;
      myScores.scores[myHighScoreRank].special = newSpecial;
      myScores.scores[myHighScoreRank].date = myNow;
      myDirty |= !myUserDefVar; // only ask when the variation was read by defintion
    }
    else
      myHighScoreRank = -1;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HighScoresDialog::deleteRank(int rank)
{
  for(uInt32 r = rank; r < NUM_RANKS - 1; ++r)
    myScores.scores[r] = std::move(myScores.scores[r + 1]);
  myScores.scores[NUM_RANKS - 1] = {};

  if (myEditRank == rank)
  {
    myHighScoreRank = myEditRank = -1;
  }
  else if (myEditRank > rank)
  {
    --myHighScoreRank;
    --myEditRank;
    // Guard: myEditRank + 1 is now the old position, still valid after decrement
    if (std::cmp_less(myEditRank + 1, static_cast<int>(NUM_RANKS)))
      myEditNameWidgets[myEditRank]->setText(
          myEditNameWidgets[myEditRank + 1]->getText());
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

      msg.emplace_back("Do you want to save the changes");
      msg.emplace_back("for this variation?");
      msg.emplace_back("");
      myConfirmMsg = std::make_unique<GUI::MessageBox>
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
    return string{instance().console().properties().get(PropType::Cart_Name)};
  else
  {
    Properties props;

    instance().propSet().getMD5(myScores.md5, props);
    if(props.get(PropType::Cart_Name).empty())
      return instance().launcher().currentDir().getBaseName();
    else
      return string{props.get(PropType::Cart_Name)};
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string HighScoresDialog::now()
{
  const std::tm t = BSPF::localTime();

  return std::format("{:02}-{:02}-{:02} {:02}:{:02}",
    t.tm_year - 100,
    t.tm_mon + 1,
    t.tm_mday,
    t.tm_hour,
    t.tm_min);
}
