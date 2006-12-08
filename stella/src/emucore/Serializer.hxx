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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Serializer.hxx,v 1.11 2006-12-08 16:49:28 stephena Exp $
//============================================================================

#ifndef SERIALIZER_HXX
#define SERIALIZER_HXX

#include <fstream>
#include "bspf.hxx"

/**
  This class implements a Serializer device, whereby data is
  serialized and sent to an output binary file in a system-
  independent way.

  All bytes and integers are written as int's.  Strings are
  written as characters prepended by the length of the string.
  Boolean values are written using a special pattern.

  @author  Stephen Anthony
  @version $Id: Serializer.hxx,v 1.11 2006-12-08 16:49:28 stephena Exp $
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
      Writes an int value to the current output stream.

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
      TruePattern  = 0xfab1fab2,
      FalsePattern = 0xbad1bad2
    };
};

#endif
