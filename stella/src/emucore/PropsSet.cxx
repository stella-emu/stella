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
// $Id: PropsSet.cxx,v 1.21 2006-03-19 00:46:04 stephena Exp $
//============================================================================

#include "OSystem.hxx"
#include "GuiUtils.hxx"
#include "DefProps.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::PropertiesSet(OSystem* osystem)
  : myOSystem(osystem),
    myRoot(NULL),
    mySize(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::~PropertiesSet()
{
  deleteNode(myRoot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::getMD5(const string& md5, Properties &properties)
{
  properties.setDefaults();
  bool found = false;

  // First check our dynamic BST for the object
  if(myRoot != 0)
  {
    bool found = false;
    TreeNode* current = myRoot;

    while(current)
    {
      string currentMd5 = current->props->get(Cartridge_MD5);

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
  }

  // Otherwise, search the internal BST array
  if(!found)
  {
    int i = 0;
    while(i < ARRAYSIZE(DefProps))
    {
      int cmp = strncmp(md5.c_str(), DefProps[i][Cartridge_MD5], 32);
      if(cmp == 0)
      {
        for(int p = 0; p < LastPropType; ++p)
          if(DefProps[i][p][0] != 0)
            properties.set((PropertyType)p, DefProps[i][p]);

        found = true;
        break;
      }
      else if(cmp < 0)
        i = 2*i + 1;   // left child
      else
        i = 2*i + 2;   // right child
    }
  }

  // Reset TIA positioning to defaults if option is enabled
  if(myOSystem->settings().getBool("tiadefaults"))
  {
    properties.set(Display_XStart, Properties::ourDefaultProperties[Display_XStart]);
    properties.set(Display_Width,  Properties::ourDefaultProperties[Display_Width]);
    properties.set(Display_YStart, Properties::ourDefaultProperties[Display_YStart]);
    properties.set(Display_Height, Properties::ourDefaultProperties[Display_Height]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::insert(const Properties& properties, bool save)
{
  insertNode(myRoot, properties, save);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::insertNode(TreeNode* &t, const Properties& properties,
                               bool save)
{
  if(t)
  {
    string md5 = properties.get(Cartridge_MD5);
    string currentMd5 = t->props->get(Cartridge_MD5);

    if(md5 < currentMd5)
      insertNode(t->left, properties, save);
    else if(md5 > currentMd5)
      insertNode(t->right, properties, save);
    else
    {
      delete t->props;
      t->props = new Properties(properties);
      t->save = save;
    }
  }
  else
  {
    t = new TreeNode;
    t->props = new Properties(properties);
    t->left = 0;
    t->right = 0;
    t->save = save;

    ++mySize;
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
void PropertiesSet::load(const string& filename, bool save)
{
  ifstream in(filename.c_str(), ios::in);

  // Loop reading properties
  for(;;)
  {
    // Make sure the stream is still good or we're done 
    if(!in)
      break;

    // Get the property list associated with this profile
    Properties prop;
    prop.load(in);

    // If the stream is still good then insert the properties
    if(in)
      insert(prop, save);
  }
  if(in)
    in.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::save(ostream& out)
{
  saveNode(out, myRoot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::print()
{
  cout << size() << endl;
  printNode(myRoot);  // FIXME - print out internal properties as well
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::saveNode(ostream& out, TreeNode *node)
{
  if(node)
  {
    if(node->save)
      node->props->save(out);
    saveNode(out, node->left);
    saveNode(out, node->right);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::printNode(TreeNode *node)
{
  if(node)
  {
    node->props->print();
    printNode(node->left);
    printNode(node->right);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 PropertiesSet::size() const
{
  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PropertiesSet::merge(const Properties& properties, const string& filename)
{
  ofstream out(filename.c_str());
  if(out.is_open())
  {
    insert(properties, true);  // always save merged properties
    save(out);
    out.close();
    return true;
  }
  else
    return false;
}
