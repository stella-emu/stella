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
// $Id: PropsSet.cxx,v 1.5 2002-05-09 16:58:04 gunfight Exp $
//============================================================================

#include <assert.h>
#include "Props.hxx"
#include "PropsSet.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::PropertiesSet()
{
  root = 0;
  mySize = 0;
  useMemList = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::~PropertiesSet()
{
  deleteNode(root);
  proStream.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::getMD5(string md5, Properties &properties)
{
  bool found = false;

  if(useMemList)
  {
    // Make sure tree isn't empty
    if(root == 0)
    {
      properties = defProps;
      return;
    }

    // Else, do a BST search for the node with the given md5
    TreeNode *current = root;

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
      properties = defProps;
  }
  else
  {
    // Loop reading properties until required properties found
    for(;;)
    {
      // Make sure the stream is still good or we're done 
      if(!proStream)
      {
        break;
      }

      // Get the property list associated with this profile
      Properties currentProperties(defProps);
      currentProperties.load(proStream);

      // If the stream is still good then insert the properties
      if(proStream)
      {
        string currentMd5 = currentProperties.get("Cartridge.MD5");

        if(currentMd5 == md5)
        {
          properties = currentProperties;
          return;
        }
      }
    }

    properties = defProps;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::insert(const Properties& properties)
{
	insertNode(root, properties);
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
    useMemList = useList;
    defProps   = defaults;

    // Cyberstella crashes without this:
    if(filename.length() <= 0)  return;
    // Cybergoth 09.05.02

    proStream.open(filename.c_str());

    if(useMemList)
    {
    // Loop reading properties
    for(;;)
    {
      // Make sure the stream is still good or we're done 
      if(!proStream)
      {
        break;
      }

      // Get the property list associated with this profile
      Properties properties(defProps);
      properties.load(proStream);

      // If the stream is still good then insert the properties
      if(proStream)
      {
        insert(properties);
      }
    }
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::save(ostream& out)
{
  saveNode(out, root);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::saveNode(ostream& out, TreeNode *node)
{
  if(node)
  {
    saveNode(out, node->left);
    saveNode(out, node->right);
    node->props->save(out);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 PropertiesSet::size() const
{
  return mySize;
}
