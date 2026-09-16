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

#include <chrono>

#ifdef BSPF_WINDOWS
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <cerrno>
  #include <cstring>
  #include <fcntl.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <sys/select.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

#include "Logger.hxx"
#include "FujiNetLink.hxx"

namespace {

// BSD sockets and Winsock are the same API modulo these few names, so the
// platform split is a handful of aliases rather than a class hierarchy.
#ifdef BSPF_WINDOWS
  using sock_t = SOCKET;
  using socklen_arg = int;
  inline int closeSocket(sock_t s) { return ::closesocket(s); }
  inline int lastSocketError() { return ::WSAGetLastError(); }
  inline bool wouldBlock(int e) { return e == WSAEWOULDBLOCK; }
  inline bool inProgress(int e) { return e == WSAEWOULDBLOCK; }
  constexpr int SEND_FLAGS = 0;

  // Winsock needs starting exactly once per process.  httplib does its own
  // WSAStartup, but only in translation units that include it, so this one
  // cannot lean on that.
  void initSockets()
  {
    static const bool init = [] {
      WSADATA data;
      return ::WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();
    (void)init;
  }

  inline bool setNonBlocking(sock_t s)
  {
    u_long on = 1;
    return ::ioctlsocket(s, FIONBIO, &on) == 0;
  }
#else
  using sock_t = int;
  using socklen_arg = socklen_t;
  inline int closeSocket(sock_t s) { return ::close(s); }
  inline int lastSocketError() { return errno; }
  inline bool wouldBlock(int e) { return e == EAGAIN || e == EWOULDBLOCK; }
  inline bool inProgress(int e) { return e == EINPROGRESS; }
  void initSockets() { }

  // Linux suppresses SIGPIPE per call; macOS has no MSG_NOSIGNAL and does it
  // per socket instead (see the SO_NOSIGPIPE setsockopt in open()).  Either
  // way a vanished fujinet-pc must surface as a return value, not a signal.
  #ifdef MSG_NOSIGNAL
    constexpr int SEND_FLAGS = MSG_NOSIGNAL;
  #else
    constexpr int SEND_FLAGS = 0;
  #endif

  inline bool setNonBlocking(sock_t s)
  {
    const int flags = ::fcntl(s, F_GETFL, 0);
    return flags != -1 && ::fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
  }
#endif

  inline string socketErrorText(int err)
  {
  #ifdef BSPF_WINDOWS
    return "error " + std::to_string(err);
  #else
    return std::strerror(err);
  #endif
  }

  // Wait until the socket is readable (or writable, while connecting), with
  // the deadline expressed in whole milliseconds remaining.
  int waitFor(sock_t s, int64_t remainMs, bool forWrite)
  {
    if(remainMs <= 0)
      return 0;

    fd_set set;
    FD_ZERO(&set);
    FD_SET(s, &set);  // NOLINT: the macro is the platform's

    timeval tv{};
    tv.tv_sec = static_cast<decltype(tv.tv_sec)>(remainMs / 1000);
    tv.tv_usec = static_cast<decltype(tv.tv_usec)>((remainMs % 1000) * 1000);

    return ::select(static_cast<int>(s) + 1, forWrite ? nullptr : &set,
                    forWrite ? &set : nullptr, nullptr, &tv);
  }

  int64_t msSince(std::chrono::steady_clock::time_point start)
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now() - start).count();
  }
} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FujiNetLink::~FujiNetLink()
{
  close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FujiNetLink::fail(string_view what)
{
  myLastError = what;
  Logger::error("FujiNet: " + myLastError);
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FujiNetLink::open(string_view host, int port)
{
  close();
  initSockets();

  const string hostName = host.empty() ? "127.0.0.1" : string{host};
  const string portName = std::to_string(port);
  const string endpoint = hostName + ":" + portName;

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* results = nullptr;
  if(::getaddrinfo(hostName.c_str(), portName.c_str(), &hints, &results) != 0
     || results == nullptr)
    return fail("cannot resolve " + endpoint);

  // Walk every result rather than trusting the first: "localhost" usually
  // resolves to ::1 ahead of 127.0.0.1, while fujinet-pc's BoIP listener
  // binds 127.0.0.1 only.
  int lastError = 0;
  for(const addrinfo* ai = results; ai != nullptr; ai = ai->ai_next)
  {
    const sock_t s = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if(s == static_cast<sock_t>(INVALID))
      continue;

  #if !defined(BSPF_WINDOWS) && defined(SO_NOSIGPIPE)
    const int on = 1;
    ::setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof on);
  #endif

    // Non-blocking throughout, so neither the connect nor any later read can
    // outlast its deadline and strand the cartridge's worker thread.
    if(!setNonBlocking(s))
    {
      closeSocket(s);
      continue;
    }

    bool connected =
      ::connect(s, ai->ai_addr, static_cast<socklen_arg>(ai->ai_addrlen)) == 0;
    if(!connected && inProgress(lastSocketError()))
    {
      if(waitFor(s, CONNECT_TIMEOUT_MS, true) > 0)
      {
        int err = 0;
        auto len = static_cast<socklen_arg>(sizeof err);
        connected = ::getsockopt(s, SOL_SOCKET, SO_ERROR,
                                 reinterpret_cast<char*>(&err), &len) == 0
                    && err == 0;
        if(!connected)
          lastError = err;
      }
    }
    else if(!connected)
      lastError = lastSocketError();

    if(connected)
    {
      const int one = 1;
      ::setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
                   reinterpret_cast<const char*>(&one), sizeof one);
      mySocket = static_cast<intptr_t>(s);
      break;
    }
    closeSocket(s);
  }
  ::freeaddrinfo(results);

  if(mySocket == INVALID)
    return fail("cannot connect to " + endpoint +
                (lastError ? " (" + socketErrorText(lastError) + ")" : ""));

  myRxPendLen = myRxPendPos = 0;
  myLastError = "";
  Logger::info("FujiNet: connected to " + endpoint);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FujiNetLink::close()
{
  if(mySocket == INVALID)
    return;

  const auto s = static_cast<sock_t>(mySocket);
  mySocket = INVALID;

  // Shut down before closing so a worker parked in select() wakes at once
  // rather than serving out a mount's 60-second deadline.
#ifdef BSPF_WINDOWS
  ::shutdown(s, SD_BOTH);
#else
  ::shutdown(s, SHUT_RDWR);
#endif
  closeSocket(s);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
fb_status_t FujiNetLink::readFrame(size_t& outLen, uInt32 timeoutMs)
{
  const auto start = std::chrono::steady_clock::now();
  size_t n = 0;
  int ends = 0;

  for(;;)
  {
    // Drain what the last recv() over-read before asking for more: one read
    // can straddle two frames, and the remainder belongs to the next one.
    while(myRxPendPos < myRxPendLen)
    {
      if(n >= myRxRaw.size())
        return FB_ETOOBIG;

      const uInt8 c = myRxPend[myRxPendPos++];
      myRxRaw[n++] = c;
      if(c == 0xC0 && ++ends == 2)
      {
        outLen = n;
        return FB_OK;
      }
    }

    if(mySocket == INVALID)
      return FB_ENOLINK;

    const auto s = static_cast<sock_t>(mySocket);
    if(waitFor(s, static_cast<int64_t>(timeoutMs) - msSince(start), false) <= 0)
      return FB_ETIMEOUT;

    const auto r = ::recv(s, reinterpret_cast<char*>(myRxPend.data()),
                          static_cast<int>(myRxPend.size()), 0);
    if(r > 0)
    {
      myRxPendLen = static_cast<size_t>(r);
      myRxPendPos = 0;
    }
    else if(r == 0)
      return FB_ENOLINK;                 // fujinet-pc closed the connection
    else if(!wouldBlock(lastSocketError()))
      return FB_ENOLINK;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
fb_status_t FujiNetLink::transact(uInt8 device, uInt8 command,
                                  const fb_param_t* params, unsigned nparams,
                                  const uInt8* payload, uInt16 payloadLen,
                                  uInt32 timeoutMs, fb_reply_t* reply)
{
  if(!isOpen())
    return FB_ENOLINK;

  const size_t reqLen =
    fujibus_build_request(device, command, params, nparams, payload, payloadLen,
                          myTxRaw.data(), myTxRaw.size());
  if(reqLen == 0)
    return FB_ETOOBIG;

  const auto s = static_cast<sock_t>(mySocket);
  if(::send(s, reinterpret_cast<const char*>(myTxRaw.data()),
            static_cast<int>(reqLen), SEND_FLAGS) != static_cast<int>(reqLen))
    return FB_ENOLINK;

  for(;;)
  {
    size_t rawLen = 0;
    const fb_status_t status = readFrame(rawLen, timeoutMs);
    if(status != FB_OK)
      return status;

    if(!fujibus_parse_reply(myRxRaw.data(), rawLen, reply))
      return FB_EBADFRAME;

    // Push frames arrive interleaved with the reply we are waiting for;
    // consuming one proves the link is alive, so the deadline restarts
    // rather than counting down.
    if(!myInbound || !myInbound(*reply))
      return FB_OK;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FujiNetLink::sendBare(uInt8 device, uInt8 command,
                           const uInt8* payload, uInt16 payloadLen)
{
  if(!isOpen())
    return;

  const size_t n =
    fujibus_build_request(device, command, nullptr, 0, payload, payloadLen,
                          myTxRaw.data(), myTxRaw.size());
  if(n)
    ::send(static_cast<sock_t>(mySocket),
           reinterpret_cast<const char*>(myTxRaw.data()),
           static_cast<int>(n), SEND_FLAGS);
}
