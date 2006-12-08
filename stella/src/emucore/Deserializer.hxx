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
// $Id: Deserializer.hxx,v 1.10 2006-12-08 16:49:24 stephena Exp $
//============================================================================

#ifndef DESERIALIZER_HXX
#define DESERIALIZER_HXX

#include <fstream>
#include "bspf.hxx"

/**
  This class implements a Deserializer device, whereby data is
  deserialized from an input binary file in a system-independent
  way.

  All ints should be cast to their appropriate data type upon method
  return.

  @author  Stephen Anthony
  @version $Id: Deserializer.hxx,v 1.10 2006-12-08 16:49:24 stephena Exp $
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
    bool open(const string& fileName);

    /**
      Closes the current input stream.
    */
    void close(void);

    /**
      Answers whether the deserializer is currently opened
    */
    bool isOpen(void);

    /**
      Reads an int value from the current input stream.

      @result The int value which has been read from the stream.
    */
    int getInt(void);

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
    fstream myStream;

    enum {
      TruePattern  = 0xfab1fab2,
      FalsePattern = 0xbad1bad2
    };
};

#endif
