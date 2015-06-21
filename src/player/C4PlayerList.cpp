/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Dynamic list to hold runtime player data */

#include "C4Include.h"
#include "player/C4PlayerList.h"

#ifndef BIG_C4INCLUDE
#include "C4Components.h"
#include "game/C4FullScreen.h"
#include "C4Console.h"
#include "C4Log.h"
#include "player/C4Player.h"
#include "object/C4Object.h"
#endif

C4PlayerList::C4PlayerList() { Default(); }

C4PlayerList::~C4PlayerList() { Clear(); }

void C4PlayerList::Default() { First = NULL; }

void C4PlayerList::Clear() {
  C4Player *pPlr;
  while (pPlr = First) {
    First = pPlr->Next;
    delete pPlr;
  }
  First = NULL;
}

void C4PlayerList::Execute() {
  C4Player *pPlr;
  // Execute
  for (pPlr = First; pPlr; pPlr = pPlr->Next) pPlr->Execute();
  // Check retirement
  for (pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->Eliminated && !pPlr->RetireDelay) {
      Retire(pPlr);
      break;
    }
}

void C4PlayerList::ClearPointers(C4Object *pObj) {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    pPlr->ClearPointers(pObj, false);
}

BOOL C4PlayerList::Valid(int iPlayer) const {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->Number == iPlayer) return TRUE;
  return FALSE;
}

BOOL C4PlayerList::Hostile(int iPlayer1, int iPlayer2) const {
  C4Player *pPlr1 = Get(iPlayer1);
  C4Player *pPlr2 = Get(iPlayer2);
  if (!pPlr1 || !pPlr2) return FALSE;
  if (pPlr1->Number == pPlr2->Number) return FALSE;
  if (pPlr1->Hostility.GetIDCount(pPlr2->Number + 1) ||
      pPlr2->Hostility.GetIDCount(pPlr1->Number + 1))
    return TRUE;
  return FALSE;
}

bool C4PlayerList::HostilityDeclared(int iPlayer1, int iPlayer2) const {
  // check one-way-hostility
  C4Player *pPlr1 = Get(iPlayer1);
  C4Player *pPlr2 = Get(iPlayer2);
  if (!pPlr1 || !pPlr2) return false;
  if (pPlr1->Number == pPlr2->Number) return false;
  if (pPlr1->Hostility.GetIDCount(pPlr2->Number + 1)) return true;
  return false;
}

BOOL C4PlayerList::PositionTaken(int iPosition) const {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->Position == iPosition) return TRUE;
  return FALSE;
}

BOOL C4PlayerList::ColorTaken(int iColor) const {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->Color == iColor) return TRUE;
  return FALSE;
}

int C4PlayerList::CheckColorDw(DWORD dwColor, C4Player *pExclude) {
  // maximum difference
  int iDiff = 255 + 255 + 255;
  // check all player's color difference
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr != pExclude) {
      // get color
      DWORD dwClr2 = pPlr->ColorDw;
      // assign difference, if less than smallest difference found
      iDiff = Min(iDiff, Abs(GetRValue(dwColor) - GetRValue(dwClr2)) +
                             Abs(GetGValue(dwColor) - GetGValue(dwClr2)) +
                             Abs(GetBValue(dwColor) - GetBValue(dwClr2)));
    }
  // return the difference
  return iDiff;
}

BOOL C4PlayerList::ControlTaken(int iControl) const {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->Control == iControl)
      if (pPlr->LocalControl) return TRUE;
  return FALSE;
}

C4Player *C4PlayerList::Get(int iNumber) const {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->Number == iNumber) return pPlr;
  return NULL;
}

int C4PlayerList::GetIndex(C4Player *tPlr) const {
  int cindex = 0;
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next, cindex++)
    if (pPlr == tPlr) return cindex;
  return -1;
}

C4Player *C4PlayerList::GetByIndex(int iIndex) const {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (!iIndex--) return pPlr;
  return NULL;
}

C4Player *C4PlayerList::GetByIndex(int iIndex, C4PlayerType eType) const {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->GetType() == eType)
      if (!iIndex--) return pPlr;
  return NULL;
}

C4Player *C4PlayerList::GetLocalByKbdSet(int iKbdSet) const {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->LocalControl)
      if (pPlr->Control == iKbdSet) return pPlr;
  return NULL;
}

C4Player *C4PlayerList::GetByInfoID(int iInfoID) const {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->ID == iInfoID) return pPlr;
  return NULL;
}

int C4PlayerList::GetCount() const {
  int iCount = 0;
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next) iCount++;
  return iCount;
}

int C4PlayerList::GetCount(C4PlayerType eType) const {
  int iCount = 0;
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->GetType() == eType) iCount++;
  return iCount;
}

int C4PlayerList::GetFreeNumber() const {
  int iNumber = -1;
  BOOL fFree;
  do {
    iNumber++;
    fFree = TRUE;
    for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
      if (pPlr->Number == iNumber) fFree = FALSE;
  } while (!fFree);
  return iNumber;
}

BOOL C4PlayerList::Remove(int iPlayer, bool fDisconnect, bool fNoCalls) {
  return Remove(Get(iPlayer), fDisconnect, fNoCalls);
}

bool C4PlayerList::RemoveUnjoined(int32_t iPlayer) {
  // Savegame resume missing player: Remove player objects only
  C4Object *pObj;
  for (C4ObjectLink *clnk = Game.Objects.First; clnk && (pObj = clnk->Obj);
       clnk = clnk->Next)
    if (pObj->Status)
      if (pObj->IsPlayerObject(iPlayer)) pObj->AssignRemoval(TRUE);
  return true;
}

BOOL C4PlayerList::Remove(C4Player *pPlr, bool fDisconnect, bool fNoCalls) {
  if (!pPlr) return FALSE;

  // inform script
  if (!fNoCalls)
    Game.Script.GRBroadcast(PSF_RemovePlayer, &C4AulParSet(C4VInt(pPlr->Number),
                                                           C4VInt(pPlr->Team)));

  // Transfer ownership of other objects to team members
  if (!fNoCalls) pPlr->NotifyOwnedObjects();

  // NET2: update player info list
  if (pPlr->ID) {
    C4PlayerInfo *pInfo = Game.PlayerInfos.GetPlayerInfoByID(pPlr->ID);
    if (pInfo) {
      pInfo->SetRemoved();
      if (fDisconnect) pInfo->SetDisconnected();
    }
    // if player wasn't evaluated, store round results anyway
    if (!pPlr->Evaluated) Game.RoundResults.EvaluatePlayer(pPlr);
  }

  // for (C4Player *pPrev=First; pPrev; pPrev=pPrev->Next)
  //	if (pPrev->Next==pPlr) break;
  C4Player *pPrev = First;
  while (pPrev && pPrev->Next != pPlr) pPrev = pPrev->Next;
  if (pPrev)
    pPrev->Next = pPlr->Next;
  else
    First = pPlr->Next;

  // Remove eliminated crew
  if (!fNoCalls) pPlr->RemoveCrewObjects();

  // Clear object info pointers
  pPlr->CrewInfoList.DetachFromObjects();

  // Clear viewports
  Game.GraphicsSystem.CloseViewport(pPlr->Number, fNoCalls);
  // Check fullscreen viewports
  FullScreen.ViewportCheck();

  // Remove player
  delete pPlr;

  // Validate object owners
  Game.Objects.ValidateOwners();

  // Update console
  Console.UpdateMenus();
  return TRUE;
}

C4Player *C4PlayerList::Join(const char *szFilename, BOOL fScenarioInit,
                             int iAtClient, const char *szAtClientName,
                             C4PlayerInfo *pInfo) {
  assert(pInfo);

  // safeties
  if (szFilename && !*szFilename) szFilename = NULL;

  // Log
  LogF(LoadResStr(fScenarioInit ? "IDS_PRC_JOINPLR" : "IDS_PRC_RECREATE"),
       pInfo->GetName());

  // Too many players
  if (1)  // replay needs to check, too!
    if (GetCount() + 1 > Game.Parameters.MaxPlayers) {
      LogF(LoadResStr("IDS_PRC_TOOMANYPLRS"), Game.Parameters.MaxPlayers);
      return NULL;
    }

  // Check duplicate file usage
  if (szFilename)
    if (FileInUse(szFilename)) {
      Log(LoadResStr("IDS_PRC_PLRFILEINUSE"));
      return NULL;
    }

  // Create
  C4Player *pPlr = new C4Player;

  // Append to player list
  C4Player *pLast = First;
  for (; pLast && pLast->Next; pLast = pLast->Next)
    ;
  if (pLast)
    pLast->Next = pPlr;
  else
    First = pPlr;

  // Init
  if (!pPlr->Init(GetFreeNumber(), iAtClient, szAtClientName, szFilename,
                  fScenarioInit, pInfo)) {
    Remove(pPlr, false, false);
    Log(LoadResStr("IDS_PRC_JOINFAIL"));
    return NULL;
  }

  // Done
  return pPlr;
}

BOOL C4PlayerList::CtrlJoinLocalNoNetwork(const char *szFilename, int iAtClient,
                                          const char *szAtClientName) {
  assert(!Game.Network.isEnabled());
  // Create temp copy of player file without portraits
  // Why? This is local join!
  /*
  char szTempFilename[_MAX_PATH + 1] = "";
  const char *szOriginalFilename = szFilename;
  if (!Config.Network.SendPortraits)
          {
          SCopy(Config.AtTempPath(GetFilename(szFilename)), szTempFilename,
  _MAX_PATH);
          if (!CopyItem(szFilename, szTempFilename)) return FALSE;
          C4Group hGroup;
          if (hGroup.Open(szTempFilename))
                  {
                  hGroup.Delete(C4CFN_Portraits, true);
                  hGroup.Close();
                  }
          szFilename = szTempFilename;
          } */
  // pack - not needed for new res system
  /*if(DirectoryExists(szFilename))
          if(!C4Group_PackDirectory(szFilename))
                  return FALSE;*/
  // security
  if (!ItemExists(szFilename)) return FALSE;
  // join via player info
  BOOL fSuccess = Game.PlayerInfos.DoLocalNonNetworkPlayerJoin(szFilename);
  // Delete temp player file
  /*if(*szTempFilename) EraseItem(szTempFilename);*/

  // Done
  return fSuccess;
}

void SetClientPrefix(char *szFilename, const char *szClient) {
  char szTemp[1024 + 1];
  // Compose prefix
  char szPrefix[1024 + 1];
  SCopy(szClient, szPrefix);
  SAppendChar('-', szPrefix);
  // Prefix already set?
  SCopy(GetFilename(szFilename), szTemp, SLen(szPrefix));
  if (SEqualNoCase(szTemp, szPrefix)) return;
  // Insert prefix
  SCopy(GetFilename(szFilename), szTemp);
  SCopy(szPrefix, GetFilename(szFilename));
  SAppend(szTemp, szFilename);
}

BOOL C4PlayerList::Save(C4Group &hGroup, bool fStoreTiny,
                        const C4PlayerInfoList &rStoreList) {
  StdStrBuf sTempFilename;
  bool fSuccess = TRUE;
  // Save to external player files and add to group
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next) {
    // save only those in the list, and only those with a filename
    C4PlayerInfo *pNfo = rStoreList.GetPlayerInfoByID(pPlr->ID);
    if (!pNfo) continue;
    if (!pNfo->GetFilename() || !*pNfo->GetFilename()) continue;
    ;
    // save over original file?
    bool fStoreOnOriginal = (!fStoreTiny && pNfo->GetType() == C4PT_User);
    // Create temporary file
    sTempFilename.Copy(Config.AtTempPath(pNfo->GetFilename()));
    if (fStoreOnOriginal)
      if (!C4Group_CopyItem(pPlr->Filename, sTempFilename.getData()))
        return FALSE;
    // Open group
    C4Group PlrGroup;
    if (!PlrGroup.Open(sTempFilename.getData(), !fStoreOnOriginal))
      return FALSE;
    // Save player
    if (!pPlr->Save(PlrGroup, true, fStoreOnOriginal)) return FALSE;
    PlrGroup.Close();
    // Add temp file to group
    if (!hGroup.Move(sTempFilename.getData(), pNfo->GetFilename()))
      return FALSE;
  }
  return fSuccess;
}

BOOL C4PlayerList::Save(bool fSaveLocalOnly) {
  // do not save in replays
  if (Game.C4S.Head.Replay) return TRUE;
  // Save to external player files
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->GetType() != C4PT_Script)
      if (!fSaveLocalOnly || pPlr->LocalControl)
        if (!pPlr->Save()) return FALSE;
  return TRUE;
}

BOOL C4PlayerList::Evaluate() {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next) pPlr->Evaluate();
  return TRUE;
}

BOOL C4PlayerList::Retire(C4Player *pPlr) {
  if (!pPlr) return FALSE;

  if (!pPlr->Evaluated) {
    pPlr->Evaluate();
    if (!Game.Control.isReplay() && pPlr->GetType() != C4PT_Script)
      pPlr->Save();
  }
  Remove(pPlr, false, false);

  return TRUE;
}

int C4PlayerList::AverageValueGain() const {
  int iResult = 0;
  if (First) {
    for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
      iResult += Max<int32_t>(pPlr->ValueGain, 0);
    iResult /= GetCount();
  }
  return iResult;
}

C4Player *C4PlayerList::GetByName(const char *szName, int iExcluding) const {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (SEqual(pPlr->GetName(), szName))
      if (pPlr->Number != iExcluding) return pPlr;
  return NULL;
}

BOOL C4PlayerList::FileInUse(const char *szFilename) const {
  // Check original player files
  C4Player *cPlr = First;
  for (; cPlr; cPlr = cPlr->Next)
    if (ItemIdentical(cPlr->Filename, szFilename)) return TRUE;
  // Compare to any network path player files with prefix (hack)
  if (Game.Network.isEnabled()) {
    char szWithPrefix[_MAX_PATH + 1];
    SCopy(GetFilename(szFilename), szWithPrefix);
    SetClientPrefix(szWithPrefix, Game.Clients.getLocalName());
    for (cPlr = First; cPlr; cPlr = cPlr->Next)
      if (SEqualNoCase(GetFilename(cPlr->Filename), szWithPrefix)) return TRUE;
  }
  // Not in use
  return FALSE;
}

C4Player *C4PlayerList::GetLocalByIndex(int iIndex) const {
  int cindex = 0;
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->LocalControl) {
      if (cindex == iIndex) return pPlr;
      cindex++;
    }
  return NULL;
}

BOOL C4PlayerList::RemoveAtClient(int iClient, bool fDisconnect) {
  C4Player *pPlr;
  // Get players
  while (pPlr = GetAtClient(iClient)) {
    // Log
    Log(FormatString(LoadResStr("IDS_PRC_REMOVEPLR"), pPlr->GetName())
            .getData());
    // Remove
    Remove(pPlr, fDisconnect, false);
  }
  return TRUE;
}

BOOL C4PlayerList::RemoveAtClient(const char *szName, bool fDisconnect) {
  C4Player *pPlr;
  // Get players
  while (pPlr = GetAtClient(szName)) {
    // Log
    Log(FormatString(LoadResStr("IDS_PRC_REMOVEPLR"), pPlr->GetName())
            .getData());
    // Remove
    Remove(pPlr, fDisconnect, false);
  }
  return TRUE;
}

BOOL C4PlayerList::CtrlRemove(int iPlayer, bool fDisconnect) {
  // Add packet to input
  Game.Input.Add(CID_RemovePlr, new C4ControlRemovePlr(iPlayer, fDisconnect));
  return true;
}

BOOL C4PlayerList::CtrlRemoveAtClient(int iClient, bool fDisconnect) {
  // Get players
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->AtClient == iClient)
      if (!CtrlRemove(pPlr->Number, fDisconnect)) return FALSE;
  return TRUE;
}

BOOL C4PlayerList::CtrlRemoveAtClient(const char *szName, bool fDisconnect) {
  // Get players
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (SEqual(pPlr->AtClientName, szName))
      if (!CtrlRemove(pPlr->Number, fDisconnect)) return FALSE;
  return TRUE;
}

C4Player *C4PlayerList::GetAtClient(int iClient, int iIndex) const {
  int cindex = 0;
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->AtClient == iClient) {
      if (cindex == iIndex) return pPlr;
      cindex++;
    }
  return NULL;
}

C4Player *C4PlayerList::GetAtClient(const char *szName, int iIndex) const {
  int cindex = 0;
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (SEqualNoCase(pPlr->AtClientName, szName)) {
      if (cindex == iIndex) return pPlr;
      cindex++;
    }
  return NULL;
}

BOOL C4PlayerList::RemoveAtRemoteClient(bool fDisconnect, bool fNoCalls) {
  C4Player *pPlr;
  // Get players
  while (pPlr = GetAtRemoteClient()) {
    // Log
    Log(FormatString(LoadResStr("IDS_PRC_REMOVEPLR"), pPlr->GetName())
            .getData());
    // Remove
    Remove(pPlr, fDisconnect, fNoCalls);
  }
  return TRUE;
}

C4Player *C4PlayerList::GetAtRemoteClient(int iIndex) const {
  int cindex = 0;
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->AtClient != Game.Control.ClientID()) {
      if (cindex == iIndex) return pPlr;
      cindex++;
    }
  return NULL;
}

BOOL C4PlayerList::RemoveLocal(bool fDisconnect, bool fNoCalls) {
  // (used by league system the set local fate)
  C4Player *pPlr;
  do
    for (pPlr = First; pPlr; pPlr = pPlr->Next)
      if (pPlr->LocalControl) {
        // Log
        Log(FormatString(LoadResStr("IDS_PRC_REMOVEPLR"), pPlr->GetName())
                .getData());
        // Remove
        Remove(pPlr, fDisconnect, fNoCalls);
        break;
      }
  while (pPlr);

  return TRUE;
}

void C4PlayerList::EnumeratePointers() {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    pPlr->EnumeratePointers();
}

void C4PlayerList::DenumeratePointers() {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    pPlr->DenumeratePointers();
}

int C4PlayerList::ControlTakenBy(int iControl) const {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->Control == iControl)
      if (pPlr->LocalControl) return pPlr->Number;
  return NO_OWNER;
}

BOOL C4PlayerList::MouseControlTaken() const {
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (pPlr->MouseControl)
      if (pPlr->LocalControl) return TRUE;
  return FALSE;
}

int C4PlayerList::GetCountNotEliminated() const {
  int iCount = 0;
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    if (!pPlr->Eliminated) iCount++;
  return iCount;
}

BOOL C4PlayerList::SynchronizeLocalFiles() {
  // message
  Log(LoadResStr("IDS_PRC_SYNCPLRS"));
  bool fSuccess = true;
  // check all players
  for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
    // eliminated players will be saved soon, anyway
    if (!pPlr->Eliminated)
      if (!pPlr->LocalSync()) fSuccess = false;
  // done
  return fSuccess;
}

void C4PlayerList::RecheckPlayerSort(C4Player *pForPlayer) {
  if (!pForPlayer || !First) return;
  int iNumber = pForPlayer->Number;
  // get entry that should be the previous
  // (use '<=' to run past pForPlayer itself)
  C4Player *pPrev = First;
  while (pPrev->Next && pPrev->Next->Number <= iNumber) pPrev = pPrev->Next;
  // if it's correctly sorted, pPrev should point to pForPlayer itself
  if (pPrev == pForPlayer) return;
  // otherwise, pPrev points to the entry that should be the new previous
  // or to First if pForPlayer should be the head entry
  // re-link it there
  // first remove from old link pos
  C4Player **pOldLink = &First;
  while (*pOldLink && *pOldLink != pForPlayer) pOldLink = &((*pOldLink)->Next);
  if (*pOldLink) *pOldLink = pForPlayer->Next;
  // then link into new
  if (pPrev == First && pPrev->Number > iNumber) {
    // at head
    pForPlayer->Next = pPrev;
    First = pForPlayer;
  } else {
    // after prev
    pForPlayer->Next = pPrev->Next;
    pPrev->Next = pForPlayer;
  }
}
