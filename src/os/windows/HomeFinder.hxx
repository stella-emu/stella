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

#ifndef HOME_FINDER_HXX
#define HOME_FINDER_HXX

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
  #define NOMINMAX
#endif
#include <shlobj.h>

#include <mutex>
#include "bspf.hxx"

struct HomeFinder
{
  HomeFinder() = delete;

  static const string& getAppDataPath() {
    std::call_once(ourAppDataFlag, [] {
      initPath(ourAppDataPath, FOLDERID_RoamingAppData); });
    return ourAppDataPath;
  }
  static const string& getDesktopPath() {
    std::call_once(ourDesktopFlag, [] {
      initPath(ourDesktopPath, FOLDERID_Desktop); });
    return ourDesktopPath;
  }
  static const string& getDocumentsPath() {
    std::call_once(ourDocumentsFlag, [] {
      initPath(ourDocumentsPath, FOLDERID_Documents); });
    return ourDocumentsPath;
  }
  static const string& getHomePath() {
    std::call_once(ourHomeFlag, [] {
      initPath(ourHomePath, FOLDERID_Profile); });
    return ourHomePath;
  }

private:
  static void initPath(string& cache, const KNOWNFOLDERID& id)
  {
    PWSTR raw = nullptr;
    if (FAILED(SHGetKnownFolderPath(id, KF_FLAG_CREATE, nullptr, &raw)))
      return;

    // Ensure raw is freed however we exit
    struct CoTaskDeleter {
      void operator()(void* p) const { CoTaskMemFree(p); }
    };
    std::unique_ptr<std::remove_pointer_t<PWSTR>, CoTaskDeleter> path{raw};

    // First call: query the required UTF-8 buffer size (includes null terminator)
    int needed = WideCharToMultiByte(CP_UTF8, 0, path.get(), -1,
                                     nullptr, 0, nullptr, nullptr);
    if (needed <= 0)
      return;

    // Second call: do the actual conversion into a right-sized string
    string result(needed - 1, '\0');
    int written = WideCharToMultiByte(CP_UTF8, 0, path.get(), -1,
                                      result.data(), needed,
                                      nullptr, nullptr);
    if (written > 0)
      cache = std::move(result);
  }

  inline static std::once_flag ourHomeFlag, ourAppDataFlag,
                               ourDesktopFlag, ourDocumentsFlag;
  inline static string ourHomePath, ourAppDataPath,
                       ourDesktopPath, ourDocumentsPath;
};

#endif // HOME_FINDER_HXX
