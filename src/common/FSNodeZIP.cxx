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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <set>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "FSNodeFactory.hxx"
#include "FSNodeZIP.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeZIP::FilesystemNodeZIP()
  : _error(ZIPERR_NOT_A_FILE),
    _numFiles(0),
    _isDirectory(false),
    _isFile(false)
{
  // We need a name, else the node is invalid
  _realNode = shared_ptr<AbstractFSNode>(nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeZIP::FilesystemNodeZIP(const string& p)
  : _error(ZIPERR_NONE),
    _numFiles(0),
    _isDirectory(false),
    _isFile(false)
{
  // Is this a valid file?
  auto isFile = [](const string& file)
  {
    return BSPF_endsWithIgnoreCase(file, ".a26") ||
           BSPF_endsWithIgnoreCase(file, ".bin") ||
           BSPF_endsWithIgnoreCase(file, ".rom");
  };

  // Extract ZIP file and virtual file (if specified)
  size_t pos = BSPF_findIgnoreCase(p, ".zip");
  if(pos == string::npos)
    return;

  _zipFile = p.substr(0, pos+4);

  // Open file at least once to initialize the virtual file count
  ZipHandler& zip = open(_zipFile);
  _numFiles = zip.romFiles();
  if(_numFiles == 0)
  {
    _error = ZIPERR_NO_ROMS;
    return;
  }

  // We always need a virtual file/path
  // Either one is given, or we use the first one
  if(pos+5 < p.length())
  {
    _virtualPath = p.substr(pos+5);
    _isFile = isFile(_virtualPath);
    _isDirectory = !_isFile;
  }
  else if(_numFiles == 1)
  {
    bool found = false;
    while(zip.hasNext() && !found)
    {
      const string& file = zip.next();
      if(isFile(file))
      {
        _virtualPath = file;
        _isFile = true;

        found = true;
      }
    }
    if(!found)
      return;
  }
  else
    _isDirectory = true;

  AbstractFSNode* tmp =
    FilesystemNodeFactory::create(_zipFile, FilesystemNodeFactory::SYSTEM);
  _realNode = shared_ptr<AbstractFSNode>(tmp);

  setFlags(_zipFile, _virtualPath, _realNode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeZIP::FilesystemNodeZIP(
    const string& zipfile, const string& virtualpath,
    shared_ptr<AbstractFSNode> realnode, bool isdir)
  : _isDirectory(isdir),
    _isFile(!isdir)
{
  setFlags(zipfile, virtualpath, realnode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilesystemNodeZIP::setFlags(const string& zipfile,
                                 const string& virtualpath,
                                 shared_ptr<AbstractFSNode> realnode)
{
  _zipFile = zipfile;
  _virtualPath = virtualpath;
  _realNode = realnode;

  _path = _realNode->getPath();
  _shortPath = _realNode->getShortPath();

  // Is a file component present?
  if(_virtualPath.size() != 0)
  {
    _path += ("/" + _virtualPath);
    _shortPath += ("/" + _virtualPath);
    _name = lastPathComponent(_path);
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

  set<string> dirs;
  ZipHandler& zip = open(_zipFile);
  while(zip.hasNext())
  {
    // Only consider entries that start with '_virtualPath'
    const string& next = zip.next();
    if(BSPF_startsWithIgnoreCase(next, _virtualPath))
    {
      // First strip off the leading directory
      const string& curr = next.substr(_virtualPath == "" ? 0 : _virtualPath.size()+1);
      // Only add sub-directory entries once
      auto pos = curr.find_first_of("/\\");
      if(pos != string::npos)
        dirs.emplace(curr.substr(0, pos));
      else
        myList.emplace_back(new FilesystemNodeZIP(_zipFile, next, _realNode, false));
    }
  }
  for(const auto& dir: dirs)
  {
    // Prepend previous path
    const string& vpath = _virtualPath != "" ? _virtualPath + "/" + dir : dir;
    myList.emplace_back(new FilesystemNodeZIP(_zipFile, vpath, _realNode, true));
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FilesystemNodeZIP::read(uInt8*& image) const
{
  switch(_error)
  {
    case ZIPERR_NONE:         break;
    case ZIPERR_NOT_A_FILE:   throw runtime_error("ZIP file contains errors/not found");
    case ZIPERR_NOT_READABLE: throw runtime_error("ZIP file not readable");
    case ZIPERR_NO_ROMS:      throw runtime_error("ZIP file doesn't contain any ROMs");
  }

  ZipHandler& zip = open(_zipFile);

  bool found = false;
  while(zip.hasNext() && !found)
    found = zip.next() == _virtualPath;

  return found ? zip.decompress(image) : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNode* FilesystemNodeZIP::getParent() const
{
  if(_virtualPath == "")
    return _realNode ? _realNode->getParent() : nullptr;

  const char* start = _path.c_str();
  const char* end = lastPathComponent(_path);

  return new FilesystemNodeZIP(string(start, end - start - 1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<ZipHandler> FilesystemNodeZIP::myZipHandler = make_ptr<ZipHandler>();
