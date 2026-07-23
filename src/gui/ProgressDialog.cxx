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

#include "bspf.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "EventHandler.hxx"
#include "TimerManager.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "Font.hxx"
#include "DialogContainer.hxx"
#include "Layout.hxx"
#include "ProgressDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ProgressDialog::ProgressDialog(GuiObject* boss, const GUI::Font& font,
                               string_view message)
  : Dialog(boss->instance(), boss->parent(), font),
    myMessageText{message}
{
  WidgetArray wid;

  myMessage = new StaticTextWidget(this, font, message,
                                   TextAlign::Center);
  myMessage->setTextColor(kTextColorEm);

  mySlider = new SliderWidget(this, font, 1, 0);
  mySlider->setMinValue(1);
  mySlider->setMaxValue(100);

  auto* b = new ButtonWidget(this, font, "Cancel", Event::UICancel);
  wid.push_back(b);
  addCancelWidget(b);
  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::stretchedItem;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int VBORDER = Dialog::vBorder(),
            HBORDER = Dialog::hBorder(),
            VGAP    = Dialog::vGap();

  // The Cancel button keeps the width its label needs, centered on its row
  auto cancelRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  cancelRow->addStretchSpace();
  cancelRow->addAuto(anchoredItem(_cancelWidget));
  cancelRow->addStretchSpace();

  // The message and the slider below it both span the dialog's width; how much
  // room the message needs is what that width is derived from (the button row
  // below widens it further if the message is a short one)
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addAuto(stretchedItem(myMessage, _font.getStringWidth(myMessageText)));
  root->addSpace(VGAP * 2);
  root->addAuto(stretchedItem(mySlider));
  root->addSpace(VGAP * 4);
  root->addAuto(std::move(cancelRow));

  const Common::Size natural = root->naturalSize();

  _w = static_cast<int>(natural.w);
  _h = static_cast<int>(natural.h);

  root->doLayout(0, 0, _w, _h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressDialog::setMessage(string_view message)
{
  myMessageText = message;
  myMessage->setLabel(message);
  layout();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressDialog::setRange(int start, int finish, int step)
{
  myStart = start;
  myFinish = finish;
  myStep = static_cast<int>((step / 100.0) * (myFinish - myStart + 1));

  mySlider->setMinValue(myStart + myStep);
  mySlider->setMaxValue(myFinish);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressDialog::resetProgress()
{
  myLastTick = TimerManager::getTicks();
  myProgress = 0;
  mySlider->setValue(0);
  myIsCancelled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressDialog::setProgress(int progress)
{
  // Only increase the progress bar if some time has passed
  if(TimerManager::getTicks() - myLastTick > 100000) // update every 1/10th second
  {
    myLastTick = TimerManager::getTicks();
    mySlider->setValue(progress % (myFinish - myStart + 1));

    // Since this dialog is usually called in a tight loop that doesn't
    // yield, we need to manually:
    // - tell the framebuffer that a redraw is necessary
    // - poll the events
    // This isn't really an ideal solution, since all redrawing and
    // event handling is suspended until the dialog is closed
    instance().frameBuffer().update();
    instance().eventHandler().poll(TimerManager::getTicks());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressDialog::incProgress()
{
  setProgress(++myProgress);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  if(cmd == Event::UICancel)
    myIsCancelled = true;
  else
    Dialog::handleCommand(sender, cmd, data, 0);
}
