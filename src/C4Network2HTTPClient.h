#pragma once

#include "network/C4NetIO.h"

const int C4Network2HTTPQueryTimeout = 20;  // (s)

// mini HTTP client
class C4Network2HTTPClient : public C4NetIOTCP, private C4NetIO::CBClass {
 public:
  C4Network2HTTPClient();
  virtual ~C4Network2HTTPClient();

 private:
  // Address information
  C4NetIO::addr_t ServerAddr, PeerAddr;
  StdCopyStrBuf Server, RequestPath;

  bool fBinary;
  bool fBusy, fSuccess, fConnected;
  size_t iDataOffset;
  StdCopyBuf Request;
  time_t iRequestTimeout;

  // Response header data
  size_t iDownloadedSize, iTotalSize;
  bool fCompressed;

  // Event queue to use for notify when something happens
  class C4InteractiveThread *pNotify;

 protected:
  StdCopyBuf ResultBin;        // set if fBinary
  StdCopyStrBuf ResultString;  // set if !fBinary

 protected:
  // Overridden
  virtual void PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf);
  virtual size_t UnpackPacket(const StdBuf &rInBuf,
                              const C4NetIO::addr_t &addr);

  // Callbacks
  bool OnConn(const C4NetIO::addr_t &AddrPeer,
              const C4NetIO::addr_t &AddrConnect,
              const C4NetIO::addr_t *pOwnAddr, C4NetIO *pNetIO);
  void OnDisconn(const C4NetIO::addr_t &AddrPeer, C4NetIO *pNetIO,
                 const char *szReason);
  void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO);

  void ResetRequestTimeout();
  virtual int32_t GetDefaultPort() { return 80; }

 public:
  bool Query(const StdBuf &Data, bool fBinary);
  bool Query(const char *szData, bool fBinary) {
    return Query(StdBuf(szData, SLen(szData)), fBinary);
  }

  bool isBusy() const { return fBusy; }
  bool isSuccess() const { return fSuccess; }
  bool isConnected() const { return fConnected; }
  size_t getTotalSize() const { return iTotalSize; }
  size_t getDownloadedSize() const { return iDownloadedSize; }
  const StdBuf &getResultBin() const {
    assert(fBinary);
    return ResultBin;
  }
  const char *getResultString() const {
    assert(!fBinary);
    return ResultString.getData();
  }
  const char *getServerName() const { return Server.getData(); }
  const char *getRequest() const { return RequestPath.getData(); }
  const C4NetIO::addr_t &getServerAddress() const { return ServerAddr; }

  void Cancel(const char *szReason);
  void Clear();

  bool SetServer(const char *szServerAddress);

  void SetNotify(class C4InteractiveThread *pnNotify) { pNotify = pnNotify; }

  // Overridden
  virtual bool Execute(int iMaxTime = TO_INF);
  virtual int GetTimeout();

 private:
  bool ReadHeader(StdStrBuf Data);
  bool Decompress(StdBuf *pData);
};