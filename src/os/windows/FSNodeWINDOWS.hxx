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

#ifndef FS_NODE_WINDOWS_HXX
#define FS_NODE_WINDOWS_HXX

#include <optional>

#include "FSNode.hxx"

/*
 * Implementation of the Stella file system API based on Windows API.
 *
 * Parts of this class are documented in the base interface class,
 * AbstractFSNode.
 */
class FSNodeWINDOWS : public AbstractFSNode
{
  public:
    /**
     * Creates a FSNodeWINDOWS with the root node as path.
     * The root node consists of all the drives on the system (A - Z).
     */
    FSNodeWINDOWS() : _isPseudoRoot{true}, _isDirectory{true} { }

    /**
     * Creates a FSNodeWINDOWS for a given path.
     *
     * @param path  String with the path the new node should point to.
     */
    explicit FSNodeWINDOWS(string_view path);
    explicit FSNodeWINDOWS(std::wstring_view path);

    bool exists() const override;
    const string& getName() const override  { return _displayName; }
    void setName(string_view name) override { _displayName = name; }
    const string& getPath() const override  { return _pathUtf8;    }
    string getShortPath() const override;
    bool isDirectory() const override { return _isDirectory; }
    bool isFile() const override      { return _isFile;      }
    bool isReadable() const override;
    bool isWritable() const override;
    bool makeDir() override;
    bool rename(string_view newfile) override;

    size_t getSize() const override;
    bool hasParent() const override { return !_isPseudoRoot; }
    AbstractFSNodePtr getParent() const override;
    bool getChildren(AbstractFSList& fslist, ListMode mode) const override;

    std::ifstream openIFStream(std::ios::openmode mode) const override {
      return std::ifstream(_pathW, mode);
    }
    std::ofstream openOFStream(std::ios::openmode mode) const override {
      return std::ofstream(_pathW, mode);
    }
    std::fstream openFStream(std::ios::openmode mode) const override {
      return std::fstream(_pathW, mode);
    }

    // String conversion functions; name is self-explanatory
    static std::wstring utf8ToWide(std::string_view s);
    static std::string  wideToUtf8(const std::wstring_view w);

  private:
    /**
     * Set the _isDirectory/_isFile/_size flags using GetFileAttributes().
     *
     * @return  Success/failure of GetFileAttributes() function
     */
    bool setFlags();

    /**
     * Returns the last component of a given path (UTF-16 wide characters).
     *
     * @param s  String containing the path.
     * @return   View of the last component inside s
     */
    static constexpr std::wstring_view lastPathComponentW(std::wstring_view s)
    {
      if(s.empty())
        return std::wstring_view{};

      const auto pos = s.find_last_of(L"/\\", s.size() - 2);
      return (pos != std::wstring_view::npos) ? s.substr(pos + 1) : s;
    }

    /**
     * Returns the part *before* the last component of a given path
     * (UTF-16 wide characters).
     *
     * @param s  String containing the path.
     * @return   View of the preceding (before the last) component inside s.
     */
    static constexpr std::wstring_view stemPathComponentW(std::wstring_view s)
    {
      if (s.empty())
        return std::wstring_view{};

      const auto pos = s.find_last_of(L"/\\", s.size() - 2);

      return (pos != std::wstring_view::npos)
        ? s.substr(0, pos + 1)
        : std::wstring_view{};
    }

  private:
    std::wstring _pathW;
    std::string  _pathUtf8;
    std::string  _displayName;

    bool _isPseudoRoot{false}, _isDirectory{false}, _isFile{false};
    mutable std::optional<size_t> _size{std::nullopt};
};

#endif  // FS_NODE_WINDOWS_HXX
