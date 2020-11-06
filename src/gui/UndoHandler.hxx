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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================


#ifndef UNDO_HANDLER_HXX
#define UNDO_HANDLER_HXX

#include "bspf.hxx"
#include <deque>

/**
 * Class for providing undo/redo functionality
 *
 * @author Thomas Jentzsch
 */
class UndoHandler
{
  public:
    UndoHandler(size_t size = 100);
    ~UndoHandler() = default;

    void reset();
    void doo(const string& text);
    bool undo(string& text);
    bool redo(string& text);

  private:
    std::deque<string> myBuffer;
    size_t  mySize{0};
    uInt32  myRedoCount{0};

  private:
    // Following constructors and assignment operators not supported
    UndoHandler(const UndoHandler&) = delete;
    UndoHandler(UndoHandler&&) = delete;
    UndoHandler& operator=(const UndoHandler&) = delete;
    UndoHandler& operator=(UndoHandler&&) = delete;
};

#endif
