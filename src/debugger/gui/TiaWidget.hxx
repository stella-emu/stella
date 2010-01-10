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

#ifndef TIA_WIDGET_HXX
#define TIA_WIDGET_HXX

class GuiObject;
class ButtonWidget;
class DataGridWidget;
class StaticTextWidget;
class ToggleBitWidget;
class TogglePixelWidget;
class EditTextWidget;
class ColorWidget;

#include "Widget.hxx"
#include "Command.hxx"


class TiaWidget : public Widget, public CommandSender
{
  public:
    TiaWidget(GuiObject* boss, const GUI::Font& font,
              int x, int y, int w, int h);
    virtual ~TiaWidget();

    void handleCommand(CommandSender* sender, int cmd, int data, int id);
    void loadConfig();

  private:
    void fillGrid();
    void changeColorRegs();

  private:
    DataGridWidget* myRamGrid;
    EditTextWidget* myBinValue;
    EditTextWidget* myDecValue;
    EditTextWidget* myLabel;

    DataGridWidget* myColorRegs;

    ColorWidget* myCOLUP0Color;
    ColorWidget* myCOLUP1Color;
    ColorWidget* myCOLUPFColor;
    ColorWidget* myCOLUBKColor;

    TogglePixelWidget* myGRP0;
    TogglePixelWidget* myGRP1;

    DataGridWidget* myPosP0;
    DataGridWidget* myPosP1;
    DataGridWidget* myPosM0;
    DataGridWidget* myPosM1;
    DataGridWidget* myPosBL;

    DataGridWidget* myHMP0;
    DataGridWidget* myHMP1;
    DataGridWidget* myHMM0;
    DataGridWidget* myHMM1;
    DataGridWidget* myHMBL;

    DataGridWidget* myNusizP0;
    DataGridWidget* myNusizP1;
    DataGridWidget* myNusizM0;
    DataGridWidget* myNusizM1;
    DataGridWidget* mySizeBL;
    EditTextWidget* myNusizP0Text;
    EditTextWidget* myNusizP1Text;

    CheckboxWidget* myRefP0;
    CheckboxWidget* myRefP1;
    CheckboxWidget* myDelP0;
    CheckboxWidget* myDelP1;
    CheckboxWidget* myDelBL;

    CheckboxWidget* myEnaM0;
    CheckboxWidget* myEnaM1;
    CheckboxWidget* myEnaBL;

    CheckboxWidget* myResMP0;
    CheckboxWidget* myResMP1;

    /** Collision register bits */
    CheckboxWidget* myCollision[15];

    TogglePixelWidget* myPF[3];
    CheckboxWidget* myRefPF;
    CheckboxWidget* myScorePF;
    CheckboxWidget* myPriorityPF;

    // ID's for the various widgets
    // We need ID's, since there are more than one of several types of widgets
    enum {
      kP0_PFID,   kP0_BLID,   kP0_M1ID,   kP0_M0ID,   kP0_P1ID,
      kP1_PFID,   kP1_BLID,   kP1_M1ID,   kP1_M0ID,
      kM0_PFID,   kM0_BLID,   kM0_M1ID,
      kM1_PFID,   kM1_BLID,
      kBL_PFID,   // Make these first, since we want them to start from 0

      kRamID,
      kColorRegsID,
      kGRP0ID,    kGRP1ID,
      kPosP0ID,   kPosP1ID,
      kPosM0ID,   kPosM1ID,   kPosBLID,
      kHMP0ID,    kHMP1ID,
      kHMM0ID,    kHMM1ID,    kHMBLID,
      kRefP0ID,   kRefP1ID,
      kDelP0ID,   kDelP1ID,   kDelBLID,
      kNusizP0ID, kNusizP1ID,
      kNusizM0ID, kNusizM1ID, kSizeBLID,
      kEnaM0ID,   kEnaM1ID,   kEnaBLID,
      kResMP0ID,  kResMP1ID,
      kPF0ID,     kPF1ID,     kPF2ID,
      kRefPFID,   kScorePFID, kPriorityPFID
    };

    // Strobe button commands
    enum {
      kWsyncCmd = 'Swsy',
      kRsyncCmd = 'Srsy',
      kResP0Cmd = 'Srp0',
      kResP1Cmd = 'Srp1',
      kResM0Cmd = 'Srm0',
      kResM1Cmd = 'Srm1',
      kResBLCmd = 'Srbl',
      kHmoveCmd = 'Shmv',
      kHmclrCmd = 'Shmc',
      kCxclrCmd = 'Scxl'
    };

    // Color registers
    enum {
      kCOLUP0Addr,
      kCOLUP1Addr,
      kCOLUPFAddr,
      kCOLUBKAddr
    };
};

#endif
