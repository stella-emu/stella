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
// $Id: Props.cxx,v 1.2 2002-01-08 17:11:32 stephena Exp $
//============================================================================

#include "Props.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties::Properties(const Properties* defaults)
{
  myDefaults = defaults;
  myCapacity = 16;
  myProperties = new Property[myCapacity];
  mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties::Properties(const Properties& properties)
{
  copy(properties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties::~Properties()
{
  // Free the properties array
  delete[] myProperties;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Properties::get(const string& key) const
{
  // Try to find the named property and answer its value
  for(uInt32 i = 0; i < mySize; ++i)
  {
    if(key == myProperties[i].key)
    {
      return myProperties[i].value;
    }
  }

  // Oops, property wasn't found so ask defaults if we have one
  if(myDefaults != 0)
  {
    // Ask the default properties object to find the key
    return myDefaults->get(key);
  } 
  else
  {
    // No default properties object so just return the empty string
    return "";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::set(const string& key, const string& value)
{
  // See if the property already exists
  for(uInt32 i = 0; i < mySize; ++i)
  {
    if(key == myProperties[i].key)
    {
      myProperties[i].value = value;
      return;
    }
  }

  // See if the array needs to be resized
  if(mySize == myCapacity)
  {
    // Yes, so we'll make the array twice as large
    Property* newProperties = new Property[myCapacity * 2];

    for(uInt32 i = 0; i < mySize; ++i)
    {
      newProperties[i] = myProperties[i];
    }

    delete[] myProperties;

    myProperties = newProperties;
    myCapacity *= 2;
  } 

  // Add new property to the array
  myProperties[mySize].key = key;
  myProperties[mySize].value = value;

  ++mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::load(istream& in)
{
  // Empty my property array
  mySize = 0;

  string line, key, value;
  uInt32 one, two, three, four, garbage;

  // Loop reading properties
  while(getline(in, line))
  {
    // Strip all tabs from the line
    while((garbage = line.find("\t")) != string::npos)
      line.erase(garbage, 1);

    // Ignore commented and empty lines
    if((line.length() == 0) || (line[0] == ';'))
      continue;

    // End of this record
    if(line == "\"\"") 
      break;

    one = line.find("\"", 0);
    two = line.find("\"", one + 1);
    three = line.find("\"", two + 1);
    four = line.find("\"", three + 1);

    // Invalid line if it doesn't contain 4 quotes
    if((one == string::npos) || (two == string::npos) ||
       (three == string::npos) || (four == string::npos))
      break;

    // Otherwise get the key and value
    key = line.substr(one + 1, two - one - 1);
    value = line.substr(three + 1, four - three - 1);

    // Set the property 
    set(key, value);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::save(ostream& out)
{
  // Write out each of the key and value pairs
  for(uInt32 i = 0; i < mySize; ++i)
  {
    writeQuotedString(out, myProperties[i].key);
    out.put(' ');
    writeQuotedString(out, myProperties[i].value);
    out.put('\n');
  }

  // Put a trailing null string so we know when to stop reading
  writeQuotedString(out, "");
  out.put('\n');
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Properties::readQuotedString(istream& in)
{
  char c;

  // Read characters until we see a quote
  while(in.get(c))
  {
    if(c == '"')
    {
      break;
    }
  }

  // Read characters until we see the close quote
  string s;
  while(in.get(c))
  {
    if((c == '\\') && (in.peek() == '"'))
    {
      in.get(c);
    }
    else if((c == '\\') && (in.peek() == '\\'))
    {
      in.get(c);
    }
    else if(c == '"')
    {
      break;
    }
    else if(c == '\r')
    {
      continue;
    }

    s += c;
  }

  return s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::writeQuotedString(ostream& out, const string& s)
{
  out.put('"');
  for(uInt32 i = 0; i < s.length(); ++i)
  {
    if(s[i] == '\\')
    {
      out.put('\\');
      out.put('\\');
    }
    else if(s[i] == '\"')
    {
      out.put('\\');
      out.put('"');
    }
    else
    {
      out.put(s[i]);
    }
  }
  out.put('"');
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties& Properties::operator = (const Properties& properties)
{
  // Do the assignment only if this isn't a self assignment
  if(this != &properties)
  {
    // Free the properties array
    delete[] myProperties;

    // Now, make myself a copy of the given object
    copy(properties);
  }

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::copy(const Properties& properties)
{
  // Remember the defaults to use
  myDefaults = properties.myDefaults;

  // Create an array of the same size as properties
  myCapacity = properties.myCapacity;
  myProperties = new Property[myCapacity];

  // Now, copy each property from properties
  mySize = properties.mySize;
  for(uInt32 i = 0; i < mySize; ++i)
  {
    myProperties[i] = properties.myProperties[i];
  }
}
