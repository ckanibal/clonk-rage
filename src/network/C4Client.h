
#ifndef C4CLIENT_H
#define C4CLIENT_H

#include "network/C4NetIO.h"
#include "C4PacketBase.h"
#include "C4InputValidation.h"

// special ids
const int32_t C4ClientIDUnknown = -1, C4ClientIDHost = 0, C4ClientIDStart = 1;

// client core difference levels
const int32_t C4ClientCoreDL_None = 0,  // identical
    C4ClientCoreDL_IDMatch = 1,         // status change
    C4ClientCoreDL_IDChange = 2,        // identification changed (host only!)
    C4ClientCoreDL_Different = 3;       // really different

class C4ClientCore : public C4PacketBase {
 public:
  C4ClientCore();
  ~C4ClientCore();

 protected:
  // identification
  int32_t iID;
  ValidatedStdCopyStrBuf<C4InVal::VAL_NameNoEmpty> Name, Nick;
  ValidatedStdCopyStrBuf<C4InVal::VAL_NameAllowEmpty> CUID;

  // version info
  int iVersion[4];

  // status
  bool fActivated, fObserver;

 public:
  // status data
  int32_t getID() const { return iID; }
  bool isHost() const { return iID == C4ClientIDHost; }
  bool isActivated() const { return fActivated; }
  bool isObserver() const { return fObserver; }
  void SetID(int32_t inID) { iID = inID; }
  void SetName(const char *sznName) { Name.CopyValidated(sznName); }
  void SetActivated(bool fnActivated) {
    fActivated = fnActivated;
    fObserver = false;
  }
  void SetObserver(bool fnObserver) { fActivated &= !(fObserver = fnObserver); }

  // misc
  const char *getName() const { return Name.getData(); }
  const char *getNick() const { return Nick.getData(); }

  // initialization
  void SetLocal(int32_t iID, bool fnActivated, bool fnObserver);
  void SetUnknown(int32_t iID);

  int32_t getDiffLevel(const C4ClientCore &CCore2) const;

  virtual void CompileFunc(StdCompiler *pComp);
};

class C4Client {
  friend class C4ClientList;

 public:
  C4Client();
  C4Client(const C4ClientCore &Core);
  ~C4Client();

 private:
  C4ClientCore Core;

  bool fLocal;                         // Local, NoSync
  class C4Network2Client *pNetClient;  // Local, NoSync

  C4Client *pNext;

 public:
  const C4ClientCore &getCore() const { return Core; }
  int32_t getID() const { return Core.getID(); }
  bool isHost() const { return Core.isHost(); }
  const char *getName() const { return Core.getName(); }
  const char *getNick() const { return Core.getNick(); }
  bool isActivated() const { return Core.isActivated(); }
  bool isObserver() const { return Core.isObserver(); }

  bool isLocal() const { return fLocal; }
  C4Network2Client *getNetClient() const { return pNetClient; }

  void SetCore(const C4ClientCore &NewCore) { Core = NewCore; }
  void SetID(int32_t iID) { Core.SetID(iID); }

  void SetActivated(bool fnActivated);
  void SetObserver() { Core.SetObserver(true); }
  void SetLocal();

  void UnlinkNetClient() { pNetClient = NULL; }

  void Remove();

  void CompileFunc(StdCompiler *pComp);
};

class C4ClientList {
 public:
  C4ClientList();
  ~C4ClientList();

  C4ClientList &operator=(const C4ClientList &List);

  void Clear();

 protected:
  // client list
  C4Client *pFirst;

  // pointer to local client
  C4Client *pLocal;

  // network client list (may be NULL if network is not active)
  class C4Network2ClientList *pNetClients;

  void Add(C4Client *pClient);

 public:
  C4Client *getClient(const C4Client *pAfter = NULL) const {
    return pAfter ? pAfter->pNext : pFirst;
  }
  C4Client *getLocal() const { return pLocal; }
  C4Client *getClientByID(int32_t iID) const;
  C4Client *getHost() const { return getClientByID(C4ClientIDHost); }
  C4Client *getNextClientByID(int32_t iAfterID) const;
  C4Client *getClientByName(const char *szName) const;

  int32_t getClientCnt() const;

  const C4ClientCore &getLocalCore() const { return getLocal()->getCore(); }
  const char *getLocalName() const {
    return pLocal ? getLocalCore().getName() : "???";
  }
  int32_t getLocalID() const {
    return pLocal ? getLocalCore().getID() : C4ClientIDUnknown;
  }

  bool Init(int32_t iLocalClientID = C4ClientIDHost);
  void InitNetwork(class C4Network2ClientList *pNetClients);
  void ClearNetwork();

  bool Remove(C4Client *pClient, bool fTemporary = false);
  C4Client *Add(const C4ClientCore &Core);
  C4Client *AddLocal(int32_t iID, bool fActivated, bool fObserver);
  void SetLocalID(int32_t iID);

  void CtrlRemove(const C4Client *pClient, const char *szReason);

  void RemoveRemote();

  void CompileFunc(StdCompiler *pComp);
};

#endif  // C4CLIENT_H
