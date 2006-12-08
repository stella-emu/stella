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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FSNodePSP.cxx,v 1.2 2006-12-08 16:49:37 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "FSNode.hxx"

#include <pspdebug.h>
#include <pspiofilemgr.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>

/*
 * Implementation of the Stella file system API based on POSIX for PSP
 */

class PSPFilesystemNode : public AbstractFilesystemNode
{
  public:
    PSPFilesystemNode();
    PSPFilesystemNode(const string& path);
    PSPFilesystemNode(const PSPFilesystemNode* node);

    virtual string displayName() const { return _displayName; }
    virtual bool isValid() const { return _isValid; }
    virtual bool isDirectory() const { return _isDirectory; }
    virtual string path() const { return _path; }

    virtual FSList listDir(ListMode mode = kListDirectoriesOnly) const;
    virtual AbstractFilesystemNode* parent() const;
    static void stripTailingSlashes(char * buf);
    protected:
    string _displayName;
    bool _isDirectory;
    bool _isValid;
    string _path;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const char* lastPathComponent(const string& str)
{
  const char *start = str.c_str();
  const char *cur = start + str.size() - 2;

  while (cur > start && *cur != '/')
    --cur;

  return cur+1;
}

static void  stripTailingSlashes(char * buf)
{
    char * ptr;
    ptr = buf + strlen(buf)-1;
    while(*(ptr)=='/') *(ptr--)='\0';
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static string validatePath(const string& p)
{
  string path = p;
  if(p.size() <= 0 || p[0] == '/')
    path = "ms0:/";

  return path;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* FilesystemNode::getRoot()
{
  return new PSPFilesystemNode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode* FilesystemNode::getNodeForPath(const string& path)
{
  return new PSPFilesystemNode(validatePath(path));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PSPFilesystemNode::PSPFilesystemNode()
{
    const char buf[] = "ms0:/stella/";
    _path = buf;
    _displayName = string("stella");
    _isValid = true;
    _isDirectory = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PSPFilesystemNode::PSPFilesystemNode(const string& p)
{
  string path = validatePath(p);

  Int32 len = 0, offset = path.size();
  SceIoStat st;

  _path = path;

  // Extract last component from path
  const char *str = path.c_str();
  while (offset > 0 && str[offset-1] == '/')
    offset--;
  while (offset > 0 && str[offset-1] != '/')
  {
    len++;
    offset--;
  }
  _displayName = string(str + offset, len);

  // Check whether it is a directory, and whether the file actually exists
  //_isValid = (0 == stat(_path.c_str(), &st));
  //_isDirectory = S_ISDIR(st.st_mode);
  _isValid = (0 == sceIoGetstat(_path.c_str(), &st));
  _isDirectory = FIO_S_ISDIR(st.st_mode);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PSPFilesystemNode::PSPFilesystemNode(const PSPFilesystemNode* node)
{
  _displayName = node->_displayName;
  _isValid = node->_isValid;
  _isDirectory = node->_isDirectory;
  _path = node->_path;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSList PSPFilesystemNode::listDir(ListMode mode) const
{
//     assert(_isDirectory);
    FSList myList;
    SceUID  dfd = sceIoDopen (_path.c_str());
    SceIoDirent *dp;
    dp = (SceIoDirent*)malloc(sizeof(SceIoDirent));
#ifdef PSP_DEBUG
    fprintf(stdout,"PSPFilesystemNode::listDir: dir='%s'\n",_path.c_str());
#endif

    if (!dfd){
#ifdef PSP_DEBUG
        fprintf(stdout,"PSPFilesystemNode::listDir: no dir handle\n");
#endif
        return myList;
    }

    while (sceIoDread(dfd,dp) > 0){

        if (dp->d_name[0]=='.')
            continue;

        PSPFilesystemNode entry;
        entry._displayName = dp->d_name;
        entry._path = _path;
        if (entry._path.length() > 0 && entry._path[entry._path.length()-1] != '/')
            entry._path += "/";

        entry._path += dp->d_name;
        entry._isDirectory =  dp->d_stat.st_attr & FIO_SO_IFDIR;

        // Honor the chosen mode
        if ((mode == kListFilesOnly && entry._isDirectory) ||
            (mode == kListDirectoriesOnly && !entry._isDirectory))
            continue;

        if (entry._isDirectory)
            entry._path += "/";

            myList.push_back(wrap(new PSPFilesystemNode(&entry)));
    }
    sceIoDclose(dfd);
    free(dp);
    return myList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFilesystemNode *PSPFilesystemNode::parent() const
{
  if (_path == "/")
    return 0;

  PSPFilesystemNode* p = new PSPFilesystemNode();
  const char *start = _path.c_str();
  const char *end = lastPathComponent(_path);

  p->_path = string(start, end - start);
  p->_displayName = lastPathComponent(p->_path);

  p->_isValid = true;
  p->_isDirectory = true;

  return p;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AbstractFilesystemNode::fileExists(const string& path)
{
    SceIoStat st;
#ifdef PSP_DEBUG
    fprintf(stdout,"AbstractFilesystemNode::fileExists '%s'\n",path.c_str());
#endif
    if(sceIoGetstat(path.c_str(), &st) != 0){
#ifdef PSP_DEBUG
        fprintf(stdout,"AbstractFilesystemNode::fileExists error \n");
#endif
        return false;
    }
#ifdef PSP_DEBUG
    fprintf(stdout,"AbstractFilesystemNode::fileExists return '%i'\n", !FIO_SO_ISREG(st.st_mode));
#endif
    return !FIO_SO_ISREG(st.st_mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AbstractFilesystemNode::dirExists(const string& in)
{
    char tmp_buf[1024];
    strncpy(tmp_buf,in.c_str(),1023);
    stripTailingSlashes(tmp_buf);
    string path = (char*)tmp_buf;
#ifdef PSP_DEBUG
    fprintf(stdout,"AbstractFilesystemNode::dirExists '%s'\n", path.c_str());
#endif
    SceIoStat st;
    if(sceIoGetstat(path.c_str(), &st) != 0)
        return false;
#ifdef PSP_DEBUG
    fprintf(stdout,"AbstractFilesystemNode::dirExists return '%i'\n", !FIO_SO_ISDIR(st.st_mode));
#endif
    return !FIO_SO_ISDIR(st.st_mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AbstractFilesystemNode::makeDir(const string& path)
{
    return sceIoMkdir(path.c_str(), 0777) == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string AbstractFilesystemNode::modTime(const string& in)
{
    char tmp_buf[1024];
    strncpy(tmp_buf,in.c_str(),1023);
    stripTailingSlashes(tmp_buf);
    string path = (char*)tmp_buf;
    SceIoStat st;
#ifdef PSP_DEBUG
    fprintf(stdout,"AbstractFilesystemNode::modTime '%s'\n",path.c_str());
#endif

    if(sceIoGetstat(path.c_str(), &st) < 0){
#ifdef PSP_DEBUG
        fprintf(stdout,"AbstractFilesystemNode::modTime returns error\n");
#endif
        return "";
    }
   ostringstream buf;
    buf     << (unsigned short)st.st_mtime.year
            << (unsigned short)st.st_mtime.month
            << (unsigned short)st.st_mtime.day
            << (unsigned short)st.st_mtime.hour
            << (unsigned short)st.st_mtime.minute
            << (unsigned short)st.st_mtime.second;

#ifdef PSP_DEBUG
    fprintf(stdout,"AbstractFilesystemNode::modTime returns '%s'\n",buf.str().c_str());
#endif
    return buf.str();
}
