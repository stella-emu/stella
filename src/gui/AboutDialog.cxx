#include "Dialog.hxx"
#include "OSystem.hxx"
#include "Version.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "WhatsNewDialog.hxx"
#include "MediaFactory.hxx"

#include "AboutDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AboutDialog::AboutDialog(OSystem& osystem, DialogContainer& parent,
                         const GUI::Font& font)
  : Dialog(osystem, parent, font, "About Akella")
{
  const int lineHeight   = Dialog::lineHeight(),
            fontHeight   = Dialog::fontHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            buttonWidth  = Dialog::buttonWidth("Previous"),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  WidgetArray wid;

  // Set real dimensions
  _w = 55 * fontWidth + HBORDER * 2;
  _h = _th + 14 * lineHeight + VGAP * 3 + buttonHeight + VBORDER * 2;

  // Add Previous, Next and Close buttons
  int xpos = HBORDER, ypos = _h - buttonHeight - VBORDER;
  myPrevButton =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Previous", GuiObject::kPrevCmd);
  myPrevButton->clearFlags(Widget::FLAG_ENABLED);
  wid.push_back(myPrevButton);

  xpos += buttonWidth + fontWidth;
  myNextButton =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Next", GuiObject::kNextCmd);
  wid.push_back(myNextButton);

  xpos = _w - buttonWidth - HBORDER;
  auto* b = new ButtonWidget(this, font, xpos, ypos,
      buttonWidth, buttonHeight, "Close", GuiObject::kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);

  xpos = HBORDER;  ypos = _th + VBORDER + (buttonHeight - fontHeight) / 2;
  const int bwidth = font.getStringWidth("What's New" + ELLIPSIS) + fontWidth * 2.5;

  myTitle = new StaticTextWidget(this, font, xpos + bwidth, ypos,
                                 _w - (xpos + bwidth) * 2,
                                 fontHeight, "", TextAlign::Center);
  myTitle->setTextColor(kTextColorEm);

  myWhatsNewButton =
    new ButtonWidget(this, font, _w - HBORDER - bwidth,
                     ypos - (buttonHeight - fontHeight) / 2,
                     bwidth, buttonHeight, "What's New" + ELLIPSIS, kWhatsNew);
  wid.push_back(myWhatsNewButton);

  xpos = HBORDER * 2;  ypos += lineHeight + VGAP * 2;
  for(int i = 0; i < myLinesPerPage; i++)
  {
    auto* s = new StaticTextWidget(this, font, xpos, ypos, _w - xpos * 2,
                                   fontHeight, "", TextAlign::Left, kNone);
    s->setID(i);
    myDesc.push_back(s);
    myDescStr.emplace_back("");
    ypos += fontHeight;
  }

  addToFocusList(wid);

  setHelpURL("https://github.com/sapbotgit/akella");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AboutDialog::~AboutDialog()  // NOLINT (we need an empty d'tor)
{
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
      title = string("Akella ") + STELLA_VERSION;
      ADD_ATEXT("\\CA multi-platform Atari 2600 VCS emulator");
      ADD_ATEXT(string("\\C\\c2Features: ") + instance().features());
      ADD_ATEXT(string("\\C\\c2") + instance().buildInfo());
      ADD_ALINE();
      ADD_ATEXT("\\CCopyright (c) 2024-2024 SAPBOT");
      ADD_ATEXT("\\C(https://sapbotgit.github.io/akella)");
      ADD_ALINE();
      ADD_ATEXT("\\CAkella is free software released under the GNU GPL.");
      ADD_ATEXT("\\CSee manual for further details.");
      break;
    default:
      return;
  }

  while(i < lines)  // NOLINT : i changes in lambda above
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
    const char* str = myDescStr[i].c_str();
    TextAlign align = TextAlign::Center;
    ColorId color  = kTextColor;

    while (str[0] == '\\')
    {
      switch (str[1])
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
          switch (str[2])
          {
            case '0':
              color = kTextColor;
              break;
            case '1':
              color = kTextColorHi;
              break;
            case '2':
              color = kColor;
              break;
            case '3':
              color = kShadowColor;
              break;
            case '4':
              color = kBGColor;
              break;
            case '5':
              color = kTextColorEm;
              break;
            default:
              break;
          }
          str++;
          break;

        default:
          break;
      }
      str += 2;
    }

    myDesc[i]->setAlign(align);
    myDesc[i]->setTextColor(color);
    myDesc[i]->setLabel(str);
    // add some labeled links
    if(BSPF::containsIgnoreCase(str, "see manual"))
      myDesc[i]->setUrl("https://stella-emu.github.io/docs/index.html#License", "manual");
    else if(BSPF::containsIgnoreCase(str, "Stephen Anthony"))
      myDesc[i]->setUrl("http://minbar.org", "Stephen Anthony");
    else if(BSPF::containsIgnoreCase(str, "Bradford W. Mott"))
      myDesc[i]->setUrl("www.intellimedia.ncsu.edu/people/bwmott", "Bradford W. Mott");
    else if(BSPF::containsIgnoreCase(str, "ScummVM project"))
      myDesc[i]->setUrl("www.scummvm.org", "ScummVM");
    else if(BSPF::containsIgnoreCase(str, "Ian Bogost"))
      myDesc[i]->setUrl("http://bogost.com", "Ian Bogost");
    else if(BSPF::containsIgnoreCase(str, "CRT Simulation"))
      myDesc[i]->setUrl("http://blargg.8bitalley.com/libs/ntsc.html", "CRT Simulation effects");
    else if(BSPF::containsIgnoreCase(str, "<AtariAge>"))
      myDesc[i]->setUrl("www.atariage.com", "AtariAge", "<AtariAge>");
    else
      // extract URL from label
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
        myWhatsNewDialog = make_unique<WhatsNewDialog>(instance(), parent(),
                                                       640 * 0.95, 480 * 0.95);
      myWhatsNewDialog->open();
      break;

    case StaticTextWidget::kOpenUrlCmd:
    {
      const string& url = myDesc[id]->getUrl();

      if(url != EmptyString && MediaFactory::supportsURL())
        MediaFactory::openURL(url);
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string AboutDialog::getUrl(string_view text)
{
  bool isUrl = false;
  size_t start = 0, len = 0;

  for(size_t i = 0; i < text.size(); ++i)
  {
    const string_view remainder = text.substr(i);
    const char ch = text[i];

    if(!isUrl
       && (BSPF::startsWithIgnoreCase(remainder, "http://")
       || BSPF::startsWithIgnoreCase(remainder, "https://")
       || BSPF::startsWithIgnoreCase(remainder, "www.")))
    {
      isUrl = true;
      start = i;
    }

    // hack, change mode without changing string length
    if(isUrl)
    {
      if((ch == ' ' || ch == ')' || ch == '>'))
        isUrl = false;
      else
        len++;
    }
  }
  if(len)
    return string{text.substr(start, len)};
  else
    return EmptyString;
}

