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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Dialog.hxx"
#include "Font.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "OSystem.hxx"
#include "Widget.hxx"
#include "StateManager.hxx"
#include "RewindManager.hxx"
#include "TimeMachineDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeMachineDialog::TimeMachineDialog(OSystem& osystem, DialogContainer& parent,
                                     int max_w, int max_h)
  : Dialog(osystem, parent)
{
  const int BUTTON_W = 16, BUTTON_H = 14;

  /*static uInt32 PAUSE[BUTTON_H] =
  {
    0b0001111001111000,
    0b0001111001111000,
    0b0001111001111000,
    0b0001111001111000,
    0b0001111001111000,
    0b0001111001111000,
    0b0001111001111000,
    0b0001111001111000,
    0b0001111001111000,
    0b0001111001111000,
    0b0001111001111000,
    0b0001111001111000,
    0b0001111001111000,
    0b0001111001111000
  };*/
  static uInt32 PLAY[BUTTON_H] =
  {
    0b0110000000000000,
    0b0111100000000000,
    0b0111111000000000,
    0b0111111110000000,
    0b0111111111100000,
    0b0111111111111000,
    0b0111111111111110,
    0b0111111111111110,
    0b0111111111111000,
    0b0111111111100000,
    0b0111111110000000,
    0b0111111000000000,
    0b0111100000000000,
    0b0110000000000000
  };
  static uInt32 REWIND_ALL[BUTTON_H] =
  {
    0,
    0b0110000110000110,
    0b0110001110001110,
    0b0110011110011110,
    0b0110111110111110,
    0b0111111111111110,
    0b0111111111111110,
    0b0111111111111110,
    0b0111111111111110,
    0b0110111110111110,
    0b0110011110011110,
    0b0110001110001110,
    0b0110000110000110,
    0
  };
  static uInt32 REWIND_10[BUTTON_H] =
  {
    0,
    0b0000010000100110,
    0b0000110001100110,
    0b0001110011100110,
    0b0011110111100110,
    0b0111111111100110,
    0b1111111111100110,
    0b1111111111100110,
    0b0111111111100110,
    0b0011110111100110,
    0b0001110011100110,
    0b0000110001100110,
    0b0000010000100110,
    0
  };
  static uInt32 REWIND_1[BUTTON_H] =
  {
    0,
    0b0000001100011100,
    0b0000011100011100,
    0b0000111100011100,
    0b0001111100011100,
    0b0011111100011100,
    0b0111111100011100,
    0b0111111100011100,
    0b0011111100011100,
    0b0001111100011100,
    0b0000111100011100,
    0b0000011100011100,
    0b0000001100011100,
    0
  };
  static uInt32 UNWIND_1[BUTTON_H] =
  {
    0,
    0b0011100011000000,
    0b0011100011100000,
    0b0011100011110000,
    0b0011100011111000,
    0b0011100011111100,
    0b0011100011111110,
    0b0011100011111110,
    0b0011100011111100,
    0b0011100011111000,
    0b0011100011110000,
    0b0011100011100000,
    0b0011100011000000,
    0
  };
  static uInt32 UNWIND_10[BUTTON_H] =
  {
    0,
    0b0110010000100000,
    0b0110011000110000,
    0b0110011100111000,
    0b0110011110111100,
    0b0110011111111110,
    0b0110011111111111,
    0b0110011111111111,
    0b0110011111111110,
    0b0110011110111100,
    0b0110011100111000,
    0b0110011000110000,
    0b0110010000100000,
    0
  };
  static uInt32 UNWIND_ALL[BUTTON_H] =
  {
    0,
    0b0110000110000110,
    0b0111000111000110,
    0b0111100111100110,
    0b0111110111110110,
    0b0111111111111110,
    0b0111111111111110,
    0b0111111111111110,
    0b0111111111111110,
    0b0111110111110110,
    0b0111100111100110,
    0b0111000111000110,
    0b0110000110000110,
    0
  };

  const GUI::Font& font = instance().frameBuffer().font();
  const int H_BORDER = 8, BUTTON_GAP = 4, V_BORDER = 4, V_GAP = 4;
  const int buttonWidth = BUTTON_W + 8,
            buttonHeight = BUTTON_H + 10,
            rowHeight = font.getLineHeight();

  WidgetArray wid;
  int xpos, ypos;

  // Set real dimensions
  _w = 20 * (buttonWidth + BUTTON_GAP) + 20;
  _h = V_BORDER * 2 + rowHeight + buttonHeight + 4;

  //this->clearFlags(WIDGET_CLEARBG); // TODO: does NOT work as expected

  xpos = H_BORDER;
  ypos = V_BORDER;

  // Add frame info
  new StaticTextWidget(this, font, xpos, ypos, "04:32 190");

  new StaticTextWidget(this, font, _w - H_BORDER - font.getStringWidth("XX:XX XXX"), ypos, "12:25 144");

  ypos += rowHeight;

  StaticTextWidget* t = new StaticTextWidget(this, font, xpos, ypos + 3, "999");
  new StaticTextWidget(this, font, _w - H_BORDER - font.getStringWidth("8888"), ypos + 3, "1000");
  xpos = t->getRight() + 16;

  myRewindAllWidget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, REWIND_ALL,
                                       BUTTON_W, BUTTON_H, kRewindAll);
  wid.push_back(myRewindAllWidget);
  xpos += buttonWidth + BUTTON_GAP;

  myRewind10Widget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, REWIND_10,
                                      BUTTON_W, BUTTON_H, kRewind10);
  wid.push_back(myRewind10Widget);
  xpos += buttonWidth + BUTTON_GAP;

  myRewind1Widget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, REWIND_1,
                                     BUTTON_W, BUTTON_H, kRewind1);
  wid.push_back(myRewind1Widget);
  xpos += buttonWidth + BUTTON_GAP*2;

  /*myPauseWidget = new ButtonWidget(this, font, xpos, ypos - 2, buttonWidth + 4, buttonHeight + 4, PAUSE,
                                   BUTTON_W, BUTTON_H, kPause);
  wid.push_back(myPauseWidget);
  myPauseWidget->clearFlags(WIDGET_ENABLED);*/
  myPlayWidget = new ButtonWidget(this, font, xpos, ypos - 2, buttonWidth + 4, buttonHeight + 4, PLAY,
                                  BUTTON_W, BUTTON_H, kPlay);
  wid.push_back(myPlayWidget);
  xpos += buttonWidth + BUTTON_GAP*2 + 4;

  myUnwind1Widget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, UNWIND_1,
                                     BUTTON_W, BUTTON_H, kUnwind1);
  wid.push_back(myUnwind1Widget);
  xpos += buttonWidth + BUTTON_GAP;

  myUnwind10Widget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, UNWIND_10,
                                      BUTTON_W, BUTTON_H, kUnwind10);
  wid.push_back(myUnwind10Widget);
  xpos += buttonWidth + BUTTON_GAP;

  myUnwindAllWidget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, UNWIND_ALL,
                                       BUTTON_W, BUTTON_H, kUnwindAll);
  wid.push_back(myUnwindAllWidget);
  xpos += buttonWidth + BUTTON_GAP;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::center()
{
  // Place on the bottom of the screen, centered horizontally
  const GUI::Size& screen = instance().frameBuffer().screenSize();
  const GUI::Rect& dst = surface().dstRect();
  surface().setDstPos((screen.w - dst.width()) >> 1, screen.h - dst.height() - 10);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::loadConfig()
{
  handleWinds();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::handleCommand(CommandSender* sender, int cmd,
                                      int data, int id)
{
cerr << cmd << endl;
  switch(cmd)
  {
    case kPlay:
      instance().eventHandler().leaveMenuMode();
      break;

    case kRewind1:
      handleWinds(-1);
      break;

    case kRewind10:
      handleWinds(-10);
      break;

    case kRewindAll:
      handleWinds(-1000);
      break;

    case kUnwind1:
      handleWinds(1);
      break;

    case kUnwind10:
      handleWinds(10);
      break;

    case kUnwindAll:
      handleWinds(1000);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::handleWinds(Int32 numWinds)
{
  RewindManager& r = instance().state().rewindManager();

  if(numWinds < 0)
    r.rewindState(-numWinds);
  else if(numWinds > 0)
    r.unwindState(numWinds);

  myRewindAllWidget->setEnabled(!r.atFirst());
  myRewind10Widget->setEnabled(!r.atFirst());
  myRewind1Widget->setEnabled(!r.atFirst());

  myUnwindAllWidget->setEnabled(!r.atLast());
  myUnwind10Widget->setEnabled(!r.atLast());
  myUnwind1Widget->setEnabled(!r.atLast());
}
