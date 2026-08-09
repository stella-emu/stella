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

#ifndef COMMAND_HXX
#define COMMAND_HXX

#include "bspf.hxx"

/**
  Allows base GUI objects to send and receive commands.

  @author  Stephen Anthony
*/
class CommandReceiver;
class CommandSender;

class CommandReceiver
{
  friend class CommandSender;

  public:
    CommandReceiver() = default;
    virtual ~CommandReceiver() = default;

  protected:
    // Reacts to a command sent by 'sender'; the default does nothing
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id) { }

  private:
    // Following constructors and assignment operators not supported
    CommandReceiver(const CommandReceiver&) = delete;
    CommandReceiver(CommandReceiver&&) = delete;
    CommandReceiver& operator=(const CommandReceiver&) = delete;
    CommandReceiver& operator=(CommandReceiver&&) = delete;
};

class CommandSender
{
  public:
    CommandSender() = default;
    // Creates a sender already wired to 'target'
    explicit CommandSender(CommandReceiver* target)
        : _target{target} { }

    virtual ~CommandSender() = default;

    // Changes which receiver gets future commands
    virtual void setTarget(CommandReceiver* target) { _target = target; }
    // Returns the current receiver, or nullptr if none is set
    virtual CommandReceiver* getTarget() const { return _target; }

    // Dispatches (cmd, data, id) to the target's handleCommand(), if both are set
    virtual void sendCommand(int cmd, int data, int id)
    {
      if(_target && cmd)
        _target->handleCommand(this, cmd, data, id);
    }

  protected:
    // The receiver that gets commands sent via sendCommand()
    CommandReceiver* _target{nullptr};

  private:
    // Following constructors and assignment operators not supported
    CommandSender(const CommandSender&) = delete;
    CommandSender(CommandSender&&) = delete;
    CommandSender& operator=(const CommandSender&) = delete;
    CommandSender& operator=(CommandSender&&) = delete;
};

#endif  // COMMAND_HXX
