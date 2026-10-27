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
#include <thread>

#include "Windows.hxx"
#include "RawStreamOutputWINDOWS.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RawStreamOutputWINDOWS::~RawStreamOutputWINDOWS()
{
  if(myHandle != INVALID_HANDLE_VALUE && !myIsStdout)
    CloseHandle(myHandle);
  if(myWriter)
    CloseHandle(myWriter);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RawStreamOutputWINDOWS::open(const string& path)
{
  myIsStdout = path == "-";
  myHandle = myIsStdout
    ? GetStdHandle(STD_OUTPUT_HANDLE)
    : CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  // GetStdHandle() gives a null handle when there is no stdout
  if(myHandle == nullptr)
    myHandle = INVALID_HANDLE_VALUE;

  return myHandle != INVALID_HANDLE_VALUE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RawStreamOutputWINDOWS::write(ByteSpan data)
{
  // Every write() comes from the same thread; cancel() needs its handle
  if(!myWriter)
    myWriter = OpenThread(THREAD_TERMINATE, FALSE, GetCurrentThreadId());

  myWriting = true;
  while(!data.empty() && !myCancelled)
  {
    DWORD written{0};
    const auto size = static_cast<DWORD>(std::min<size_t>(data.size(), MAXDWORD));
    if(!WriteFile(myHandle, data.data(), size, &written, nullptr) || written == 0)
      break;
    data = data.subspan(written);
  }
  myWriting = false;

  return data.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStreamOutputWINDOWS::cancel()
{
  myCancelled = true;

  // The write may not have reached WriteFile() yet, so keep cancelling
  // until it has given up
  while(myWriting)
  {
    CancelSynchronousIo(myWriter);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}
