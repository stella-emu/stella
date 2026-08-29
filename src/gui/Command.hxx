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

namespace GuiCmd {

  /**
    A GUI command identifier.

    This enum deliberately has no enumerators.  Values are contributed as
    constexpr constants by whichever class defines them (conventionally a
    nested 'Cmd' struct), which an enum cannot do for itself.  The empty
    enumerator list is required rather than cosmetic: gcc's -Wswitch rejects
    any 'case' label that is not a named enumerator, and no 'default' label
    silences it, so a single enumerator here would make every dispatch switch
    fail the warnings-as-errors build.  That is also why 'None' below is a
    constant rather than an enumerator.
  */
  enum class Code : uInt32 {};

  // The command of a widget that has none assigned; never dispatched
  inline constexpr Code None{};

  /**
    Hashes a command's name into its identifier at compile time, using FNV-1a.

    Names are free-form, but qualifying each with its defining class
    ("OptionsDialog.Video") keeps two classes from colliding without someone
    typing the same string twice.
  */
  constexpr Code of(string_view name)
  {
    uInt32 hash = 2166136261U;         // FNV-1a 32-bit offset basis
    for(const char c: name)
    {
      hash ^= static_cast<uInt8>(c);
      hash *= 16777619U;               // FNV-1a 32-bit prime
    }
    // A name hashing to None would silently never dispatch
    return static_cast<Code>(hash ? hash : 1);
  }

}  // namespace GuiCmd

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
    virtual void handleCommand(CommandSender* sender, GuiCmd::Code cmd,
                               int data, int id) { }

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
    virtual void sendCommand(GuiCmd::Code cmd, int data, int id)
    {
      if(_target && cmd != GuiCmd::None)
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
