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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "SaveKey.hxx"
#include "MT24LC256.hxx"
#include "SaveKeyWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SaveKeyWidget::SaveKeyWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller)
{
  bool leftport = myController.jack() == Controller::Left;
  const string& label = leftport ? "Left (SaveKey):" : "Right (SaveKey):";

  const int fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight(),
            bwidth  = font.getStringWidth("Erase EEPROM") + 20,
            bheight = lineHeight + 4;

  int xpos = x, ypos = y, lwidth = font.getStringWidth("Right (SaveKey):");
  StaticTextWidget* t;

  t = new StaticTextWidget(boss, font, xpos, ypos+2, lwidth,
                           fontHeight, label, kTextAlignLeft);

  ypos += t->getHeight() + 20;
  myEEPROMErase =
    new ButtonWidget(boss, font, xpos+10, ypos, bwidth, bheight,
                     "Erase EEPROM", kEEPROMErase);
  myEEPROMErase->setTarget(this);
  ypos += lineHeight + 20;

  new StaticTextWidget(boss, font, xpos, ypos, fontWidth*22,
                       fontHeight, "(*) This will erase", kTextAlignLeft);
  ypos += lineHeight + 2;
  new StaticTextWidget(boss, font, xpos, ypos, fontWidth*22,
                       fontHeight, "all EEPROM data, not", kTextAlignLeft);
  ypos += lineHeight + 2;
  new StaticTextWidget(boss, font, xpos, ypos, fontWidth*22,
                       fontHeight, "just the range used", kTextAlignLeft);
  ypos += lineHeight + 2;
  new StaticTextWidget(boss, font, xpos, ypos, fontWidth*22,
                       fontHeight, "for this ROM", kTextAlignLeft);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SaveKeyWidget::handleCommand(CommandSender*, int cmd, int, int)
{
  if(cmd == kEEPROMErase)
  {
    SaveKey& skey = static_cast<SaveKey&>(myController);
    skey.myEEPROM->erase();
  }
}
