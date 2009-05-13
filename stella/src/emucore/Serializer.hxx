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

#ifndef SERIALIZER_HXX
#define SERIALIZER_HXX

#include <fstream>
#include "bspf.hxx"

/**
  This class implements a Serializer device, whereby data is
  serialized and sent to an output binary file in a system-
  independent way.

  Bytes are written as characters, integers are written as 4 characters
  (32-bit), strings are written as characters prepended by the length of the
  string, boolean values are written using a special character pattern.

  @author  Stephen Anthony
  @version $Id$
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
    bool open(const string& fileName);

    /**
      Closes the current output stream.
    */
    void close(void);

    /**
      Answers whether the serializer is currently opened
    */
    bool isOpen(void);

    /**
      Writes an byte value (8-bit) to the current output stream.

      @param value The byte value to write to the output stream.
    */
    void putByte(char value);

    /**
      Writes an int value (32-bit) to the current output stream.

      @param value The int value to write to the output stream.
    */
    void putInt(int value);

    /**
      Writes a string to the current output stream.

      @param str The string to write to the output stream.
    */
    void putString(const string& str);

    /**
      Writes a boolean value to the current output stream.

      @param b The boolean value to write to the output stream.
    */
    void putBool(bool b);

  private:
    // The stream to send the serialized data to.
    fstream myStream;

    enum {
      TruePattern  = 0xfe,
      FalsePattern = 0x01
    };
};

#endif
