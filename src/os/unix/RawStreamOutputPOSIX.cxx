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

#include <cerrno>
#include <climits>
#include <csignal>

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

#include "RawStreamOutputPOSIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RawStreamOutputPOSIX::~RawStreamOutputPOSIX()
{
  if(myHandle != INVALID_HANDLE_VALUE && !myIsStdout)
    close(myHandle);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RawStreamOutputPOSIX::open(const string& path)
{
  myIsStdout = path == "-";
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  myHandle = myIsStdout ? STDOUT_FILENO : ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if(myHandle == INVALID_HANDLE_VALUE)
    return false;

  struct stat info{};
  myIsFile = fstat(myHandle, &info) == 0 && S_ISREG(info.st_mode);

  // A reader that quits must not take Stella down with it
  std::ignore = std::signal(SIGPIPE, SIG_IGN);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RawStreamOutputPOSIX::write(ByteSpan data)
{
  while(!data.empty())
  {
    if(myCancelled)
      return false;

    if(!myIsFile)
    {
      pollfd fd{myHandle, POLLOUT, 0};
      const int ready = poll(&fd, 1, POLL_TIMEOUT_MS);
      if(ready == 0 || (ready < 0 && errno == EINTR))
        continue;
      if(ready < 0)
        return false;
    }

    // Once poll() reports room, a pipe takes PIPE_BUF bytes without blocking
    const size_t size = myIsFile ? data.size() : std::min<size_t>(data.size(), PIPE_BUF);
    const ssize_t written = ::write(myHandle, data.data(), size);
    if(written < 0)
    {
      if(errno == EINTR)
        continue;
      return false;
    }
    data = data.subspan(SZT(written));
  }
  return true;
}
