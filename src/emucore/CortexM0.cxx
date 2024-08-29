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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

//============================================================================
// This code is based on the "Thumbulator" by David Welch (dwelch@dwelch.com)
// Code is public domain and used with the author's consent
//============================================================================

#include "CortexM0.hxx"

#include <algorithm>

#include "Serializable.hxx"
#include "Base.hxx"

namespace {
#ifdef __BIG_ENDIAN__
  FORCE_INLINE uInt32 READ32(const uInt8* data, uInt32 addr) {
    return
      (((uInt8*)(data))[(addr)]) |
      (((uInt8*)(data))[(addr) + 1]  << 8) |
      (((uInt8*)(data))[(addr) + 2] << 16) |
      (((uInt8*)(data))[(addr) + 3] << 24);
  }

  FORCE_INLINE uInt16 READ16(const uInt8* data, uInt32 addr) {
    return (((uInt8*)(data))[(addr)]) | (((uInt8*)(data))[(addr) + 1]  << 8);
  }

  FORCE_INLINE void WRITE32(uInt8* data, uInt32 addr, uInt32 value) {
    ((uInt8*)(data))[(addr)] = (value);
    ((uInt8*)(data))[(addr) + 1] = (value) >> 8;
    ((uInt8*)(data))[(addr) + 2] = (value) >> 16;
    ((uInt8*)(data))[(addr) + 3] = (value) >> 24;
  }

  FORCE_INLINE void WRITE16(uInt8* data, uInt32 addr, uInt16 value) {
    ((uInt8*)(data))[(addr)] = value;
    ((uInt8*)(data))[(addr) + 1] = (value) >> 8;
  }
#else
  FORCE_INLINE uInt32 READ32(const uInt8* data, uInt32 addr) {
    return (reinterpret_cast<const uInt32*>(data))[addr >> 2];
  }

  FORCE_INLINE uInt16 READ16(const uInt8* data, uInt32 addr) {
    return (reinterpret_cast<const uInt16*>(data))[addr >> 1];
  }

  FORCE_INLINE void WRITE32(uInt8* data, uInt32 addr, uInt32 value) {
    (reinterpret_cast<uInt32*>(data))[addr >> 2] = value;
  }

  FORCE_INLINE void WRITE16(uInt8* data, uInt32 addr, uInt16 value) {
    (reinterpret_cast<uInt16*>(data))[addr >> 1] = value;
  }
#endif
}  // namespace

// #define THUMB_DISS

#ifdef THUMB_DISS
  #define DO_DISS(statement)          \
    {                                 \
      ostringstream s;                \
      s << statement;                 \
      cout << s.str();   \
    }
#else
  #define DO_DISS(statement)
#endif

#define read_register(reg)        reg_norm[reg]
#define write_register(reg, data) reg_norm[reg]=(data)

#define do_znflags(x) znFlags=(x)
#define do_cflag_bit(x) cFlag = (x)
#define do_vflag_bit(x) vFlag = (x)

#define branch_target_9(inst) (read_register(15) + 2 + ((static_cast<Int32>(inst) << 24) >> 23))
#define branch_target_12(inst) (read_register(15) + 2 + ((static_cast<Int32>(inst) << 21) >> 20))

namespace {
  constexpr uInt32 PAGEMAP_SIZE = 0x100000000 / 4096;

  enum class Op : uInt8 {
    adc,
    add1, add2, add3, add4, add5, add6, add7,
    and_,
    asr1, asr2,
    // b1 variants:
    beq, bne, bcs, bcc, bmi, bpl, bvs, bvc, bhi, bls, bge, blt, bgt, ble,
    b2,
    bic,
    bkpt,
    // blx1 variants:
    bl, blx_thumb,
    blx2,
    bx,
    cmn,
    cmp1, cmp2, cmp3,
    cpy,
    eor,
    ldmia,
    ldr1, ldr2, ldr3, ldr4,
    ldrb1, ldrb2,
    ldrh1, ldrh2,
    ldrsb,
    ldrsh,
    lsl1, lsl2,
    lsr1, lsr2,
    mov1, mov2, mov3,
    mul,
    mvn,
    neg,
    orr,
    pop,
    push,
    rev,
    rev16,
    revsh,
    ror,
    sbc,
    setend,
    stmia,
    str1, str2, str3,
    strb1, strb2,
    strh1, strh2,
    sub1, sub2, sub3, sub4,
    swi,
    sxtb,
    sxth,
    tst,
    uxtb,
    uxth,
    numOps,
    invalid,
  };

  inline Op decodeInstructionWord(uInt16 inst)
  {
    //ADC add with carry
    if((inst & 0xFFC0) == 0x4140) return Op::adc;

    //ADD(1) small immediate two registers
    if((inst & 0xFE00) == 0x1C00 && (inst >> 6) & 0x7) return Op::add1;

    //ADD(2) big immediate one register
    if((inst & 0xF800) == 0x3000) return Op::add2;

    //ADD(3) three registers
    if((inst & 0xFE00) == 0x1800) return Op::add3;

    //ADD(4) two registers one or both high no flags
    if((inst & 0xFF00) == 0x4400) return Op::add4;

    //ADD(5) rd = pc plus immediate
    if((inst & 0xF800) == 0xA000) return Op::add5;

    //ADD(6) rd = sp plus immediate
    if((inst & 0xF800) == 0xA800) return Op::add6;

    //ADD(7) sp plus immediate
    if((inst & 0xFF80) == 0xB000) return Op::add7;

    //AND
    if((inst & 0xFFC0) == 0x4000) return Op::and_;

    //ASR(1) two register immediate
    if((inst & 0xF800) == 0x1000) return Op::asr1;

    //ASR(2) two register
    if((inst & 0xFFC0) == 0x4100) return Op::asr2;

    //B(1) conditional branch, decoded into its variants
    if((inst & 0xF000) == 0xD000)
    {
      switch((inst >> 8) & 0xF)
      {
        case 0x0: //b eq  z set
          return Op::beq;

        case 0x1: //b ne  z clear
          return Op::bne;

        case 0x2: //b cs c set
          return Op::bcs;

        case 0x3: //b cc c clear
          return Op::bcc;

        case 0x4: //b mi n set
          return Op::bmi;

        case 0x5: //b pl n clear
          return Op::bpl;

        case 0x6: //b vs v set
          return Op::bvs;

        case 0x7: //b vc v clear
          return Op::bvc;

        case 0x8: //b hi c set z clear
          return Op::bhi;

        case 0x9: //b ls c clear or z set
          return Op::bls;

        case 0xA: //b ge N == V
          return Op::bge;

        case 0xB: //b lt N != V
          return Op::blt;

        case 0xC: //b gt Z==0 and N == V
          return Op::bgt;

        case 0xD: //b le Z==1 or N != V
          return Op::ble;

        default:
          return Op::invalid;
      }
    }

    //B(2) unconditional branch
    if((inst & 0xF800) == 0xE000) return Op::b2;

    //BIC
    if((inst & 0xFFC0) == 0x4380) return Op::bic;

    //BKPT
    if((inst & 0xFF00) == 0xBE00) return Op::bkpt;

    //BL/BLX(1) decoded into its variants
    if((inst & 0xE000) == 0xE000)
    {
      if((inst & 0x1800) == 0x1000) return Op::bl;
      else if((inst & 0x1800) == 0x1800) return Op::blx_thumb;
      return Op::invalid;
    }

    //BLX(2)
    if((inst & 0xFF87) == 0x4780) return Op::blx2;

    //BX
    if((inst & 0xFF87) == 0x4700) return Op::bx;

    //CMN
    if((inst & 0xFFC0) == 0x42C0) return Op::cmn;

    //CMP(1) compare immediate
    if((inst & 0xF800) == 0x2800) return Op::cmp1;

    //CMP(2) compare register
    if((inst & 0xFFC0) == 0x4280) return Op::cmp2;

    //CMP(3) compare high register
    if((inst & 0xFF00) == 0x4500) return Op::cmp3;

    //CPY copy high register
    if((inst & 0xFFC0) == 0x4600) return Op::cpy;

    //EOR
    if((inst & 0xFFC0) == 0x4040) return Op::eor;

    //LDMIA
    if((inst & 0xF800) == 0xC800) return Op::ldmia;

    //LDR(1) two register immediate
    if((inst & 0xF800) == 0x6800) return Op::ldr1;

    //LDR(2) three register
    if((inst & 0xFE00) == 0x5800) return Op::ldr2;

    //LDR(3)
    if((inst & 0xF800) == 0x4800) return Op::ldr3;

    //LDR(4)
    if((inst & 0xF800) == 0x9800) return Op::ldr4;

    //LDRB(1)
    if((inst & 0xF800) == 0x7800) return Op::ldrb1;

    //LDRB(2)
    if((inst & 0xFE00) == 0x5C00) return Op::ldrb2;

    //LDRH(1)
    if((inst & 0xF800) == 0x8800) return Op::ldrh1;

    //LDRH(2)
    if((inst & 0xFE00) == 0x5A00) return Op::ldrh2;

    //LDRSB
    if((inst & 0xFE00) == 0x5600) return Op::ldrsb;

    //LDRSH
    if((inst & 0xFE00) == 0x5E00) return Op::ldrsh;

    //LSL(1)
    if((inst & 0xF800) == 0x0000) return Op::lsl1;

    //LSL(2) two register
    if((inst & 0xFFC0) == 0x4080) return Op::lsl2;

    //LSR(1) two register immediate
    if((inst & 0xF800) == 0x0800) return Op::lsr1;

    //LSR(2) two register
    if((inst & 0xFFC0) == 0x40C0) return Op::lsr2;

    //MOV(1) immediate
    if((inst & 0xF800) == 0x2000) return Op::mov1;

    //MOV(2) two low registers
    if((inst & 0xFFC0) == 0x1C00) return Op::mov2;

    //MOV(3)
    if((inst & 0xFF00) == 0x4600) return Op::mov3;

    //MUL
    if((inst & 0xFFC0) == 0x4340) return Op::mul;

    //MVN
    if((inst & 0xFFC0) == 0x43C0) return Op::mvn;

    //NEG
    if((inst & 0xFFC0) == 0x4240) return Op::neg;

    //ORR
    if((inst & 0xFFC0) == 0x4300) return Op::orr;

    //POP
    if((inst & 0xFE00) == 0xBC00) return Op::pop;

    //PUSH
    if((inst & 0xFE00) == 0xB400) return Op::push;

    //REV
    if((inst & 0xFFC0) == 0xBA00) return Op::rev;

    //REV16
    if((inst & 0xFFC0) == 0xBA40) return Op::rev16;

    //REVSH
    if((inst & 0xFFC0) == 0xBAC0) return Op::revsh;

    //ROR
    if((inst & 0xFFC0) == 0x41C0) return Op::ror;

    //SBC
    if((inst & 0xFFC0) == 0x4180) return Op::sbc;

    //SETEND
    if((inst & 0xFFF7) == 0xB650) return Op::setend;

    //STMIA
    if((inst & 0xF800) == 0xC000) return Op::stmia;

    //STR(1)
    if((inst & 0xF800) == 0x6000) return Op::str1;

    //STR(2)
    if((inst & 0xFE00) == 0x5000) return Op::str2;

    //STR(3)
    if((inst & 0xF800) == 0x9000) return Op::str3;

    //STRB(1)
    if((inst & 0xF800) == 0x7000) return Op::strb1;

    //STRB(2)
    if((inst & 0xFE00) == 0x5400) return Op::strb2;

    //STRH(1)
    if((inst & 0xF800) == 0x8000) return Op::strh1;

    //STRH(2)
    if((inst & 0xFE00) == 0x5200) return Op::strh2;

    //SUB(1)
    if((inst & 0xFE00) == 0x1E00) return Op::sub1;

    //SUB(2)
    if((inst & 0xF800) == 0x3800) return Op::sub2;

    //SUB(3)
    if((inst & 0xFE00) == 0x1A00) return Op::sub3;

    //SUB(4)
    if((inst & 0xFF80) == 0xB080) return Op::sub4;

    //SWI SoftWare Interupt
    if((inst & 0xFF00) == 0xDF00) return Op::swi;

    //SXTB
    if((inst & 0xFFC0) == 0xB240) return Op::sxtb;

    //SXTH
    if((inst & 0xFFC0) == 0xB200) return Op::sxth;

    //TST
    if((inst & 0xFFC0) == 0x4200) return Op::tst;

    //UXTB
    if((inst & 0xFFC0) == 0xB2C0) return Op::uxtb;

    //UXTH Zero extend Halfword
    if((inst & 0xFFC0) == 0xB280) return Op::uxth;

    return Op::invalid;
  }

  string describeErrorCode(CortexM0::err_t err) {
    if (CortexM0::isErrCustom(err)) {
      ostringstream s;
      s << "custom error " << CortexM0::getErrCustom(err);

      return s.str();
    }

    switch (CortexM0::getErrInstrinsic(err)) {
      case CortexM0::ERR_UNMAPPED_READ32:
        return "unmapped read32";

      case CortexM0::ERR_UNMAPPED_READ16:
        return "unmapped read16";

      case CortexM0::ERR_UNMAPPED_READ8:
        return "unmapped read8";

      case CortexM0::ERR_UNMAPPED_WRITE32:
        return "unmapped write32";

      case CortexM0::ERR_UNMAPPED_WRITE16:
        return "unmapped write16";

      case CortexM0::ERR_UNMAPPED_WRITE8:
        return "unmapped write8";

      case CortexM0::ERR_UNMAPPED_FETCH16:
        return "unmapped fetch";

      case CortexM0::ERR_WRITE_ACCESS_DENIED:
        return "write access denied";

      case CortexM0::ERR_ACCESS_ALIGNMENT_FAULT:
        return "alignment fault";

      case CortexM0::ERR_BKPT:
        return "breakpoint encountered";

      case CortexM0::ERR_INVALID_OPERATING_MODE:
        return "invalid operation mode";

      case CortexM0::ERR_UNIMPLEMENTED_INST:
        return "instruction not implemented";

      case CortexM0::ERR_SWI:
        return "supervisor call";

      case CortexM0::ERR_UNDEFINED_INST:
        return "undefined instruction";

      default:
        break;
    }

    ostringstream s;
    s << "unknown instrinsic error " << CortexM0::getErrInstrinsic(err);

    return s.str();
  }
} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::BusTransactionDelegate::read32(uInt32 address, uInt32& value, CortexM0& cortex)
{
  return errIntrinsic(ERR_UNMAPPED_READ32, address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::BusTransactionDelegate::read16(uInt32 address, uInt16& value, CortexM0& cortex)
{
  return errIntrinsic(ERR_UNMAPPED_READ16, address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::BusTransactionDelegate::read8(uInt32 address, uInt8& value, CortexM0& cortex)
{
  return errIntrinsic(ERR_UNMAPPED_READ8, address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::BusTransactionDelegate::write32(uInt32 address, uInt32 value, CortexM0& cortex)
{
  return errIntrinsic(ERR_UNMAPPED_WRITE32, address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::BusTransactionDelegate::write16(uInt32 address, uInt16 value, CortexM0& cortex)
{
  return errIntrinsic(ERR_UNMAPPED_WRITE16, address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::BusTransactionDelegate::write8(uInt32 address, uInt8 value, CortexM0& cortex)
{
  return errIntrinsic(ERR_UNMAPPED_WRITE8, address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::BusTransactionDelegate::fetch16(
  uInt32 address, uInt16& value, uInt8& op, CortexM0& cortex)
{
  const err_t err = read16(address, value, cortex);
  if (err) return err;

  op = decodeInstructionWord(value);

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CortexM0::describeError(err_t err) {
  if (err == ERR_NONE) return "no error";

  ostringstream s;
  s
    << describeErrorCode(err) << " : 0x"
    << std::hex << std::setw(8) << std::setfill('0')
    << getErrExtra(err);

  return s.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::CortexM0()
  : myPageMap{make_unique<uInt8[]>(PAGEMAP_SIZE)}
{
  resetMappings();
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CortexM0::MemoryRegion::saveDirtyBits(Serializer& out) const
{
  if (type != MemoryRegionType::directCode && type != MemoryRegionType::directData) return;

  out.putBool(dirty);
  if (!dirty) return;

  out.putInt(accessWatermarkLow);
  out.putInt(accessWatermarkHigh);

  switch (type) {
    case MemoryRegionType::directCode:
      out.putByteArray(
        std::get<1>(access).backingStore + (accessWatermarkLow - base),
        accessWatermarkHigh - accessWatermarkLow + 1
      );

      break;

    case MemoryRegionType::directData:
      out.putByteArray(
        std::get<0>(access).backingStore + (accessWatermarkLow - base),
        accessWatermarkHigh - accessWatermarkLow + 1
      );

      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CortexM0::MemoryRegion::loadDirtyBits(Serializer& in)
{
  if (type != MemoryRegionType::directCode && type != MemoryRegionType::directData) return;

  dirty = in.getBool();
  if (!dirty) return;

  accessWatermarkLow = in.getInt();
  accessWatermarkHigh = in.getInt();

  switch (type) {
    case MemoryRegionType::directCode:
      in.getByteArray(
        std::get<1>(access).backingStore + (accessWatermarkLow - base),
        accessWatermarkHigh - accessWatermarkLow + 1
      );

      break;

    case MemoryRegionType::directData:
      in.getByteArray(
        std::get<0>(access).backingStore + (accessWatermarkLow - base),
        accessWatermarkHigh - accessWatermarkLow + 1
      );

      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CortexM0::save(Serializer& out) const
{
  try {
    out.putIntArray(reg_norm.data(), reg_norm.size());

    out.putInt(znFlags);
    out.putInt(cFlag);
    out.putInt(vFlag);
    out.putLong(myCycleCounter);
  }
  catch (...) {
    cerr << "ERROR: failed to save cortex M0\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CortexM0::load(Serializer& in)
{
  try {
    reset();

    in.getIntArray(reg_norm.data(), reg_norm.size());

    znFlags = in.getInt();
    cFlag = in.getInt();
    vFlag = in.getInt();
    myCycleCounter = in.getLong();
  }
  catch (...) {
    cerr << "ERROR: failed to load cortex M0\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CortexM0::saveDirtyRegions(Serializer& out) const
{
  for (size_t i = 0; i < myNextRegionIndex; i++)
    myRegions[i].saveDirtyBits(out);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CortexM0::loadDirtyRegions(Serializer& in)
{
  for (size_t i = 0; i < myNextRegionIndex; i++)
    myRegions[i].loadDirtyBits(in);

  recompileCodeRegions();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CortexM0::MemoryRegion::reset()
{
  type = MemoryRegionType::unmapped;
  access.emplace<std::monostate>();

  accessWatermarkHigh = 0;
  accessWatermarkLow = ~0U;
  dirty = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0& CortexM0::resetMappings()
{
  for (auto& region: myRegions) region.reset();

  myNextRegionIndex = 0;
  std::fill_n(myPageMap.get(), PAGEMAP_SIZE, 0xff);

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0& CortexM0::mapRegionData(uInt32 pageBase, uInt32 pageCount,
                                  bool readOnly, uInt8* backingStore)
{
  MemoryRegion& region =
    setupMapping(pageBase, pageCount, readOnly, MemoryRegionType::directData);

  region.access.emplace<0>(MemoryRegionAccessData{backingStore});

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0& CortexM0::mapRegionCode(uInt32 pageBase, uInt32 pageCount,
                                  bool readOnly, uInt8* backingStore)  // NOLINT
{
  MemoryRegion& region =
    setupMapping(pageBase, pageCount, readOnly, MemoryRegionType::directCode);

  region.access.emplace<1>(MemoryRegionAccessCode{backingStore,
                           make_unique<uInt8[]>((pageCount * PAGE_SIZE) >> 1)});

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0& CortexM0::mapRegionDelegate(uInt32 pageBase, uInt32 pageCount, bool readOnly,
                                      CortexM0::BusTransactionDelegate* delegate)
{
  MemoryRegion& region =
    setupMapping(pageBase, pageCount, readOnly, MemoryRegionType::delegate);

  region.access.emplace<2>(delegate);

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0& CortexM0::mapDefault(CortexM0::BusTransactionDelegate* delegate)
{
  myDefaultDelegate = delegate;

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0& CortexM0::reset()
{
  reg_norm.fill(0);
  znFlags = cFlag = vFlag = 0;
  myCycleCounter = 0;

  recompileCodeRegions();

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0& CortexM0::setPc(uInt32 pc)
{
  return setRegister(15, (pc & ~1) + 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0& CortexM0::setRegister(uInt8 regno, uInt32 value)
{
  write_register(regno, value);

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CortexM0::getRegister(uInt32 regno) const
{
  return read_register(regno);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CortexM0::getN() const
{
  return znFlags & 0x80000000;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CortexM0::getZ() const
{
  return znFlags == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CortexM0::getC() const
{
  return cFlag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CortexM0::getV() const
{
  return vFlag;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CortexM0::decodeInstructionWord(uInt16 instructionWord)
{
  return static_cast<uInt8>(::decodeInstructionWord(instructionWord));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::run(uInt32 maxCycles, uInt32& cycles)
{
  for (cycles = 0; cycles < maxCycles; cycles++, myCycleCounter++) {
    const uInt32 pc = read_register(15);

    uInt16 inst = 0;
    uInt8 op = 0;
    err_t err = fetch16(pc - 2, inst, op);

    if (err) return err;

    write_register(15, pc + 2);

    err = execute(inst, op);

    #ifdef THUMB_DISS
      cout << std::flush;
    #endif

    if (err) {
      write_register(15, pc);
      return err;
    }
  }

  return ERR_NONE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::MemoryRegion& CortexM0::setupMapping(uInt32 pageBase, uInt32 pageCount,
    bool readOnly, CortexM0::MemoryRegionType type)
{
  if (myNextRegionIndex == 0xff) throw runtime_error("no free memory region");
  const uInt8 regionIndex = myNextRegionIndex++;

  MemoryRegion& region = myRegions[regionIndex];

  region.type = type;
  region.base = pageBase * PAGE_SIZE;
  region.size = pageCount * PAGE_SIZE;
  region.readOnly = readOnly;

  for (uInt32 page = pageBase; page < pageBase + pageCount; page++)
    myPageMap[page] = regionIndex;

  return region;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CortexM0::recompileCodeRegions()
{
  for (const auto& region: myRegions) {
    if (!std::holds_alternative<MemoryRegionAccessCode>(region.access))
      continue;

    for (uInt32 i = 0; i < region.size; i += 2)
      std::get<1>(region.access).ops[i >> 1] =
        decodeInstructionWord(READ16(std::get<1>(region.access).backingStore, i));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::read32(uInt32 address, uInt32& value)
{
  if (address & 0x03) return errIntrinsic(ERR_ACCESS_ALIGNMENT_FAULT, address);

  MemoryRegion& region = myRegions[myPageMap[address / PAGE_SIZE]];

  switch (region.type) {
    case MemoryRegionType::delegate:
      return std::get<2>(region.access)->read32(address, value, *this);

    case MemoryRegionType::directCode:
      value = READ32(std::get<1>(region.access).backingStore, address - region.base);
      return ERR_NONE;

    case MemoryRegionType::directData:
      value = READ32(std::get<0>(region.access).backingStore, address - region.base);
      return ERR_NONE;

    default:
      return myDefaultDelegate
        ? myDefaultDelegate->read32(address, value, *this)
        : errIntrinsic(ERR_UNMAPPED_READ32, address);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::read16(uInt32 address, uInt16& value)
{
  if (address & 0x01) return errIntrinsic(ERR_ACCESS_ALIGNMENT_FAULT, address);

  MemoryRegion& region = myRegions[myPageMap[address / PAGE_SIZE]];

  switch (region.type) {
    case MemoryRegionType::delegate:
      return std::get<2>(region.access)->read16(address, value, *this);

    case MemoryRegionType::directCode:
      value = READ16(std::get<1>(region.access).backingStore, address - region.base);
      return ERR_NONE;

    case MemoryRegionType::directData:
      value = READ16(std::get<0>(region.access).backingStore, address - region.base);
      return ERR_NONE;

    default:
      return myDefaultDelegate
        ? myDefaultDelegate->read16(address, value, *this)
        : errIntrinsic(ERR_UNMAPPED_READ16, address);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::read8(uInt32 address, uInt8& value)
{
  MemoryRegion& region = myRegions[myPageMap[address / PAGE_SIZE]];

  switch (region.type) {
    case MemoryRegionType::delegate:
      return std::get<2>(region.access)->read8(address, value, *this);

    case MemoryRegionType::directCode:
      value = std::get<1>(region.access).backingStore[address - region.base];
      return ERR_NONE;

    case MemoryRegionType::directData:
      value = std::get<0>(region.access).backingStore[address - region.base];
      return ERR_NONE;

    default:
      return myDefaultDelegate
        ? myDefaultDelegate->read8(address, value, *this)
        : errIntrinsic(ERR_UNMAPPED_READ8, address);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::write32(uInt32 address, uInt32 value)
{
  if (address & 0x03) return errIntrinsic(ERR_ACCESS_ALIGNMENT_FAULT, address);

  MemoryRegion& region = myRegions[myPageMap[address / PAGE_SIZE]];
  if (region.readOnly) return errIntrinsic(ERR_WRITE_ACCESS_DENIED, address);

  switch (region.type) {
    case MemoryRegionType::delegate:
      return std::get<2>(region.access)->write32(address, value, *this);

    case MemoryRegionType::directCode: {
      const uInt32 offset = address - region.base;

      region.dirty = true;
      region.accessWatermarkLow = std::min(address, region.accessWatermarkLow);
      region.accessWatermarkHigh = std::max(address + 3, region.accessWatermarkHigh);

      WRITE32(std::get<1>(region.access).backingStore, offset, value);
      std::get<1>(region.access).ops[offset >> 1] = decodeInstructionWord(value);
      std::get<1>(region.access).ops[(offset + 2) >> 1] = decodeInstructionWord(value >> 16);

      return ERR_NONE;
    }

    case MemoryRegionType::directData:
      region.dirty = true;
      region.accessWatermarkLow = std::min(address, region.accessWatermarkLow);
      region.accessWatermarkHigh = std::max(address + 3, region.accessWatermarkHigh);

      WRITE32(std::get<0>(region.access).backingStore, address - region.base, value);
      return ERR_NONE;

    default:
      return myDefaultDelegate
        ? myDefaultDelegate->write32(address, value, *this)
        : errIntrinsic(ERR_UNMAPPED_WRITE32, address);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::write16(uInt32 address, uInt16 value)
{
  if (address & 0x01) return errIntrinsic(ERR_ACCESS_ALIGNMENT_FAULT, address);

  MemoryRegion& region = myRegions[myPageMap[address / PAGE_SIZE]];
  if (region.readOnly) return errIntrinsic(ERR_WRITE_ACCESS_DENIED, address);

  switch (region.type) {
    case MemoryRegionType::delegate:
      return std::get<2>(region.access)->write16(address, value, *this);

    case MemoryRegionType::directCode: {
      const uInt32 offset = address - region.base;

      region.dirty = true;
      region.accessWatermarkLow = std::min(address, region.accessWatermarkLow);
      region.accessWatermarkHigh = std::max(address + 1, region.accessWatermarkHigh);

      WRITE16(std::get<1>(region.access).backingStore, offset, value);
      std::get<1>(region.access).ops[offset >> 1] = decodeInstructionWord(value);

      return ERR_NONE;
    }

    case MemoryRegionType::directData:
      region.dirty = true;
      region.accessWatermarkLow = std::min(address, region.accessWatermarkLow);
      region.accessWatermarkHigh = std::max(address + 1, region.accessWatermarkHigh);

      WRITE16(std::get<0>(region.access).backingStore, address - region.base, value);
      return ERR_NONE;

    default:
      return myDefaultDelegate
        ? myDefaultDelegate->write16(address, value, *this)
        : errIntrinsic(ERR_UNMAPPED_WRITE16, address);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::write8(uInt32 address, uInt8 value)
{
  MemoryRegion& region = myRegions[myPageMap[address / PAGE_SIZE]];
  if (region.readOnly) return errIntrinsic(ERR_WRITE_ACCESS_DENIED, address);

  switch (region.type) {
    case MemoryRegionType::delegate:
      return std::get<2>(region.access)->write8(address, value, *this);

    case MemoryRegionType::directCode: {
      const uInt32 offset = address - region.base;

      region.dirty = true;
      region.accessWatermarkLow = std::min(address, region.accessWatermarkLow);
      region.accessWatermarkHigh = std::max(address, region.accessWatermarkHigh);

      std::get<1>(region.access).backingStore[offset] = value;
      std::get<1>(region.access).ops[offset >> 1] = decodeInstructionWord(value);

      return ERR_NONE;
    }

    case MemoryRegionType::directData:
      region.dirty = true;
      region.accessWatermarkLow = std::min(address, region.accessWatermarkLow);
      region.accessWatermarkHigh = std::max(address, region.accessWatermarkHigh);

      std::get<0>(region.access).backingStore[address - region.base] = value;
      return ERR_NONE;

    default:
      return myDefaultDelegate
        ? myDefaultDelegate->write8(address, value, *this)
        : errIntrinsic(ERR_UNMAPPED_WRITE8, address);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CortexM0::err_t CortexM0::fetch16(uInt32 address, uInt16& value, uInt8& op)
{
  if (address & 0x01) return errIntrinsic(ERR_ACCESS_ALIGNMENT_FAULT, address);

  MemoryRegion& region = myRegions[myPageMap[address / PAGE_SIZE]];

  switch (region.type) {
    case MemoryRegionType::delegate:
      return std::get<2>(region.access)->fetch16(address, value, op, *this);

    case MemoryRegionType::directCode: {
      const uInt32 offset = address - region.base;

      value = READ16(std::get<1>(region.access).backingStore, offset);
      op = std::get<1>(region.access).ops[offset >> 1];

      return ERR_NONE;
    }

    case MemoryRegionType::directData:
      value = READ16(std::get<0>(region.access).backingStore, address - region.base);
      op = decodeInstructionWord(value);

      return ERR_NONE;

    default:
      return myDefaultDelegate
        ? myDefaultDelegate->fetch16(address, value, op, *this)
        : errIntrinsic(ERR_UNMAPPED_FETCH16, address);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CortexM0::do_cvflag(uInt32 a, uInt32 b, uInt32 c)
{
  uInt32 rc = (a & 0x7FFFFFFF) + (b & 0x7FFFFFFF) + c; //carry in
  rc >>= 31; //carry in in lsbit
  a >>= 31;
  b >>= 31;
  uInt32 rd = (rc & 1) + (a & 1) + (b & 1); //carry out
  rd >>= 1; //carry out in lsbit

  vFlag = (rc ^ rd) & 1; //if carry in != carry out then signed overflow

  rc += a + b;             //carry out
  cFlag = rc & 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE: function exceeds recommended size/complexity thresholds
CortexM0::err_t CortexM0::execute(uInt16 inst, uInt8 op)
{
  uInt32 sp, ra, rb, rc, rm, rd, rn, rs;  // NOLINT: don't need to initialize

  #ifdef THUMB_DISS
    cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << (read_register(15) - 4) << " " << std::dec;
  #endif

  switch (static_cast<Op>(op)) {
    //ADC
    case Op::adc: {
      rd = (inst >> 0) & 0x07;
      rm = (inst >> 3) & 0x07;
      DO_DISS("adc r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra + rb;
      if(cFlag)
        ++rc;
      write_register(rd, rc);
      do_znflags(rc);
      if(cFlag) do_cvflag(ra, rb, 1);
      else      do_cvflag(ra, rb, 0);
      return ERR_NONE;
    }

    //ADD(1) small immediate two registers
    case Op::add1: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rb = (inst >> 6) & 0x7;

      DO_DISS("adds r" << dec << rd << ",r" << dec << rn << ","
                        << "#0x" << Base::HEX2 << rb << '\n');
      ra = read_register(rn);
      rc = ra + rb;
      //fprintf(stderr,"0x%08X = 0x%08X + 0x%08X\n",rc,ra,rb);
      write_register(rd, rc);
      do_znflags(rc);
      do_cvflag(ra, rb, 0);

      return ERR_NONE;
    }

    //ADD(2) big immediate one register
    case Op::add2: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x7;
      DO_DISS("adds r" << dec << rd << ",#0x" << Base::HEX2 << rb << '\n');
      ra = read_register(rd);
      rc = ra + rb;
      write_register(rd, rc);
      do_znflags(rc);
      do_cvflag(ra, rb, 0);
      return ERR_NONE;
    }

    //ADD(3) three registers
    case Op::add3: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS("adds r" << dec << rd << ",r" << dec << rn << ",r" << rm << '\n');
      ra = read_register(rn);
      rb = read_register(rm);
      rc = ra + rb;
      write_register(rd, rc);
      do_znflags(rc);
      do_cvflag(ra, rb, 0);
      return ERR_NONE;
    }

    //ADD(4) two registers one or both high no flags
    case Op::add4: {
      if((inst >> 6) & 3)
      {
        //UNPREDICTABLE
      }
      rd  = (inst >> 0) & 0x7;
      rd |= (inst >> 4) & 0x8;
      rm  = (inst >> 3) & 0xF;
      DO_DISS("add r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra + rb;
      if(rd == 15)
      {
        if ((rc & 1) == 0)
          return errIntrinsic(ERR_INVALID_OPERATING_MODE, read_register(15) - 4);

        rc &= ~1; //write_register may f this as well
        rc += 2;  //The program counter is special
      }
      //fprintf(stderr,"0x%08X = 0x%08X + 0x%08X\n",rc,ra,rb);
      write_register(rd, rc);
      return ERR_NONE;
    }

    //ADD(5) rd = pc plus immediate
    case Op::add5: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x7;
      rb <<= 2;
      DO_DISS("add r" << dec << rd << ",PC,#0x" << Base::HEX2 << rb << '\n');
      ra = read_register(15);
      rc = (ra & (~3U)) + rb;
      write_register(rd, rc);
      return ERR_NONE;
    }

    //ADD(6) rd = sp plus immediate
    case Op::add6: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x7;
      rb <<= 2;
      DO_DISS("add r" << dec << rd << ",SP,#0x" << Base::HEX2 << rb << '\n');
      ra = read_register(13);
      rc = ra + rb;
      write_register(rd, rc);
      return ERR_NONE;
    }

    //ADD(7) sp plus immediate
    case Op::add7: {
      rb = (inst >> 0) & 0x7F;
      rb <<= 2;
      DO_DISS("add SP,#0x" << Base::HEX2 << rb << '\n');
      ra = read_register(13);
      rc = ra + rb;
      write_register(13, rc);
      return ERR_NONE;
    }

    //AND
    case Op::and_: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("ands r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra & rb;
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    //ASR(1) two register immediate
    case Op::asr1: {
      rd = (inst >> 0) & 0x07;
      rm = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      DO_DISS("asrs r" << dec << rd << ",r" << dec << rm << ",#0x" << Base::HEX2 << rb << '\n');
      rc = read_register(rm);
      if(rb == 0)
      {
        if(rc & 0x80000000)
        {
          do_cflag_bit(1);
          rc = ~0U;
        }
        else
        {
          do_cflag_bit(0);
          rc = 0;
        }
      }
      else
      {
        do_cflag_bit(rc & (1 << (rb-1)));
        ra = rc & 0x80000000;
        rc >>= rb;
        if(ra) //asr, sign is shifted in
          rc |= (~0U) << (32-rb);
      }
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    //ASR(2) two register
    case Op::asr2: {
      rd = (inst >> 0) & 0x07;
      rs = (inst >> 3) & 0x07;
      DO_DISS("asrs r" << dec << rd << ",r" << dec << rs << '\n');
      rc = read_register(rd);
      rb = read_register(rs);
      rb &= 0xFF;
      if(rb == 0)
      {
      }
      else if(rb < 32)
      {
        do_cflag_bit(rc & (1 << (rb-1)));
        ra = rc & 0x80000000;
        rc >>= rb;
        if(ra) //asr, sign is shifted in
        {
          rc |= (~0U) << (32-rb);
        }
      }
      else
      {
        if(rc & 0x80000000)
        {
          do_cflag_bit(1);
          rc = (~0U);
        }
        else
        {
          do_cflag_bit(0);
          rc = 0;
        }
      }
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    //B(1) conditional branch variants:
    // (beq, bne, bcs, bcc, bmi, bpl, bvs, bvc, bhi, bls, bge, blt, bgt, ble)
    case Op::beq: {
      if(!znFlags)
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    case Op::bne: {
      DO_DISS("bne 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if(znFlags)
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    case Op::bcs: {
      DO_DISS("bcs 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if(cFlag)
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    case Op::bcc: {
      DO_DISS("bcc 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if(!cFlag)
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    case Op::bmi: {
      DO_DISS("bmi 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if(znFlags & 0x80000000)
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    case Op::bpl: {
      DO_DISS("bpl 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if(!(znFlags & 0x80000000))
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    case Op::bvs: {
      DO_DISS("bvs 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if(vFlag)
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    case Op::bvc: {
      DO_DISS("bvc 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if(!vFlag)
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    case Op::bhi: {
      DO_DISS("bhi 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if(cFlag && znFlags)
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    case Op::bls: {
      DO_DISS("bls 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if(!znFlags || !cFlag)
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    case Op::bge: {
      DO_DISS("bge 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if(((znFlags & 0x80000000) && vFlag) ||
         ((!(znFlags & 0x80000000)) && !vFlag))
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    case Op::blt: {
      DO_DISS("blt 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if((!(znFlags & 0x80000000) && vFlag) ||
         (((znFlags & 0x80000000)) && !vFlag))
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    case Op::bgt: {
      DO_DISS("bgt 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if(znFlags)
      {
        if(((znFlags & 0x80000000) && vFlag) ||
           ((!(znFlags & 0x80000000)) && !vFlag))
          write_register(15, branch_target_9(inst));
      }
      return ERR_NONE;
    }

    case Op::ble: {
      DO_DISS("ble 0x" << Base::HEX8 << branch_target_9(inst) << "\n");
      if(!znFlags ||
         (!(znFlags & 0x80000000) && vFlag) ||
         (((znFlags & 0x80000000)) && !vFlag))
        write_register(15, branch_target_9(inst));
      return ERR_NONE;
    }

    //B(2) unconditional branch
    case Op::b2: {
      DO_DISS("b 0x" << Base::HEX8 << branch_target_12(inst) << "\n");
      write_register(15, branch_target_12(inst));
      return ERR_NONE;
    }

    //BIC
    case Op::bic: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("bics r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra & (~rb);
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    case Op::bkpt:
      return errIntrinsic(ERR_BKPT, read_register(15) - 4);

    //BL/BLX(1) variants
    // (bl, blx_thumb)
    case Op::bl: {
      // branch to label
      DO_DISS("bkpt\n");
      rb = inst & ((1 << 11) - 1);
      if(rb & 1 << 10) rb |= (~((1 << 11) - 1)); //sign extend
      rb <<= 12;
      rb += read_register(15);
      write_register(14, rb);
      return ERR_NONE;
    }

    case Op::blx_thumb: {
      // branch to label, switch to thumb
      rb = read_register(14);
      rb += (inst & ((1 << 11) - 1)) << 1;
      rb += 2;
      DO_DISS("bl 0x" << Base::HEX8 << (rb-3) << '\n');
      write_register(14, (read_register(15)-2) | 1);
      write_register(15, rb);
      return ERR_NONE;
    }

    //BLX(2)
    case Op::blx2: {
      rm = (inst >> 3) & 0xF;
      DO_DISS("blx r" << dec << rm << '\n');
      rc = read_register(rm);
      //fprintf(stderr,"blx r%u 0x%X 0x%X\n",rm,rc,pc);
      rc += 2;
      if(rc & 1)
      {
        write_register(14, (read_register(15)-2) | 1);
        rc &= ~1; // not checked and corrected in write_register
        write_register(15, rc);
        return ERR_NONE;
      }
      else return errIntrinsic(ERR_INVALID_OPERATING_MODE, read_register(15) - 4);
    }

    //BX
    case Op::bx: {
      rm = (inst >> 3) & 0xF;
      DO_DISS("bx r" << dec << rm << '\n');
      rc = read_register(rm);
      rc += 2;
      //fprintf(stderr,"bx r%u 0x%X 0x%X\n",rm,rc,pc);
      if(rc & 1)
      {
        // branch to odd address denotes 16 bit ARM code
        rc &= ~1;
        write_register(15, rc);
        return ERR_NONE;
      }
      else return errIntrinsic(ERR_INVALID_OPERATING_MODE, read_register(15) - 4);
    }

    //CMN
    case Op::cmn: {
      rn = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("cmns r" << dec << rn << ",r" << dec << rm << '\n');
      ra = read_register(rn);
      rb = read_register(rm);
      rc = ra + rb;
      do_znflags(rc);
      do_cvflag(ra, rb, 0);
      return ERR_NONE;
    }

    //CMP(1) compare immediate
    case Op::cmp1: {
      rb = (inst >> 0) & 0xFF;
      rn = (inst >> 8) & 0x07;
      DO_DISS("cmp r" << dec << rn << ",#0x" << Base::HEX2 << rb << '\n');
      ra = read_register(rn);
      rc = ra - rb;
      //fprintf(stderr,"0x%08X 0x%08X\n",ra,rb);
      do_znflags(rc);
      do_cvflag(ra, ~rb, 1);
      return ERR_NONE;
    }

    //CMP(2) compare register
    case Op::cmp2: {
      rn = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("cmps r" << dec << rn << ",r" << dec << rm << '\n');
      ra = read_register(rn);
      rb = read_register(rm);
      rc = ra - rb;
      //fprintf(stderr,"0x%08X 0x%08X\n",ra,rb);
      do_znflags(rc);
      do_cvflag(ra, ~rb, 1);
      return ERR_NONE;
    }

    //CMP(3) compare high register
    case Op::cmp3: {
      if(((inst >> 6) & 3) == 0x0)
      {
        //UNPREDICTABLE
      }
      rn = (inst >> 0) & 0x7;
      rn |= (inst >> 4) & 0x8;
      if(rn == 0xF)
      {
        //UNPREDICTABLE
      }
      rm = (inst >> 3) & 0xF;
      DO_DISS("cmps r" << dec << rn << ",r" << dec << rm << '\n');
      ra = read_register(rn);
      rb = read_register(rm);
      rc = ra - rb;
      do_znflags(rc);
      do_cvflag(ra, ~rb, 1);
      return ERR_NONE;
    }

    //CPY copy high register
    case Op::cpy: {
      //same as mov except you can use both low registers
      //going to let mov handle high registers
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("cpy r" << dec << rd << ",r" << dec << rm << '\n');
      rc = read_register(rm);
      write_register(rd, rc);
      return ERR_NONE;
    }

    //EOR
    case Op::eor: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("eors r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra ^ rb;
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    //LDMIA
    case Op::ldmia: {
      rn = (inst >> 8) & 0x7;
    #if defined(THUMB_DISS)
      {
        ostringstream s;

        s <<"ldmia r" << dec << rn << "!,{";
        for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,++ra)
        {
          if(inst&rb)
          {
            if(rc) s << ",";
            s << "r" << dec << ra;
            rc++;
          }
        }
        s << "}\n";

        cout << s.str();
      }
    #endif
      sp = read_register(rn);

      const std::array<uInt32, 16> regsOld = reg_norm;

      for(ra = 0, rb = 0x01; rb; rb = (rb << 1) & 0xFF, ++ra)
      {
        if(inst & rb)
        {
          const err_t err = read32(sp, reg_norm[ra]);

          if (err) {
            reg_norm = regsOld;
            return err;
          }

          sp += 4;
        }
      }
      // write back?
      if((inst & (1 << rn)) == 0)
        write_register(rn, sp);
      return ERR_NONE;
    }

    //LDR(1) two register immediate
    case Op::ldr1: {
      rd = (inst >> 0) & 0x07;
      rn = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      rb <<= 2;
      DO_DISS("ldr r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX2 << rb << "]\n");
      rb = read_register(rn) + rb;

      const err_t err = read32(rb, rc);
      if (err) return err;

      write_register(rd, rc);
      return ERR_NONE;
    }

    //LDR(2) three register
    case Op::ldr2: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS("ldr r" << dec << rd << ",[r" << dec << rn << ",r" << dec << "]\n");
      rb = read_register(rn) + read_register(rm);

      const err_t err = read32(rb, rc);
      if (err) return err;

      write_register(rd, rc);
      return ERR_NONE;
    }

    //LDR(3)
    case Op::ldr3: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x07;
      rb <<= 2;
      DO_DISS("ldr r" << dec << rd << ",[PC+#0x" << Base::HEX2 << rb << "] ");
      ra = read_register(15);
      ra &= ~3;
      rb += ra;
      DO_DISS(";@ 0x" << Base::HEX2 << rb << '\n');

      const err_t err = read32(rb, rc);
      if (err) return err;

      write_register(rd, rc);
      return ERR_NONE;
    }

    //LDR(4)
    case Op::ldr4: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x07;
      rb <<= 2;
      DO_DISS("ldr r" << dec << rd << ",[SP+#0x" << Base::HEX2 << rb << "]\n");
      ra = read_register(13);
      //ra&=~3;
      rb += ra;

      const err_t err = read32(rb, rc);
      if (err) return err;

      write_register(rd, rc);
      return ERR_NONE;
    }

    //LDRB(1)
    case Op::ldrb1: {
      rd = (inst >> 0) & 0x07;
      rn = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      DO_DISS("ldrb r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX2 << rb << "]\n");
      rb = read_register(rn) + rb;

      uInt8 val8 = 0;
      const err_t err = read8(rb, val8);
      if (err) return err;

      rc = val8;

      write_register(rd, rc & 0xFF);
      return ERR_NONE;
    }

    //LDRB(2)
    case Op::ldrb2: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS("ldrb r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]\n");
      rb = read_register(rn) + read_register(rm);

      uInt8 val8 = 0;
      const err_t err = read8(rb, val8);
      if (err) return err;

      rc = val8;

      write_register(rd, rc & 0xFF);
      return ERR_NONE;
    }

    //LDRH(1)
    case Op::ldrh1: {
      rd = (inst >> 0) & 0x07;
      rn = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      rb <<= 1;
      DO_DISS("ldrh r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX2 << rb << "]\n");
      rb = read_register(rn) + rb;

      uInt16 val16 = 0;
      const err_t err = read16(rb, val16);
      if (err) return err;

      rc = val16;

      write_register(rd, rc & 0xFFFF);
      return ERR_NONE;
    }

    //LDRH(2)
    case Op::ldrh2: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS("ldrh r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]\n");
      rb = read_register(rn) + read_register(rm);

      uInt16 val16 = 0;
      const err_t err = read16(rb, val16);
      if (err) return err;

      rc = val16;

      write_register(rd, rc & 0xFFFF);
      return ERR_NONE;
    }

    //LDRSB
    case Op::ldrsb: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS("ldrsb r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]\n");
      rb = read_register(rn) + read_register(rm);

      uInt8 val8 = 0;
      const err_t err = read8(rb, val8);
      if (err) return err;

      rc = (static_cast<Int32>(val8) << 24) >> 24;

      write_register(rd, rc);
      return ERR_NONE;
    }

    //LDRSH
    case Op::ldrsh: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS("ldrsh r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]\n");
      rb = read_register(rn) + read_register(rm);

      uInt16 val16 = 0;
      const err_t err = read16(rb, val16);
      if (err) return err;

      rc = (static_cast<Int16>(val16) << 8) >> 8;

      write_register(rd, rc);
      return ERR_NONE;
    }

    //LSL(1)
    case Op::lsl1: {
      rd = (inst >> 0) & 0x07;
      rm = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      DO_DISS("lsls r" << dec << rd << ",r" << dec << rm << ",#0x" << Base::HEX2 << rb << '\n');
      rc = read_register(rm);
      if(rb == 0)
      {
        //if immed_5 == 0
        //C unaffected
        //result not shifted
      }
      else
      {
        //else immed_5 > 0
        do_cflag_bit(rc & (1 << (32-rb)));
        rc <<= rb;
      }
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    //LSL(2) two register
    case Op::lsl2: {
      rd = (inst >> 0) & 0x07;
      rs = (inst >> 3) & 0x07;
      DO_DISS("lsls r" << dec << rd << ",r" << dec << rs << '\n');
      rc = read_register(rd);
      rb = read_register(rs);
      rb &= 0xFF;
      if(rb == 0)
      {
      }
      else if(rb < 32)
      {
        do_cflag_bit(rc & (1 << (32-rb)));
        rc <<= rb;
      }
      else if(rb == 32)
      {
        do_cflag_bit(rc & 1);
        rc = 0;
      }
      else
      {
        do_cflag_bit(0);
        rc = 0;
      }
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    //LSR(1) two register immediate
    case Op::lsr1: {
      rd = (inst >> 0) & 0x07;
      rm = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      DO_DISS("lsrs r" << dec << rd << ",r" << dec << rm << ",#0x" << Base::HEX2 << rb << '\n');
      rc = read_register(rm);
      if(rb == 0)
      {
        do_cflag_bit(rc & 0x80000000);
        rc = 0;
      }
      else
      {
        do_cflag_bit(rc & (1 << (rb-1)));
        rc >>= rb;
      }
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    //LSR(2) two register
    case Op::lsr2: {
      rd = (inst >> 0) & 0x07;
      rs = (inst >> 3) & 0x07;
      DO_DISS("lsrs r" << dec << rd << ",r" << dec << rs << '\n');
      rc = read_register(rd);
      rb = read_register(rs);
      rb &= 0xFF;
      if(rb == 0)
      {
      }
      else if(rb < 32)
      {
        do_cflag_bit(rc & (1 << (rb-1)));
        rc >>= rb;
      }
      else if(rb == 32)
      {
        do_cflag_bit(rc & 0x80000000);
        rc = 0;
      }
      else
      {
        do_cflag_bit(0);
        rc = 0;
      }
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    //MOV(1) immediate
    case Op::mov1: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x07;
      DO_DISS("movs r" << dec << rd << ",#0x" << Base::HEX2 << rb << '\n');
      write_register(rd, rb);
      do_znflags(rb);
      return ERR_NONE;
    }

    //MOV(2) two low registers
    case Op::mov2: {
      rd = (inst >> 0) & 7;
      rn = (inst >> 3) & 7;
      DO_DISS("movs r" << dec << rd << ",r" << dec << rn << '\n');
      rc = read_register(rn);
      //fprintf(stderr,"0x%08X\n",rc);
      write_register(rd, rc);
      do_znflags(rc);
      do_cflag_bit(0);
      do_vflag_bit(0);
      return ERR_NONE;
    }

    //MOV(3)
    case Op::mov3: {
      rd  = (inst >> 0) & 0x7;
      rd |= (inst >> 4) & 0x8;
      rm  = (inst >> 3) & 0xF;
      DO_DISS("mov r" << dec << rd << ",r" << dec << rm << '\n');
      rc = read_register(rm);
      if((rd == 14) && (rm == 15))
      {
        //printf("mov lr,pc warning 0x%08X\n",pc-2);
        //rc|=1;
      }
      if(rd == 15)
      {
        //rc &= ~1; //write_register may do this as well
        rc += 2;  //The program counter is special
      }
      write_register(rd, rc);
      return ERR_NONE;
    }

    //MUL
    case Op::mul: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("muls r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra * rb;
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    //MVN
    case Op::mvn: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("mvns r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rm);
      rc = (~ra);
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    //NEG
    case Op::neg: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("negs r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rm);
      rc = 0 - ra;
      write_register(rd, rc);
      do_znflags(rc);
      do_cvflag(0, ~ra, 1);
      return ERR_NONE;
    }

    //ORR
    case Op::orr: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("orrs r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra | rb;
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    //POP
    case Op::pop: {
    #if defined(THUMB_DISS)
      {
        ostringstream s;

        s << "pop {";
        for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,++ra)
        {
          if(inst&rb)
          {
            if(rc) s << ",";
            s << "r" << dec << ra;
            rc++;
          }
        }
        if(inst&0x100)
        {
          if(rc) s << ",";
          s << "pc";
        }
        s << "}\n";

        cout << s.str();
      }
    #endif

      const std::array<uInt32, 16> regOld = reg_norm;

      sp = read_register(13);
      for(ra = 0, rb = 0x01; rb; rb = (rb << 1) & 0xFF, ++ra)
      {
        if(inst & rb)
        {
          const err_t err = read32(sp, reg_norm[ra]);
          if (err) {
            reg_norm = regOld;
            return err;
          }

          sp += 4;
        }
      }

      if(inst & 0x100)
      {
        const err_t err = read32(sp, rc);
        if (err) {
          reg_norm = regOld;
          return err;
        }

        if ((rc & 0x01) == 0) {
          reg_norm = regOld;
          return errIntrinsic(ERR_INVALID_OPERATING_MODE, read_register(15) - 4);
        }

        rc += 2;
        write_register(15, rc & ~0x01);
        sp += 4;
      }
      write_register(13, sp);
      return ERR_NONE;
    }

    //PUSH
    case Op::push: {
    #if defined(THUMB_DISS)
      {
        ostringstream s;

        s << "push {";
        for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,++ra)
        {
          if(inst&rb)
          {
            if(rc) s << ",";
            s << "r" << dec << ra;
            rc++;
          }
        }
        if(inst&0x100)
        {
          if(rc) s << ",";
          s << "lr";
        }
        s << "}\n";

        cout << s.str();
      }
    #endif

      sp = read_register(13);
      //fprintf(stderr,"sp 0x%08X\n",sp);
      for(ra = 0, rb = 0x01, rc = 0; rb; rb = (rb << 1) & 0xFF, ++ra)
      {
        if(inst & rb)
        {
          ++rc;
        }
      }
      if(inst & 0x100) ++rc;
      rc <<= 2;
      sp -= rc;

      rd = sp;
      for(ra = 0, rb = 0x01; rb; rb = (rb << 1) & 0xFF, ++ra)
      {
        if(inst & rb)
        {
          const err_t err = write32(rd, read_register(ra));
          if (err) return err;

          rd += 4;
        }
      }
      if(inst & 0x100)
      {
        rc = read_register(14);
        const err_t err = write32(rd, rc);
        if (err) return err;
      }
      write_register(13, sp);
      return ERR_NONE;
    }

    //REV
    case Op::rev: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      DO_DISS("rev r" << dec << rd << ",r" << dec << rn << '\n');
      ra = read_register(rn);
      rc  = ((ra >>  0) & 0xFF) << 24;
      rc |= ((ra >>  8) & 0xFF) << 16;
      rc |= ((ra >> 16) & 0xFF) <<  8;
      rc |= ((ra >> 24) & 0xFF) <<  0;
      write_register(rd, rc);
      return ERR_NONE;
    }

    //REV16
    case Op::rev16: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      DO_DISS("rev16 r" << dec << rd << ",r" << dec << rn << '\n');
      ra = read_register(rn);
      rc  = ((ra >>  0) & 0xFF) <<  8;
      rc |= ((ra >>  8) & 0xFF) <<  0;
      rc |= ((ra >> 16) & 0xFF) << 24;
      rc |= ((ra >> 24) & 0xFF) << 16;
      write_register(rd, rc);
      return ERR_NONE;
    }

    //REVSH
    case Op::revsh: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      DO_DISS("revsh r" << dec << rd << ",r" << dec << rn << '\n');
      ra = read_register(rn);
      rc  = ((ra >> 0) & 0xFF) << 8;
      rc |= ((ra >> 8) & 0xFF) << 0;
      if(rc & 0x8000) rc |= 0xFFFF0000;
      else            rc &= 0x0000FFFF;
      write_register(rd, rc);
      return ERR_NONE;
    }

    //ROR
    case Op::ror: {
      rd = (inst >> 0) & 0x7;
      rs = (inst >> 3) & 0x7;
      DO_DISS("rors r" << dec << rd << ",r" << dec << rs << '\n');
      rc = read_register(rd);
      ra = read_register(rs);
      ra &= 0xFF;
      if(ra == 0)
      {
      }
      else
      {
        ra &= 0x1F;
        if(ra == 0)
        {
          do_cflag_bit(rc & 0x80000000);
        }
        else
        {
          do_cflag_bit(rc & (1 << (ra-1)));
          rb = rc << (32-ra);
          rc >>= ra;
          rc |= rb;
        }
      }
      write_register(rd, rc);
      do_znflags(rc);
      return ERR_NONE;
    }

    //SBC
    case Op::sbc: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("sbc r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra - rb;
      if(!cFlag) --rc;
      write_register(rd, rc);
      do_znflags(rc);
      if(cFlag) do_cvflag(ra, ~rb, 1);
      else      do_cvflag(ra, ~rb, 0);
      return ERR_NONE;
    }

#ifndef UNSAFE_OPTIMIZATIONS
    //SETEND
    case Op::setend: {
      return errIntrinsic(ERR_UNIMPLEMENTED_INST, read_register(15) - 4);
    }
#endif

    //STMIA
    case Op::stmia: {
      rn = (inst >> 8) & 0x7;
    #if defined(THUMB_DISS)
      {
        ostringstream s;

        s << "stmia r" << dec << rn << "!,{";
        for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,++ra)
        {
          if(inst & rb)
          {
            if(rc) s << ",";
            s << "r" << dec << ra;
            rc++;
          }
        }
        s << "}\n";

        cout << s.str();
      }
    #endif

      sp = read_register(rn);
      for(ra = 0, rb = 0x01; rb; rb = (rb << 1) & 0xFF, ++ra)
      {
        if(inst & rb)
        {
          const err_t err = write32(sp, read_register(ra));
          if (err) return err;

          sp += 4;
        }
      }
      write_register(rn, sp);
      return ERR_NONE;
    }

    //STR(1)
    case Op::str1: {
      rd = (inst >> 0) & 0x07;
      rn = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      rb <<= 2;
      DO_DISS("str r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX2 << rb << "]\n");
      rb = read_register(rn) + rb;
      rc = read_register(rd);

      const err_t err = write32(rb, rc);
      if (err) return err;

      return ERR_NONE;
    }

    //STR(2)
    case Op::str2: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS("str r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]\n");
      rb = read_register(rn) + read_register(rm);
      rc = read_register(rd);

      const err_t err = write32(rb, rc);
      if (err) return err;

      return ERR_NONE;
    }

    //STR(3)
    case Op::str3: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x07;
      rb <<= 2;
      DO_DISS("str r" << dec << rd << ",[SP,#0x" << Base::HEX2 << rb << "]\n");
      rb = read_register(13) + rb;
      //fprintf(stderr,"0x%08X\n",rb);
      rc = read_register(rd);

      const err_t err = write32(rb, rc);
      if (err) return err;

      return ERR_NONE;
    }

    //STRB(1)
    case Op::strb1: {
      rd = (inst >> 0) & 0x07;
      rn = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      DO_DISS("strb r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX8 << rb << "]\n");
      rb = read_register(rn) + rb;
      rc = read_register(rd);

      const err_t err = write8(rb, rc);
      if (err) return err;

      return ERR_NONE;
    }

    //STRB(2)
    case Op::strb2: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS("strb r" << dec << rd << ",[r" << dec << rn << ",r" << rm << "]\n");
      rb = read_register(rn) + read_register(rm);
      rc = read_register(rd);

      const err_t err = write8(rb, rc);
      if (err) return err;

      return ERR_NONE;
    }

    //STRH(1)
    case Op::strh1: {
      rd = (inst >> 0) & 0x07;
      rn = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      rb <<= 1;
      DO_DISS("strh r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX2 << rb << "]\n");
      rb = read_register(rn) + rb;
      rc=  read_register(rd);

      const err_t err = write16(rb, rc & 0xFFFF);
      if (err) return err;

      return ERR_NONE;
    }

    //STRH(2)
    case Op::strh2: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS("strh r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]\n");
      rb = read_register(rn) + read_register(rm);
      rc = read_register(rd);

      const err_t err = write16(rb, rc & 0xFFFF);
      if (err) return err;

      return ERR_NONE;
    }

    //SUB(1)
    case Op::sub1: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rb = (inst >> 6) & 0x7;
      DO_DISS("subs r" << dec << rd << ",r" << dec << rn << ",#0x" << Base::HEX2 << rb << '\n');
      ra = read_register(rn);
      rc = ra - rb;
      write_register(rd, rc);
      do_znflags(rc);
      do_cvflag(ra, ~rb, 1);
      return ERR_NONE;
    }

    //SUB(2)
    case Op::sub2: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x07;
      DO_DISS("subs r" << dec << rd << ",#0x" << Base::HEX2 << rb << '\n');
      ra = read_register(rd);
      rc = ra - rb;
      write_register(rd, rc);
      do_znflags(rc);
      do_cvflag(ra, ~rb, 1);
      return ERR_NONE;
    }

    //SUB(3)
    case Op::sub3: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS("subs r" << dec << rd << ",r" << dec << rn << ",r" << dec << rm << '\n');
      ra = read_register(rn);
      rb = read_register(rm);
      rc = ra - rb;
      write_register(rd, rc);
      do_znflags(rc);
      do_cvflag(ra, ~rb, 1);
      return ERR_NONE;
    }

    //SUB(4)
    case Op::sub4: {
      rb = inst & 0x7F;
      rb <<= 2;
      DO_DISS("sub SP,#0x" << Base::HEX2 << rb << '\n');
      ra = read_register(13);
      ra -= rb;
      write_register(13, ra);
      return ERR_NONE;
    }

    //SWI
    case Op::swi:
      DO_DISS("\n\nswi 0x" << Base::HEX2 << (inst & 0xff) << '\n');
      return errIntrinsic(ERR_SWI, inst & 0xff);

    //SXTB
    case Op::sxtb: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("sxtb r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rm);
      rc = ra & 0xFF;
      if(rc & 0x80)
        rc |= (~0U) << 8;
      write_register(rd, rc);
      return ERR_NONE;
    }

    //SXTH
    case Op::sxth: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("sxth r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rm);
      rc = ra & 0xFFFF;
      if(rc & 0x8000)
        rc |= (~0U) << 16;
      write_register(rd, rc);
      return ERR_NONE;
    }

    //TST
    case Op::tst: {
      rn = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("tst r" << dec << rn << ",r" << dec << rm << '\n');
      ra = read_register(rn);
      rb = read_register(rm);
      rc = ra & rb;
      do_znflags(rc);
      return ERR_NONE;
    }

    //UXTB
    case Op::uxtb: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("uxtb r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rm);
      rc = ra & 0xFF;
      write_register(rd, rc);
      return ERR_NONE;
    }

    //UXTH
    case Op::uxth: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS("uxth r" << dec << rd << ",r" << dec << rm << '\n');
      ra = read_register(rm);
      rc = ra & 0xFFFF;
      write_register(rd, rc);
      return ERR_NONE;
    }

    default:
      return errIntrinsic(ERR_UNDEFINED_INST, read_register(15) - 4);
  }
}
