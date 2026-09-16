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

#ifndef FUJINET_LINK_HXX
#define FUJINET_LINK_HXX

#include <functional>

#include "bspf.hxx"
#include "fujibus.hxx"

/**
  SLIP-framed FujiBus over a TCP socket, to fujinet-pc's "Bus over IP"
  listener (default 127.0.0.1:9995).  This is the transport half of the
  FujiNet cartridge; the protocol itself lives in the vendored fujibus/
  fujimail sources, which are the RP2040 firmware's own.

  Two facts about that listener shape everything here:

    - It accepts ONE client (listen backlog 1).  A stale connection starves
      the next one, and the symptom is a hang rather than an error.
    - It binds 127.0.0.1 only, while "localhost" usually resolves to ::1
      first.  open() therefore walks every getaddrinfo result rather than
      trusting the first.

  All calls block up to their deadline, so every one of them belongs on the
  cartridge's worker thread, never on the emulation thread.  close() is the
  exception and is safe from any thread: it shuts the socket down so a
  worker parked in select() returns promptly instead of sitting out a
  60-second mount.

  @author  Thomas Cherryhomes
*/
class FujiNetLink
{
  public:
    /**
      Offered every frame that arrives while a reply is outstanding.  The
      server interleaves unsolicited ROM-push frames (device 0xFF) with the
      reply to a mount, so a handler that consumes one says so and transact()
      keeps reading.  Without a handler every frame is taken as the reply.
    */
    using InboundHandler = std::function<bool(const fb_reply_t&)>;

    FujiNetLink() = default;
    ~FujiNetLink();

    /**
      Connect to the given endpoint.  Any previous connection is closed
      first.  Failure is reported through lastError(), not by throwing;
      a down link is an ordinary state the client already handles.

      @param host  Host name or address; empty means 127.0.0.1
      @param port  TCP port
      @return  Whether the connection was established
    */
    bool open(string_view host, int port);

    /**
      Close the connection, unblocking any thread parked in transact().
      Safe to call from any thread, and safe to call when already closed.
    */
    void close();

    bool isOpen() const { return mySocket != INVALID; }

    /**
      Describes the most recent failure, for the UI's status line.
    */
    const string& lastError() const { return myLastError; }

    /**
      Send one request and return its reply, consuming any push frames that
      arrive first.  Matches fujimail_port_t::transact.

      @param reply  On FB_OK, points into this object's receive buffer and
                    stays valid only until the next call
    */
    fb_status_t transact(uInt8 device, uInt8 command,
                         const fb_param_t* params, unsigned nparams,
                         const uInt8* payload, uInt16 payloadLen,
                         uInt32 timeoutMs, fb_reply_t* reply);

    /**
      Send a frame and expect nothing back; used to acknowledge push frames.
      Matches fujimail_port_t::send_bare.
    */
    void sendBare(uInt8 device, uInt8 command,
                  const uInt8* payload, uInt16 payloadLen);

    void setInboundHandler(const InboundHandler& handler) {
      myInbound = handler;
    }

  private:
    // A SLIP-encoded 512-byte push frame must fit whole: undersizing this
    // silently truncates every ROM push (the 1088 trap, paid for once on
    // the Intellivision)
    static constexpr size_t RX_RAW_MAX = 1088;
    // The builder assembles at most 384 decoded bytes, and SLIP escaping can
    // double every one of them
    static constexpr size_t TX_RAW_MAX = 2 * 384 + 2;
    static constexpr int CONNECT_TIMEOUT_MS = 3000;

    // Held as intptr_t so the platform's socket headers stay out of this
    // one; Windows' SOCKET is an unsigned pointer-width handle whose
    // INVALID_SOCKET is all-ones, which is this -1
    static constexpr intptr_t INVALID = -1;

    /**
      Read one whole SLIP frame, i.e. up to the second 0xC0 delimiter.
    */
    fb_status_t readFrame(size_t& outLen, uInt32 timeoutMs);

    bool fail(string_view what);

  private:
    intptr_t mySocket{INVALID};
    string myLastError;
    InboundHandler myInbound;

    std::array<uInt8, RX_RAW_MAX> myRxRaw{};
    std::array<uInt8, TX_RAW_MAX> myTxRaw{};

    // A frame boundary is not a packet boundary: one recv() can straddle two
    // frames, so whatever a frame did not consume is kept for the next one
    std::array<uInt8, RX_RAW_MAX> myRxPend{};
    size_t myRxPendLen{0}, myRxPendPos{0};

  private:
    FujiNetLink(const FujiNetLink&) = delete;
    FujiNetLink(FujiNetLink&&) = delete;
    FujiNetLink& operator=(const FujiNetLink&) = delete;
    FujiNetLink& operator=(FujiNetLink&&) = delete;
};

#endif  // FUJINET_LINK_HXX
