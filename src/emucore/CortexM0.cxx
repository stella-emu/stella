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

#ifdef __BIG_ENDIAN__
  #define READ32(data, addr) (              \
    (((uInt8*)(data))[(addr)]) |            \
    (((uInt8*)(data))[(addr) + 1]  << 8) |  \
    (((uInt8*)(data))[(addr) + 2] << 16) |  \
    (((uInt8*)(data))[(addr) + 3] << 24)    \
  )

  #define READ16(data, addr) (              \
    (((uInt8*)(data))[(addr)]) |            \
    (((uInt8*)(data))[(addr) + 1]  << 8)    \
  )

  #define WRITE32(data, addr, value)                \
    ((uInt8*)(data))[(addr)] = (value);             \
    ((uInt8*)(data))[(addr) + 1] = (value) >> 8;    \
    ((uInt8*)(data))[(addr) + 2] = (value) >> 16;   \
    ((uInt8*)(data))[(addr) + 3] = (value) >> 24;

  #define WRITE16(data, addr, value)              \
    ((uInt8*)(data))[(addr)] = value;             \
    ((uInt8*)(data))[(addr) + 1] = (value) >> 8;
#else
  #define READ32(data, addr) (((uInt32*)(data))[(addr) >> 2])
  #define READ16(data, addr) (((uInt16*)(data))[(addr) >> 1])

  #define WRITE32(data, addr, value) ((uInt32*)(data))[(addr) >> 2] = value;
  #define WRITE16(data, addr, value) ((uInt16*)(data))[(addr) >> 1] = value;
#endif

#define read_register(reg)        reg_norm[reg]
#define write_register(reg, data) reg_norm[reg]=(data)

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
      bl, blx_thumb, blx_arm,
      blx2,
      bx,
      cmn,
      cmp1, cmp2, cmp3,
      cps,
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
        else if((inst & 0x1800) == 0x0800) return Op::blx_arm;
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

      //CPS
      if((inst & 0xFFE8) == 0xB660) return Op::cps;

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
}

CortexM0::CortexM0()
{
  myPageMap = make_unique<uInt8[]>(PAGEMAP_SIZE);
  std::memset(myPageMap.get(), 0xff, PAGEMAP_SIZE);

  reset();
}

CortexM0& CortexM0::mapRegionData(uInt32 pageBase,
                                  uInt32 pageCount, bool readOnly, uInt8* backingStore)
{
  MemoryRegion& region =
    setupMapping(pageBase, pageCount, readOnly, MemoryRegionType::directData);

  region.access.accessData.backingStore = backingStore;

  return *this;
}

CortexM0& CortexM0::mapRegionCode(uInt32 pageBase,
                                  uInt32 pageCount, bool readOnly, uInt8* backingStore)
{
  MemoryRegion& region =
    setupMapping(pageBase, pageCount, readOnly, MemoryRegionType::directCode);

  region.access.accessCode.backingStore = backingStore;
  region.access.accessCode.ops = static_cast<uInt8*>(std::malloc((pageCount * PAGE_SIZE) >> 1));

  for (size_t i = 0; i < pageCount * PAGE_SIZE; i += 2)
    region.access.accessCode.ops[i >> 1] =
      decodeInstructionWord(READ16(backingStore, i));

  return *this;
}

CortexM0& CortexM0::mapRegionDelegate(uInt32 pageBase, uInt32 pageCount, bool readOnly,
                                      CortexM0::BusTransactionDelegate* delegate)
{
  MemoryRegion& region =
    setupMapping(pageBase, pageCount, readOnly, MemoryRegionType::delegate);

  region.access.delegate = delegate;

  return *this;
}

CortexM0& CortexM0::mapDefault(CortexM0::BusTransactionDelegate* delegate)
{
  myDefaultDelegate = delegate;

  return *this;
}

CortexM0& CortexM0::reset()
{
  reg_norm.fill(0);
  znFlags = cFlag = vFlag = 0;

  return *this;
}

CortexM0& CortexM0::setRegister(uInt8 regno, uInt32 value)
{
  write_register(regno, value);

  return *this;
}

uInt8 CortexM0::decodeInstructionWord(uInt16 instructionWord)
{
  return static_cast<uInt8>(::decodeInstructionWord(instructionWord));
}

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

CortexM0::BusTransactionResult CortexM0::read32(uInt32 address, uInt32& value)
{
  MemoryRegion& region = myRegions[myPageMap[address / PAGE_SIZE]];

  switch (region.type) {
    case MemoryRegionType::delegate:
      return region.access.delegate->read32(address, value);

    case MemoryRegionType::directCode:
      value = READ32(region.access.accessCode.backingStore, address - region.base);
      return BusTransactionResult::ok;

    case MemoryRegionType::directData:
      value = READ32(region.access.accessData.backingStore, address - region.base);
      return BusTransactionResult::ok;

    default:
      return myDefaultDelegate ?
        myDefaultDelegate->read32(address, value) : BusTransactionResult::fail;
  }
}

CortexM0::BusTransactionResult CortexM0::read16(uInt32 address, uInt16& value)
{
  MemoryRegion& region = myRegions[myPageMap[address / PAGE_SIZE]];

  switch (region.type) {
    case MemoryRegionType::delegate:
      return region.access.delegate->read16(address, value);

    case MemoryRegionType::directCode:
      value = READ16(region.access.accessCode.backingStore, address - region.base);
      return BusTransactionResult::ok;

    case MemoryRegionType::directData:
      value = READ16(region.access.accessData.backingStore, address - region.base);
      return BusTransactionResult::ok;

    default:
      return myDefaultDelegate ?
        myDefaultDelegate->read16(address, value) : BusTransactionResult::fail;
  }
}

CortexM0::BusTransactionResult CortexM0::write32(uInt32 address, uInt32 value)
{
  MemoryRegion& region = myRegions[myPageMap[address / PAGE_SIZE]];

  switch (region.type) {
    case MemoryRegionType::delegate:
      return region.access.delegate->write32(address, value);

    case MemoryRegionType::directCode:
      WRITE32(region.access.accessCode.backingStore, address - region.base, value);
      return BusTransactionResult::ok;

    case MemoryRegionType::directData:
      WRITE32(region.access.accessData.backingStore, address - region.base, value);
      return BusTransactionResult::ok;

    default:
      return myDefaultDelegate ?
        myDefaultDelegate->write32(address, value) : BusTransactionResult::fail;
  }
}

CortexM0::BusTransactionResult CortexM0::write16(uInt32 address, uInt16 value)
{
  MemoryRegion& region = myRegions[myPageMap[address / PAGE_SIZE]];

  switch (region.type) {
    case MemoryRegionType::delegate:
      return region.access.delegate->write16(address, value);

    case MemoryRegionType::directCode: {
      const uInt32 offset = address - region.base;

      WRITE16(region.access.accessCode.backingStore, offset, value);
      region.access.accessCode.ops[offset >> 1] = decodeInstructionWord(value);

      return BusTransactionResult::ok;
    }

    case MemoryRegionType::directData:
      WRITE16(region.access.accessData.backingStore, address - region.base, value);
      return BusTransactionResult::ok;

    default:
      return myDefaultDelegate ?
        myDefaultDelegate->write16(address, value) : BusTransactionResult::fail;
  }
}

CortexM0::BusTransactionResult CortexM0::fetch16(uInt32 address, uInt16& value, uInt8& op)
{
  MemoryRegion& region = myRegions[myPageMap[address / PAGE_SIZE]];

  switch (region.type) {
    case MemoryRegionType::delegate:
      return region.access.delegate->fetch16(address, value, op);

    case MemoryRegionType::directCode: {
      const uInt32 offset = address - region.base;

      value = READ16(region.access.accessCode.backingStore, offset);
      op = region.access.accessCode.ops[offset >> 1];

      return BusTransactionResult::ok;
    }

    case MemoryRegionType::directData:
      value = READ16(region.access.accessCode.backingStore, address - region.base);
      op = decodeInstructionWord(value);

      return BusTransactionResult::ok;

    default:
      return myDefaultDelegate ?
        myDefaultDelegate->fetch16(address, value, op) : BusTransactionResult::fail;
  }
}

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
