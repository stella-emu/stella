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

#ifndef FS_NODE_LIBRETRO_HXX
#define FS_NODE_LIBRETRO_HXX

#include <optional>

#include "FSNode.hxx"

/*
 * Implementation of the Stella file system API based on the libretro VFS
 * (Virtual File System) interface.  Falls back to standard C++ file streams
 * for actual I/O, which work on all platforms where RetroArch provides real
 * filesystem paths.
 *
 * Parts of this class are documented in the base interface class,
 * AbstractFSNode.
 */
class FSNodeLIBRETRO : public AbstractFSNode
{
  public:
    /**
     * Creates a FSNodeLIBRETRO with the save directory (or current
     * directory as fallback) as the root path.
     */
    FSNodeLIBRETRO();

    /**
     * Creates a FSNodeLIBRETRO for a given path.
     *
     * @param path    String with the path the new node should point to.
     * @param verify  true if the isValid and isDirectory/isFile flags should
     *                be verified during the construction.
     */
    explicit FSNodeLIBRETRO(string_view path, bool verify = true);

    bool exists() const override;
    const string& getName() const override  { return _displayName; }
    void setName(string_view name) override { _displayName = name; }
    const string& getPath() const override { return _path; }
    string getShortPath() const override   { return _path; }
    bool hasParent() const override;
    bool isDirectory() const override { return _isDirectory; }
    bool isFile() const override      { return _isFile;      }
    bool isReadable() const override  { return exists(); }
    bool isWritable() const override  { return exists(); }
    bool makeDir() override;
    bool rename(string_view newfile) override;

    size_t getSize() const override;
    bool getChildren(AbstractFSList& list, ListMode mode) const override;
    AbstractFSNodePtr getParent() const override;

    // Used to serve the in-memory ROM on platforms where the ROM file path
    // is not directly accessible (e.g. Android with need_fullpath = false).
    // Returns 0 when the file exists on disk so the base class uses openIFStream.
    size_t read(ByteArray& image, size_t) const override;

    std::ifstream openIFStream(std::ios::openmode mode) const override {
      return std::ifstream(_path, mode);
    }
    std::ofstream openOFStream(std::ios::openmode mode) const override {
      return std::ofstream(_path, mode);
    }
    std::fstream openFStream(std::ios::openmode mode) const override {
      return std::fstream(_path, mode);
    }

  private:
    /**
     * Set the _isDirectory/_isFile/_size flags using the VFS stat() call.
     *
     * @return  true if the path was found and flags were set
     */
    bool setFlags();

  private:
    string _path, _displayName;
    bool _isFile{false}, _isDirectory{true};
    mutable std::optional<size_t> _size{std::nullopt};
};

#endif  // FS_NODE_LIBRETRO_HXX
