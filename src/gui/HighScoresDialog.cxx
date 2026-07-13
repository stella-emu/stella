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

namespace {
  // The score table's columns
  enum Column: int {
    COL_RANK, COL_SCORE, COL_SPECIAL, COL_NAME, COL_DATE, COL_DELETE, NUM_COLUMNS
  };

  // A name is the player's initials
  constexpr string_view NAME_FIELD = "ABC";
  // The format the dates are shown in, which is what sizes their column
  constexpr string_view DATE_FIELD = "YY-MM-DD HH:MM";

  // An MD5 is far longer than there is room for, so these two lines show only
  // the first 16 characters of one (and clip the rest)
  constexpr string_view MD5_FIELD   = "MD5: 1234567890123456.";
  constexpr string_view PROPS_FIELD = "Props: 1234567890123456.";
}  // namespace

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
  const int fontWidth = Dialog::fontWidth();
  const int bWidth = fontWidth * 5;
  const bool largeFont = _font.isLarge();
  const int buttonSize = largeFont ? BUTTON_GFX_H_LARGE : BUTTON_GFX_H;
  const int numRanks = static_cast<int>(NUM_RANKS);
  const VariantList items;
  WidgetArray wid;

  // Widgets are only created here (at placeholder geometry); layout() assigns
  // all geometry from the current font, so the dialog reflows on font change.
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myGameNameWidget = new StaticTextWidget(this, _font, 0, 0, "");

  myVariationLabel = new StaticTextWidget(this, _font, 0, 0, "Variation");
  // The list is filled per ROM in loadConfig(), so the pop-up cannot size its
  // box to it; the widest variation there can be is what it must show
  myVariationPopup = new PopUpWidget(this, _font, 0, 0,
      fontWidth * HSM::MAX_VARIATION_DIGITS, items, "", 0, kVariationChanged);
  wid.push_back(myVariationPopup);
  myPrevVarButton = new ButtonWidget(this, _font, 0, 0, bWidth, myVariationPopup->getHeight(),
      largeFont ? PREV_GFX_LARGE.data() : PREV_GFX.data(),
      buttonSize, buttonSize, kPrevVariation);
  wid.push_back(myPrevVarButton);
  myNextVarButton = new ButtonWidget(this, _font, 0, 0, bWidth, myVariationPopup->getHeight(),
      largeFont ? NEXT_GFX_LARGE.data() : NEXT_GFX.data(),
      buttonSize, buttonSize, kNextVariation);
  wid.push_back(myNextVarButton);

  // Score-table column headers.  The special value's heading is the game's own
  // word for it ("Round", "Level", ...), so it arrives with the scores
  myRankLabel          = new StaticTextWidget(this, _font, 0, 0, "Rank");
  myScoreLabel         = new StaticTextWidget(this, _font, 0, 0, "Score");
  mySpecialLabelWidget = new StaticTextWidget(this, _font, 0, 0, "");
  myNameLabel          = new StaticTextWidget(this, _font, 0, 0, "Name");
  myDateLabel          = new StaticTextWidget(this, _font, 0, 0, "Date   Time");

  // Score-table data rows.  Every one of these is filled in as the scores load,
  // so each takes its width from the column it sits in and starts out empty; the
  // rank is the exception, being the one thing here that is known up front.  The
  // rank and the special value are narrower than their headings, so they center
  // their text under them
  for(int r = 0; r < numRanks; ++r)
  {
    myRankWidgets[r] = new StaticTextWidget(this, _font, 0, 0,
        (r < 9 ? " " : "") + std::to_string(r + 1), TextAlign::Center);
    myScoreWidgets[r] = new StaticTextWidget(this, _font, 0, 0, "");
    mySpecialWidgets[r] = new StaticTextWidget(this, _font, 0, 0, "",
                                               TextAlign::Center);
    myNameWidgets[r] = new StaticTextWidget(this, _font, 0, 0, "");
    myEditNameWidgets[r] = new EditTextWidget(this, _font, 0, 0,
        EditTextWidget::calcWidth(_font, NAME_FIELD));
    myEditNameWidgets[r]->setFlags(EditTextWidget::FLAG_INVISIBLE);
    myEditNameWidgets[r]->setEnabled(false);
    myDateWidgets[r] = new StaticTextWidget(this, _font, 0, 0, "");
    myDeleteButtons[r] = new ButtonWidget(this, _font, 0, 0, fontWidth * 2,
                                          Dialog::fontHeight(), "X", kDeleteSingle);
    myDeleteButtons[r]->setID(r);
    myDeleteButtons[r]->setToolTip("Click to delete this high score.");

    wid.push_back(myEditNameWidgets[r]);
    wid.push_back(myDeleteButtons[r]);
  }

  myNotesWidget = new StaticTextWidget(this, ifont, 0, 0, "");
  myMD5Widget = new StaticTextWidget(this, ifont, 0, 0, "");
  myCheckSumWidget = new StaticTextWidget(this, ifont, 0, 0, "");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

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
  using GUI::BoxLayout;
  using GUI::GridLayout;
  using GUI::alignedItem;
  using GUI::anchoredItem;
  using GUI::stretchedItem;
  using GUI::HAlign;
  using GUI::VAlign;
  using Dir = BoxLayout::Dir;

  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            BUTTON_GAP   = Dialog::buttonGap(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  const int numRanks = static_cast<int>(NUM_RANKS);
  // Two characters between the table's columns
  const int COL_GAP = fontWidth * 2;

  // The only label of its kind in the dialog, so it is a group on its own --
  // which is where its clearance from the pop-up comes from
  GUI::alignLabels({{myVariationLabel}});

  // Variation: the pop-up beside its label, and the prev/next buttons at the
  // far end of the row (i.e. at the end of the table below)
  auto varRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  varRow->addAuto(anchoredItem(myVariationLabel));
  varRow->addAuto(anchoredItem(myVariationPopup));
  varRow->addStretchSpace();
  varRow->addAuto(anchoredItem(myPrevVarButton));
  varRow->addSpace(BUTTON_GAP);
  varRow->addAuto(anchoredItem(myNextVarButton));

  // The score table: a header row plus a row per rank.  A column is as wide as
  // the widest thing in it -- except where the FIELD it shows is wider than its
  // heading, which no widget in it can say, since the values arrive later
  auto table = std::make_unique<GridLayout>(NUM_COLUMNS, 1 + numRanks,
                                            COL_GAP, VGAP);
  for(int col = 0; col < NUM_COLUMNS; ++col)
    table->columnAuto(col);
  table->columnFixed(COL_SCORE, fontWidth * HSM::MAX_SCORE_DIGITS)
        .columnFixed(COL_SPECIAL, fontWidth * HSM::MAX_SPECIAL_NAME)
        .columnFixed(COL_DATE, _font.getStringWidth(DATE_FIELD));
  for(int row = 0; row < 1 + numRanks; ++row)
    table->rowAuto(row);

  // A score is right-aligned in its field, so its heading sits at the right of
  // the column; the date's heading is shorter than the dates and centers on them
  table->place(COL_RANK, 0, anchoredItem(myRankLabel));
  table->place(COL_SCORE, 0, alignedItem(myScoreLabel, HAlign::Right,
                                         VAlign::Center));
  table->place(COL_SPECIAL, 0, stretchedItem(mySpecialLabelWidget));
  table->place(COL_NAME, 0, anchoredItem(myNameLabel));
  table->place(COL_DATE, 0, alignedItem(myDateLabel, HAlign::Center,
                                        VAlign::Center));

  for(int r = 0; r < numRanks; ++r)
  {
    const int row = r + 1;

    // Every cell here fills its column, so what is loaded into it can neither
    // stretch it nor collapse it.  The name's editor takes the name's place, so
    // it shares its cell -- and the row is as tall as the editor, which frames
    // its text
    table->place(COL_RANK, row, stretchedItem(myRankWidgets[r]));
    table->place(COL_SCORE, row, stretchedItem(myScoreWidgets[r]));
    table->place(COL_SPECIAL, row, stretchedItem(mySpecialWidgets[r]));
    table->place(COL_NAME, row, stretchedItem(myNameWidgets[r]));
    table->place(COL_NAME, row, stretchedItem(myEditNameWidgets[r]));
    table->place(COL_DATE, row, stretchedItem(myDateWidgets[r]));
    table->place(COL_DELETE, row, anchoredItem(myDeleteButtons[r]));
  }

  // How much of an MD5 to show is the dialog's choice, not the widgets': the
  // full one is put in them as the ROM loads, and they clip it
  myMD5Widget->setWidth(ifont.getStringWidth(MD5_FIELD));
  myCheckSumWidget->setWidth(ifont.getStringWidth(PROPS_FIELD));

  auto md5Row = std::make_unique<BoxLayout>(Dir::Horizontal);
  md5Row->addAuto(anchoredItem(myMD5Widget));
  md5Row->addStretchSpace(1, ifont.getMaxCharWidth() * 2);
  md5Row->addAuto(anchoredItem(myCheckSumWidget));

  // The game name and the notes fill the width they are given and say nothing
  // about how wide the dialog should be; the table is what decides that
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addAuto(stretchedItem(myGameNameWidget));
  root->addSpace(VGAP * 2);
  root->addAuto(std::move(varRow));
  root->addSpace(VGAP * 4);
  root->addAuto(std::move(table));
  root->addSpace(VGAP * 2);
  root->addAuto(stretchedItem(myNotesWidget));
  root->addSpace(VGAP);
  root->addAuto(std::move(md5Row));

  const Common::Size natural = root->naturalSize();

  _w = std::max(static_cast<int>(natural.w), Dialog::buttonGroupWidth());
  _h = _th + static_cast<int>(natural.h) + buttonHeight + VBORDER;

  root->doLayout(0, _th, _w, _h - _th);

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
    VarList::push_back(items,
                       std::format("{:{}}", i, HSM::MAX_VARIATION_DIGITS), i);
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

  // Right-align the game's word for the special value in the field it is shown in
  string label = "   " + instance().highScores().specialLabel();
  if (label.length() > HSM::MAX_SPECIAL_NAME)
    label = label.substr(label.length() - HSM::MAX_SPECIAL_NAME);
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
