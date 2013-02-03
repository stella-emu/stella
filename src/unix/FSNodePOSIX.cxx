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

#include "FSNode.hxx"

#ifdef MACOSX
  #include <sys/types.h>
#endif

#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include <sstream>

#ifndef MAXPATHLEN // No MAXPATHLEN, as happens on Hurd
  #define MAXPATHLEN 1024
#endif

/*
 * Implementation of the Stella file system API based on POSIX (for Linux and OSX)
 *
 * Parts of this class are documented in the base interface class, AbstractFilesystemNode.
 */
class POSIXFilesystemNode : public AbstractFilesystemNode
{
  public:
    /**
     * Creates a POSIXFilesystemNode with the root node as path.
     */
    POSIXFilesystemNode();

    /**
     * Creates a POSIXFilesystemNode for a given path.
     *
     * @param path    String with the path the new node should point to.
     * @param verify  true if the isValid and isDirectory/isFile flags should
     *                be verified during the construction.
     */
    POSIXFilesystemNode(const string& path, bool verify = true);

    bool exists() const { return access(_path.c_str(), F_OK) == 0; }
    const string& getName() const   { return _displayName; }
    const string& getPath() const   { return _path; }
    string getShortPath() const;
    bool isDirectory() const { return _isDirectory; }
    bool isFile() const      { return _isFile;      }
    bool isReadable() const  { return access(_path.c_str(), R_OK) == 0; }
    bool isWritable() const  { return access(_path.c_str(), W_OK) == 0; }
    bool isAbsolute() const;
    bool makeDir();
    bool rename(const string& newfile);

    bool getChildren(AbstractFSList& list, ListMode mode, bool hidden) const;
    AbstractFilesystemNode* getParent() const;

  protected:
    string _displayName;
    string _path;
    bool _isDirectory;
    bool _isFile;
    bool _isValid;

  private:
    /**
     * Tests and sets the _isValid and _isDirectory/_isFile flags,
     * using the stat() function.
     */
    virtual void setFlags();
};

/**
 * Returns the last component of a given path.
 *
 * Examples:
 *			/foo/bar.txt would return /bar.txt
 *			/foo/bar/    would return /bar/
 *
 * @param str String containing the path.
 * @return Pointer to the first char of the last component inside str.
 */
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* lastPathComponent(const string& str)
{
  if(str.empty())
    return "";

  const char *start = str.c_str();
  const char *cur = start + str.size() - 2;

  while (cur >= start && *cur != '/')
    --cur;

  return cur + 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void POSIXFilesystemNode::setFlags()
{
  struct stat st;

  _isValid = (0 == stat(_path.c_str(), &st));
  if(_isValid)
  {
    _isDirectory = S_ISDIR(st.st_mode);
    _isFile = S_ISREG(st.st_mode);
  }
  else
    _isDirectory = _isFile = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
POSIXFilesystemNode::POSIXFilesystemNode()
{
  // The root dir.
  _path = "/";
  _displayName = _path;
  _isValid = true;
  _isDirectory = true;
  _isFile = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
POSIXFilesystemNode::POSIXFilesystemNode(const string& p, bool verify)
{
  // Default to home directory
  _path = p.length() > 0 ? p : "~";

  // Expand '~' to the HOME environment variable
  if(_path[0] == '~')
  {
    const char* home = getenv("HOME");
    if (home != NULL)
      _path.replace(0, 1, home);
  }

  // Get absolute path  
  char buf[MAXPATHLEN];
  if(realpath(_path.c_str(), buf))
    _path = buf;

  _displayName = lastPathComponent(_path);

  if (verify)
  {
    setFlags();

    // Add a trailing slash, if necessary
    if (_isDirectory && _path.length() > 0 && _path[_path.length()-1] != '/')
      _path += '/';
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string POSIXFilesystemNode::getShortPath() const
{
  // If the path starts with the home directory, replace it with '~'
  const char* home = getenv("HOME");
  if(home != NULL && BSPF_startsWithIgnoreCase(_path, home))
  {
    string path = "~";
    const char* offset = _path.c_str() + strlen(home);
    if(*offset != '/') path += "/";
    path += offset;
    return path;
  }
  return _path;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool POSIXFilesystemNode::getChildren(AbstractFSList& myList, ListMode mode,
                                      bool hidden) const
{
  assert(_isDirectory);

  DIR *dirp = opendir(_path.c_str());
  struct dirent *dp;

  if (dirp == NULL)
    return false;

  // loop over dir entries using readdir
  while ((dp = readdir(dirp)) != NULL)
  {
    // Skip 'invisible' files if necessary
    if (dp->d_name[0] == '.' && !hidden)
      continue;

    // Skip '.' and '..' to avoid cycles
    if ((dp->d_name[0] == '.' && dp->d_name[1] == 0) || (dp->d_name[0] == '.' && dp->d_name[1] == '.'))
      continue;

    string newPath(_path);
    if (newPath.length() > 0 && newPath[newPath.length()-1] != '/')
      newPath += '/';
    newPath += dp->d_name;

    POSIXFilesystemNode entry(newPath, false);

#if defined(SYSTEM_NOT_SUPPORTING_D_TYPE)
    /* TODO: d_type is not part of POSIX, so it might not be supported
     * on some of our targets. For those systems where it isn't supported,
     * add this #elif case, which tries to use stat() instead.
     *
     * The d_type method is used to avoid costly recurrent stat() calls in big
     * directories.
     */
    entry.setFlags();
#else
    if (dp->d_type == DT_UNKNOWN)
    {
      // Fall back to stat()
      entry.setFlags();
    }
    else
    {
      entry._isValid = (dp->d_type == DT_DIR) || (dp->d_type == DT_REG) || (dp->d_type == DT_LNK);
      if (dp->d_type == DT_LNK)
      {
        struct stat st;
        if (stat(entry._path.c_str(), &st) == 0)
        {
          entry._isDirectory = S_ISDIR(st.st_mode);
          entry._isFile = S_ISREG(st.st_mode);
        }
        else
          entry._isDirectory = entry._isFile = false;
      }
      else
      {
        entry._isDirectory = (dp->d_type == DT_DIR);
        entry._isFile = (dp->d_type == DT_REG);
      }
    }
#endif

    // Skip files that are invalid for some reason (e.g. because we couldn't
    // properly stat them).
    if (!entry._isValid)
      continue;

    // Honor the chosen mode
    if ((mode == FilesystemNode::kListFilesOnly && !entry._isFile) ||
        (mode == FilesystemNode::kListDirectoriesOnly && !entry._isDirectory))
      continue;

    if (entry._isDirectory)
      entry._path += "/";

    myList.push_back(new POSIXFilesystemNode(entry));
  }
  closedir(dirp);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool POSIXFilesystemNode::isAbsolute() const
{
  return _path.length() > 0 && (_path[0] == '~' || _path[0] == '/');
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool POSIXFilesystemNode::makeDir()
{
  if(mkdir(_path.c_str(), 0777) == 0)
  {
    // Get absolute path  
    char buf[MAXPATHLEN];
    if(realpath(_path.c_str(), buf))
      _path = buf;

    _displayName = lastPathComponent(_path);
    setFlags();

    // Add a trailing slash, if necessary
    if (_path.length() > 0 && _path[_path.length()-1] != '/')
      _path += '/';
    
    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool POSIXFilesystemNode::rename(const string& newfile)
{
  if(std::rename(_path.c_str(), newfile.c_str()) == 0)
  {
    _path = newfile;

    // Get absolute path  
    char buf[MAXPATHLEN];
    if(realpath(_path.c_str(), buf))
      _path = buf;

    _displayName = lastPathComponent(_path);
    setFlags();

    // Add a trailing slash, if necessary
    if (_isDirectory && _path.length() > 0 && _path[_path.length()-1] != '/')
      _path += '/';
    
    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* POSIXFilesystemNode::getParent() const
{
  if (_path == "/")
    return 0;

  const char *start = _path.c_str();
  const char *end = lastPathComponent(_path);

  return new POSIXFilesystemNode(string(start, end - start));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* AbstractFilesystemNode::makeFileNodePath(const string& path)
{
  return new POSIXFilesystemNode(path);
}
