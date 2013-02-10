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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"
#include "FSNodeFactory.hxx"
#include "FSNodeZIP.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeZIP::FilesystemNodeZIP()
{
  // We need a name, else the node is invalid
  _path = _shortPath = _virtualFile = "";
  _isValid = _isDirectory = _isFile = _isVirtual = false;

  AbstractFSNode* tmp = 0;
  _realNode = Common::SharedPtr<AbstractFSNode>(tmp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeZIP::FilesystemNodeZIP(const string& p)
{
  // Extract ZIP file and virtual file (if specified)
  size_t pos = BSPF_findIgnoreCase(p, ".zip");
  if(pos == string::npos)
  {
    // Not a ZIP file
    _path = _shortPath = _virtualFile = "";
    _isValid = _isDirectory = _isFile = _isVirtual = false;
    cerr << "Not a ZIP file\n";
    return;
  }

  _zipFile = p.substr(0, pos+4);

  // A ZIP file is, behind the scenes, still a real file in the filesystem
  // Hence, we need to create a real filesystem node for it
  AbstractFSNode* tmp = FilesystemNodeFactory::create(_zipFile,
                          FilesystemNodeFactory::SYSTEM);
  _realNode = Common::SharedPtr<AbstractFSNode>(tmp);
  _path = _realNode->getPath();
  _shortPath = _realNode->getShortPath();

  // Is a file component present?
  if(pos+5 < p.length())
  {
    _isVirtual = true;
    _virtualFile = p.substr(pos+5);
    _path += (BSPF_PATH_SEPARATOR + _virtualFile);
    _shortPath += (BSPF_PATH_SEPARATOR + _virtualFile);
  }
  else
  {
    _isVirtual = false;
    _virtualFile = "";
  }

cerr << "FilesystemNodeZIP: " << p << endl
  << "path: " << _path << endl
  << "spath: " << getShortPath() << endl
  << "zipFile: " << _zipFile << endl
  << "virtualFile: " << _virtualFile << endl
  << endl;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeZIP::getChildren(AbstractFSList& myList, ListMode mode,
                                    bool hidden) const
{
cerr << "getChildren: " << _path << endl;

  // Files within ZIP archives don't contain children
  if(_isVirtual)
    return false;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeZIP::isAbsolute() const
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNode* FilesystemNodeZIP::getParent() const
{
  return _realNode ? _realNode->getParent() : 0;
}
