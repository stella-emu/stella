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
#include <memory>
#include <string>

struct HomeFinder
{
  HomeFinder() = delete;

  static const std::wstring& getHomePathW() {
    std::call_once(ourHomeFlag, [] {
      initPath(ourHomePathW, FOLDERID_Profile);
      });
    return ourHomePathW;
  }

  static const std::wstring& getAppDataPathW() {
    std::call_once(ourAppDataFlag, [] {
      initPath(ourAppDataPathW, FOLDERID_RoamingAppData);
      });
    return ourAppDataPathW;
  }

  static const std::wstring& getDesktopPathW() {
    std::call_once(ourDesktopFlag, [] {
      initPath(ourDesktopPathW, FOLDERID_Desktop);
      });
    return ourDesktopPathW;
  }

  static const std::wstring& getDocumentsPathW() {
    std::call_once(ourDocumentsFlag, [] {
      initPath(ourDocumentsPathW, FOLDERID_Documents);
      });
    return ourDocumentsPathW;
  }

  // UTF-8 accessors (used by FSNode API layer)
  static const std::string& getHomePath() {
    std::call_once(ourHomeUtf8Flag, [] {
      ourHomePath = wideToUtf8(getHomePathW());
      });
    return ourHomePath;
  }

private:
  static void initPath(std::wstring& cache, const KNOWNFOLDERID& id)
  {
    PWSTR raw = nullptr;
    if(FAILED(SHGetKnownFolderPath(id, KF_FLAG_CREATE, nullptr, &raw)))
      return;

    // Ensure raw is freed however we exit
    struct CoTaskDeleter {
      void operator()(void* p) const { CoTaskMemFree(p); }
    };

    std::unique_ptr<std::remove_pointer_t<PWSTR>, CoTaskDeleter> ptr{raw};

    cache = ptr.get();
  }

  static std::string wideToUtf8(const std::wstring& w)
  {
    if (w.empty()) return {};

    const int needed = WideCharToMultiByte(
      CP_UTF8, 0,
      w.data(), static_cast<int>(w.size()),
      nullptr, 0, nullptr, nullptr);

    std::string out(needed, '\0');

    WideCharToMultiByte(
      CP_UTF8, 0,
      w.data(), static_cast<int>(w.size()),
      out.data(), needed,
      nullptr, nullptr);

    return out;
  }

  inline static std::once_flag ourHomeFlag, ourAppDataFlag,
                               ourDesktopFlag, ourDocumentsFlag;
  inline static std::once_flag ourHomeUtf8Flag;

  inline static std::wstring ourHomePathW, ourAppDataPathW,
                             ourDesktopPathW, ourDocumentsPathW;
  inline static std::string  ourHomePath;
};

#endif  // HOME_FINDER_HXX
