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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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
  @version $Id$
*/
class CommandReceiver;
class CommandSender;

class CommandReceiver
{
  friend class CommandSender;

  public:
    virtual ~CommandReceiver() {}

  protected:
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id) {}
};

class CommandSender
{
  // TODO - allow for multiple targets, i.e. store targets in a list
  // and add methods addTarget/removeTarget.
  public:
    CommandSender(CommandReceiver* target)
        : _target(target) {}

    virtual ~CommandSender() {}

    void setTarget(CommandReceiver* target) { _target = target; }
    CommandReceiver* getTarget() const { return _target; }

    virtual void sendCommand(int cmd, int data, int id)
    {
      if(_target && cmd)
        _target->handleCommand(this, cmd, data, id);
	}

  protected:
    CommandReceiver* _target;
};

#endif
