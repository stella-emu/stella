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
// $Id: PropsSet.cxx,v 1.2 2002-01-08 17:11:32 stephena Exp $
//============================================================================

#include <assert.h>
#include "Props.hxx"
#include "PropsSet.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::PropertiesSet()
{
  root = 0;
  mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::~PropertiesSet()
{
  deleteNode(root);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties* PropertiesSet::getMD5(string md5)
{
  // Make sure tree isn't empty
  if(root == 0)
    return 0;

  // Else, do a BST search for the node with the given md5
  TreeNode *current = root;
  bool found = false;

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
    return current->props;
  else
    return 0;
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
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::load(istream& in, const Properties* defaults)
{
  // Loop reading properties
  for(;;)
  {
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
