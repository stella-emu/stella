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
// $Id: Serializer.cxx,v 1.1 2002-05-13 19:14:17 stephena Exp $
//============================================================================

#include <iostream>
#include <fstream>
#include <string>

#include "Serializer.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Serializer::Serializer(void)
{
  TruePattern = 0xfab1fab2;
  FalsePattern = 0xbad1bad2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Serializer::~Serializer(void)
{
  close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Serializer::open(string& fileName)
{
  close();
  myStream.open(fileName.c_str(), ios::out | ios::binary);

  return myStream.is_open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::close(void)
{
  if(myStream.is_open())
    myStream.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putLong(long value)
{
  myStream.write(reinterpret_cast<char *> (&value), sizeof (long));
  if(myStream.bad())
    throw "Serializer: file write failed";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putString(string& str)
{
  int len = str.length();
  putLong(len);
  myStream.write(str.data(), len);

  if(myStream.bad())
    throw "Serializer: file write failed";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putBool(bool b)
{
  long l = b ? TruePattern: FalsePattern;
  putLong(l);

  if(myStream.bad ())
    throw "Serializer: file write failed";
}
