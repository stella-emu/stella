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
// $Id: PropsSet.hxx,v 1.1.1.1 2001-12-27 19:54:23 bwmott Exp $
//============================================================================

#ifndef PROPERTIESSET_HXX
#define PROPERTIESSET_HXX

class Properties;

#include "bspf.hxx"
#include "Props.hxx"

/**
  This class maintains a sorted collection of properties.  Upon
  construction one property is distinguished as the key for sorting.
  
  @author  Bradford W. Mott
  @version $Id: PropsSet.hxx,v 1.1.1.1 2001-12-27 19:54:23 bwmott Exp $
*/
class PropertiesSet
{
  public:
    /**
      Create an empty properties set object using the specified
      property as the key for sorting.

      @param key The property to use as the key
    */
    PropertiesSet(const string& key);

    /**
      Create a properties set object by copying another one

      @param set The properties set to copy
    */
    PropertiesSet(const PropertiesSet& set);

    /**
      Destructor
    */
    virtual ~PropertiesSet();

  public:
    /**
      Get the i'th properties from the set

      @param i The index of the properties to get
      @return The properties stored at the i'th location
    */
    const Properties& get(uInt32 i);

    /**
      Insert the properties into the set.  If a duplicate is inserted 
      the old properties are overwritten with the new ones.

      @param properties The collection of properties
    */
    void insert(const Properties& properties);

    /**
      Get the number of properties in the collection.

      @return The number of properties in the collection
    */
    uInt32 size() const;

    /**
      Erase the i'th properties from the collection.

      @param i The profile index
    */
    void erase(uInt32 i);

  public:
    /** 
      Load properties from the specified input stream.  Use the given 
      defaults properties as the defaults for any properties loaded.

      @param in The input stream to use
      @param defaults The default properties to use
    */
    void load(istream& in, const Properties* defaults);
 
    /**
      Save properties to the specified output stream 

      @param out The output stream to use
    */
    void save(ostream& out);

  public:
    /**
      Overloaded assignment operator

      @param propertiesSet The properties set to set myself equal to
      @return Myself after assignment has taken place
    */
    PropertiesSet& operator = (const PropertiesSet& propertiesSet);

  private:
    // Property to use as the key
    string myKey;

    // Pointer to a dynamically allocated array of properties
    Properties* myProperties;

    // Current capacity of the properties array 
    unsigned int myCapacity;

    // The size of the properties array (i.e. the number of properties in it)
    unsigned int mySize;
};
#endif

