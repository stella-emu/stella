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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Deserializer.hxx,v 1.1 2002-05-13 19:14:17 stephena Exp $
//============================================================================

#ifndef DESERIALIZER_HXX
#define DESERIALIZER_HXX

#include <fstream>
#include <string>

/**
  This class implements a Deserializer device, whereby data is
  deserialized from an input binary file in a system-independent
  way.

  All longs should be cast to their appropriate data type upon method
  return.

  @author  Stephen Anthony
  @version $Id: Deserializer.hxx,v 1.1 2002-05-13 19:14:17 stephena Exp $
*/
class Deserializer
{
  public:
    /**
      Creates a new Deserializer device.

      Open must be called with a valid file before this Deserializer can
      be used.
    */
    Deserializer(void);

    /**
      Destructor
    */
    virtual ~Deserializer(void);

  public:
    /**
      Opens the given file for input.  Multiple calls to this method
      will close previously opened files.

      @param fileName The filename to get the deserialized data from.
      @return Result of opening the file.  True on success, false on failure
    */
    bool open(string& fileName);

    /**
      Closes the current input stream.
    */
    void close(void);

    /**
      Reads a long value from the current input stream.

      @result The long value which has been read from the stream.
    */
    long getLong(void);

    /**
      Reads a string from the current input stream.

      @result The string which has been read from the stream.
    */
    string getString(void);

    /**
      Reads a boolean value from the current input stream.

      @result The boolean value which has been read from the stream.
    */
    bool getBool(void);

  private:
    // The stream to get the deserialized data from.
    ifstream myStream;

    // A long pattern that represents a boolean value of true.
    long TruePattern;

    // A long pattern that represents a boolean value of false.
    long FalsePattern;
};

#endif
