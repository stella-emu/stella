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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartELFWidget.hxx"

#include "CartELF.hxx"
#include "Widget.hxx"
#include "StringParser.hxx"
#include "ScrollBarWidget.hxx"
#include "StringListWidget.hxx"
#include "BrowserDialog.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "bspf.hxx"

namespace {
  constexpr int SAVE_ARM_IMAGE_CMD = 'sarm';
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELFWidget::CartridgeELFWidget(GuiObject* boss, const GUI::Font& lfont,
                       const GUI::Font& nfont,
                       int x, int y, int w, int h,
                       CartridgeELF& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h), myCart(cart)
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELFWidget::initialize()
{
  addBaseInformation(myCart.myImageSize, "AtariAge", "see log below", 1);

  const auto lineHeight = _font.getLineHeight();
  const auto width = _w - 12;
  constexpr uInt32 visibleLogLines = 19;
  constexpr int x = 2;

  int y = (9 * lineHeight) / 2;

  const StringParser parser(
    myCart.getDebugLog(),
    (width - ScrollBarWidget::scrollBarWidth(_font)) / _font.getMaxCharWidth()
  );

  const auto& logLines = parser.stringList();
  const bool useScrollbar = logLines.size() > visibleLogLines;

  auto* logWidget = new StringListWidget(
    _boss, _font, x, y, width, visibleLogLines * lineHeight, false, useScrollbar
  );

  logWidget->setEditable(false);
  logWidget->setEnabled(true);
  logWidget->setList(logLines);

  y += visibleLogLines * lineHeight + lineHeight / 2;

  WidgetArray wid;

  auto* saveImageButton = new ButtonWidget(_boss, _font, x, y, "Save ARM image", SAVE_ARM_IMAGE_CMD);
  saveImageButton->setTarget(this);

  wid.push_back(saveImageButton);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELFWidget::saveArmImage(const FSNode& node)
{
  try {
    const auto [buffer, size] = myCart.getArmImage();

    const size_t sizeWritten = node.write(buffer, size);
    if (sizeWritten != size) throw runtime_error("failed to write arm image");

    instance().frameBuffer().showTextMessage("Successfully exported ARM executable image", MessagePosition::MiddleCenter, true);
  }
  catch (...) {
    instance().frameBuffer().showTextMessage("Failed to export ARM executable image", MessagePosition::MiddleCenter, true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELFWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  if (cmd == SAVE_ARM_IMAGE_CMD)
    BrowserDialog::show(
      _boss,
      "Save ARM image",
      instance().userDir().getPath() + "arm_image.bin",
      BrowserDialog::Mode::FileSave,
      [this](bool ok, const FSNode& node) {
        if (ok) saveArmImage(node);
      }
    );
}
