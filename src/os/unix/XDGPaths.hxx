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

#ifndef XDG_PATHS_HXX
#define XDG_PATHS_HXX

#include <cstdlib>
#include <optional>
#include <string>

class XDGPaths
{
  public:
    static XDGPaths& instance() { static XDGPaths inst; return inst; }

    const std::string& home() const       { return myHome;       }
    const std::string& configHome() const { return myConfigHome; }
    const std::string& dataHome() const   { return myDataHome;   }
    const std::string& cacheHome() const  { return myCacheHome;  }

    // Optional convenience helpers
    std::string configDir(const std::string& app) const {
      return myConfigHome + normalizeApp(app) + '/';
    }
    std::string dataDir(const std::string& app) const {
      return myDataHome + normalizeApp(app) + '/';
    }
    std::string cacheDir(const std::string& app) const {
      return myCacheHome + normalizeApp(app) + '/';
    }

  private:
    XDGPaths()
    {
      const auto homeEnv   = getEnv("HOME");
      const auto configEnv = getEnvPath("XDG_CONFIG_HOME");
      const auto dataEnv   = getEnvPath("XDG_DATA_HOME");
      const auto cacheEnv  = getEnvPath("XDG_CACHE_HOME");

      // If HOME doesn't exist, try something that is writeable for all users
      myHome = ensureTrailingSlash(homeEnv.value_or("/tmp/stella"));

      myConfigHome = ensureTrailingSlash(
        configEnv.value_or(myHome + ".config")
      );
      myDataHome = ensureTrailingSlash(
        dataEnv.value_or(myHome + ".local/share")
      );
      myCacheHome = ensureTrailingSlash(
        cacheEnv.value_or(myHome + ".cache")
      );
    }
    ~XDGPaths() = default;

    static std::optional<std::string> getEnv(const char* name) {
      if(const char* val = std::getenv(name))  // NOLINT(concurrency-mt-unsafe)
        return std::string(val);
      return std::nullopt;
    }

    // XDG spec prefers absolute paths; ignore invalid values
    static std::optional<std::string> getEnvPath(const char* name) {
      if(const char* val = std::getenv(name))  // NOLINT(concurrency-mt-unsafe)
      {
        std::string path(val);
        if(!path.empty() && path[0] == '/')
          return path;
      }
      return std::nullopt;
    }

    static std::string ensureTrailingSlash(std::string s) {
      if(!s.empty() && s.back() != '/')
        s.push_back('/');
      return s;
    }

    static std::string normalizeApp(std::string_view s) {
      // Remove leading slashes
      while(!s.empty() && s.front() == '/')
        s.remove_prefix(1);

      // Remove trailing slashes
      while(!s.empty() && s.back() == '/')
        s.remove_suffix(1);

      return std::string(s);
    }

  private:
    std::string myHome;
    std::string myConfigHome;
    std::string myDataHome;
    std::string myCacheHome;

  private:
    // Following constructors and assignment operators not supported
    XDGPaths(const XDGPaths&) = delete;
    XDGPaths(XDGPaths&&) = delete;
    XDGPaths& operator=(const XDGPaths&) = delete;
    XDGPaths& operator=(XDGPaths&&) = delete;
};

#endif  // XDG_PATHS_HXX
