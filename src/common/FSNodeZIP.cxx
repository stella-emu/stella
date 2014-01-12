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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "bspf.hxx"
#include "OSystem.hxx"
#include "FSNodeFactory.hxx"
#include "FSNodeZIP.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeZIP::FilesystemNodeZIP()
{
  // We need a name, else the node is invalid
  _path = _shortPath = _virtualFile = "";
  _error = ZIPERR_NOT_A_FILE;
  _numFiles = 0;

  AbstractFSNode* tmp = 0;
  _realNode = Common::SharedPtr<AbstractFSNode>(tmp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeZIP::FilesystemNodeZIP(const string& p)
{
  _path = _shortPath = _virtualFile = "";
  _error = ZIPERR_NOT_A_FILE;

  // Extract ZIP file and virtual file (if specified)
  size_t pos = BSPF_findIgnoreCase(p, ".zip");
  if(pos == string::npos)
    return;

  _zipFile = p.substr(0, pos+4);

  // Open file at least once to initialize the virtual file count
  ZipHandler& zip = OSystem::zip(_zipFile);
  _numFiles = zip.romFiles();
  if(_numFiles == 0)
  {
    _error = ZIPERR_NO_ROMS;
    return;
  }

  // We always need a virtual file
  // Either one is given, or we use the first one
  if(pos+5 < p.length())
    _virtualFile = p.substr(pos+5);
  else if(_numFiles == 1)
  {
    bool found = false;
    while(zip.hasNext() && !found)
    {
      const std::string& file = zip.next();
      if(BSPF_endsWithIgnoreCase(file, ".a26") ||
         BSPF_endsWithIgnoreCase(file, ".bin") ||
         BSPF_endsWithIgnoreCase(file, ".rom"))
      {
        _virtualFile = file;
        found = true;
      }
    }
    if(!found)
      return;
  }

  AbstractFSNode* tmp =
    FilesystemNodeFactory::create(_zipFile, FilesystemNodeFactory::SYSTEM);
  _realNode = Common::SharedPtr<AbstractFSNode>(tmp);

  setFlags(_zipFile, _virtualFile, _realNode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeZIP::FilesystemNodeZIP(const string& zipfile, const string& virtualfile,
    Common::SharedPtr<AbstractFSNode> realnode)
{
  setFlags(zipfile, virtualfile, realnode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilesystemNodeZIP::setFlags(const string& zipfile,
                                 const string& virtualfile,
                                 Common::SharedPtr<AbstractFSNode> realnode)
{
  _zipFile = zipfile;
  _virtualFile = virtualfile;
  _realNode = realnode;

  _path = _realNode->getPath();
  _shortPath = _realNode->getShortPath();

  // Is a file component present?
  if(_virtualFile.size() != 0)
  {
    _path += ("/" + _virtualFile);
    _shortPath += ("/" + _virtualFile);
    _numFiles = 1;
  }
  _error = ZIPERR_NONE;
  if(!_realNode->isFile())
    _error = ZIPERR_NOT_A_FILE;
  if(!_realNode->isReadable())
    _error = ZIPERR_NOT_READABLE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeZIP::getChildren(AbstractFSList& myList, ListMode mode,
                                    bool hidden) const
{
  // Files within ZIP archives don't contain children
  if(!isDirectory() || _error != ZIPERR_NONE)
    return false;

  ZipHandler& zip = OSystem::zip(_zipFile);
  while(zip.hasNext())
  {
    FilesystemNodeZIP entry(_path, zip.next(), _realNode);
    myList.push_back(new FilesystemNodeZIP(entry));
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FilesystemNodeZIP::read(uInt8*& image) const
{
  switch(_error)
  {
    case ZIPERR_NONE:         break;
    case ZIPERR_NOT_A_FILE:   throw "ZIP file contains errors/not found";
    case ZIPERR_NOT_READABLE: throw "ZIP file not readable";
    case ZIPERR_NO_ROMS:      throw "ZIP file doesn't contain any ROMs";
  }

  ZipHandler& zip = OSystem::zip(_zipFile);

  bool found = false;
  while(zip.hasNext() && !found)
    found = zip.next() == _virtualFile;

  return found ? zip.decompress(image) : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNode* FilesystemNodeZIP::getParent() const
{
  return _realNode ? _realNode->getParent() : 0;
}
