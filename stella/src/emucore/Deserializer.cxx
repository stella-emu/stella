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
// $Id: Deserializer.cxx,v 1.1 2002-05-13 19:14:17 stephena Exp $
//============================================================================

#include <iostream>
#include <fstream>
#include <string>

#include "Deserializer.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Deserializer::Deserializer(void)
{
  TruePattern = 0xfab1fab2;
  FalsePattern = 0xbad1bad2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Deserializer::~Deserializer(void)
{
  close();
  if(myStream.is_open())
    myStream.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Deserializer::open(string& fileName)
{
  close();
  myStream.open(fileName.c_str(), ios::in | ios::binary);

  return myStream.is_open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Deserializer::close(void)
{
  if(myStream.is_open())
    myStream.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
long Deserializer::getLong(void)
{
  if(myStream.eof())
    throw "Deserializer: end of file";

  long l;
  myStream.read(reinterpret_cast<char *> (&l), sizeof (long));
  if(myStream.bad())
    throw "Deserializer: file read failed";

  return l;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Deserializer::getString(void)
{
  long len = getLong();
  string str;
  str.resize(len);
  myStream.read(&str[0], len);

  if(myStream.bad())
    throw "Deserializer: file read failed";

  return str;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Deserializer::getBool(void)
{
  bool result = false;

  long b = getLong();
  if(myStream.bad())
    throw "Deserializer: file read failed";

  if(b == TruePattern)
    result = true;
  else if(b == FalsePattern)
    result = false;
  else
    throw "Deserializer: data corruption";

  return result;
}
