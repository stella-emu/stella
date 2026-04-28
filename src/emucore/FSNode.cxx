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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <fstream>

#include "FSNodeFactory.hxx"
#include "FSNode.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNode::FSNode(const AbstractFSNodePtr& realNode)
  : _realNode{realNode}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNode::FSNode(string_view path)
{
  setPath(path);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FSNode::setPath(string_view path)
{
  // Only create a new object when necessary
  if(path == getPath())
    return;

  // Is this potentially a ZIP archive?
#ifdef ZIP_SUPPORT
  if(BSPF::containsIgnoreCase(path, ".zip"))
    _realNode = FSNodeFactory::create(path, FSNodeFactory::Type::ZIP);
  else
#endif
    _realNode = FSNodeFactory::create(path, FSNodeFactory::Type::SYSTEM);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNode& FSNode::operator/=(string_view path)
{
  if(path != EmptyString())
  {
    string newPath = getPath();
    newPath.reserve(newPath.size() + 1 + path.size());
    if(newPath != EmptyString() && newPath[newPath.length()-1] != PATH_SEPARATOR)
      newPath += PATH_SEPARATOR;
    newPath += path;
    setPath(newPath);
  }

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::exists() const
{
  return _realNode ? _realNode->exists() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::getAllChildren(FSList& fslist, ListMode mode,
                            const NameFilter& filter,
                            bool includeParentDirectory,
                            const CancelCheck& isCancelled) const
{
  if(getChildren(fslist, mode, filter, true, includeParentDirectory, isCancelled))
  {
    std::ranges::sort(fslist, [](const FSNode& node1, const FSNode& node2)
    {
      if(node1.isDirectory() != node2.isDirectory())
        return node1.isDirectory();
      else
        return BSPF::compareIgnoreCase(node1.getName(), node2.getName()) < 0;
    });

  #ifdef ZIP_SUPPORT
    // Convert system nodes for ZIP paths into proper FSNodeZIP nodes
    for(auto& i: fslist)
    {
      if(BSPF::endsWithIgnoreCase(i.getPath(), ".zip"))
      {
        const AbstractFSNodePtr ptr = FSNodeFactory::create(
            i.getPath(), FSNodeFactory::Type::ZIP);
        i = FSNode(ptr);
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
  if(!_realNode || !_realNode->isDirectory())
    return false;

  AbstractFSList tmp;
  tmp.reserve(fslist.capacity());

  if(!_realNode->getChildren(tmp, mode))
    return false;

  // When including child directories, everything must be sorted once at the end
  if(!includeChildDirectories)
  {
    if(isCancelled())
      return false;

    std::ranges::sort(tmp,
        [](const AbstractFSNodePtr& node1, const AbstractFSNodePtr& node2)
    {
      if(node1->isDirectory() != node2->isDirectory())
        return node1->isDirectory();
      else
        return BSPF::compareIgnoreCase(node1->getName(), node2->getName()) < 0;
    });
  }

  // Add parent node, if it is valid to do so
  if(includeParentDirectory && hasParent() && mode != ListMode::FilesOnly)
  {
    FSNode parent = getParent();
    parent.setName("..");
    fslist.emplace_back(std::move(parent));
  }

  // And now add the rest of the entries
  for(const auto& i: tmp)
  {
    if(isCancelled())
      return false;

  #ifdef ZIP_SUPPORT
    if(BSPF::endsWithIgnoreCase(i->getPath(), ".zip"))
    {
      // Force ZIP c'tor to be called
      const AbstractFSNodePtr ptr = FSNodeFactory::create(
          i->getPath(), FSNodeFactory::Type::ZIP);

      if(FSNode zipNode(ptr); filter(zipNode))
      {
        if(!includeChildDirectories)
          fslist.emplace_back(std::move(zipNode));
        else
        {
          // Filter by zip node but add the underlying file node
          FSNode node(i);
          fslist.emplace_back(std::move(node));
        }
      }
    }
    else
  #endif
    {
      if(FSNode node(i); includeChildDirectories)
      {
        if(i->isDirectory())
          node.getChildren(fslist, mode, filter, includeChildDirectories, false, isCancelled);
        else if(filter(node))
          fslist.emplace_back(std::move(node));
      }
      else
      {
        if(filter(node))
          fslist.emplace_back(std::move(node));
      }
    }
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& FSNode::getName() const
{
  return _realNode ? _realNode->getName() : EmptyString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FSNode::setName(string_view name)
{
  if(_realNode)
    _realNode->setName(name);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& FSNode::getPath() const
{
  return _realNode ? _realNode->getPath() : EmptyString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FSNode::getShortPath() const
{
  return _realNode ? _realNode->getShortPath() : EmptyString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNode FSNode::getSiblingNode(string_view ext) const
{
  if(_realNode)
    if(const auto sibling = _realNode->getSiblingNode(ext); sibling)
      return FSNode(sibling);

  string s = getPath();
  const size_t dot = s.find_last_of('.');
  if(dot != string::npos)
    s.replace(dot, string::npos, ext);
  else
    s.append(ext);

  return FSNode(s);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FSNode::getBaseName() const
{
  if(!_realNode)
    return {};

  const string& name = _realNode->getName();
  const size_t dot = name.find_last_of('.');
  return dot != string::npos ? name.substr(0, dot) : name;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FSNode::getNameWithExt(string_view ext) const
{
  if(!_realNode)
    return {};
  if(ext.empty())
    return _realNode->getName();

  string name = _realNode->getName();
  const size_t dot = name.find_last_of('.');
  if(dot != string::npos)
    name.replace(dot, string::npos, ext);
  else
    name.append(ext);

  return name;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FSNode::hasParent() const
{
  return _realNode ? _realNode->hasParent() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FSNode FSNode::getParent() const
{
  if(!_realNode)
    return *this;

  const AbstractFSNodePtr node = _realNode->getParent();
  return node ? FSNode(node) : *this;
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
bool FSNode::rename(string_view newfile)
{
  return (_realNode && _realNode->exists()) ? _realNode->rename(newfile) : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNode::getSize() const
{
  return (_realNode && _realNode->exists()) ? _realNode->getSize() : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNode::read(ByteBuffer& buffer, size_t size) const
{
  // File must actually exist
  if(!(exists() && isReadable()))
    throw std::runtime_error("File not found/readable");

  // First let the private subclass attempt to open the file
  if(_realNode)
    if(const auto sizeRead = _realNode->read(buffer, size); sizeRead > 0)
      return sizeRead;

  // Otherwise, the default behaviour is to read from a normal C++ ifstream
  auto in = openIFStream(std::ios::binary);
  if(!in)
    throw std::runtime_error("File open/read error");

  // Guard against seek failures
  in.seekg(0, std::ios::end);
  if(!in)
    throw std::runtime_error("File seek error");

  const std::streampos fileSize = in.tellg();
  if(fileSize <= 0)
    throw std::runtime_error("Zero-byte file");

  in.seekg(0, std::ios::beg);
  if(!in)
    throw std::runtime_error("File seek error");

  // If a requested size to read is provided (size > 0), honour it
  const auto sizeRead = (size > 0)
    ? std::min(static_cast<size_t>(fileSize), size)
    : static_cast<size_t>(fileSize);

  buffer = std::make_unique<uInt8[]>(sizeRead);
  in.read(reinterpret_cast<char*>(buffer.get()),
          static_cast<std::streamsize>(sizeRead));

  if(!in)
    throw std::runtime_error("File read error");

  return static_cast<size_t>(in.gcount());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNode::read(std::stringstream& buffer) const
{
  // File must actually exist
  if(!(exists() && isReadable()))
    throw std::runtime_error("File not found/readable");

  // First let the private subclass attempt to open the file
  if(_realNode)
    if(const auto sizeRead = _realNode->read(buffer); sizeRead > 0)
      return sizeRead;

  // Otherwise, the default behaviour is to read from a normal C++ ifstream
  // and pipe into the stringstream
  auto in = openIFStream();
  if(!in)
    throw std::runtime_error("File open/read error");

  // Get file size, guarding against seek failures
  in.seekg(0, std::ios::end);
  if(!in)
    throw std::runtime_error("File seek error");

  const std::streampos fileSize = in.tellg();
  if(fileSize <= 0)
    throw std::runtime_error("Zero-byte file");

  in.seekg(0, std::ios::beg);
  if(!in)
    throw std::runtime_error("File seek error");

  // Read into buffer and verify
  buffer << in.rdbuf();
  if(!in)
    throw std::runtime_error("File read error");

  return static_cast<size_t>(fileSize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNode::write(const ByteBuffer& buffer, size_t size) const
{
  size_t sizeWritten = 0;

  // First let the private subclass attempt to open the file
  if(_realNode)
    if(sizeWritten = _realNode->write(buffer, size); sizeWritten > 0)
      return sizeWritten;

  // Otherwise, the default behaviour is to write to a normal C++ ofstream
  auto out = openOFStream(std::ios::binary);
  if(out)
  {
    out.write(reinterpret_cast<const char*>(buffer.get()),
              static_cast<std::streamsize>(size));
    if(out)
      sizeWritten = size;
    else
      throw std::runtime_error("File write error");
  }
  else
    throw std::runtime_error("File open error");

  return sizeWritten;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t FSNode::write(string_view buffer) const
{
  size_t sizeWritten = 0;

  // First let the private subclass attempt to open the file
  if(_realNode)
    if(sizeWritten = _realNode->write(buffer); sizeWritten > 0)
      return sizeWritten;

  // Otherwise, the default behaviour is to write to a normal C++ ofstream
  auto out = openOFStream();
  if(out)
  {
    out.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    if(out)
      sizeWritten = buffer.size();
    else
      throw std::runtime_error("File write error");
  }
  else
    throw std::runtime_error("File open/write error");

  return sizeWritten;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::ifstream FSNode::openIFStream(std::ios::openmode mode) const
{
  return _realNode ? _realNode->openIFStream(mode) : std::ifstream{};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::ofstream FSNode::openOFStream(std::ios::openmode mode) const
{
  return _realNode ? _realNode->openOFStream(mode) : std::ofstream{};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::fstream FSNode::openFStream(std::ios::openmode mode) const
{
  return _realNode ? _realNode->openFStream(mode) : std::fstream{};
}
