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
// $Id: PropsSet.cxx,v 1.6 2002-11-11 02:52:02 stephena Exp $
//============================================================================

#include <assert.h>

#include "Props.hxx"
#include "PropsSet.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::PropertiesSet()
   : myRoot(0), 
     mySize(0),
     myUseMemList(true),
     myPropertiesFilename(""),
     mySaveOnExit(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::~PropertiesSet()
{
  if(myPropertiesStream.is_open())
    myPropertiesStream.close();

  if(myUseMemList && mySaveOnExit && (myPropertiesFilename != ""))
  {
    ofstream out(myPropertiesFilename.c_str());
    if(out.is_open())
    {
      save(out);
      out.close();
    }
  }

  deleteNode(myRoot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::getMD5(string md5, Properties &properties)
{
  bool found = false;

  if(myUseMemList)
  {
    // Make sure tree isn't empty
    if(myRoot == 0)
    {
      properties = myDefaultProperties;
      return;
    }

    // Else, do a BST search for the node with the given md5
    TreeNode *current = myRoot;

    while(current)
    {
      string currentMd5 = current->props->get("Cartridge.MD5");

      if(currentMd5 == md5)
      {
        found = true;
        break;
      }
      else
      {
        if(md5 < currentMd5)
          current = current->left;
        else 
           current = current->right;
      }
    }

    if(found)
      properties = *(current->props);
    else
      properties = myDefaultProperties;
  }
  else
  {
    // Loop reading properties until required properties found
    for(;;)
    {
      // Make sure the stream is still good or we're done 
      if(!myPropertiesStream)
      {
        break;
      }

      // Get the property list associated with this profile
      Properties currentProperties(myDefaultProperties);
      currentProperties.load(myPropertiesStream);

      // If the stream is still good then insert the properties
      if(myPropertiesStream)
      {
        string currentMd5 = currentProperties.get("Cartridge.MD5");

        if(currentMd5 == md5)
        {
          properties = currentProperties;
          return;
        }
      }
    }

    properties = myDefaultProperties;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::insert(const Properties& properties)
{
	insertNode(myRoot, properties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::insertNode(TreeNode* &t, const Properties& properties)
{
  if(t)
  {
    string md5 = properties.get("Cartridge.MD5");
    string currentMd5 = t->props->get("Cartridge.MD5");

    if(md5 < currentMd5)
      insertNode(t->left, properties);
    else if(md5 > currentMd5)
      insertNode(t->right, properties);
    else
    {
      delete t->props;
      t->props = new Properties(properties);
    }
  }
  else
  {
    t = new TreeNode;
    t->props = new Properties(properties);
    t->left = 0;
    t->right = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::deleteNode(TreeNode *node)
{
  if(node)
  {
    deleteNode(node->left);
    deleteNode(node->right);
    delete node->props;
    delete node;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::load(string filename, const Properties* defaults, bool useList)
{
    myUseMemList = useList;
    myDefaultProperties = defaults;

    if(filename == "")
      return;

    myPropertiesStream.open(filename.c_str(), ios::in);

    if(myUseMemList)
    {
      // Loop reading properties
      for(;;)
      {
        // Make sure the stream is still good or we're done 
        if(!myPropertiesStream)
        {
          break;
        }

        // Get the property list associated with this profile
        Properties properties(myDefaultProperties);
        properties.load(myPropertiesStream);

        // If the stream is still good then insert the properties
        if(myPropertiesStream)
        {
          insert(properties);
        }
      }
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::save(ostream& out)
{
  saveNode(out, myRoot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::saveNode(ostream& out, TreeNode *node)
{
  if(node)
  {
    node->props->save(out);
    saveNode(out, node->left);
    saveNode(out, node->right);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 PropertiesSet::size() const
{
  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PropertiesSet::merge(Properties& properties, string& filename, bool saveOnExit)
{
  myPropertiesFilename = filename;
  mySaveOnExit = saveOnExit;

  // Can't merge the properties if the PropertiesSet isn't in memory
  if(!myUseMemList)
    return false;

  insert(properties);

  return true;
}
