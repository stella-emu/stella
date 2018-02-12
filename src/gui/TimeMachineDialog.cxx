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
#include "TimeLineWidget.hxx"

#include "Console.hxx"
#include "TIA.hxx"
#include "System.hxx"

#include "TimeMachineDialog.hxx"
#include "Base.hxx"
using Common::Base;


const int BUTTON_W = 14, BUTTON_H = 14;

static uInt32 RECORD[BUTTON_H] =
{
  0b00000111100000,
  0b00011111111000,
  0b00111111111100,
  0b01111111111110,
  0b01111111111110,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b01111111111110,
  0b01111111111110,
  0b00111111111100,
  0b00011111111000,
  0b00000111100000
};

static uInt32 STOP[BUTTON_H] =
{
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111
};

static uInt32 PLAY[BUTTON_H] =
{
  0b11000000000000,
  0b11110000000000,
  0b11111100000000,
  0b11111111000000,
  0b11111111110000,
  0b11111111111100,
  0b11111111111111,
  0b11111111111111,
  0b11111111111100,
  0b11111111110000,
  0b11111111000000,
  0b11111100000000,
  0b11110000000000,
  0b11000000000000
};
static uInt32 REWIND_ALL[BUTTON_H] =
{
  0,
  0b11000011000011,
  0b11000111000111,
  0b11001111001111,
  0b11011111011111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11011111011111,
  0b11001111001111,
  0b11000111000111,
  0b11000011000011,
  0
};
static uInt32 REWIND_1[BUTTON_H] =
{
  0,
  0b00000110001110,
  0b00001110001110,
  0b00011110001110,
  0b00111110001110,
  0b01111110001110,
  0b11111110001110,
  0b11111110001110,
  0b01111110001110,
  0b00111110001110,
  0b00011110001110,
  0b00001110001110,
  0b00000110001110,
  0
};
static uInt32 UNWIND_1[BUTTON_H] =
{
  0,
  0b01110001100000,
  0b01110001110000,
  0b01110001111000,
  0b01110001111100,
  0b01110001111110,
  0b01110001111111,
  0b01110001111111,
  0b01110001111110,
  0b01110001111100,
  0b01110001111000,
  0b01110001110000,
  0b01110001100000,
  0
};
static uInt32 UNWIND_ALL[BUTTON_H] =
{
  0,
  0b11000011000011,
  0b11100011100011,
  0b11110011110011,
  0b11111011111011,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111111111111,
  0b11111011111011,
  0b11110011110011,
  0b11100011100011,
  0b11000011000011,
  0
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeMachineDialog::TimeMachineDialog(OSystem& osystem, DialogContainer& parent,
                                     int width)
  : Dialog(osystem, parent),
    _enterWinds(0)
{
  const GUI::Font& font = instance().frameBuffer().font();
  const int H_BORDER = 6, BUTTON_GAP = 4, V_BORDER = 4;
  const int buttonWidth = BUTTON_W + 10,
            buttonHeight = BUTTON_H + 10,
            rowHeight = font.getLineHeight();

  int xpos, ypos;

  // Set real dimensions
  _w = width;  // Parent determines our width (based on window size)
  _h = V_BORDER * 2 + rowHeight + buttonHeight + 2;

  this->clearFlags(WIDGET_CLEARBG); // does only work combined with blending (0..100)!
  this->clearFlags(WIDGET_BORDER);

  xpos = H_BORDER;
  ypos = V_BORDER;

  // Add index info
  myCurrentIdxWidget = new StaticTextWidget(this, font, xpos, ypos, "1000", TextAlign::Left, kBGColor);
  myCurrentIdxWidget->setTextColor(kColorInfo);
  myLastIdxWidget = new StaticTextWidget(this, font, _w - H_BORDER - font.getStringWidth("1000"), ypos,
                                         "1000", TextAlign::Right, kBGColor);
  myLastIdxWidget->setTextColor(kColorInfo);

  // Add timeline
  const uInt32 tl_h = myCurrentIdxWidget->getHeight() / 2 + 6,
    tl_x = xpos + myCurrentIdxWidget->getWidth() + 8,
    tl_y = ypos + (myCurrentIdxWidget->getHeight() - tl_h) / 2 - 1,
    tl_w = myLastIdxWidget->getAbsX() - tl_x - 8;
  myTimeline = new TimeLineWidget(this, font, tl_x, tl_y, tl_w, tl_h, "", 0, kTimeline);
  myTimeline->setMinValue(0);
  ypos += rowHeight;

  // Add time info
  myCurrentTimeWidget = new StaticTextWidget(this, font, xpos, ypos + 3, "00:00.00", TextAlign::Left, kBGColor);
  myCurrentTimeWidget->setTextColor(kColorInfo);
  myLastTimeWidget = new StaticTextWidget(this, font, _w - H_BORDER - font.getStringWidth("00:00.00"), ypos + 3,
                                          "00:00.00", TextAlign::Right, kBGColor);
  myLastTimeWidget->setTextColor(kColorInfo);
  xpos = myCurrentTimeWidget->getRight() + BUTTON_GAP * 4;

  // Add buttons
  myToggleWidget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, STOP,
                                    BUTTON_W, BUTTON_H, kToggle);
  xpos += buttonWidth + BUTTON_GAP;

  myPlayWidget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, PLAY,
                                  BUTTON_W, BUTTON_H, kPlay);
  xpos += buttonWidth + BUTTON_GAP * 4;

  myRewindAllWidget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, REWIND_ALL,
                                       BUTTON_W, BUTTON_H, kRewindAll);
  xpos += buttonWidth + BUTTON_GAP;

  myRewind1Widget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, REWIND_1,
                                     BUTTON_W, BUTTON_H, kRewind1);
  xpos += buttonWidth + BUTTON_GAP;

  myUnwind1Widget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, UNWIND_1,
                                     BUTTON_W, BUTTON_H, kUnwind1);
  xpos += buttonWidth + BUTTON_GAP;

  myUnwindAllWidget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, UNWIND_ALL,
                                       BUTTON_W, BUTTON_H, kUnwindAll);
  xpos = myUnwindAllWidget->getRight() + BUTTON_GAP * 4;

  // Add message
  myMessageWidget = new StaticTextWidget(this, font, xpos, ypos + 3, "                                             ",
                                         TextAlign::Left, kBGColor);
  myMessageWidget->setTextColor(kColorInfo);
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
  RewindManager& r = instance().state().rewindManager();
  IntArray cycles = r.cyclesList();

  // Set range and intervals for timeline
  uInt32 maxValue = cycles.size() > 1 ? uInt32(cycles.size() - 1) : 0;
  myTimeline->setMaxValue(maxValue);
  myTimeline->setStepValues(cycles);

  // Enable blending (only once is necessary)
  if(!surface().attributes().blending)
  {
    surface().attributes().blending = true;
    surface().attributes().blendalpha = 92;
    surface().applyAttributes();
  }

  myMessageWidget->setLabel("");
  handleWinds(_enterWinds);
  _enterWinds = 0;
  handleToggle();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::handleKeyDown(StellaKey key, StellaMod mod)
{
  // The following 'Alt' shortcuts duplicate the shortcuts in EventHandler
  // It is best to keep them the same, so changes in EventHandler mean we
  // need to update the logic here too
  if(StellaModTest::isAlt(mod))
  {
    switch(key)
    {
      case KBDK_LEFT:  // Alt-left(-shift) rewinds 1(10) states
        handleCommand(nullptr, StellaModTest::isShift(mod) ? kRewind10 : kRewind1, 0, 0);
        break;

      case KBDK_RIGHT:  // Alt-right(-shift) unwinds 1(10) states
        handleCommand(nullptr, StellaModTest::isShift(mod) ? kUnwind10 : kUnwind1, 0, 0);
        break;

      case KBDK_DOWN:  // Alt-down rewinds to start of list
        handleCommand(nullptr, kRewindAll, 0, 0);
        break;

      case KBDK_UP:  // Alt-up rewinds to end of list
        handleCommand(nullptr, kUnwindAll, 0, 0);
        break;

      default:
        Dialog::handleKeyDown(key, mod);
    }
  }
  else if(key == KBDK_SPACE || key == KBDK_ESCAPE)
    handleCommand(nullptr, kPlay, 0, 0);
  else
    Dialog::handleKeyDown(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::handleCommand(CommandSender* sender, int cmd,
                                      int data, int id)
{
  switch(cmd)
  {
    case kTimeline:
    {
      Int32 winds = myTimeline->getValue() -
          instance().state().rewindManager().getCurrentIdx() + 1;
      handleWinds(winds);
      break;
    }

    case kToggle:
      instance().state().toggleTimeMachine();
      handleToggle();
      break;

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
string TimeMachineDialog::getTimeString(uInt64 cycles)
{
  const Int32 scanlines = std::max(instance().console().tia().scanlinesLastFrame(), 240u);
  const bool isNTSC = scanlines <= 287;
  const Int32 NTSC_FREQ = 1193182; // ~76*262*60
  const Int32 PAL_FREQ  = 1182298; // ~76*312*50
  const Int32 freq = isNTSC ? NTSC_FREQ : PAL_FREQ; // = cycles/second

  uInt32 minutes = uInt32(cycles / (freq * 60));
  cycles -= minutes * (freq * 60);
  uInt32 seconds = uInt32(cycles / freq);
  cycles -= seconds * freq;
  uInt32 frames = uInt32(cycles / (scanlines * 76));

  stringstream time;
  time << Common::Base::toString(minutes, Common::Base::F_10_02) << ":";
  time << Common::Base::toString(seconds, Common::Base::F_10_02) << ".";
  time << Common::Base::toString(frames, Common::Base::F_10_02);

  return time.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::handleWinds(Int32 numWinds)
{
  RewindManager& r = instance().state().rewindManager();

  if(numWinds)
  {
    uInt64 startCycles = instance().console().tia().cycles();
    if(numWinds < 0)      r.rewindStates(-numWinds);
    else if(numWinds > 0) r.unwindStates(numWinds);

    uInt64 elapsed = instance().console().tia().cycles() - startCycles;
    if(elapsed > 0)
    {
      string message = r.getUnitString(elapsed);

      // TODO: add message text from addState()
      myMessageWidget->setLabel((numWinds < 0 ? "(-" : "(+") + message + ")");
    }
  }

  // Update time
  myCurrentTimeWidget->setLabel(getTimeString(r.getCurrentCycles() - r.getFirstCycles()));
  myLastTimeWidget->setLabel(getTimeString(r.getLastCycles() - r.getFirstCycles()));
  myTimeline->setValue(r.getCurrentIdx()-1);
  // Update index
  myCurrentIdxWidget->setValue(r.getCurrentIdx());
  myLastIdxWidget->setValue(r.getLastIdx());
  // Enable/disable buttons
  myRewindAllWidget->setEnabled(!r.atFirst());
  myRewind1Widget->setEnabled(!r.atFirst());
  myUnwindAllWidget->setEnabled(!r.atLast());
  myUnwind1Widget->setEnabled(!r.atLast());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::handleToggle()
{
  myToggleWidget->setBitmap(instance().state().mode() == StateManager::Mode::Off ? RECORD : STOP,
                            BUTTON_W, BUTTON_H);
}

