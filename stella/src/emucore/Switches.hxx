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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef SWITCHES_HXX
#define SWITCHES_HXX

class Event;
class Properties;

#include "Serializable.hxx"
#include "bspf.hxx"

/**
  This class represents the console switches of the game console.

  @author  Bradford W. Mott
  @version $Id$
*/
class Switches : public Serializable
{
  /**
    Riot debug class needs special access to the underlying controller state
  */
  friend class RiotDebug;

  public:
    /**
      Create a new set of switches using the specified events and
      properties

      @param event The event object to use for events
    */
    Switches(const Event& event, const Properties& properties);
 
    /**
      Destructor
    */
    virtual ~Switches();

  public:
    /**
      Get the value of the console switches

      @return The 8 bits which represent the state of the console switches
    */
    uInt8 read() const { return mySwitches; }

    /**
      Update the switches variable
    */
    void update();

    /**
      Save the current state of the switches to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const;

    /**
      Load the current state of the switches from the given Deserializer.

      @param in  The Deserializer object to use
      @return  False on any errors, else true
    */
    bool load(Deserializer& in);

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const { return "Switches"; }

  private:
    // Reference to the event object to use
    const Event& myEvent;

    // State of the console switches
    uInt8 mySwitches;
};

#endif
