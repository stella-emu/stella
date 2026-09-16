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

#ifndef CARTRIDGE_FUJI_HXX
#define CARTRIDGE_FUJI_HXX

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class System;
class Settings;
#ifdef DEBUGGER_SUPPORT
  #include "CartFUJIWidget.hxx"
#endif

#include "bspf.hxx"
#include "Cart.hxx"
#include "FujiNetLink.hxx"
#include "fuji_mailbox.hxx"
#include "vcs_cart.hxx"

/**
  The FujiNet cartridge for the Atari 2600: a 4K window at $1000-$1FFF that
  is part banked client image, part mailbox painted by the cartridge, and
  part cart-composed text planes.  It talks to a fujinet-pc instance over a
  TCP socket.

  The protocol (fujimail), the wire codec (fujibus), the bus decode
  (vcs_cart) and the glyph compositor (vcs_render) are NOT implemented here.
  They are the RP2040 cartridge firmware's own sources, vendored verbatim
  into src/emucore/fujinet/ by its sync.sh, for the reason fujimail.hxx
  gives: two copies of a protocol drift, and every browse-and-boot run in
  Stella should exercise what the cartridge actually ships.  This class is
  only the port -- bytes into the window, frames onto the socket.

  Three things here are worth knowing before changing anything.

  THE WINDOW IS WRITTEN FROM TWO THREADS.  A transaction can block for five
  seconds, or sixty on a mount, so the mailbox service runs on its own
  thread, exactly as it runs on the RP2040's core0 while core1 drives the
  bus.  The handoff is the protocol's own SEQ/ACKSEQ interlock: the worker
  publishes every reply byte, then publishes ACKSEQ last, and a client that
  sees ACKSEQ == SEQ is guaranteed the rest is already readable.  That is a
  release/acquire pair, and it is used as one.

  THERE IS NO RESET.  The cartridge connector carries A0-A12, D0-D7, +5V and
  ground -- no R/W, no clock, no reset.  A console RESET restarts the client
  but not the cartridge, and the client's "next sequence is the cartridge's
  persisted ACKSEQ + 1" rule depends on exactly that, so reset() must not
  touch ACKSEQ, the bank, or the armed state.

  A BOOTED GAME IS NOT SERVED FROM HERE.  When the client boots something,
  the pushed image goes to Stella's own CartDetector/CartCreator and the
  resulting cartridge takes over the bus; this class becomes a forwarding
  shell.  That is why the vendored decode is built with
  VCS_CART_HOST_MAPPER: the cartridge's own nine-scheme vcsmap would be a
  downgrade here.

  @author  Thomas Cherryhomes
*/
class CartridgeFUJI : public Cartridge
{
  friend class CartFUJIWidget;

  public:
    /**
      Create a new FujiNet cartridge.

      @param image     Pointer to the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the settings; NOT const, because
                       building a booted game's cartridge needs one
    */
    CartridgeFUJI(ByteSpan image, string_view md5, Settings& settings);
    ~CartridgeFUJI() override;

  public:
    void reset() override;
    void install(System& system) override;

    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

    uInt8 peek(uInt16 address) override;
    bool poke(uInt16 address, uInt8 value) override;

    // The debugger and the high-score reader browse memory through these;
    // neither may disturb a transaction or move a booted game's bank
    uInt8 peekOob(uInt16 address) override;
    bool pokeOob(uInt16 address, uInt8 value) override;

    bool patch(uInt16 address, uInt8 value) override;
    ByteSpan getImage() const override;
    string name() const override { return "CartridgeFUJI"; }

    uInt16 romBankCount() const override;
    uInt16 bankSize(uInt16 bank = 0) const override;
    uInt16 getBank(uInt16 address = 0) const override;
    uInt16 segmentCount() const override;

    /**
      Whether the link is enabled AND this is the cartridge serving the bus.
      Mirrors PlusROM's present-and-enabled rule, so a toggle never has to
      re-install pages.  While this is true the emulation is talking to a
      live external process and rewind is refused; see StateManager.
    */
    bool isFujiNetActive() const { return myEnabled && !myInner; }

    // A live link is exactly the state rewinding cannot undo: fujinet-pc has
    // its own mounts, open directories and file positions, and nothing the
    // Time Machine does can wind those back
    bool hasExternalState() const override { return isFujiNetActive(); }

    bool takePendingSwap() override
    {
      return myPendingSwap.exchange(false, std::memory_order_acquire);
    }

    /**
      Enable or disable the link.  Disabling closes the socket and retires
      the worker, which also makes the cartridge deterministic again.
    */
    void enableFujiNet(bool enable) override;

    /**
      Connection state for the options dialog's status line.
    */
    string linkStatus() const;

    string externalStateInfo() const override { return linkStatus(); }

  #ifdef DEBUGGER_SUPPORT
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
                                 const GUI::Font& nfont) override
    {
      return new CartFUJIWidget(boss, lfont, nfont, *this);
    }
  #endif

  public:
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // The fujimail port.  Public only because that port is C function
    // pointers with no context argument, so file-scope trampolines have to
    // reach them; nothing else should call these.  Every one of them runs
    // on the worker thread.
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void portPoke(unsigned offset, uInt8 value);
    bool linkUp() const { return myLink.isOpen(); }
    fb_status_t transact(uInt8 device, uInt8 command,
                         const fb_param_t* params, unsigned nparams,
                         const uInt8* payload, uInt16 payloadLen,
                         uInt32 timeoutMs, fb_reply_t* reply);
    void sendBare(uInt8 device, uInt8 command,
                  const uInt8* payload, uInt16 payloadLen)
      { myLink.sendBare(device, command, payload, payloadLen); }
    uInt8 streamOpen(int stream, uInt32 size);
    void streamWrite(int stream, const uInt8* chunk, unsigned len);
    uInt8 streamClose(int stream, uInt32 got, bool aborted);
    void armSwap() { myMem.swap_armed = true; }

  private:
    /**
      Hand one decoded hotspot access to the mailbox: to the worker when the
      link is live, inline when it is not (nothing can block then, and a
      cartridge that cannot reach the network should stay deterministic).
    */
    void note(uInt16 offset);

    void startWorker();
    void stopWorker();
    void workerLoop();

    /**
      Copy anything the worker published into the window.  Called from the
      bus path, guarded by an acquire load, so the cost when nothing is in
      flight is one atomic read.
    */
    void drainPublished();

    void serve(ByteSpan image);

    /**
      Replace the served client with the staged image, by handing it to
      Stella's own detector and factory.  Runs inline on the bus, which is
      what the client's swap stub expects: it stores to the swap hotspot
      from zero-page RAM, so its very next fetch is already outside this
      window.
    */
    void doSwap();

    /**
      Report a boot failure to the client the way the cartridge would, and
      keep serving it.
    */
    void failBoot(uInt8 error);

  private:
    // The served window plus the decode's state; the vendored bus model
    vcs_mem_t myMem{};

    // What is being served, and what a push is building
    ByteArray myImage, myStaged;
    // Per-stream push buffers: 0 is the ROM, 1 the .cfg sibling
    std::array<ByteArray, 2> myRx;
    bool myHaveStaged{false};

    Settings& myRWSettings;

    // - - - the link - - -
    FujiNetLink myLink;
    bool myEnabled{false};

    // - - - the worker - - -
    std::thread myWorker;
    std::atomic<bool> myStop{false};
    std::atomic<bool> myTxnInFlight{false};
    mutable std::mutex myWakeMutex;
    std::condition_variable myWake;

    // Hotspot accesses waiting for the worker.  Single producer (whichever
    // thread is in peek/poke) and single consumer, so a plain ring with two
    // atomic indices is enough.
    static constexpr size_t QUEUE_SIZE = 2048;
    std::array<uInt16, QUEUE_SIZE> myQueue{};
    std::atomic<size_t> myQueueHead{0}, myQueueTail{0};

    // What the worker has published but the bus has not yet picked up
    std::atomic<bool> myPublishReady{false};
    mutable std::mutex myPublishMutex;
    std::array<uInt8, FN_WINDOW_SIZE> myPublished{};
    size_t myPublishLo{FN_WINDOW_SIZE}, myPublishHi{0};

    // Set once the boot swap has happened; from then on this class only
    // forwards, and the mailbox is dead for the session
    unique_ptr<Cartridge> myInner;

    // Raised by the swap, taken by the main loop one frame boundary later
    std::atomic<bool> myPendingSwap{false};

  private:
    CartridgeFUJI() = delete;
    CartridgeFUJI(const CartridgeFUJI&) = delete;
    CartridgeFUJI(CartridgeFUJI&&) = delete;
    CartridgeFUJI& operator=(const CartridgeFUJI&) = delete;
    CartridgeFUJI& operator=(CartridgeFUJI&&) = delete;
};

#endif  // CARTRIDGE_FUJI_HXX
