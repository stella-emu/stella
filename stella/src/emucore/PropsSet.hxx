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
// $Id: PropsSet.hxx,v 1.8 2005-09-11 22:55:51 stephena Exp $
//============================================================================

#ifndef PROPERTIES_SET_HXX
#define PROPERTIES_SET_HXX

#include <fstream>

#include "bspf.hxx"

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

  public:
    /**
      Get the property from the set with the given MD5.

      @param md5         The md5 of the property to get
      @param properties  The property with the given MD5, or the default
                         properties if not found
    */
    void getMD5(const string& md5, Properties& properties);

    /** 
      Load properties from the specified file.  Use the given 
      defaults properties as the defaults for any properties loaded.

      @param filename  Full pathname of input file to use
    */
    void load(const string& filename);

    /**
      Save properties to the specified output stream 

      @param out  The output stream to use
    */
    void save(ostream& out);

    /**
      Get the number of properties in the collection.

      @return  The number of properties in the collection
    */
    uInt32 size() const;

    /**
      Prints the contents of the PropertiesSet as a flat file.
    */
    void print();

    /**
      Merge the given properties into the collection.

      @param properties  The properties to merge
      @param filename    Full pathname of properties file to save

      @return  True on success, false on failure
               Failure occurs if file couldn't be opened for writing
    */
    bool merge(const Properties& properties, const string& filename);

  private:
    struct TreeNode {
      Properties* props;
      TreeNode* left;
      TreeNode* right;
      int count;
    };

    /**
      Insert the properties into the set.  If a duplicate is inserted 
      the old properties are overwritten with the new ones.

      @param properties  The collection of properties
    */
    void insert(const Properties& properties);

    /**
      Insert a node in the bst, keeping the tree sorted.

      @param node        The current subroot of the tree
      @param properties  The collection of properties
    */
    void insertNode(TreeNode* &node, const Properties& properties);

    /**
      Deletes a node from the bst.  Does not preserve bst sorting.

      @param node  The current subroot of the tree
    */
    void deleteNode(TreeNode *node);

    /**
      Save current node properties to the specified output stream 

      @param out   The output stream to use
      @param node  The current subroot of the tree
    */
    void saveNode(ostream& out, TreeNode *node);

    /**
      Prints the current node properties

      @param node  The current subroot of the tree
    */
    void printNode(TreeNode *node);

    /**
      Get the default properties object to use for other properties objects

      @return  The default properties object
    */
    static const Properties& defaultProperties();

  private:
    // The root of the BST
    TreeNode* myRoot;

    // The size of the properties bst (i.e. the number of properties in it)
    uInt32 mySize;

    // The default properties set
    const Properties* myDefaultProperties;

    // Default properties to use for properties objects
    static Properties ourDefaultProperties;
};

#endif
