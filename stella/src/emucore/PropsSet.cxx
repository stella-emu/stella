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
// $Id: PropsSet.cxx,v 1.1.1.1 2001-12-27 19:54:23 bwmott Exp $
//============================================================================

#include <assert.h>
#include "Props.hxx"
#include "PropsSet.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::PropertiesSet(const string& key)
    : myKey(key)
{
  myCapacity = 16;
  myProperties = new Properties[myCapacity];
  mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::PropertiesSet(const PropertiesSet& p)
    : myKey(p.myKey)
{
  myCapacity = p.myCapacity;
  myProperties = new Properties[myCapacity];
  mySize = p.mySize;

  // Copy the properties from the other set
  for(uInt32 i = 0; i < mySize; ++i)
  {
    myProperties[i] = p.myProperties[i];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::~PropertiesSet()
{
  delete[] myProperties;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Properties& PropertiesSet::get(uInt32 i)
{
  // Make sure index is within range
  assert(i < mySize);

  return myProperties[i]; 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::insert(const Properties& properties)
{
  uInt32 i;
  uInt32 j;

  // Get the key of the properties
  string name = properties.get(myKey);

  // See if the key already exists (we could use a binary search here...)
  for(i = 0; i < mySize; ++i)
  {
    if(name == myProperties[i].get(myKey))
    {
      // Copy the properties which are being inserted
      myProperties[i] = properties;
      return;
    }
  }

  // See if the properties array needs to be resized
  if(mySize == myCapacity)
  {
    Properties* newProperties = new Properties[myCapacity *= 2];

    for(i = 0; i < mySize; ++i)
    {
      newProperties[i] = myProperties[i];
    }

    delete[] myProperties;

    myProperties = newProperties;
  }

  // Find the correct place to insert the properties at
  for(i = 0; (i < mySize) && (myProperties[i].get(myKey) < name); ++i);

  // Okay, make room for the properties
  for(j = mySize; j > i; --j)
  {
    myProperties[j] = myProperties[j - 1];
  }
 
  // Now, put the properties in the array
  myProperties[i] = properties;

  ++mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 PropertiesSet::size() const
{
  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::erase(uInt32 i)
{
  // Make sure index is within range
  assert(i < mySize);

  for(uInt32 j = i + 1; j < mySize; ++j)
  {
    myProperties[j - 1] = myProperties[j];
  }

  --mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::load(istream& in, const Properties* defaults)
{
  // Empty my properties array
  mySize = 0;

  // Loop reading properties
  for(;;)
  {
    // Read char's until we see a quote as the next char or EOF is reached
    while(in && (in.peek() != '"'))
    {
      char c;
      in.get(c);

      // If we see the comment character then ignore the line
      if(c == ';')
      {
        while(in && (c != '\n'))
        {
          in.get(c);
        }
      }
    }
   
    // Make sure the stream is still good or we're done 
    if(!in)
    {
      break;
    }

    // Get the property list associated with this profile
    Properties properties(defaults);
    properties.load(in);

    // If the stream is still good then insert the properties
    if(in)
    {
      insert(properties);
    }
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::save(ostream& out)
{
  // Write each of the properties out
  for(uInt32 i = 0; i < mySize; ++i)
  {
    myProperties[i].save(out);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet& PropertiesSet::operator = (const PropertiesSet& p)
{
  if(this != &p)
  {
    delete[] myProperties;

    myKey = p.myKey;
    myCapacity = p.myCapacity;
    myProperties = new Properties[myCapacity];
    mySize = p.mySize;

    // Copy the properties from the other set
    for(uInt32 i = 0; i < mySize; ++i)
    {
      myProperties[i] = p.myProperties[i];
    }
  }

  return *this;
}

