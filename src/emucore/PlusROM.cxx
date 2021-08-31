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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <regex>
#include <atomic>
#include <sstream>
#include <thread>

#include "bspf.hxx"
#include "PlusROM.hxx"
#include "Logger.hxx"
#include "Version.hxx"

#if defined(HTTP_LIB_SUPPORT)
  #include "http_lib.hxx"

namespace {
  constexpr int MAX_CONCURRENT_REQUESTS = 5;
  constexpr int CONNECTION_TIMEOUT_MSEC = 3000;
  constexpr int READ_TIMEOUT_MSEC = 3000;
  constexpr int WRITE_TIMEOUT_MSEC = 3000;
}
#endif

using std::chrono::milliseconds;

class PlusROMRequest {
  public:

    struct Destination {
      Destination(string _host, string _path) : host(_host), path(_path) {}

      string host;
      string path;
    };

    struct PlusStoreId {
      PlusStoreId(string _nick, string _id) : nick(_nick), id(_id) {}

      string nick;
      string id;
    };

    enum class State : uInt8 {
      created,
      pending,
      done,
      failed
    };

  public:

    PlusROMRequest(Destination destination, PlusStoreId id, const uInt8* request, uInt8 requestSize)
      : myDestination(destination), myId(id), myRequestSize(requestSize)
    {
      myState = State::created;
      memcpy(myRequest.data(), request, myRequestSize);
    }

  #if defined(HTTP_LIB_SUPPORT)
    void execute() {
      myState = State::pending;

      httplib::Client client(myDestination.host);
      httplib::Headers headers = {
        {"PlusStore-ID", myId.nick + " WE" + myId.id},
        {"User-Agent", string("Stella ") + STELLA_VERSION}
      };

      client.set_connection_timeout(milliseconds(CONNECTION_TIMEOUT_MSEC));
      client.set_read_timeout(milliseconds(READ_TIMEOUT_MSEC));
      client.set_write_timeout(milliseconds(WRITE_TIMEOUT_MSEC));

      auto response = client.Post(
        myDestination.path.c_str(),
        headers,
        reinterpret_cast<const char*>(myRequest.data()),
        myRequestSize,
        "application/octet-stream"
      );

      if (response->status != 200) {
        ostringstream ss;
        ss
          << "PlusCart: request to "
          << myDestination.host
          << "/"
          << myDestination.path
          << ": failed with HTTP status "
          << response->status;

        Logger::error(ss.str());

        myState = State::failed;

        return;
      }

      if (response->body.size() < 1 || static_cast<unsigned char>(response->body[0]) != (response->body.size() - 1)) {
        ostringstream ss;
        ss << "PlusCart: request to " << myDestination.host << "/" << myDestination.path << ": invalid response";

        Logger::error(ss.str());

        myState = State::failed;

        return;
      }

      myResponse = response->body;
      myState = State::done;
    }

    State getState() {
      return myState;
    }

    std::pair<size_t, const uInt8*> getResponse() {
      if (myState != State::done) throw runtime_error("invalid access to response");

      return std::pair<size_t, const uInt8*>(
        myResponse.size() - 1,
        myResponse.size() > 1 ? reinterpret_cast<const uInt8*>(myResponse.data() + 1) : nullptr
      );
    }
  #endif

  private:
    std::atomic<State> myState;

    Destination myDestination;
    PlusStoreId myId;

    std::array<uInt8, 256> myRequest;
    uInt8 myRequestSize;

    string myResponse;

  private:
    PlusROMRequest(const PlusROMRequest&) = delete;
    PlusROMRequest(PlusROMRequest&&) = delete;
    PlusROMRequest& operator=(const PlusROMRequest&) = delete;
    PlusROMRequest& operator=(PlusROMRequest&&) = delete;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::initialize(const ByteBuffer& image, size_t size)
{
#if defined(HTTP_LIB_SUPPORT)
  // Host and path are stored at the NMI vector
  size_t i = ((image[size - 5] - 16) << 8) | image[size - 6];  // NMI @ $FFFA
  if(i >= size)
    return myIsPlusROM = false;  // Invalid NMI

  // Path stored first, 0-terminated
  string path;
  while(i < size && image[i] != 0)
    path += static_cast<char>(image[i++]);

  // Did we get a valid, 0-terminated path?
  if(i >= size || image[i] != 0 || !isValidPath(path))
    return myIsPlusROM = false;  // Invalid path

  i++;  // advance past 0 terminator

  // Host stored next, 0-terminated
  string host;
  while(i < size && image[i] != 0)
    host += static_cast<char>(image[i++]);

  // Did we get a valid, 0-terminated host?
  if(i >= size || image[i] != 0 || !isValidHost(host))
    return myIsPlusROM = false;  // Invalid host

  myHost = host;
  myPath = "/" + path;

  reset();

  return myIsPlusROM = true;
#else
  return myIsPlusROM = false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::peekHotspot(uInt16 address, uInt8& value)
{
#if defined(HTTP_LIB_SUPPORT)
  switch(address & 0x0FFF)
  {
    case 0x0FF2:  // Read next byte from Rx buffer
      receive();

      value = myRxBuffer[myRxReadPos];
      if (myRxReadPos != myRxWritePos) myRxReadPos++;

      return true;

    case 0x0FF3:  // Get number of unread bytes in Rx buffer
      receive();

      value = myRxWritePos - myRxReadPos;

      return true;
  }
#endif

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::pokeHotspot(uInt16 address, uInt8 value)
{
#if defined(HTTP_LIB_SUPPORT)
  switch(address & 0x0FFF)
  {
    case 0x0FF0:  // Write byte to Tx buffer
      myTxBuffer[myTxPos++] = value;

      return true;

    case 0x0FF1:  // Write byte to Tx buffer and send to backend
                  // (and receive into Rx buffer)

      myTxBuffer[myTxPos++] = value;
      send();

      return true;
  }
#endif

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::save(Serializer& out) const
{
  try
  {
    out.putByteArray(myRxBuffer.data(), myRxBuffer.size());
    out.putByteArray(myTxBuffer.data(), myTxBuffer.size());
    out.putInt(myRxReadPos);
    out.putInt(myRxWritePos);
    out.putInt(myTxPos);
  }
  catch(...)
  {
    cerr << "ERROR: PlusROM::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::load(Serializer& in)
{
  myPendingRequests.clear();

  try
  {
    in.getByteArray(myRxBuffer.data(), myRxBuffer.size());
    in.getByteArray(myTxBuffer.data(), myTxBuffer.size());
    myRxReadPos = in.getInt();
    myRxWritePos = in.getInt();
    myTxPos = in.getInt();
  }
  catch(...)
  {
    cerr << "ERROR: PlusROM::load" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlusROM::reset()
{
  myRxReadPos = myRxWritePos = myTxPos = 0;
  myPendingRequests.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::isValidHost(const string& host) const
{
  // TODO: This isn't 100% either, as we're supposed to check for the length
  //       of each part between '.' in the range 1 .. 63
  //  Perhaps a better function will be included with whatever network
  //  library we decide to use
  static std::regex rgx(R"(^(([a-z0-9]|[a-z0-9][a-z0-9\-]*[a-z0-9])\.)*([a-z0-9]|[a-z0-9][a-z0-9\-]*[a-z0-9])$)", std::regex_constants::icase);

  return std::regex_match(host, rgx);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::isValidPath(const string& path) const
{
  // TODO: This isn't 100%
  //  Perhaps a better function will be included with whatever network
  //  library we decide to use
  for(auto c: path)
    if(!((c > 44 && c < 58) || (c > 64 && c < 91) || (c > 96 && c < 122)))
      return false;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlusROM::send()
{
#if defined(HTTP_LIB_SUPPORT)
  // Try to make room by cosuming any requests that have completed.
  receive();

  if (myPendingRequests.size() >= MAX_CONCURRENT_REQUESTS) {
    Logger::error("PlusCart: max number of concurrent requests exceeded");

    myTxPos = 0;
    return;
  }

  auto request = make_shared<PlusROMRequest>(
    PlusROMRequest::Destination(myHost, myPath),
    PlusROMRequest::PlusStoreId("DirtyHairy", "0123456789012345678912"),
    myTxBuffer.data(),
    myTxPos
  );

  myTxPos = 0;

  // We push to the back in order to avoid reverse_iterator in receive()
  myPendingRequests.push_back(request);

  // The lambda will retain a copy of the shared_ptr that is alive as long as the
  // thread is running. Thus, the request can only be destructed once the thread has
  // finished, and we can safely evict it from the deque at any time.
  std::thread thread([=](){ request->execute(); });

  thread.detach();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlusROM::receive()
{
#if defined(HTTP_LIB_SUPPORT)
  auto iter = myPendingRequests.begin();

  while (iter != myPendingRequests.end()) {
    switch ((*iter)->getState()) {
      case PlusROMRequest::State::failed:
        // Request has failed? -> remove it and start over
        myPendingRequests.erase(iter);
        iter = myPendingRequests.begin();

        continue;

      case PlusROMRequest::State::done:
      {
        // Request has finished sucessfully? -> consume the response, remove it
        // and start over
        auto [responseSize, response] = (*iter)->getResponse();

        for (uInt8 i = 0; i < responseSize; i++)
          myRxBuffer[myRxWritePos++] = response[i];

        myPendingRequests.erase(iter);
        iter = myPendingRequests.begin();

        continue;
      }

      default:
        iter++;
    }
  }
#endif
}
