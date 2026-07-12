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

#include "Dialog.hxx"
#include "OSystem.hxx"
#include "Version.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "WhatsNewDialog.hxx"
#include "MediaFactory.hxx"
#include "Layout.hxx"

#include "AboutDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AboutDialog::AboutDialog(OSystem& osystem, DialogContainer& parent,
                         const GUI::Font& font)
  : Dialog(osystem, parent, font, "About Stella")
{
  WidgetArray wid;

  // Previous, Next and Close buttons
  myPrevButton =
    new ButtonWidget(this, font, 0, 0, "Previous", GuiObject::kPrevCmd);
  myPrevButton->clearFlags(Widget::FLAG_ENABLED);
  wid.push_back(myPrevButton);

  myNextButton =
    new ButtonWidget(this, font, 0, 0, "Next", GuiObject::kNextCmd);
  wid.push_back(myNextButton);

  auto* b = new ButtonWidget(this, font, 0, 0, "Close", GuiObject::kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);

  myTitle = new StaticTextWidget(this, font, 0, 0, "", TextAlign::Center);
  myTitle->setTextColor(kTextColorEm);

  myWhatsNewButton =
    new ButtonWidget(this, font, 0, 0, "What's New" + ELLIPSIS, kWhatsNew);
  wid.push_back(myWhatsNewButton);

  for(int i = 0; i < myLinesPerPage; i++)
  {
    auto* s = new StaticTextWidget(this, font, 0, 0, "", TextAlign::Left, kNone);
    s->setID(i);
    myDesc.push_back(s);
    myDescStr.emplace_back("");
  }

  addToFocusList(wid);

  setHelpURL("https://stella-emu.github.io/index.html");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AboutDialog::~AboutDialog() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AboutDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::stretchedItem;
  using GUI::widgetItem;
  using Dir = BoxLayout::Dir;

  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // Previous and Next share one width, the wider of the two
  GUI::alignButtons({myPrevButton, myNextButton});

  // Title row: a centered title with the "What's New" button at the right; the
  // leading spacer (= that button's own width) keeps the title centered across
  // the dialog.  The title is centered in the (taller) button row, so it needs
  // no offset of its own
  const int whatsNewWidth = myWhatsNewButton->getWidth();
  auto titleRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  titleRow->addSpace(whatsNewWidth);
  titleRow->addStretch(stretchedItem(myTitle));
  titleRow->addAuto(anchoredItem(myWhatsNewButton));

  // Description lines, indented an extra border on each side
  auto descCol = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, 0);
  for(auto* s: myDesc)
    descCol->addAuto(stretchedItem(s));

  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addAuto(std::move(titleRow));
  root->addSpace(VGAP * 2);
  root->addAuto(std::move(descCol));

  // The pages are written to 55 characters, so that is the dialog's width; its
  // height is however much room they ask for, plus the button row below them
  _w = 55 * fontWidth + HBORDER * 2;
  _h = _th + static_cast<int>(root->naturalSize().h) + buttonHeight + VBORDER;

  root->doLayout(0, _th, _w, _h - _th);

  // Bottom row: Previous / Next on the left, Close (the cancel widget) at right
  auto navButtons = std::make_unique<BoxLayout>(Dir::Horizontal, fontWidth);
  navButtons->addAuto(anchoredItem(myPrevButton));
  navButtons->addAuto(anchoredItem(myNextButton));
  const Common::Size navSize = navButtons->naturalSize();
  navButtons->doLayout(HBORDER, _h - buttonHeight - VBORDER,
                       static_cast<int>(navSize.w), static_cast<int>(navSize.h));
  layoutButtonGroup();
}

// The following commands can be put at the start of a line (all subject to change):
//   \C, \L, \R  -- set center/left/right alignment
//   \c0 - \c5   -- set a custom color:
//                  0 normal text (green)
//                  1 highlighted text (light green)
//                  2 light border (light gray)
//                  3 dark border (dark gray)
//                  4 background (black)
//                  5 emphasized text (red)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AboutDialog::updateStrings(int page, int lines, string& title)
{
  int i = 0;
  const auto ADD_ATEXT = [&](string_view d) { myDescStr[i] = d; i++; };
  const auto ADD_ALINE = [&]() { ADD_ATEXT(""); };

  switch(page)
  {
    case 1:
      title = STELLA_FULL_TITLE;
      ADD_ATEXT("\\CA multi-platform Atari 2600 VCS emulator");
      ADD_ATEXT(string("\\C\\c2Features: ") + instance().features());
      ADD_ATEXT(string("\\C\\c2") + instance().buildInfo());
      ADD_ALINE();
      ADD_ATEXT("\\CCopyright (c) 1995-2026 The Stella Team");
      ADD_ATEXT("\\C(https://stella-emu.github.io)");
      ADD_ALINE();
      ADD_ATEXT("\\CStella is now DonationWare!");
      ADD_ATEXT("\\C(https://stella-emu.github.io/donations.html)");
      ADD_ALINE();
      ADD_ATEXT("\\CStella is free software released under the GNU GPL.");
      ADD_ATEXT("\\CSee manual for further details.");
      break;

    case 2:
      title = "The Stella Team";
      ADD_ATEXT("\\L\\c0""Stephen Anthony");
      ADD_ATEXT("\\L\\c2""  Lead developer, current maintainer for the");
      ADD_ATEXT("\\L\\c2""  Linux, macOS and Windows ports ");
      ADD_ATEXT("\\L\\c0""Christian Speckner");
      ADD_ATEXT("\\L\\c2""  Emulation core development, TIA core");
      ADD_ATEXT("\\L\\c0""Eckhard Stolberg");
      ADD_ATEXT("\\L\\c2""  Emulation core development");
      ADD_ATEXT("\\L\\c0""Thomas Jentzsch");
      ADD_ATEXT("\\L\\c2""  Emulation core development, jack-of-all-trades");
      ADD_ATEXT("\\L\\c0""Brian Watson");
      ADD_ATEXT("\\L\\c2""  Emulation core enhancement, debugger support");
      ADD_ATEXT("\\L\\c0""Bradford W. Mott");
      ADD_ATEXT("\\L\\c2""  Original author of Stella");
      break;

    case 3:
      title = "Contributors";
      ADD_ATEXT("\\L\\c0""See https://stella-emu.github.io/credits.html for");
      ADD_ATEXT("\\L\\c0""people that have contributed to Stella.");
      ADD_ALINE();
      ADD_ATEXT("\\L\\c0""Thanks to the ScummVM project for the GUI code.");
      ADD_ALINE();
      ADD_ATEXT("\\L\\c0""Thanks to Ian Bogost and the Georgia Tech Atari Team");
      ADD_ATEXT("\\L\\c0""for the CRT Simulation effects.");
      break;

    case 4:
      title = "Cast of thousands";
      ADD_ATEXT("\\L\\c0""Special thanks to <AtariAge> for introducing the");
      ADD_ATEXT("\\L\\c0""Atari 2600 to a whole new generation.");
      ADD_ALINE();
      ADD_ATEXT("\\L\\c0""Finally, a huge thanks to the original Atari 2600");
      ADD_ATEXT("\\L\\c0""VCS team for giving us the magic, and to the");
      ADD_ATEXT("\\L\\c0""homebrew developers for keeping the magic alive.");
      break;

    default:
      return;
  }

  while(i < lines)
    ADD_ALINE();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AboutDialog::displayInfo()
{
  string titleStr;
  updateStrings(myPage, myLinesPerPage, titleStr);

  myTitle->setLabel(titleStr);
  for(int i = 0; i < myLinesPerPage; i++)
  {
    string_view sv = myDescStr[i];
    TextAlign align = TextAlign::Center;
    ColorId color  = kTextColor;

    while(sv.size() >= 2 && sv[0] == '\\')
    {
      switch(sv[1])
      {
        case 'C':
          align = TextAlign::Center;
          break;

        case 'L':
          align = TextAlign::Left;
          break;

        case 'R':
          align = TextAlign::Right;
          break;

        case 'c':
          if(sv.size() >= 3)
          {
            switch(sv[2])
            {
              case '0':  color = kTextColor;    break;
              case '1':  color = kTextColorHi;  break;
              case '2':  color = kColor;        break;
              case '3':  color = kShadowColor;  break;
              case '4':  color = kBGColor;      break;
              case '5':  color = kTextColorEm;  break;
              default:                          break;
            }
          }
          sv.remove_prefix(1);
          break;

        default:
          break;
      }
      sv.remove_prefix(2);
    }

    myDesc[i]->setAlign(align);
    myDesc[i]->setTextColor(color);
    myDesc[i]->setLabel(sv);
    // add some labeled links
    if(BSPF::containsIgnoreCase(sv, "see manual"))
      myDesc[i]->setUrl("https://stella-emu.github.io/docs/index.html#License", "manual");
    else if(BSPF::containsIgnoreCase(sv, "Stephen Anthony"))
      myDesc[i]->setUrl("http://minbar.org", "Stephen Anthony");
    else if(BSPF::containsIgnoreCase(sv, "Bradford W. Mott"))
      myDesc[i]->setUrl("www.intellimedia.ncsu.edu/people/bwmott", "Bradford W. Mott");
    else if(BSPF::containsIgnoreCase(sv, "ScummVM project"))
      myDesc[i]->setUrl("www.scummvm.org", "ScummVM");
    else if(BSPF::containsIgnoreCase(sv, "Ian Bogost"))
      myDesc[i]->setUrl("http://bogost.com", "Ian Bogost");
    else if(BSPF::containsIgnoreCase(sv, "CRT Simulation"))
      myDesc[i]->setUrl("http://blargg.8bitalley.com/libs/ntsc.html", "CRT Simulation effects");
    else if(BSPF::containsIgnoreCase(sv, "<AtariAge>"))
      myDesc[i]->setUrl("www.atariage.com", "AtariAge", "<AtariAge>");
    else
      myDesc[i]->setUrl();
  }

  // Redraw entire dialog
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AboutDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kNextCmd:
      myPage++;
      if(myPage >= myNumPages)
        myNextButton->clearFlags(Widget::FLAG_ENABLED);
      if(myPage >= 2)
        myPrevButton->setFlags(Widget::FLAG_ENABLED);

      displayInfo();
      break;

    case GuiObject::kPrevCmd:
      myPage--;
      if(myPage <= myNumPages)
        myNextButton->setFlags(Widget::FLAG_ENABLED);
      if(myPage <= 1)
        myPrevButton->clearFlags(Widget::FLAG_ENABLED);

      displayInfo();
      break;

    case kWhatsNew:
      if(myWhatsNewDialog == nullptr)
        myWhatsNewDialog = std::make_unique<WhatsNewDialog>(instance(), parent());
      myWhatsNewDialog->open();
      break;

    case StaticTextWidget::kOpenUrlCmd:
    {
      const string& url = myDesc[id]->getUrl();

      if(!url.empty())
        MediaFactory::openURL(url);
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}


