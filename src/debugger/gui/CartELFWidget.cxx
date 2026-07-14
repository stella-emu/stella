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

#include "CartELFWidget.hxx"

#include "CartELF.hxx"
#include "Widget.hxx"
#include "WrappedTextWidget.hxx"
#include "Layout.hxx"
#include "BrowserDialog.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Debugger.hxx"
#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELFWidget::CartridgeELFWidget(GuiObject* boss,
                       const GUI::Font& lfont, const GUI::Font& nfont,
                       int x, int y, int w, int h,
                       CartridgeELF& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart{cart}
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELFWidget::initialize()
{
  createBaseInformation(myCart.myImage.size(), "AtariAge", "see log below", 1);

  // The log wraps itself to whatever width it is given, and scrolls beyond
  // VISIBLE_LOG_LINES of it
  myLog = new WrappedTextWidget(_boss, _font, 0, 0, 1, 1,
                                myCart.getDebugLog(), VISIBLE_LOG_LINES);
  myLog->setEditable(false);
  myLog->setEnabled(true);

  mySaveImageButton = new ButtonWidget(_boss, _font, 0, 0,
                                       "Save ARM image" + ELLIPSIS, kSaveArmImageCmd);
  mySaveImageButton->setTarget(this);
  addFocusWidget(mySaveImageButton);

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELFWidget::layoutContent(GUI::BoxLayout& col)
{
  using GUI::anchoredItem;
  using GUI::stretchedItem;

  // Word wrap couples width to height, so the log is given its width before the
  // column holding it is built (see the heightForWidth note in Layout.hxx)
  myLog->setWidth(contentWidth(_w));

  col.addSpace(_lineHeight / 2);
  col.addAuto(stretchedItem(myLog));
  col.addSpace(_lineHeight / 2);
  col.addAuto(anchoredItem(mySaveImageButton));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELFWidget::saveArmImage(const FSNode& node)
{
  try
  {
    const ByteArray buffer = myCart.getArmImage();

    const size_t sizeWritten = node.write(buffer);
    if(sizeWritten != buffer.size()) throw std::runtime_error("failed to write arm image");

    instance().frameBuffer().showTextMessage("Successfully exported ARM executable image", MessagePosition::BottomCenter, true);
  }
  catch(...)
  {
    instance().frameBuffer().showTextMessage("Failed to export ARM executable image", MessagePosition::BottomCenter, true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELFWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  if(cmd == kSaveArmImageCmd)
    BrowserDialog::show(
      instance().debugger().baseDialog(),
      "Save ARM image",
      instance().userDir().getPath() + "arm_image.bin",
      BrowserDialog::Mode::FileSave,
      [this](bool ok, const FSNode& node) {
        if (ok) saveArmImage(node);
      }
    );
}
