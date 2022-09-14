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
// Copyright (c) 1995-2022 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "FSNodeFactory.hxx"
#include "FSNode.hxx"
#include "CartDetector.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNode::FSNode(const AbstractFSNodePtr& realNode)
  : _realNode{realNode}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNode::FSNode(const string& path)
{
  setPath(path);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FSNode::setPath(const string& path)
{
  // Only create a new object when necessary
  if (path == getPath())
    return;

  // Is this potentially a ZIP archive?
#if defined(ZIP_SUPPORT)
  if (BSPF::containsIgnoreCase(path, ".zip"))
    _realNode = FSNodeFactory::create(path, FSNodeFactory::Type::ZIP);
  else
#endif
    _realNode = FSNodeFactory::create(path, FSNodeFactory::Type::REGULAR);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNode& FSNode::operator/=(const string& path)
{
#if 0
  _fspath /= path;
  setFlags();

  return *this;
#else
  if (path != EmptyString)
  {
    string newPath = getPath();
    if (newPath != EmptyString && newPath[newPath.length()-1] != PATH_SEPARATOR)
      newPath += PATH_SEPARATOR;
    newPath += path;
    setPath(newPath);
  }

  return *this;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::getAllChildren(FSList& fslist, ListMode mode,
                            const NameFilter& filter,
                            bool includeParentDirectory,
                            const CancelCheck& isCancelled) const
{
  if(getChildren(fslist, mode, filter, includeParentDirectory, true, isCancelled))
  {
    // Sort only once at the end
  #if defined(ZIP_SUPPORT)
    // before sorting, replace single file ZIP archive names with contained
    // file names because they are displayed using their contained file names
    for(auto& i : fslist)
    {
      if(BSPF::endsWithIgnoreCase(i.getPath(), ".zip"))
      {
        const FSNodeZIP zipNode(i.getPath());
        i.setName(zipNode.getName());
      }
    }
  #endif

    std::sort(fslist.begin(), fslist.end(),
              [](const FSNode& node1, const FSNode& node2)
    {
      if(node1.isDirectory() != node2.isDirectory())
        return node1.isDirectory();
      else
        return BSPF::compareIgnoreCase(node1.getName(), node2.getName()) < 0;
    }
    );

  #if defined(ZIP_SUPPORT)
    // After sorting replace zip files with zip nodes
    for(auto& i : fslist)
    {
      if(BSPF::endsWithIgnoreCase(i.getPath(), ".zip"))
      {
        // Force ZIP c'tor to be called
        const AbstractFSNodePtr ptr = FSNodeFactory::create(
            i.getPath(), FSNodeFactory::Type::ZIP);
        const FSNode zipNode(ptr);
        i = zipNode;
      }
    }
  #endif
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::getChildren(FSList& fslist, ListMode mode,
                         const NameFilter& filter,
                         bool includeChildDirectories,
                         bool includeParentDirectory,
                         const CancelCheck& isCancelled) const
{
  if (!_realNode || !_realNode->isDirectory())
    return false;

  AbstractFSList tmp;
  tmp.reserve(fslist.capacity());

  if (!_realNode->getChildren(tmp, mode))
    return false;

  // when incuding child directories, everything must be sorted once at the end
  if(!includeChildDirectories)
  {
    if(isCancelled())
      return false;

  #if defined(ZIP_SUPPORT)
    // before sorting, replace single file ZIP archive names with contained
    // file names because they are displayed using their contained file names
    for(auto& i : tmp)
    {
      if(BSPF::endsWithIgnoreCase(i->getPath(), ".zip"))
      {
        const FSNodeZIP node(i->getPath());
        i->setName(node.getName());
      }
    }
  #endif

    std::sort(tmp.begin(), tmp.end(),
              [](const AbstractFSNodePtr& node1, const AbstractFSNodePtr& node2)
    {
      if(node1->isDirectory() != node2->isDirectory())
        return node1->isDirectory();
      else
        return BSPF::compareIgnoreCase(node1->getName(), node2->getName()) < 0;
    }
    );
  }

  // Add parent node, if it is valid to do so
  if (includeParentDirectory && hasParent())
  {
    FSNode parent = getParent();
    parent.setName("..");
    fslist.emplace_back(parent);
  }

  // And now add the rest of the entries
  for (const auto& i: tmp)
  {
    if(isCancelled())
      return false;

  #if defined(ZIP_SUPPORT)
    if (BSPF::endsWithIgnoreCase(i->getPath(), ".zip"))
    {
      // Force ZIP c'tor to be called
      const AbstractFSNodePtr ptr = FSNodeFactory::create(
          i->getPath(), FSNodeFactory::Type::ZIP);
      const FSNode zipNode(ptr);

      if(filter(zipNode))
      {
        if(!includeChildDirectories)
          fslist.emplace_back(zipNode);
        else
        {
          // filter by zip node but add file node
          const FSNode node(i);
          fslist.emplace_back(node);
        }
      }
    }
    else
  #endif
    {
      const FSNode node(i);

      if(includeChildDirectories)
      {
        if(i->isDirectory())
          node.getChildren(fslist, mode, filter, includeChildDirectories, false, isCancelled);
        else
          // do not add directories in this mode
          if(filter(node))
            fslist.emplace_back(node);
      }
      else
      {
        if(filter(node))
          fslist.emplace_back(node);
      }
    }
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& FSNode::getName() const
{
  return _realNode ? _realNode->getName() : EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FSNode::setName(const string& name)
{
  if (_realNode) _realNode->setName(name);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& FSNode::getPath() const
{
  return _realNode ? _realNode->getPath() : EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FSNode::getShortPath() const
{
  return _realNode ? _realNode->getShortPath() : EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FSNode::getNameWithExt(const string& ext) const
{
  fs::path p = getName();
  if (ext != EmptyString)
    p.replace_extension(ext);
  else
    p.replace_extension();

  return p.string();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FSNode::getPathWithExt(const string& ext) const
{
  fs::path p = getPath();
  if (ext != EmptyString)
    p.replace_extension(ext);
  else
    p.replace_extension();

  return p.string();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNode::read(ByteBuffer& buffer, size_t size) const
{
  // File must actually exist
  if (!(exists() && isReadable()))
    throw runtime_error("File not found/readable");

  size_t sizeRead = 0;
  if (_realNode && (sizeRead = _realNode->read(buffer, size)) > 0)
    return sizeRead;

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNode::read(stringstream& buffer) const
{
  // File must actually exist
  if (!(exists() && isReadable()))
    throw runtime_error("File not found/readable");

  size_t sizeRead = 0;
  if (_realNode && (sizeRead = _realNode->read(buffer)) > 0)
    return sizeRead;

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNode::write(const ByteBuffer& buffer, size_t size) const
{
  size_t sizeWritten = 0;
  if (_realNode && (sizeWritten = _realNode->write(buffer, size)) > 0)
    return sizeWritten;

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNode::write(const stringstream& buffer) const
{
  size_t sizeWritten = 0;
  if (_realNode && (sizeWritten = _realNode->write(buffer)) > 0)
    return sizeWritten;

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNode::getSize() const
{
  return (_realNode && _realNode->exists()) ? _realNode->getSize() : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::hasParent() const
{
  return _realNode ? _realNode->hasParent() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNode FSNode::getParent() const
{
  if (!_realNode)
    return *this;

  AbstractFSNodePtr parent_ptr = _realNode->getParent();
  return parent_ptr ? FSNode(parent_ptr) : *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::exists() const
{
  return _realNode ? _realNode->exists() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::isDirectory() const
{
  return _realNode ? _realNode->isDirectory() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::isFile() const
{
  return _realNode ? _realNode->isFile() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::isReadable() const
{
  return _realNode ? _realNode->isReadable() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::isWritable() const
{
  return _realNode ? _realNode->isWritable() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::makeDir()
{
  return (_realNode && !_realNode->exists()) ? _realNode->makeDir() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::rename(const string& newfile)
{
  return (_realNode && _realNode->exists()) ? _realNode->rename(newfile) : false;
}
