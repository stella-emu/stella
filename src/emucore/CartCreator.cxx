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

#include "bspf.hxx"
#include "Cart.hxx"
#include "Cart03E0.hxx"
#include "Cart0840.hxx"
#include "Cart0FA0.hxx"
#include "Cart2K.hxx"
#include "Cart3E.hxx"
#include "Cart3EX.hxx"
#include "Cart3EPlus.hxx"
#include "Cart3F.hxx"
#include "Cart4A50.hxx"
#include "Cart4K.hxx"
#include "Cart4KSC.hxx"
#include "CartAR.hxx"
#include "CartBF.hxx"
#include "CartBFSC.hxx"
#include "CartBUS.hxx"
#include "CartCDF.hxx"
#include "CartCM.hxx"
#include "CartCTY.hxx"
#include "CartCV.hxx"
#include "CartDevCard.hxx"
#include "CartDF.hxx"
#include "CartDFSC.hxx"
#include "CartDPC.hxx"
#include "CartDPCPlus.hxx"
#include "CartE0.hxx"
#include "CartE7.hxx"
#include "CartEF.hxx"
#include "CartEFSC.hxx"
#include "CartEFF.hxx"
#include "CartF0.hxx"
#include "CartF4.hxx"
#include "CartF4SC.hxx"
#include "CartF6.hxx"
#include "CartF6SC.hxx"
#include "CartF8.hxx"
#include "CartF8SC.hxx"
#include "CartFA.hxx"
#include "CartFA2.hxx"
#include "CartFC.hxx"
#include "CartFE.hxx"
#include "CartGL.hxx"
#include "CartJANE.hxx"
#include "CartMDM.hxx"
#include "CartMVC.hxx"
#include "CartSB.hxx"
#include "CartTVBoy.hxx"
#include "CartUA.hxx"
#include "CartWD.hxx"
#include "CartWF8.hxx"
#include "CartX07.hxx"
#include "CartELF.hxx"
#include "MD5.hxx"
#include "Settings.hxx"

#include "CartDetector.hxx"
#include "CartCreator.hxx"

namespace  // anonymous namespace, to keep these functions private
{
  /**
    Create a cartridge from the entire image pointer.

    @param image    A pointer to the complete ROM image
    @param type     The bankswitch type of the ROM image
    @param md5      The md5sum for the ROM image
    @param settings The settings container

    @return  Pointer to the new cartridge object allocated on the heap
  */
  unique_ptr<Cartridge>
  createFromImage(ByteSpan image, Bankswitch::Type type, string_view md5,
                  Settings& settings)
  {
    // We should know the cart's type by now so let's create it
    switch(type)
    {
      using enum Bankswitch::Type;
      case _03E0: return std::make_unique<Cartridge03E0>(image, md5, settings);
      case _0840: return std::make_unique<Cartridge0840>(image, md5, settings);
      case _0FA0: return std::make_unique<Cartridge0FA0>(image, md5, settings);
      case _2K:   return std::make_unique<Cartridge2K>(image, md5, settings);
      case _3E:   return std::make_unique<Cartridge3E>(image, md5, settings);
      case _3EX:  return std::make_unique<Cartridge3EX>(image, md5, settings);
      case _3EP:  return std::make_unique<Cartridge3EPlus>(image, md5, settings);
      case _3F:   return std::make_unique<Cartridge3F>(image, md5, settings);
      case _4A50: return std::make_unique<Cartridge4A50>(image, md5, settings);
      case _4K:   return std::make_unique<Cartridge4K>(image, md5, settings);
      case _4KSC: return std::make_unique<Cartridge4KSC>(image, md5, settings);
      case AR:    return std::make_unique<CartridgeAR>(image, md5, settings);
      case BF:    return std::make_unique<CartridgeBF>(image, md5, settings);
      case BFSC:  return std::make_unique<CartridgeBFSC>(image, md5, settings);
      case BUS:   return std::make_unique<CartridgeBUS>(image, md5, settings);
      case CDF:   return std::make_unique<CartridgeCDF>(image, md5, settings);
      case CM:    return std::make_unique<CartridgeCM>(image, md5, settings);
      case CTY:   return std::make_unique<CartridgeCTY>(image, md5, settings);
      case CV:    return std::make_unique<CartridgeCV>(image, md5, settings);
      case DEVC:  return std::make_unique<CartridgeDevCard>(image, md5, settings);
      case DF:    return std::make_unique<CartridgeDF>(image, md5, settings);
      case DFSC:  return std::make_unique<CartridgeDFSC>(image, md5, settings);
      case DPC:   return std::make_unique<CartridgeDPC>(image, md5, settings);
      case DPCP:  return std::make_unique<CartridgeDPCPlus>(image, md5, settings);
      case E0:    return std::make_unique<CartridgeE0>(image, md5, settings);
      case E7:    return std::make_unique<CartridgeE7>(image, md5, settings);
      case EF:    return std::make_unique<CartridgeEF>(image, md5, settings);
      case EFF:   return std::make_unique<CartridgeEFF>(image, md5, settings);
      case EFSC:  return std::make_unique<CartridgeEFSC>(image, md5, settings);
      case F0:    return std::make_unique<CartridgeF0>(image, md5, settings);
      case F4:    return std::make_unique<CartridgeF4>(image, md5, settings);
      case F4SC:  return std::make_unique<CartridgeF4SC>(image, md5, settings);
      case F6:    return std::make_unique<CartridgeF6>(image, md5, settings);
      case F6SC:  return std::make_unique<CartridgeF6SC>(image, md5, settings);
      case F8:    return std::make_unique<CartridgeF8>(image, md5, settings);
      case F8SC:  return std::make_unique<CartridgeF8SC>(image, md5, settings);
      case FA:    return std::make_unique<CartridgeFA>(image, md5, settings);
      case FA2:   return std::make_unique<CartridgeFA2>(image, md5, settings);
      case FC:    return std::make_unique<CartridgeFC>(image, md5, settings);
      case FE:    return std::make_unique<CartridgeFE>(image, md5, settings);
      case GL:    return std::make_unique<CartridgeGL>(image, md5, settings);
      case JANE:  return std::make_unique<CartridgeJANE>(image, md5, settings);
      case MDM:   return std::make_unique<CartridgeMDM>(image, md5, settings);
      case UA:    return std::make_unique<CartridgeUA>(image, md5, settings);
      case UASW:  return std::make_unique<CartridgeUA>(image, md5, settings, true);
      case SB:    return std::make_unique<CartridgeSB>(image, md5, settings);
      case TVBOY: return std::make_unique<CartridgeTVBoy>(image, md5, settings);
      case WD:    [[fallthrough]];
      case WDSW:  return std::make_unique<CartridgeWD>(image, md5, settings);
      case WF8:   return std::make_unique<CartridgeWF8>(image, md5, settings);
      case X07:   return std::make_unique<CartridgeX07>(image, md5, settings);
      case ELF:   return std::make_unique<CartridgeELF>(image, md5, settings);
      default:    return nullptr;  // The remaining types have already been handled
    }
  }

  /**
    Create a cartridge from a multi-cart image pointer; internally this
    takes a slice of the ROM image ues that for the cartridge.

    @param image    A pointer to the complete ROM image
    @param numRoms  The number of ROMs in the multicart
    @param md5      The md5sum for the slice of the ROM image
    @param type     The detected type of the slice of the ROM image
    @param id       The ID for the slice of the ROM image
    @param settings The settings container

    @return  Pointer to the new cartridge object allocated on the heap
  */
  unique_ptr<Cartridge>
  createFromMultiCart(ByteSpan image, uInt32 numRoms, string& md5,
                      Bankswitch::Type& type, string& id, Settings& settings)
  {
    // Get a piece of the larger image
    uInt32 i = settings.getInt("romloadcount");

    // Move to the next game
    if(!settings.getBool("romloadprev"))
      i = (i + 1) % numRoms;
    else
      i = (i - 1) % numRoms;
    settings.setValue("romloadcount", i);

    const size_t size = image.size() / numRoms;
    const ByteSpan slice = image.subspan(i * size, size);

    // We need a new md5 and name
    md5 = MD5::hash(slice);
    id = std::format(" [G{}]", i + 1);

    // TODO: allow using ROM properties instead of autodetect only
    if(size <= 2_KB)
      type = Bankswitch::Type::_2K;
    else if(size == 4_KB)
      type = Bankswitch::Type::_4K;
    else if(size == 8_KB || size == 16_KB || size == 32_KB || size == 64_KB || size == 128_KB)
      type = CartDetector::autodetectType(slice);
    else  /* default */
      type = Bankswitch::Type::_4K;

    return createFromImage(slice, type, md5, settings);
  }
  /**
    Strip a leading collection-index prefix of the form "NN - " from a stem.
    E.g. "05 - Escape From The Mindmaster Load 2 (Ntsc)" → "Escape From..."
  */
  string_view stripCollectionPrefix(string_view stem)
  {
    size_t p = 0;
    while(p < stem.size() && std::isdigit(static_cast<unsigned char>(stem[p])))
      ++p;
    if(p > 0 && p + 2 < stem.size() && stem.substr(p, 3) == " - ")
      return stem.substr(p + 3);
    return stem;
  }

  /**
    Find a load-indicator token ("Load N", "Side N", "Tape N", "Part N",
    "Disk N") in a stem.  Returns {prefix, loadNum} where prefix is the stem
    text before the token (used to match companions), or {"", 0} if no token
    is found.  The suffix after the number is intentionally dropped because
    companion tapes may carry per-load subtitles that differ.
  */
  std::pair<string, int> extractLoadInfo(string_view stem)
  {
    const string_view s = stripCollectionPrefix(stem);

    string lower{s};
    BSPF::toLowerCase(lower);

    static constexpr std::array<string_view, 5> tokens{
      "load ", "side ", "tape ", "part ", "disk "
    };

    for(string_view tok : tokens)
    {
      const size_t pos = lower.rfind(tok);
      if(pos == string::npos) continue;
      const size_t nStart = pos + tok.size();
      if(nStart >= s.size() ||
         !std::isdigit(static_cast<unsigned char>(s[nStart]))) continue;
      size_t nEnd = nStart;
      while(nEnd < s.size() &&
            std::isdigit(static_cast<unsigned char>(s[nEnd]))) ++nEnd;
      const int num = BSPF::stoi(s.substr(nStart, nEnd - nStart));
      // Return only the prefix before the token — the suffix can differ
      // between companion tapes (e.g. per-side subtitles), so we must not
      // include it in the comparison key.
      return {string{s.substr(0, pos)}, num};
    }
    return {"", 0};
  }

  /**
    Look for sequentially-numbered companion tape files alongside a Supercharger
    audio file.

    Two strategies are tried in order:

    1. Load-indicator scan (handles collection-prefix mismatches):
       Strips any leading "NN - " prefix, finds a token such as "Load N",
       lists the parent directory, and collects every sibling file whose
       extension matches and whose core name (token replaced with "{}") equals
       ours.  Companions are sorted by load number and returned consecutively.

    2. Digit-substitution probe (fallback for simple numbered names like
       "Game (1).mp3"):  The rightmost digit sequence is incremented and
       successive filenames are probed directly.

    @param firstTape  The file the user opened
    @return  Ordered list of companion files to append (not including firstTape)
  */
  std::vector<FSNode> findCompanionTapes(const FSNode& firstTape)
  {
    const string& fileName = firstTape.getName();
    const size_t dotPos = fileName.rfind('.');
    if(dotPos == string::npos) return {};
    const string stem = fileName.substr(0, dotPos);
    const string ext  = fileName.substr(dotPos);

    // Strategy 1: load-indicator token + directory scan
    const auto [myCore, myLoadNum] = extractLoadInfo(stem);
    if(!myCore.empty())
    {
      // Extract the last (...) group (e.g. "(ntsc)", "(pal)") so that NTSC
      // and PAL variants in the same directory don't produce duplicate load
      // numbers that break the consecutive check.
      const auto trailingTag = [](string_view s) -> string {
        const size_t rp = s.rfind(')');
        if(rp == string::npos) return {};
        const size_t lp = s.rfind('(', rp);
        if(lp == string::npos) return {};
        string tag{s.substr(lp, rp - lp + 1)};
        BSPF::toLowerCase(tag);
        return tag;
      };
      const string myTag = trailingTag(stripCollectionPrefix(stem));

      FSList siblings;
      firstTape.getParent().getChildren(siblings, FSNode::ListMode::FilesOnly);

      std::vector<std::pair<int, FSNode>> candidates;
      for(FSNode& sib : siblings)
      {
        const string& sibName = sib.getName();
        if(!BSPF::endsWithIgnoreCase(sibName, ext)) continue;
        if(sibName == fileName) continue;
        const size_t sibDot = sibName.rfind('.');
        if(sibDot == string::npos) continue;
        const string sibStem = sibName.substr(0, sibDot);
        const auto [sibCore, sibNum] = extractLoadInfo(sibStem);
        if(sibCore != myCore || sibNum <= myLoadNum) continue;
        if(!myTag.empty() && trailingTag(stripCollectionPrefix(sibStem)) != myTag) continue;
        candidates.emplace_back(sibNum, std::move(sib));
      }

      if(!candidates.empty())
      {
        std::ranges::sort(candidates,
          [](const auto& a, const auto& b){ return a.first < b.first; });
        std::vector<FSNode> result;
        int expected = myLoadNum + 1;
        for(auto& [num, node] : candidates)
        {
          if(num != expected) break;
          result.push_back(std::move(node));
          ++expected;
        }
        return result;
      }
    }

    // Strategy 2: digit-substitution probe (fallback)
    const string& fullPath = firstTape.getPath();
    const string dirPath = fullPath.substr(0, fullPath.size() - fileName.size());

    int seqEnd = static_cast<int>(stem.size()) - 1;
    while(seqEnd >= 0)
    {
      if(!std::isdigit(static_cast<unsigned char>(stem[seqEnd])))
      {
        --seqEnd;
        continue;
      }
      int seqStart = seqEnd;
      while(seqStart > 0 &&
            std::isdigit(static_cast<unsigned char>(stem[seqStart - 1])))
        --seqStart;

      const int    curNum = BSPF::stoi(stem.substr(seqStart, seqEnd - seqStart + 1));
      const int    width  = seqEnd - seqStart + 1;
      const string prefix = stem.substr(0, seqStart);
      const string suffix = stem.substr(seqEnd + 1);

      const FSNode nextFile(std::format("{}{}{:0{}}{}{}", dirPath, prefix, curNum + 1, width, suffix, ext));
      if(nextFile.isFile() && nextFile.isReadable())
      {
        std::vector<FSNode> companions;
        for(int n = curNum + 1; ; ++n)
        {
          FSNode f(std::format("{}{}{:0{}}{}{}", dirPath, prefix, n, width, suffix, ext));
          if(!f.isFile() || !f.isReadable()) break;
          companions.push_back(std::move(f));
        }
        return companions;
      }
      seqEnd = seqStart - 1;
    }

    return {};
  }

  /**
    Create a CartridgeAR in sound-load mode from a WAV or MP3 tape file.
    Locates the Supercharger BIOS ROM in baseDir, loads and concatenates all
    companion tapes, and returns the ready-to-run cartridge.  Returns nullptr
    if the file is not a WAV/MP3, the BIOS is missing, or PCM loading fails.
  */
  unique_ptr<Cartridge>
  createFromSoundLoad(const FSNode& file, string& md5,
                      Settings& settings, const FSNode& baseDir)
  {
    const string_view path = file.getPath();
    if(!BSPF::endsWithIgnoreCase(path, ".wav") &&
       !BSPF::endsWithIgnoreCase(path, ".mp3"))
      return nullptr;

    static constexpr std::array<string_view, 4> biosNames{
      "Supercharger BIOS.bin", "Supercharger.BIOS.bin",
      "Supercharger_BIOS.bin", "supercharger_bios.bin"
    };

    ByteArray biosData;
    for(const auto& name : biosNames)
    {
      FSNode biosFile{baseDir};
      biosFile /= name;
      ByteArray tmp;
      if(biosFile.isFile() && biosFile.read(tmp) == 2048)
      {
        biosData = std::move(tmp);
        break;
      }
    }

    if(biosData.empty())
    {
      cerr << std::format(
        "CartCreator: Supercharger BIOS not found in '{}'\n"
        "  (looked for: Supercharger BIOS.bin, Supercharger.BIOS.bin, "
        "Supercharger_BIOS.bin, supercharger_bios.bin)\n",
        baseDir.getPath());
      return nullptr;
    }

    auto [pcmData, sampleRate] = CartridgeAR::loadPCM(file);
    if(pcmData.empty())
    {
      cerr << std::format("CartCreator: failed to load PCM from '{}'\n", path);
      return nullptr;
    }
    cerr << std::format("CartCreator: tape 1: '{}' ({} samples @ {} Hz)\n",
                        file.getName(), pcmData.size(), sampleRate);

    // Probe for sequentially-numbered companion tapes and concatenate them
    // with a ~2-second silence gap so the BIOS can sync to each in turn.
    const auto companions = findCompanionTapes(file);
    if(companions.empty())
      cerr << "CartCreator: no companion tapes found\n";

    int tapeNum = 2;
    for(const FSNode& companion : companions)
    {
      auto [nextPCM, nextRate] = CartridgeAR::loadPCM(companion);
      if(nextPCM.empty() || nextRate != sampleRate)
      {
        cerr << std::format(
          "CartCreator: skipping companion tape '{}' (load failed or "
          "sample rate mismatch)\n", companion.getName());
        break;
      }
      const size_t silenceSamples = static_cast<size_t>(sampleRate) * 2;
      pcmData.reserve(pcmData.size() + silenceSamples + nextPCM.size());
      pcmData.insert(pcmData.end(), silenceSamples, 1.F);
      pcmData.insert(pcmData.end(), nextPCM.begin(), nextPCM.end());
      cerr << std::format("CartCreator: tape {}: '{}' ({} samples @ {} Hz)\n",
                          tapeNum++, companion.getName(), nextPCM.size(), nextRate);
    }
    cerr << std::format("CartCreator: total PCM stream: {} samples ({:.1f}s)\n",
                        pcmData.size(),
                        static_cast<double>(pcmData.size()) / sampleRate);

    auto cart = std::make_unique<CartridgeAR>(
      ByteSpan{biosData}, std::move(pcmData), sampleRate, md5, settings);
    cart->setAbout("AR (2K) ", "AR", "");
    return cart;
  }

};  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Cartridge> CartCreator::create(const FSNode& file, ByteSpan image,
                                          string& md5, string_view dtype,
                                          Settings& settings, const FSNode& baseDir)
{
  unique_ptr<Cartridge> cartridge;
  Bankswitch::Type type = Bankswitch::nameToType(dtype), detectedType = type;
  string id;

  // Supercharger audio files (WAV/MP3): stream PCM bits through the real BIOS.
  if(auto cart = createFromSoundLoad(file, md5, settings, baseDir))
    return cart;

  // First inspect the file extension itself
  // If a valid type is found, it will override the one passed into this method
  const Bankswitch::Type typeByName = Bankswitch::typeFromExtension(file);
  if(typeByName != Bankswitch::Type::AUTO)
    type = detectedType = typeByName;

  // See if we should try to auto-detect the cartridge type
  // If we ask for extended info, always do an autodetect
  bool autodetected = false;
  if(type == Bankswitch::Type::AUTO || settings.getBool("rominfo"))
  {
    detectedType = CartDetector::autodetectType(image);
    if(type != Bankswitch::Type::AUTO && type != detectedType)
      cerr << "Auto-detection not consistent: "
           << Bankswitch::typeToName(type) << ", "
           << Bankswitch::typeToName(detectedType) << '\n';

    type = detectedType;
    autodetected = true;
  }

  // Check for multicart first; if found, get the correct part of the image
  bool validMultiSize = true;
  uInt32 numMultiRoms = 0;
  const size_t size = image.size();
  switch(type)
  {
    using enum Bankswitch::Type;
    case _2IN1:
      numMultiRoms = 2;
      validMultiSize = (size == 2 * 2_KB || size == 2 * 4_KB || size == 2 * 8_KB || size == 2 * 16_KB || size == 2 * 32_KB);
      break;

    case _4IN1:
      numMultiRoms = 4;
      validMultiSize = (size == 4 * 2_KB || size == 4 * 4_KB || size == 4 * 8_KB || size == 4 * 16_KB);
      break;

    case _8IN1:
      numMultiRoms = 8;
      validMultiSize = (size == 8 * 2_KB || size == 8 * 4_KB || size == 8 * 8_KB);
      break;

    case _16IN1:
      numMultiRoms = 16;
      validMultiSize = (size == 16 * 2_KB || size == 16 * 4_KB || size == 16 * 8_KB);
      break;

    case _32IN1:
      numMultiRoms = 32;
      validMultiSize = (size == 32 * 2_KB || size == 32 * 4_KB);
      break;

    case _64IN1:
      numMultiRoms = 64;
      validMultiSize = (size == 64 * 2_KB || size == 64 * 4_KB);
      break;

    case _128IN1:
      numMultiRoms = 128;
      validMultiSize = (size == 128 * 2_KB || size == 128 * 4_KB);
      break;

    case MVC:
      cartridge = std::make_unique<CartridgeMVC>(file.getPath(), size, md5, settings);
      break;

    default:
      cartridge = createFromImage(image, detectedType, md5, settings);
      break;
  }

  if(numMultiRoms)
  {
    if(validMultiSize)
      cartridge = createFromMultiCart(image, numMultiRoms, md5, detectedType, id, settings);
    else
      throw std::runtime_error(std::format(
          "Invalid cart size for type '{}'", Bankswitch::typeToName(type)));
  }

  const string_view typeName = Bankswitch::typeToName(type);
  const string_view detectedTypeName = Bankswitch::typeToName(detectedType);

  const string sizeStr = (size < 1_KB)
      ? std::format("{}B", size)
      : std::format("{}K", size / 1_KB);

  const bool showDetectedType = numMultiRoms &&
             detectedType != Bankswitch::Type::_2K &&
             detectedType != Bankswitch::Type::_4K;

  const string detectedSuffix = showDetectedType
      ? std::format(" {}", detectedTypeName) : "";

  const string about = std::format("{}{} ({}{}) ",
      typeName, autodetected ? "*" : "", sizeStr, detectedSuffix);

  cartridge->setAbout(about, typeName, id);

  return cartridge;
}
