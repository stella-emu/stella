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

#include "Bankswitch.hxx"
#include "CartCreator.hxx"
#include "CartDetector.hxx"
#include "Logger.hxx"
#include "MD5.hxx"
#include "Props.hxx"
#include "Serializer.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "CartFUJI.hxx"

#include "fujimail.hxx"
#include "vcs_render.hxx"

namespace {
  // fujimail's port is C function pointers with no context argument, so the
  // one cartridge is reached through this.  One cart slot, one cartridge --
  // the constraint is real hardware's too.
  CartridgeFUJI* s_fuji{nullptr};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFUJI::CartridgeFUJI(ByteSpan image, string_view md5, Settings& settings)
  : Cartridge(settings, md5),
    myRWSettings{settings}
{
  myImage.assign(image.begin(), image.end());

  // Sized to the whole image so Cartridge::getAccessCounters(), which walks
  // romBankCount() * bankSize() entries, always stays inside it.  The pages
  // themselves are installed with a null romAccessBase, so System falls back
  // to the Device hooks and no read in this window is ever a side effect.
  createRomAccessArrays(std::max<size_t>(myImage.size(), FN_WINDOW_SIZE));

  myEnabled = settings.getBool("fujinet");

  s_fuji = this;
  serve(myImage);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFUJI::~CartridgeFUJI()
{
  stopWorker();
  if(s_fuji == this)
    s_fuji = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
  // The port vtable.  Every trampoline tolerates a null instance: a frame can
  // in principle still be in flight while the cartridge is being torn down.
  void c_poke(unsigned offset, uint8_t value)
  {
    if(s_fuji) s_fuji->portPoke(offset, value);
  }
  bool c_link_up()
  {
    return s_fuji && s_fuji->linkUp();
  }
  fb_status_t c_transact(uint8_t device, uint8_t command,
                         const fb_param_t* params, unsigned nparams,
                         const uint8_t* payload, uint16_t payload_len,
                         uint32_t timeout_ms, fb_reply_t* reply)
  {
    return s_fuji ? s_fuji->transact(device, command, params, nparams,
                                           payload, payload_len, timeout_ms,
                                           reply)
                  : FB_ENOLINK;
  }
  void c_send_bare(uint8_t device, uint8_t command,
                   const uint8_t* payload, uint16_t payload_len)
  {
    if(s_fuji) s_fuji->sendBare(device, command, payload, payload_len);
  }
  uint8_t c_stream_open(int stream, uint32_t size)
  {
    return s_fuji ? s_fuji->streamOpen(stream, size) : FN_BOOT_ERR_NOMAP;
  }
  void c_stream_write(int stream, const uint8_t* chunk, unsigned len)
  {
    if(s_fuji) s_fuji->streamWrite(stream, chunk, len);
  }
  uint8_t c_stream_close(int stream, uint32_t got, bool aborted)
  {
    return s_fuji ? s_fuji->streamClose(stream, got, aborted) : 0;
  }
  void c_arm_swap()
  {
    if(s_fuji) s_fuji->armSwap();
  }

  // fujimail offers these purely so a port can trace; nothing depends on
  // them.  Wired to Logger::debug so "-loglevel 2" shows the conversation,
  // which is the first thing wanted when a client is not doing what its
  // author expected.
  void c_on_txn(const fujimail_txn_t* txn)
  {
    string text;
    for(uInt16 i = 0; i < txn->rxlen && text.size() < 40; ++i)
    {
      const uInt8 c = txn->rx[i];
      if(c == 0)
        break;
      text += (c >= 0x20 && c < 0x7F) ? static_cast<char>(c) : '.';
    }
    Logger::debug(std::format(
      "FujiNet: dev={:02X} cmd={:02X} nparam={} txlen={} seq={}"
      " -> err={} reply={:02X} rxlen={}{}",
      txn->device, txn->command, txn->nparam, txn->txlen, txn->seq,
      txn->status, txn->reply_cmd, txn->rxlen,
      text.empty() ? "" : " \"" + text + "\""));
  }

  void c_on_dbc(fujimail_dbc_ev_t ev, int stream, uint32_t expect,
                unsigned got, bool aborted)
  {
    Logger::debug(ev == FUJIMAIL_DBC_OPEN
      ? std::format("FujiNet: push open stream={} size={}", stream, expect)
      : std::format("FujiNet: push close stream={} got={}{}", stream, got,
                    aborted ? " ABORTED" : ""));
  }

  const fujimail_port_t stellaPort = {
    c_poke, c_link_up, c_transact, c_send_bare,
    c_stream_open, c_stream_write, c_stream_close, c_arm_swap,
    nullptr,   // wait_link_ms: open() already bounds its own connect
    nullptr,   // bootsel: nothing to reboot into under emulation
    c_on_txn, c_on_dbc
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::serve(ByteSpan image)
{
  vcs_set_image(&myMem, image.data(), static_cast<uInt32>(image.size()));

  if(myMem.mailbox)
  {
    // Painting republishes ACKSEQ as 0 and resets fujimail's own sequence
    // interlock, which is right: a fresh image is a fresh session.
    fujimail_init(&stellaPort);
    fujimail_paint();
    drainPublished();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::reset()
{
  // Deliberately does NOT repaint, rewind ACKSEQ, change the bank or close
  // the arming gate.  There is no reset line on the cartridge connector, and
  // the client deriving its next sequence from the cartridge's persisted
  // ACKSEQ is exactly the behaviour that depends on it.
  Cartridge::reset();
  if(myInner)
    myInner->reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::install(System& system)
{
  mySystem = &system;

  if(myInner)
  {
    myInner->install(system);
    return;
  }

  // Every page dispatches through peek()/poke(): reads must reach the
  // published-window drain, and the console -> cartridge half of the mailbox
  // is real stores, which no stock VCS cartridge needs.  A null
  // directPeekBase is what makes System call us at all.
  const System::PageAccess access(this, System::PageAccessType::READWRITE);

  for(uInt16 addr = 0x1000; addr < 0x2000; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);

  startWorker();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFUJI::peek(uInt16 address)
{
  if(myInner)
    return myInner->peek(address);

  if(myPublishReady.load(std::memory_order_acquire)) [[unlikely]]
    drainPublished();

  // No read in the mailbox window has a side effect, so there is nothing
  // here to guard with hotspotsLocked() and the debugger may browse freely.
  return vcs_read_ex(&myMem, address, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFUJI::peekOob(uInt16 address)
{
  if(myInner)
    return myInner->peekOob(address);

  // Same as peek(), minus the drain: an out-of-band read must not move
  // published bytes into the window underneath the console.
  return vcs_read_ex(&myMem, address, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFUJI::poke(uInt16 address, uInt8 value)
{
  if(myInner)
    return myInner->poke(address, value);

  if(myPublishReady.load(std::memory_order_acquire)) [[unlikely]]
    drainPublished();

  if(hotspotsLocked())
    return false;

  uInt8 evA = 0, evB = 0;
  const vcs_ev_t ev = vcs_write(&myMem, address, value, &evA, &evB);

  switch(ev)
  {
    case VCS_EV_REG:
      // One completed arm-and-commit is one whole register write.  Expanded
      // into the REGSEL/REGDATA pair the shared fujimail decodes, so that
      // file stays byte-identical to the cartridge's and the siblings'.
      note(static_cast<uInt16>(FN_H_REGSEL + evA));
      note(static_cast<uInt16>(FN_H_REGDATA + evB));
      break;

    case VCS_EV_TX:
      note(static_cast<uInt16>(FN_H_DATA + evB));
      break;

    case VCS_EV_PATHTX:
    case VCS_EV_PATHRAW:
    {
      // The cartridge holds the working directory because the console has
      // nowhere to put it.  Expanded on THIS thread so the worker never
      // reads myMem.path[] while the bus is writing it.  Padded to 256 when
      // the payload is nothing but the path; raw when the client will append
      // a filename and pad it itself.
      const unsigned sel = myMem.path_sel;
      const unsigned n = (ev == VCS_EV_PATHTX) ? FN_PATH_MAX
                                               : myMem.path_len[sel];
      for(unsigned i = 0; i < n; ++i)
        note(static_cast<uInt16>(FN_H_DATA + vcs_path_byte(&myMem, i)));
      break;
    }

    case VCS_EV_BANK:
      // Inline: the console's very next fetch may come from the new bank
      vcs_set_bank(&myMem, evB);
      break;

    case VCS_EV_SWAP:
      doSwap();
      break;

    case VCS_EV_TROW:
    case VCS_EV_TCHR:
      // The half-composed row lives in vcs_mem_t, maintained by the shared
      // decode, so this class and the cartridge cannot drift on what one
      // looks like
      break;

    case VCS_EV_TEND:
      vcs_render_row(myMem.win, myMem.trow, myMem.tbuf, myMem.tlen);
      myMem.win[FN_B_TEXTGEN - FN_WINDOW_BASE] = static_cast<uInt8>(
        (myMem.win[FN_B_TEXTGEN - FN_WINDOW_BASE] ^ 0x80) | (myMem.trow & 0x1F));
      break;

    case VCS_EV_BLIT:
      // FN_BLIT_PATH's source is the cartridge's path buffer rather than the
      // reply window, so it is routed past vcs_blit() exactly as the
      // cartridge's own fuji_cart.c does; these two must not drift.
      if(evB == FN_BLIT_TCELL)
        vcs_render_cell(myMem.win,
                        static_cast<uInt8>(myMem.blit_dst / FN_T_COLS),
                        static_cast<uInt8>(myMem.blit_dst % FN_T_COLS),
                        static_cast<uInt8>(myMem.blit_src));
      else if(evB == FN_BLIT_PATH)
        vcs_render_path_row(myMem.win, myMem.path[myMem.path_sel],
                            myMem.path_len[myMem.path_sel],
                            myMem.blit_src,
                            static_cast<uInt8>(myMem.blit_dst),
                            myMem.blit_cnt);
      else
        vcs_blit(myMem.win, myMem.board, myMem.blit_src, myMem.blit_dst,
                 myMem.blit_cnt, evB);
      myMem.win[FN_B_BLITGEN - FN_WINDOW_BASE]++;
      break;

    case VCS_EV_ARMED:
      myMem.win[FN_B_FLAGS - FN_WINDOW_BASE] |= 0x01;
      break;

    case VCS_EV_NONE:
    default:
      break;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFUJI::pokeOob(uInt16 address, uInt8 value)
{
  // An out-of-band write must never reach the decode: the debugger poking a
  // control-page address would otherwise launch a network transaction.
  if(myInner)
    return myInner->pokeOob(address, value);
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::note(uInt16 offset)
{
  // Console::autodetectFrameLayout runs the client for sixty frames while
  // the console is still being built.  The client arms and fires a real
  // request in that time, so the whole mailbox stays shut until it is over.
  if(mySystem && mySystem->autodetectMode())
    return;

  if(!myWorker.joinable())
  {
    // No link, so nothing can block: run the mailbox inline and keep the
    // cartridge deterministic, which is what lets rewind stay available.
    fujimail_read_hotspot(offset);
    drainPublished();
    return;
  }

  // A well-behaved client is polling ACKSEQ and issuing nothing while a
  // transaction is out; dropping matches the RP2040, whose core0 is
  // likewise busy, and keeps a runaway client from growing the queue.
  if(myTxnInFlight.load(std::memory_order_relaxed))
    return;

  const size_t head = myQueueHead.load(std::memory_order_relaxed);
  const size_t next = (head + 1) % QUEUE_SIZE;
  if(next == myQueueTail.load(std::memory_order_acquire))
    return;                                   // full; the client will retry

  myQueue[head] = offset;
  myQueueHead.store(next, std::memory_order_release);
  myWake.notify_one();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::startWorker()
{
  if(!myEnabled || myWorker.joinable())
    return;

  myStop.store(false, std::memory_order_relaxed);
  myWorker = std::thread([this] { workerLoop(); });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::stopWorker()
{
  if(!myWorker.joinable())
    return;

  // Closed FIRST: a worker parked in select() on a sixty-second mount wakes
  // immediately instead of holding up the console's teardown.
  myLink.close();
  myStop.store(true, std::memory_order_release);
  myWake.notify_all();
  myWorker.join();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::workerLoop()
{
  if(!myLink.isOpen())
  {
    const string host = myRWSettings.getString("fujinet.host");
    const int port = myRWSettings.getInt("fujinet.port");
    if(myLink.open(host, port))
      myLink.setInboundHandler([](const fb_reply_t& frame) {
        return fujimail_inbound(&frame);
      });
  }

  for(;;)
  {
    std::unique_lock<std::mutex> lock(myWakeMutex);
    myWake.wait(lock, [this] {
      return myStop.load(std::memory_order_acquire) ||
             myQueueHead.load(std::memory_order_acquire) !=
             myQueueTail.load(std::memory_order_relaxed);
    });
    if(myStop.load(std::memory_order_acquire))
      return;
    lock.unlock();

    // Drain everything queued.  fujimail's file-scope state is touched by
    // this thread and no other, so the shared source needs no locking of
    // its own -- the same arrangement it has on the RP2040's core0.
    for(;;)
    {
      const size_t tail = myQueueTail.load(std::memory_order_relaxed);
      if(tail == myQueueHead.load(std::memory_order_acquire))
        break;
      const uInt16 offset = myQueue[tail];
      myQueueTail.store((tail + 1) % QUEUE_SIZE, std::memory_order_release);
      fujimail_read_hotspot(offset);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::portPoke(unsigned offset, uInt8 value)
{
  if(offset < FN_WINDOW_BASE || offset >= FN_WINDOW_BASE + FN_WINDOW_SIZE)
    return;

  const size_t index = offset - FN_WINDOW_BASE;
  {
    const std::lock_guard<std::mutex> lock(myPublishMutex);
    myPublished[index] = value;
    myPublishLo = std::min(myPublishLo, index);
    myPublishHi = std::max(myPublishHi, index + 1);
  }

  // Every published byte is offered, not just ACKSEQ, so that the busy flag
  // and a mount's progress byte can be watched while the transaction is
  // still out.  The interlock survives that: fujimail publishes ACKSEQ last,
  // and because a drain copies the whole pending range under the same mutex
  // a console that can see ACKSEQ can already see everything behind it.
  myPublishReady.store(true, std::memory_order_release);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
fb_status_t CartridgeFUJI::transact(uInt8 device, uInt8 command,
                                    const fb_param_t* params, unsigned nparams,
                                    const uInt8* payload, uInt16 payloadLen,
                                    uInt32 timeoutMs, fb_reply_t* reply)
{
  // FN_R_STATUS_BUSY is declared by the protocol but never raised by the
  // cartridge firmware, whose transaction is synchronous with its own bus
  // loop.  Here it is real: the console keeps running while this blocks.
  myTxnInFlight.store(true, std::memory_order_relaxed);
  portPoke(FN_R_STATUS, FN_R_STATUS_LINK | FN_R_STATUS_BUSY);

  const fb_status_t status =
    myLink.transact(device, command, params, nparams, payload, payloadLen,
                    timeoutMs, reply);

  myTxnInFlight.store(false, std::memory_order_relaxed);
  // fujimail republishes FN_R_STATUS itself once it has the reply, so the
  // busy bit clears without being cleared here
  return status;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::drainPublished()
{
  const std::lock_guard<std::mutex> lock(myPublishMutex);

  for(size_t i = myPublishLo; i < myPublishHi; ++i)
    myMem.win[i] = myPublished[i];

  myPublishLo = FN_WINDOW_SIZE;
  myPublishHi = 0;
  myPublishReady.store(false, std::memory_order_relaxed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFUJI::streamOpen(int stream, uInt32 size)
{
  if(stream < 0 || stream > 1)
    return FN_BOOT_ERR_NOMAP;

  // A client is (N+1) * 2048; a game is whatever board it shipped on, and
  // the smallest real 2600 cartridge is 2K.  Which one this is cannot be
  // known until the claim arrives, so accept any whole number of 2K blocks.
  // Refusing here rather than at close is the point: it happens before the
  // server drags the whole file over the network.
  if(stream == FN_STREAM_ROM)
  {
    if(size < FN_BANK_SIZE || (size % FN_BANK_SIZE) != 0)
      return FN_BOOT_ERR_NOMAP;
    if(size > Cartridge::maxSize())
      return FN_BOOT_ERR_TOOBIG;
  }
  myRx[stream].clear();
  myRx[stream].reserve(size);
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::streamWrite(int stream, const uInt8* chunk, unsigned len)
{
  if(stream >= 0 && stream <= 1)
    myRx[stream].insert(myRx[stream].end(), chunk, chunk + len);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFUJI::streamClose(int stream, uInt32 got, bool aborted)
{
  if(stream == FN_STREAM_CFG)
  {
    // Names the board for an image whose size cannot: an 8K F8, E0, UA and
    // FE are all 8192 bytes and nothing inside the file tells them apart.
    // Held until the ROM stream is served; vcs_set_cfg spends it once.
    const ByteArray& cfg = myRx[FN_STREAM_CFG];
    vcs_set_cfg(&myMem, (aborted || cfg.empty())
                        ? nullptr : reinterpret_cast<const char*>(cfg.data()),
                static_cast<unsigned>(cfg.size()));
    myRx[FN_STREAM_CFG].clear();
    return 0;
  }
  if(stream != FN_STREAM_ROM)
    return 0;
  if(aborted)
    return FN_BOOT_ERR_TRUNCATED;
  if(got < FN_BANK_SIZE || (got % FN_BANK_SIZE) != 0)
    return FN_BOOT_ERR_NOMAP;

  myStaged = myRx[FN_STREAM_ROM];
  myHaveStaged = true;
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::failBoot(uInt8 error)
{
  portPoke(FN_R_BOOT_STATE, FN_BOOT_FAILED);
  portPoke(FN_R_BOOT_ERR, error);
  drainPublished();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::doSwap()
{
  if(!myHaveStaged || myInner)
    return;

  // The .cfg sibling names the board where size cannot: an 8K F8, E0, UA and
  // FE are all 8192 bytes and nothing inside the file tells them apart.  Its
  // vocabulary is the cartridge's, which is a subset of Stella's own scheme
  // names -- all but "FLAT", which means "2K or 4K, tell them apart by size".
  Bankswitch::Type type = Bankswitch::Type::AUTO;
  if(myMem.cfg_name[0] != '\0' &&
     BSPF::compareIgnoreCase(myMem.cfg_name, "FLAT") != 0)
    type = Bankswitch::nameToType(myMem.cfg_name);

  const ByteSpan staged{myStaged};
  if(type == Bankswitch::Type::AUTO)
    type = CartDetector::autodetectType(staged);

  // Four schemes cannot be swapped IN, however well Stella runs them from
  // the launcher, because each needs something settled before the console
  // existed:
  //   ELF  bus stuffing, which System latches from the cartridge it was
  //        constructed with and never re-reads
  //   DEVC widens the address bus from its install(), long after the TIA
  //        and RIOT claimed their pages at 13 bits
  //   AR   a Supercharger load is a tape, not an image, and has its own
  //        construction path
  //   MVC  streams from a file, and there is no file here
  //   CM   needs the CompuMate handler, which re-seats the console's own
  //        cartridge pointer -- the very thing this class is standing in for
  switch(type)
  {
    case Bankswitch::Type::ELF:
    case Bankswitch::Type::DEVC:
    case Bankswitch::Type::AR:
    case Bankswitch::Type::MVC:
    case Bankswitch::Type::CM:
      Logger::error(std::format("FujiNet: cannot boot a {} image",
                                Bankswitch::typeToName(type)));
      failBoot(FN_BOOT_ERR_NOMAP);
      return;
    default:
      break;
  }

  unique_ptr<Cartridge> inner;
  try
  {
    const string md5 = MD5::hash(staged);
    inner = CartCreator::createFromKnownImage(staged, type, md5, myRWSettings);
  }
  catch(const std::exception& e)
  {
    // CartCreator throws on a malformed image, and this is running inside
    // the 6507's store cycle: an exception must not leave here
    Logger::error(string{"FujiNet: "} + e.what());
    inner.reset();
  }
  catch(...)
  {
    inner.reset();
  }

  if(inner == nullptr)
  {
    failBoot(FN_BOOT_ERR_NOMAP);
    return;
  }

  // Everything the console does for a cartridge it built itself.  Without
  // the start-bank function in particular, initializeStartBank() calls an
  // empty std::function and throws straight back through the CPU.
  inner->setProperties(myProperties);
  {
    // The same function Console gives a cartridge it built itself
    // (Console.cxx), rebuilt here against our own properties pointer
    const Properties* props = myProperties;
    inner->setStartBankFromPropsFunc([props]() -> int {
      if(props == nullptr)
        return -1;
      const string_view startbank = props->get(PropType::Cart_StartBank);
      return (startbank.empty() || BSPF::equalsIgnoreCase(startbank, "AUTO"))
          ? -1 : BSPF::stoi(startbank);
    });
  }
  inner->setMessageCallback(myMsgCallback);
  inner->enableRandomHotspots(myRandomHotspots);

  // The link is finished with: this cartridge is a game now, and the
  // mailbox is dead for the session
  stopWorker();

  myInner = std::move(inner);
  myInner->install(*mySystem);
  myInner->reset();

  setAbout("", Bankswitch::typeToName(type), "");
  Logger::info(std::format("FujiNet: booted a {} image, {} bytes",
                           Bankswitch::typeToName(type), myStaged.size()));

  // The console rebuilds what it cached from the client, one frame boundary
  // from now, where redetectFrameLayout() can safely run the TIA
  myPendingSwap.store(true, std::memory_order_release);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFUJI::enableFujiNet(bool enable)
{
  if(enable == myEnabled)
    return;

  myEnabled = enable;
  if(enable)
    startWorker();
  else
    stopWorker();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeFUJI::linkStatus() const
{
  if(myInner)
    return "booted a game; mailbox closed";
  if(!myEnabled)
    return "disabled";
  if(myLink.isOpen())
    return "connected to " + myRWSettings.getString("fujinet.host") + ":" +
           std::to_string(myRWSettings.getInt("fujinet.port"));
  return myLink.lastError().empty() ? "connecting" : myLink.lastError();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeFUJI::romBankCount() const
{
  if(myInner)
    return myInner->romBankCount();
  // The fixed half is not selectable, so it is not one of them
  const size_t banks = myImage.size() / FN_BANK_SIZE;
  return static_cast<uInt16>(banks > 1 ? banks - 1 : 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeFUJI::bankSize(uInt16 bank) const
{
  return myInner ? myInner->bankSize(bank) : FN_BANK_SIZE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeFUJI::getBank(uInt16 address) const
{
  if(myInner)
    return myInner->getBank(address);
  // The high half is the fixed one, and it is always the image's last 2K
  return (address & 0x800) ? static_cast<uInt16>(romBankCount()) : myMem.bank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeFUJI::segmentCount() const
{
  // The banked low half and the fixed high half
  return myInner ? myInner->segmentCount() : 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ByteSpan CartridgeFUJI::getImage() const
{
  return myInner ? myInner->getImage() : ByteSpan{myImage};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFUJI::patch(uInt16 address, uInt8 value)
{
  if(myInner)
    return myInner->patch(address, value);

  const uInt16 offset = address & 0x0FFF;
  if(offset < FN_BANK_SIZE)
  {
    const size_t index = static_cast<size_t>(myMem.bank) * FN_BANK_SIZE + offset;
    if(index >= myImage.size())
      return false;
    myImage[index] = value;
    // The bank is served by pointer, so re-point it at the patched image
    vcs_set_bank(&myMem, myMem.bank);
  }
  else
    myMem.win[offset] = value;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFUJI::save(Serializer& out) const
{
  try
  {
    // Never blocks and never joins: the Time Machine would call this sixty
    // times a second, and a transaction can be out for a minute.  Rewind is
    // refused for a live FujiNet cartridge instead; see StateManager.
    out.putByteArray(ByteSpan{myMem.win});
    out.putByte(myMem.bank);
    out.putBool(myMem.mailbox);
    out.putBool(myMem.armed);
    out.putByte(myMem.arm_step);
    out.putBool(myMem.swap_armed);
    out.putBool(myMem.regsel_armed);
    out.putByte(myMem.regsel);
    out.putByte(myMem.path_sel);
    out.putByte(myMem.trow);
    out.putByte(myMem.tlen);
    out.putBool(myInner != nullptr);
    if(myInner && !myInner->save(out))
      return false;
  }
  catch(...)
  {
    cerr << "ERROR: " << name() << "::save\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFUJI::load(Serializer& in)
{
  try
  {
    in.getByteArray(ByteMSpan{myMem.win});

    const uInt8 bank = in.getByte();
    myMem.mailbox = in.getBool();
    myMem.armed = in.getBool();
    myMem.arm_step = in.getByte();
    myMem.swap_armed = in.getBool();
    myMem.regsel_armed = in.getBool();
    myMem.regsel = in.getByte();
    myMem.path_sel = in.getByte();
    myMem.trow = in.getByte();
    myMem.tlen = in.getByte();

    vcs_set_bank(&myMem, bank);

    const bool hadInner = in.getBool();
    if(hadInner != (myInner != nullptr))
      return false;                 // a state from the other side of a boot
    if(myInner && !myInner->load(in))
      return false;
  }
  catch(...)
  {
    cerr << "ERROR: " << name() << "::load\n";
    return false;
  }
  return true;
}
