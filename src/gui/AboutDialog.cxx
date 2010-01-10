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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "Dialog.hxx"
#include "OSystem.hxx"
#include "Version.hxx"
#include "Widget.hxx"

#include "AboutDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AboutDialog::AboutDialog(OSystem* osystem, DialogContainer* parent,
                         const GUI::Font& font)
  : Dialog(osystem, parent, 0, 0, 0, 0),
    myPage(1),
    myNumPages(5)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  WidgetArray wid;

  // Set real dimensions
  _w = 52 * fontWidth + 8;
  _h = 11 * lineHeight + 20;

  // Add Previous, Next and Close buttons
  xpos = 10;  ypos = _h - buttonHeight - 10;
  myPrevButton =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Previous", kPrevCmd);
  myPrevButton->clearFlags(WIDGET_ENABLED);
  wid.push_back(myPrevButton);

  xpos += buttonWidth + 7;
  myNextButton =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Next", kNextCmd);
  wid.push_back(myNextButton);

  xpos = _w - buttonWidth - 10;
  ButtonWidget* b =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Close", kCloseCmd);
  wid.push_back(b);
  addOKWidget(b);  addCancelWidget(b);

  xpos = 5;  ypos = 5;
  myTitle = new StaticTextWidget(this, font, xpos, ypos, _w - 10, fontHeight,
                                 "", kTextAlignCenter);
  myTitle->setTextColor(kTextColorEm);

  xpos = 10;  ypos += lineHeight + 4;
  for(int i = 0; i < kLINES_PER_PAGE; i++)
  {
    myDesc[i] = new StaticTextWidget(this, font, xpos, ypos, _w - 20,
                                     fontHeight, "", kTextAlignLeft);
    ypos += fontHeight;
  }

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AboutDialog::~AboutDialog()
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
#define ADD_ATEXT(d) do { myDescStr[i] = d; i++; } while(0)
#define ADD_ALINE ADD_ATEXT("")

  int i = 0;
  switch(page)
  {
    case 1:
      title = string("Stella ") + STELLA_VERSION;
      ADD_ATEXT("\\CA multi-platform Atari 2600 VCS emulator");
      ADD_ATEXT(string("\\C\\c2") + instance().features());
      ADD_ATEXT(string("\\C\\c2") + instance().buildInfo());
      ADD_ALINE;
      ADD_ATEXT("\\CCopyright (C) 1995-2010 The Stella Team");
      ADD_ATEXT("\\Chttp://stella.sourceforge.net");
      ADD_ALINE;
      ADD_ATEXT("Stella is free software released under the GNU GPL");
      ADD_ATEXT("See manual for further details");
      break;

    case 2:
      title = "The Stella Team";
      ADD_ATEXT("\\L\\c0""  Bradford W. Mott");
      ADD_ATEXT("\\L\\c2""    Original author, lead developer");
      ADD_ATEXT("\\L\\c0""  Stephen Anthony");
      ADD_ATEXT("\\L\\c2""    Lead developer, Linux/OSX/Win32 maintainer");
      ADD_ATEXT("\\L\\c0""  Mark Grebe");
      ADD_ATEXT("\\L\\c2""    Original author/maintainer for OSX port");
      ADD_ATEXT("\\L\\c0""  Brian Watson");
      ADD_ATEXT("\\L\\c2""    Emulation core enhancement, debugger support");
      break;

    case 3:
      title = "The Stella Team";
      ADD_ATEXT("\\L\\c0""  Eckhard Stolberg");
      ADD_ATEXT("\\L\\c2""    Emulation core development");
      ADD_ATEXT("\\L\\c0""  Kostas Nakos");
      ADD_ATEXT("\\L\\c2""    Author/maintainer for WinCE port");
      break;

    case 4:
      title = "Contributors";
      ADD_ATEXT("\\L\\c0""  See Stella manual for contribution details");
      ADD_ATEXT("\\L\\c0""  and for many other people not listed here");
      ADD_ALINE;
      ADD_ATEXT("\\L\\c0""  Thanks to the ScummVM project for the GUI code");
      ADD_ALINE;
      ADD_ATEXT("\\L\\c0""  Thanks to Ian Bogost and the Georgia Tech");
      ADD_ATEXT("\\L\\c0""  Atari Team for the CRT Simulation effects");
      break;

    case 5:
      title = "Cast of thousands";
      ADD_ATEXT("\\L\\c0""Special thanks to AtariAge for introducing the");
      ADD_ATEXT("\\L\\c0""Atari 2600 to a whole new generation");
      ADD_ATEXT("\\L\\c2""  http://www.atariage.com");
      ADD_ALINE;
      ADD_ATEXT("\\L\\c0""Finally, a huge thanks to the original Atari 2600");
      ADD_ATEXT("\\L\\c0""VCS team for giving us the magic, and to the");
      ADD_ATEXT("\\L\\c0""homebrew developers for keeping the magic alive");
      break;
  }

  while(i < lines)
    ADD_ALINE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AboutDialog::displayInfo()
{
  string titleStr;
  updateStrings(myPage, kLINES_PER_PAGE, titleStr);

  myTitle->setLabel(titleStr);
  for(int i = 0; i < kLINES_PER_PAGE; i++)
  {
    const char* str = myDescStr[i].c_str();
    TextAlignment align = kTextAlignCenter;
    uInt32 color  = kTextColor;

    while (str[0] == '\\')
    {
      switch (str[1])
      {
        case 'C':
          align = kTextAlignCenter;
          break;

        case 'L':
          align = kTextAlignLeft;
          break;

        case 'R':
          align = kTextAlignRight;
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
  }

  // Redraw entire dialog
  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AboutDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kNextCmd:
      myPage++;
      if(myPage >= myNumPages)
        myNextButton->clearFlags(WIDGET_ENABLED);
      if(myPage >= 2)
        myPrevButton->setFlags(WIDGET_ENABLED);

      displayInfo();
      break;

    case kPrevCmd:
      myPage--;
      if(myPage <= myNumPages)
        myNextButton->setFlags(WIDGET_ENABLED);
      if(myPage <= 1)
        myPrevButton->clearFlags(WIDGET_ENABLED);

      displayInfo();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
