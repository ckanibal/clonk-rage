
#ifndef C4NETWORK2REFERENCE_H
#define C4NETWORK2REFERENCE_H

#ifdef C4ENGINE
#include "network/C4Network2.h"
#include "network/C4Network2Client.h"
#include "C4Network2HTTPClient.h"
#else
#include "network/C4NetIO.h"
#endif
#include "control/C4GameParameters.h"
#include "C4Version.h"
#include "game/C4GameVersion.h"
#include "C4InputValidation.h"

// Session data
class C4Network2Reference {
 public:
  C4Network2Reference();
  ~C4Network2Reference();

  // Game parameters
  C4GameParameters Parameters;

 private:
  // General information
  int32_t Icon;
  ValidatedStdCopyStrBuf<C4InVal::VAL_NameExNoEmpty> Title;
  C4Network2Status GameStatus;
  int32_t Time;
  int32_t Frame;
  int32_t StartTime;
  int32_t LeaguePerformance;  // custom settlement league performance if
                              // scenario doesn't use elapsed frames
  ValidatedStdCopyStrBuf<C4InVal::VAL_Comment> Comment;
  bool JoinAllowed;
  bool ObservingAllowed;
  bool PasswordNeeded;
  bool OfficialServer;
  bool RegJoinOnly;

  // Engine information
  C4GameVersion Game;

  // Network addresses
  uint8_t iAddrCnt;
  C4Network2Address Addrs[C4ClientMaxAddr];

 public:
  const C4Network2Address &getAddr(int i) const { return Addrs[i]; }
  int getAddrCnt() const { return iAddrCnt; }
  const char *getTitle() const { return Title.getData(); }
  int32_t getIcon() const { return Icon; }
  C4Network2Status getGameStatus() const { return GameStatus; }
  const char *getComment() const { return Comment.getData(); }
  const C4GameVersion &getGameVersion() const { return Game; }
  bool isPasswordNeeded() const { return PasswordNeeded; }
  bool isRegJoinOnly() const { return RegJoinOnly; }
  bool isJoinAllowed() const { return JoinAllowed; }
  bool isOfficialServer() const { return OfficialServer; }
  int32_t getSortOrder() const;
  int32_t getTime() const { return Time; }
  int32_t getStartTime() const { return StartTime; }

  void SetSourceIP(in_addr ip);

  void InitLocal(C4Game *pGame);

  void SortNullIPsBack();

  void CompileFunc(StdCompiler *pComp);
};

// Serves references (mini-HTTP-server)
class C4Network2RefServer : public C4NetIOTCP {
 public:
  C4Network2RefServer();
  virtual ~C4Network2RefServer();

 private:
  CStdCSec RefCSec;
  C4Network2Reference *pReference;

 public:
  void Clear();
  void SetReference(C4Network2Reference *pReference);

 protected:
  // Overridden
  virtual void PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf);
  virtual size_t UnpackPacket(const StdBuf &rInBuf,
                              const C4NetIO::addr_t &addr);

 private:
  // Responses
  void RespondNotImplemented(const C4NetIO::addr_t &addr,
                             const char *szMessage);
  void RespondReference(const C4NetIO::addr_t &addr);
};

// Loads references (mini-HTTP-client)
class C4Network2RefClient : public C4Network2HTTPClient {
  C4GameVersion MasterVersion;
  StdCopyStrBuf MessageOfTheDay, MessageOfTheDayHyperlink;
  StdCopyStrBuf LeagueServerRedirect;
  bool fVerSet;

 protected:
  virtual int32_t GetDefaultPort() { return C4NetStdPortRefServer; }

 public:
  C4Network2RefClient() : fVerSet(false), C4Network2HTTPClient() {}

  bool QueryReferences();
  bool GetReferences(C4Network2Reference **&rpReferences, int32_t &rRefCount);
  bool GetMasterVersion(
      C4GameVersion *pSaveToVer);  // call only after GetReferences
  const char *GetMessageOfTheDay() const { return MessageOfTheDay.getData(); }
  const char *GetMessageOfTheDayHyperlink() const {
    return MessageOfTheDayHyperlink.getData();
  }
  const char *GetLeagueServerRedirect() const {
    return LeagueServerRedirect.getLength() ? LeagueServerRedirect.getData()
                                            : NULL;
  }
};

#endif  // C4NETWORK2REFERENCE_H
