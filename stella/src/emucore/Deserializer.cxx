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
// $Id: Deserializer.cxx,v 1.7 2005-12-17 01:23:07 stephena Exp $
//============================================================================

#include "Deserializer.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Deserializer::Deserializer(void)
  : myStream(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Deserializer::~Deserializer(void)
{
  close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Deserializer::open(const string& fileName)
{
  close();
  myStream = new ifstream(fileName.c_str(), ios::in | ios::binary);

  return isOpen();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Deserializer::close(void)
{
  if(myStream)
  {
    if(myStream->is_open())
      myStream->close();

    delete myStream;
    myStream = (ifstream*) 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Deserializer::isOpen(void)
{
  return myStream && myStream->is_open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Deserializer::getInt(void)
{
  if(myStream->eof())
    throw "Deserializer: end of file";

  int val = 0;
  unsigned char buf[4];
  myStream->read((char*)buf, 4);
  for(int i = 0; i < 4; ++i)
    val += (int)(buf[i]) << (i<<3);

  return val;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Deserializer::getString(void)
{
  int len = getInt();
  string str;
  str.resize((string::size_type)len);
  myStream->read(&str[0], (streamsize)len);

  if(myStream->bad())
    throw "Deserializer: file read failed";

  return str;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Deserializer::getBool(void)
{
  bool result = false;

  int b = getInt();
  if(myStream->bad())
    throw "Deserializer: file read failed";

  if(b == (int)TruePattern)
    result = true;
  else if(b == (int)FalsePattern)
    result = false;
  else
    throw "Deserializer: data corruption";

  return result;
}
