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
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "GuiObject.hxx"
#include "Widget.hxx"

// The constructor and destructor are defined here (instead of inline in the
// header) so that Widget is a complete type wherever ~vector<unique_ptr<Widget>>
// (the type of _children) needs to be instantiated.

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GuiObject::GuiObject(OSystem& osystem, DialogContainer& parent, Dialog& dialog,
                     int w, int h)
  : myOSystem{osystem},
    myParent{parent},
    myDialog{dialog},
    _w{w}, _h{h} { }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GuiObject::~GuiObject() = default;
