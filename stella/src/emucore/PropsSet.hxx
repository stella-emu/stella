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
// $Id: PropsSet.hxx,v 1.2 2002-01-08 17:11:32 stephena Exp $
//============================================================================

#ifndef PROPERTIESSET_HXX
#define PROPERTIESSET_HXX

#include <string>

#include "bspf.hxx"
#include "Props.hxx"

class Properties;

/**
  This class maintains a sorted collection of properties.  The objects
  are maintained in a binary search tree sorted by md5, since this is
  the attribute most likely to be present in each entry in stella.pro
  and least likely to change.  A change in MD5 would mean a change in
  the game rom image (essentially a different game) and this would
  necessitate a new entry in the stella.pro file anyway.
  
  @author  Stephen Anthony
*/
class PropertiesSet
{
  public:
    /**
      Create an empty properties set object using the md5 as the
      key to the BST.
    */
    PropertiesSet();

    /**
      Destructor
    */
    virtual ~PropertiesSet();

    /**
      Get the property from the set with the given MD5.

      @param md5 The md5 of the property to get
      @return The property with the given MD5, or 0 if not found
    */
    Properties* getMD5(string md5);

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

    /**
      Get the number of properties in the collection.

      @return The number of properties in the collection
    */
    uInt32 size() const;

  private:

	struct TreeNode
	{
   	    Properties *props;
		TreeNode *left;
		TreeNode *right;

	};

    /**
      Insert the properties into the set.  If a duplicate is inserted 
      the old properties are overwritten with the new ones.

      @param properties The collection of properties
    */
    void insert(const Properties& properties);

    /**
      Insert a node in the bst, keeping the tree sorted.

      @param node The current subroot of the tree
      @param properties The collection of properties
    */
    void insertNode(TreeNode* &node, const Properties& properties);

    /**
      Deletes a node from the bst.  Does not preserve bst sorting.

      @param node The current subroot of the tree
    */
    void deleteNode(TreeNode *node);

    /**
      Save current node properties to the specified output stream 

      @param out The output stream to use
      @param node The current subroot of the tree
    */
    void saveNode(ostream& out, TreeNode *node);

    // The root of the BST
    TreeNode* root;

    // Property to use as the key
    string myKey;

    // The size of the properties bst (i.e. the number of properties in it)
    uInt32 mySize;
};
#endif
