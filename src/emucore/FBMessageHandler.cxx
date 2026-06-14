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
#include "TIA.hxx"
#include "Settings.hxx"
#include "FBSurface.hxx"
#include "FrameBuffer.hxx"
#include "FBMessageHandler.hxx"

#ifdef GUI_SUPPORT
  #include "Font.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBMessageHandler::FBMessageHandler(FrameBuffer& fb, OSystem& osystem)
  : myFB{fb},
    myOSystem{osystem}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBMessageHandler::init()
{
#ifdef GUI_SUPPORT
  myMsg.enabled = false;

  // Create surfaces for TIA statistics and general messages
  const GUI::Font& f = myFB.hidpiEnabled() ? myFB.infoFont() : myFB.font();
  myStatsMsg.color = kColorInfo;
  myStatsMsg.w = f.getMaxCharWidth() * 40 + 3;
  myStatsMsg.h = (f.getFontHeight() + 2) * 3;

  if(!myStatsMsg.surface)
  {
    myStatsMsg.surface = myFB.allocateSurface(myStatsMsg.w, myStatsMsg.h);
    myStatsMsg.surface->enableBlend(true);
    myStatsMsg.surface->setBlendLevel(92); // aligned with TimeMachineDialog
  }

  if(!myMsg.surface)
  {
    const int fontWidth = myFB.font().getMaxCharWidth(),
              HBORDER = fontWidth * 1.25 / 2.0;
    myMsg.surface = myFB.allocateSurface(fontWidth * MESSAGE_WIDTH + HBORDER * 2,
                                         myFB.font().getFontHeight() * 1.5);
  }
#endif  // GUI_SUPPORT
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBMessageHandler::create(string_view message, MessagePosition position,
                              bool force)
{
#ifdef GUI_SUPPORT
  // Only show messages if they've been enabled
  if(myMsg.surface == nullptr || !(force || myOSystem.settings().getBool("uimessages")))
    return;

  const int fontHeight = myFB.font().getFontHeight();
  const int VBORDER = fontHeight / 4;

  // Show message for 2 seconds
  myMsg.counter = std::min(static_cast<Int32>(myOSystem.frameRate()) * 2, MESSAGE_TIME);
  if(myMsg.counter == 0)
    myMsg.counter = MESSAGE_TIME;

  myMsg.text      = message;
  myMsg.color     = kBtnTextColor;
  myMsg.h         = fontHeight + VBORDER * 2;
  myMsg.position  = position;
  myMsg.enabled   = true;
  myMsg.dirty     = true;

  myMsg.surface->setSrcSize(myMsg.w, myMsg.h);
  myMsg.surface->setDstSize(myMsg.w * myFB.hidpiScaleFactor(),
                            myMsg.h * myFB.hidpiScaleFactor());
#endif  // GUI_SUPPORT
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBMessageHandler::showText(string_view message, MessagePosition position,
                                bool force)
{
#ifdef GUI_SUPPORT
  // If called off the main thread, queue and let drainPending() show it later
  if(std::this_thread::get_id() != myMainThreadId)
  {
    const std::lock_guard<std::mutex> lock(myPendingMutex);
    myPending.push_back({string{message}, position, force});
    return;
  }

  const int fontWidth = myFB.font().getMaxCharWidth();
  const int HBORDER = fontWidth * 1.25 / 2.0;

  myMsg.showGauge = false;
  myMsg.w         = std::min(fontWidth * MESSAGE_WIDTH - HBORDER * 2,
                             myFB.font().getStringWidth(message) + HBORDER * 2);

  create(message, position, force);
#endif  // GUI_SUPPORT
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBMessageHandler::drainPending()
{
#ifdef GUI_SUPPORT
  vector<PendingMessage> pending;
  {
    const std::lock_guard<std::mutex> lock(myPendingMutex);
    if(myPending.empty())
      return;
    std::swap(pending, myPending);
  }

  for(const auto& msg : pending)
    showText(msg.text, msg.position, msg.force);
#endif  // GUI_SUPPORT
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBMessageHandler::showGauge(string_view message, string_view valueText,
                                 float value, float minValue, float maxValue)
{
#ifdef GUI_SUPPORT
  const int fontWidth = myFB.font().getMaxCharWidth();
  const int HBORDER = fontWidth * 1.25 / 2.0;

  myMsg.showGauge  = true;
  if(maxValue != minValue)
    myMsg.value = (value - minValue) / (maxValue - minValue) * 100.F;
  else
    myMsg.value = 100.F;
  myMsg.valueText  = valueText;
  myMsg.w          = std::min(fontWidth * MESSAGE_WIDTH,
                              myFB.font().getStringWidth(message)
                              + fontWidth * (GAUGEBAR_WIDTH + 2)
                              + myFB.font().getStringWidth(valueText))
                              + HBORDER * 2;

  create(message, MessagePosition::BottomCenter);
#endif  // GUI_SUPPORT
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBMessageHandler::showStats(bool enable)
{
  myStatsEnabled = myStatsMsg.enabled = enable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBMessageHandler::enable(bool enable)
{
  if(enable)
  {
    // Only re-enable frame stats if they were already enabled before
    myStatsMsg.enabled = myStatsEnabled;
  }
  else
  {
    // Temporarily disable frame stats
    myStatsMsg.enabled = false;

    // Erase any onscreen message
    hide();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBMessageHandler::setPauseDelay()
{
  myPausedCount = static_cast<Int32>(2 * myOSystem.frameRate());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBMessageHandler::tickPause()
{
  if(myMsg.counter >= MESSAGE_TIME)
    return false;  // a non-pause message was just shown; don't override it
  if(myPausedCount-- > 0)
    return false;
  myPausedCount = static_cast<Int32>(7 * myOSystem.frameRate());
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBMessageHandler::onEmulationFrame()
{
  myLastScanlines = myOSystem.console().tia().frameBufferScanlinesLastFrame();
  myPausedCount = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBMessageHandler::hide()
{
  if(myMsg.enabled)
    myFB.setPendingRender();
  myMsg.enabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBMessageHandler::draw()
{
#ifdef GUI_SUPPORT
  // Either erase the entire message (when time is reached),
  // or show again this frame
  if(myMsg.counter == 0)
  {
    hide();
    return false;
  }

  if(myMsg.dirty)
  {
  #ifdef DEBUG_BUILD
    cerr << "m";
  #endif

    // Draw the bounded box and text
    const Common::Rect& dst = myMsg.surface->dstRect();
    const int fontWidth  = myFB.font().getMaxCharWidth(),
              fontHeight = myFB.font().getFontHeight();
    const int VBORDER = fontHeight / 4;
    const int HBORDER = fontWidth * 1.25 / 2.0;
    constexpr int BORDER = 1;

    switch(myMsg.position)
    {
      case MessagePosition::TopLeft:
        myMsg.x = 5;
        myMsg.y = 5;
        break;

      case MessagePosition::TopCenter:
        myMsg.x = (myFB.imageRect().w() - dst.w()) >> 1;
        myMsg.y = 5;
        break;

      case MessagePosition::TopRight:
        myMsg.x = myFB.imageRect().w() - dst.w() - 5;
        myMsg.y = 5;
        break;

      case MessagePosition::MiddleLeft:
        myMsg.x = 5;
        myMsg.y = (myFB.imageRect().h() - dst.h()) >> 1;
        break;

      case MessagePosition::MiddleCenter:
        myMsg.x = (myFB.imageRect().w() - dst.w()) >> 1;
        myMsg.y = (myFB.imageRect().h() - dst.h()) >> 1;
        break;

      case MessagePosition::MiddleRight:
        myMsg.x = myFB.imageRect().w() - dst.w() - 5;
        myMsg.y = (myFB.imageRect().h() - dst.h()) >> 1;
        break;

      case MessagePosition::BottomLeft:
        myMsg.x = 5;
        myMsg.y = myFB.imageRect().h() - dst.h() - 5;
        break;

      case MessagePosition::BottomCenter:
        myMsg.x = (myFB.imageRect().w() - dst.w()) >> 1;
        myMsg.y = myFB.imageRect().h() - dst.h() - 5;
        break;

      case MessagePosition::BottomRight:
        myMsg.x = myFB.imageRect().w() - dst.w() - 5;
        myMsg.y = myFB.imageRect().h() - dst.h() - 5;
        break;

      default:
        break;  // Not supposed to get here
    }

    myMsg.surface->setDstPos(myMsg.x + myFB.imageRect().x(),
                             myMsg.y + myFB.imageRect().y());
    myMsg.surface->fillRect(0, 0, myMsg.w, myMsg.h, kColor);
    myMsg.surface->fillRect(BORDER, BORDER, myMsg.w - BORDER * 2,
                            myMsg.h - BORDER * 2, kBtnColor);
    myMsg.surface->drawString(myFB.font(), myMsg.text, HBORDER, VBORDER,
                              myMsg.w, myMsg.color);

    if(myMsg.showGauge)
    {
      constexpr int NUM_TICKMARKS = 4;
      // Limit gauge bar width if texts are too long
      const int swidth = std::min(fontWidth * GAUGEBAR_WIDTH,
                                  fontWidth * (MESSAGE_WIDTH - 2)
                                  - myFB.font().getStringWidth(myMsg.text)
                                  - myFB.font().getStringWidth(myMsg.valueText));
      const int bwidth  = swidth * myMsg.value / 100.F;
      const int bheight = fontHeight >> 1;
      const int x = HBORDER + myFB.font().getStringWidth(myMsg.text) + fontWidth;
      // Align bar with bottom of text
      const int y = VBORDER + myFB.font().desc().ascent - bheight;

      // Draw gauge bar
      myMsg.surface->fillRect(x - BORDER, y, swidth + BORDER * 2, bheight, kSliderBGColor);
      myMsg.surface->fillRect(x, y + BORDER, bwidth, bheight - BORDER * 2, kSliderColor);
      // Draw tickmarks
      for(int i = 1; i < NUM_TICKMARKS; ++i)
      {
        const int xt = x + swidth * i / NUM_TICKMARKS;
        const ColorId color = (bwidth < xt - x) ? kCheckColor : kSliderBGColor;
        myMsg.surface->vLine(xt, y + bheight / 2, y + bheight - 1, color);
      }
      // Draw value text
      myMsg.surface->drawString(myFB.font(), myMsg.valueText,
                                x + swidth + fontWidth, VBORDER,
                                myMsg.w, myMsg.color);
    }

    myMsg.dirty = false;
    myMsg.surface->render();
    return true;
  }

  myMsg.counter--;
  myMsg.surface->render();
#endif  // GUI_SUPPORT

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBMessageHandler::drawStats(float framesPerSecond)
{
#ifdef GUI_SUPPORT
  const ConsoleInfo& info = myOSystem.console().about();
  constexpr int xPos = 2;
  int yPos = 0;
  const GUI::Font& f = myFB.hidpiEnabled() ? myFB.infoFont() : myFB.font();
  const int dy = f.getFontHeight() + 2;

  myStatsMsg.surface->invalidate();

  // Draw scanlines / framerate / format
  ColorId color = myOSystem.console().tia().frameBufferScanlinesLastFrame() !=
    myLastScanlines ? kDbgColorRed : myStatsMsg.color;

  const string line1 = std::format("{} / {:.1f}Hz => {}",
    myOSystem.console().tia().frameBufferScanlinesLastFrame(),
    myOSystem.console().currentFrameRate(),
    info.DisplayFormat);

  myStatsMsg.surface->drawString(f, line1, xPos, yPos,
    myStatsMsg.w, color, TextAlign::Left, 0, true, kBGColor);

  yPos += dy;

  // Draw fps / speed
  const float speed = myOSystem.settings().getBool("turbo")
    ? 50.F
    : myOSystem.settings().getFloat("speed");
  const string line2 = std::format("{:.1f}fps @ {:.0f}% speed",
    framesPerSecond, 100 * speed);

  myStatsMsg.surface->drawString(f, line2, xPos, yPos,
    myStatsMsg.w, myStatsMsg.color, TextAlign::Left, 0, true, kBGColor);

  yPos += dy;

  // Draw bankswitch info, and optionally developer/vsync status
  int xPosEnd = myStatsMsg.surface->drawString(f, info.BankSwitch, xPos, yPos,
    myStatsMsg.w, myStatsMsg.color, TextAlign::Left, 0, true, kBGColor);

  if(myOSystem.settings().getBool("dev.settings"))
  {
    xPosEnd = myStatsMsg.surface->drawString(f, "| ", xPosEnd, yPos,
      myStatsMsg.w, color, TextAlign::Left, 0, true, kBGColor);

    const bool vsyncCorrect = myOSystem.console().vsyncCorrect();
    color = vsyncCorrect ? myStatsMsg.color : kDbgColorRed;
    myStatsMsg.surface->drawString(f, vsyncCorrect ? "Developer" : "VSYNC!",
      xPosEnd, yPos, myStatsMsg.w, color, TextAlign::Left, 0, true, kBGColor);
  }

  myStatsMsg.surface->setDstPos(myFB.imageRect().x() + myFB.imageRect().w() / 64,
                                myFB.imageRect().y() + myFB.imageRect().h() / 64);
  myStatsMsg.surface->setDstSize(myStatsMsg.w * myFB.hidpiScaleFactor(),
                                 myStatsMsg.h * myFB.hidpiScaleFactor());
  myStatsMsg.surface->render();
#endif  // GUI_SUPPORT
}
