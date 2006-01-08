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
// $Id: PropsSet.cxx,v 1.15 2006-01-08 02:28:03 stephena Exp $
//============================================================================

#include <assert.h>

#include "Props.hxx"
#include "PropsSet.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::PropertiesSet()
   : myRoot(NULL), 
     mySize(0)
{
  myDefaultProperties = &defaultProperties();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::~PropertiesSet()
{
  deleteNode(myRoot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::getMD5(const string& md5, Properties &properties)
{
  bool found = false;

  // Make sure tree isn't empty
  if(myRoot == 0)
  {
	properties = myDefaultProperties;
	return;
  }

  // Else, do a BST search for the node with the given md5
  TreeNode* current = myRoot;

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
    string md5 = properties.get("Cartridge.MD5");
    string currentMd5 = t->props->get("Cartridge.MD5");

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
    Properties properties(myDefaultProperties);
    properties.load(in);

    // If the stream is still good then insert the properties
    if(in)
      insert(properties, save);
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
  printNode(myRoot);
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Properties& PropertiesSet::defaultProperties()
{
  // Make sure the <key,value> pairs are in the default properties object
  ourDefaultProperties.set("Cartridge.Filename", "");
  ourDefaultProperties.set("Cartridge.MD5", "");
  ourDefaultProperties.set("Cartridge.Manufacturer", "");
  ourDefaultProperties.set("Cartridge.ModelNo", "");
  ourDefaultProperties.set("Cartridge.Name", "Untitled");
  ourDefaultProperties.set("Cartridge.Note", "");
  ourDefaultProperties.set("Cartridge.Rarity", "");
  ourDefaultProperties.set("Cartridge.Sound", "Mono");
  ourDefaultProperties.set("Cartridge.Type", "Auto-detect");

  ourDefaultProperties.set("Console.LeftDifficulty", "B");
  ourDefaultProperties.set("Console.RightDifficulty", "B");
  ourDefaultProperties.set("Console.TelevisionType", "Color");
  ourDefaultProperties.set("Console.SwapPorts", "No");

  ourDefaultProperties.set("Controller.Left", "Joystick");
  ourDefaultProperties.set("Controller.Right", "Joystick");

  ourDefaultProperties.set("Display.Format", "NTSC");
  ourDefaultProperties.set("Display.XStart", "0");
  ourDefaultProperties.set("Display.Width", "160");
  ourDefaultProperties.set("Display.YStart", "34");
  ourDefaultProperties.set("Display.Height", "210");

  ourDefaultProperties.set("Emulation.CPU", "Auto-detect");
  ourDefaultProperties.set("Emulation.HmoveBlanks", "Yes");

  return ourDefaultProperties;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties PropertiesSet::ourDefaultProperties;
