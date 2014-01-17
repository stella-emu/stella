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

#ifndef FS_NODE_POSIX_HXX
#define FS_NODE_POSIX_HXX

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
 * Parts of this class are documented in the base interface class, AbstractFSNode.
 */
class FilesystemNodePOSIX : public AbstractFSNode
{
  public:
    /**
     * Creates a FilesystemNodePOSIX with the root node as path.
     */
    FilesystemNodePOSIX();

    /**
     * Creates a FilesystemNodePOSIX for a given path.
     *
     * @param path    String with the path the new node should point to.
     * @param verify  true if the isValid and isDirectory/isFile flags should
     *                be verified during the construction.
     */
    FilesystemNodePOSIX(const string& path, bool verify = true);

    bool exists() const { return access(_path.c_str(), F_OK) == 0; }
    const string& getName() const   { return _displayName; }
    const string& getPath() const   { return _path; }
    string getShortPath() const;
    bool isDirectory() const { return _isDirectory; }
    bool isFile() const      { return _isFile;      }
    bool isReadable() const  { return access(_path.c_str(), R_OK) == 0; }
    bool isWritable() const  { return access(_path.c_str(), W_OK) == 0; }
    bool makeDir();
    bool rename(const string& newfile);

    bool getChildren(AbstractFSList& list, ListMode mode, bool hidden) const;
    AbstractFSNode* getParent() const;

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

#endif
