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
// $Id: Serializer.hxx,v 1.1 2002-05-13 19:14:17 stephena Exp $
//============================================================================

#ifndef SERIALIZER_HXX
#define SERIALIZER_HXX

#include <fstream>
#include <string>

/**
  This class implements a Serializer device, whereby data is
  serialized and sent to an output binary file in a system-
  independent way.

  All bytes and integers are written as long's.  Strings are
  written as characters prepended by the length of the string.
  Boolean values are written using a special pattern.

  @author  Stephen Anthony
  @version $Id: Serializer.hxx,v 1.1 2002-05-13 19:14:17 stephena Exp $
*/
class Serializer
{
  public:
    /**
      Creates a new Serializer device.

      Open must be called with a valid file before this Serializer can
      be used.
    */
    Serializer(void);

    /**
      Destructor
    */
    virtual ~Serializer(void);

  public:
    /**
      Opens the given file for output.  Multiple calls to this method
      will close previously opened files.

      @param fileName The filename to send the serialized data to.
      @return Result of opening the file.  True on success, false on failure
    */
    bool open(string& fileName);

    /**
      Closes the current output stream.
    */
    void close(void);

    /**
      Writes a long value to the current output stream.

      @param value The long value to write to the output stream.
    */
    void putLong(long value);

    /**
      Writes a string to the current output stream.

      @param str The string to write to the output stream.
    */
    void putString(string& str);

    /**
      Writes a boolean value to the current output stream.

      @param b The boolean value to write to the output stream.
    */
    void putBool(bool b);

  private:
    // The stream to send the serialized data to.
    ofstream myStream;

    // A long pattern that represents a boolean value of true.
    long TruePattern;

    // A long pattern that represents a boolean value of false.
    long FalsePattern;
};

#endif
