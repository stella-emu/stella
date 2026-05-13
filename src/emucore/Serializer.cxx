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

#include "FSNode.hxx"
#include "Serializer.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Serializer::Serializer(string_view filename, FileMode fm)
{
  std::ios::openmode mode = std::ios::binary;

  if(fm == FileMode::ReadOnly)
    mode |= std::ios::in;
  else
  {
    mode |= std::ios::in | std::ios::out;
    if(fm == FileMode::ReadWriteTrunc)
      mode |= std::ios::trunc;
  }

  myFile.emplace();
  myFile->stream = FSNode(filename).openFStream(mode);
  if(myFile->stream.is_open())
    myFile->stream.exceptions(std::ios::failbit | std::ios::badbit);
  else
    myFile.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Serializer::Serializer()
{
  myMemory.emplace();
  myMemory->buffer.resize(4_KB);  // tweak or remove as needed
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::setPosition(size_t pos)
{
  if(myMemory)
  {
    if(pos > myMemory->size)
    {
      myMemory->ensureSize(pos);
      myMemory->size = pos;
    }
    myMemory->pos = pos;
  }
  else if(myFile)
  {
    myFile->flushBuffer();
    myFile->stream.clear();  // clear any eof/fail flags
    myFile->stream.seekg(pos, std::ios::beg);
    myFile->stream.seekp(pos, std::ios::beg);
    if(!myFile->stream)
      throw std::runtime_error("Failed to set file position");
  }
  else
    throw std::runtime_error("Serializer not initialized");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::rewind()
{
  setPosition(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t Serializer::size()
{
  if(myMemory)
  {
    return myMemory->size;
  }
  else if(myFile)
  {
    myFile->flushBuffer();
    const auto curG = myFile->stream.tellg();
    const auto curP = myFile->stream.tellp();
    myFile->stream.seekg(0, std::ios::end);
    const auto end = myFile->stream.tellg();
    myFile->stream.seekg(curG);
    myFile->stream.seekp(curP);
    return static_cast<size_t>(end);
  }
  else
    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
inline T Serializer::readRaw()
{
  if constexpr(std::is_same_v<T, double>)
  {
    const auto raw = readRaw<uInt64>();
    return std::bit_cast<double>(raw);
  }
  else
  {
    T value{};

    if(myMemory)
    {
      if(myMemory->pos + sizeof(T) > myMemory->size)
      {
        myMemory->ensureSize(myMemory->pos + sizeof(T));
        myMemory->size = myMemory->pos + sizeof(T);
      }
      std::memcpy(&value, myMemory->buffer.data() + myMemory->pos, sizeof(T));
      myMemory->pos += sizeof(T);
    }
    else if(myFile)
    {
      myFile->stream.read(reinterpret_cast<char*>(&value), sizeof(T));
    }
    else
      throw std::runtime_error("Serializer not initialized");

    return fromLittle(value);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
inline void Serializer::writeRaw(T value)
{
  if constexpr(std::is_same_v<T, double>)
  {
    writeRaw<uInt64>(std::bit_cast<uInt64>(value));
    return;
  }
  else
  {
    value = toLittle(value);

    if(myMemory)
    {
      myMemory->ensureCapacity(sizeof(T));
      std::memcpy(myMemory->buffer.data() + myMemory->pos, &value, sizeof(T));
      myMemory->pos += sizeof(T);
      myMemory->size = std::max(myMemory->size, myMemory->pos);
    }
    else if(myFile)
    {
      myFile->writeBuffered(reinterpret_cast<const uInt8*>(&value), sizeof(T));
    }
    else
      throw std::runtime_error("Serializer not initialized");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Serializer::getByte()
{
  return readRaw<uInt8>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::getByteArray(ByteMSpan arr)
{
  if(arr.empty())
    return;

  if(myMemory)
  {
    if(myMemory->pos + arr.size() > myMemory->size)
    {
      myMemory->ensureSize(myMemory->pos + arr.size());
      myMemory->size = myMemory->pos + arr.size();
    }
    std::memcpy(arr.data(), myMemory->buffer.data() + myMemory->pos, arr.size());
    myMemory->pos += arr.size();
  }
  else if(myFile)
  {
    myFile->stream.read(reinterpret_cast<char*>(arr.data()), arr.size());
  }
  else
    throw std::runtime_error("Serializer not initialized");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Serializer::getShort()
{
  return readRaw<uInt16>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::getShortArray(ShortMSpan arr)
{
  getByteArray(ByteMSpan(reinterpret_cast<uInt8*>(arr.data()),
                                                  arr.size_bytes()));
  if constexpr(std::endian::native != std::endian::little)
    for(auto& val: arr)
      val = byteswap(val);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Serializer::getInt()
{
  return readRaw<uInt32>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::getIntArray(IntMSpan arr)
{
  getByteArray(ByteMSpan(reinterpret_cast<uInt8*>(arr.data()),
                         arr.size_bytes()));
  if constexpr(std::endian::native != std::endian::little)
    for(auto& val: arr)
      val = byteswap(val);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 Serializer::getLong()
{
  return readRaw<uInt64>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double Serializer::getDouble()
{
  return readRaw<double>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Serializer::getBool()
{
  return getByte() == TruePattern;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Serializer::getString()
{
  const uInt32 len = getInt();
  string result(len, '\0');

  getByteArray(ByteMSpan(reinterpret_cast<uInt8*>(result.data()), len));
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putByte(uInt8 value)
{
  writeRaw<uInt8>(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putByteArray(ByteSpan arr)
{
  if(arr.empty())
    return;

  if(myMemory)
  {
    myMemory->ensureCapacity(arr.size());
    std::memcpy(myMemory->buffer.data() + myMemory->pos, arr.data(), arr.size());
    myMemory->pos += arr.size();
    myMemory->size = std::max(myMemory->size, myMemory->pos);
  }
  else if(myFile)
  {
    myFile->writeBuffered(arr.data(), arr.size());
  }
  else
    throw std::runtime_error("Serializer not initialized");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putShort(uInt16 value)
{
  writeRaw<uInt16>(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putShortArray(ShortSpan arr)
{
  if constexpr(std::endian::native == std::endian::little)
    putByteArray(ByteSpan(reinterpret_cast<const uInt8*>(arr.data()),
                          arr.size_bytes()));
  else
    for(const auto& val: arr)
      writeRaw<uInt16>(val);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putInt(uInt32 value)
{
  writeRaw<uInt32>(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putIntArray(IntSpan arr)
{
  if constexpr(std::endian::native == std::endian::little)
    putByteArray(ByteSpan(reinterpret_cast<const uInt8*>(arr.data()),
                          arr.size_bytes()));
  else
    for(const auto& val: arr)
      writeRaw<uInt32>(val);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putLong(uInt64 value)
{
  writeRaw<uInt64>(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putDouble(double value)
{
  writeRaw<double>(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putString(string_view str)
{
  putInt(static_cast<uInt32>(str.size()));
  putByteArray(ByteSpan(reinterpret_cast<const uInt8*>(str.data()), str.size()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putBool(bool b)
{
  putByte(b ? TruePattern : FalsePattern);
}
