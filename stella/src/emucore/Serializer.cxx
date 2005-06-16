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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Serializer.cxx,v 1.5 2005-06-16 01:11:28 stephena Exp $
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

  myStream = (ofstream*) 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Serializer::~Serializer(void)
{
  close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Serializer::open(const string& fileName)
{
  close();
  myStream = new ofstream(fileName.c_str(), ios::out | ios::binary);

  return (myStream && myStream->is_open());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::close(void)
{
  if(myStream)
  {
    if(myStream->is_open())
      myStream->close();

    delete myStream;
    myStream = (ofstream*) 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putLong(long value)
{
  myStream->write(reinterpret_cast<char *> (&value), sizeof (long));
  if(myStream->bad())
    throw "Serializer: file write failed";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putString(const string& str)
{
  int len = str.length();
  putLong(len);
  myStream->write(str.data(), len);

  if(myStream->bad())
    throw "Serializer: file write failed";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putBool(bool b)
{
  long l = b ? TruePattern: FalsePattern;
  putLong(l);

  if(myStream->bad ())
    throw "Serializer: file write failed";
}
