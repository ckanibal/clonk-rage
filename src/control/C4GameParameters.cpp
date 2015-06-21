
#include "C4Include.h"
#include "C4GameParameters.h"

#ifndef BIG_C4INCLUDE
#include "C4Log.h"
#include "C4Components.h"
#include "game/C4Game.h"
#include "gui/C4Gui.h"
#include "C4Wrappers.h"
#endif

// *** C4GameRes

C4GameRes::C4GameRes() : eType(NRT_Null), pResCore(NULL), pNetRes(NULL) {}

C4GameRes::C4GameRes(const C4GameRes &Res)
    : eType(Res.getType()),
      File(Res.getFile()),
      pResCore(Res.getResCore()),
      pNetRes(Res.getNetRes()) {
  if (pResCore && !pNetRes) pResCore = new C4Network2ResCore(*pResCore);
}

C4GameRes::~C4GameRes() { Clear(); }

C4GameRes &C4GameRes::operator=(const C4GameRes &Res) {
  Clear();
  eType = Res.getType();
  File = Res.getFile();
  pResCore = Res.getResCore();
  pNetRes = Res.getNetRes();
  if (pResCore && !pNetRes) pResCore = new C4Network2ResCore(*pResCore);
  return *this;
}

void C4GameRes::Clear() {
  eType = NRT_Null;
  File.Clear();
  if (pResCore && !pNetRes) delete pResCore;
  pResCore = NULL;
  pNetRes = NULL;
}

void C4GameRes::SetFile(C4Network2ResType enType, const char *sznFile) {
  assert(!pNetRes && !pResCore);
  eType = enType;
  File = sznFile;
}

void C4GameRes::SetResCore(C4Network2ResCore *pnResCore) {
  assert(!pNetRes);
  pResCore = pnResCore;
  eType = pResCore->getType();
}

void C4GameRes::SetNetRes(C4Network2Res::Ref pnNetRes) {
  Clear();
  pNetRes = pnNetRes;
  eType = pNetRes->getType();
  File = pNetRes->getFile();
  pResCore = &pNetRes->getCore();
}

void C4GameRes::CompileFunc(StdCompiler *pComp) {
  bool fCompiler = pComp->isCompiler();
  // Clear previous data for compiling
  if (fCompiler) Clear();
  // Core is needed to decompile something meaningful
  if (!fCompiler) assert(pResCore);
  // De-/Compile core
  pComp->Value(mkPtrAdaptNoNull(const_cast<C4Network2ResCore *&>(pResCore)));
  // Compile: Set type accordingly
  if (fCompiler) eType = pResCore->getType();
}

bool C4GameRes::Publish(C4Network2ResList *pNetResList) {
  assert(isPresent());
  // Already present?
  if (pNetRes) return true;
  // determine whether it's loadable
  bool fAllowUnloadable = false;
  if (eType == NRT_Definitions) fAllowUnloadable = true;
  // Add to network resource list
  C4Network2Res::Ref pNetRes = pNetResList->AddByFile(
      File.getData(), false, eType, -1, NULL, fAllowUnloadable);
  if (!pNetRes) return false;
  // Set resource
  SetNetRes(pNetRes);
  return true;
}

bool C4GameRes::Load(C4Network2ResList *pNetResList) {
  assert(pResCore);
  // Already present?
  if (pNetRes) return true;
  // Add to network resource list
  C4Network2Res::Ref pNetRes = pNetResList->AddByCore(*pResCore);
  if (!pNetRes) return false;
  // Set resource
  SetNetRes(pNetRes);
  return true;
}

bool C4GameRes::InitNetwork(C4Network2ResList *pNetResList) {
  // Already initialized?
  if (getNetRes()) return true;
  // Present? [Host]
  if (isPresent()) {
    // Publish on network
    if (!Publish(pNetResList)) {
      LogFatal(FormatString(LoadResStr("IDS_NET_NOFILEPUBLISH"), getFile())
                   .getData());
      return false;
    }
  }
  // Got a core? [Client]
  else if (pResCore) {
    // Search/Load it
    if (!Load(pNetResList)) {
      // Give some hints to why this might happen.
      const char *szFilename = pResCore->getFileName();
      if (!pResCore->isLoadable())
        if (pResCore->getType() == NRT_System)
          LogFatal(FormatString(LoadResStr("IDS_NET_NOSAMESYSTEM"), szFilename)
                       .getData());
        else
          LogFatal(FormatString(LoadResStr("IDS_NET_NOSAMEANDTOOLARGE"),
                                szFilename).getData());
      // Should not happen
      else
        LogFatal(FormatString(LoadResStr("IDS_NET_NOVALIDCORE"), szFilename)
                     .getData());
      return false;
    }
  }
  // Okay
  return true;
}

void C4GameRes::CalcHash() {
  if (!pNetRes) return;
  pNetRes->CalculateSHA();
}

// *** C4GameResList

C4GameResList &C4GameResList::operator=(const C4GameResList &List) {
  Clear();
  // Copy the list
  iResCount = iResCapacity = List.iResCount;
  pResList = new C4GameRes *[iResCapacity];
  for (int i = 0; i < iResCount; i++)
    pResList[i] = new C4GameRes(*List.pResList[i]);
  return *this;
}

C4GameRes *C4GameResList::iterRes(C4GameRes *pLast, C4Network2ResType eType) {
  for (int i = 0; i < iResCount; i++)
    if (!pLast) {
      if (eType == NRT_Null || pResList[i]->getType() == eType)
        return pResList[i];
    } else if (pLast == pResList[i])
      pLast = NULL;
  return NULL;
}

void C4GameResList::Clear() {
  // clear them
  for (int32_t i = 0; i < iResCount; i++) delete pResList[i];
  delete[] pResList;
  pResList = NULL;
  iResCount = iResCapacity = 0;
}

bool C4GameResList::Load(const char *szDefinitionFilenames) {
  // clear any prev
  Clear();
  // no defs to be added? that's OK (LocalOnly)
  if (szDefinitionFilenames && *szDefinitionFilenames) {
    // add them
    char szSegment[_MAX_PATH + 1];
    for (int32_t cseg = 0;
         SCopySegment(szDefinitionFilenames, cseg, szSegment, ';', _MAX_PATH);
         ++cseg)
      if (*szSegment) {
        // Do a quick check whether the group file actually exists
        C4Group Def;
        if (!Def.Open(szSegment)) {
          LogFatal(FormatString(LoadResStr("IDS_PRC_DEFNOTFOUND"), szSegment)
                       .getData());
          return false;
        }
        Def.Close();
        // Okay, add it
        CreateByFile(NRT_Definitions, szSegment);
      }
  }
  // add System.c4g
  CreateByFile(NRT_System, C4CFN_System);
  // add all instances of Material.c4g, except those inside the scenario file
  C4Group *pMatParentGrp = NULL;
  while (pMatParentGrp =
             Game.GroupSet.FindGroup(C4GSCnt_Material, pMatParentGrp))
    if (pMatParentGrp != &Game.ScenarioFile) {
      StdStrBuf MaterialPath =
          pMatParentGrp->GetFullName() + DirSep C4CFN_Material;
      CreateByFile(NRT_Material, (pMatParentGrp->GetFullName() +
                                  DirSep C4CFN_Material).getData());
    }
  // add global Material.c4g
  CreateByFile(NRT_Material, C4CFN_Material);
  // done; success
  return true;
}

C4GameRes *C4GameResList::CreateByFile(C4Network2ResType eType,
                                       const char *szFile) {
  // Create & set
  C4GameRes *pRes = new C4GameRes();
  pRes->SetFile(eType, szFile);
  // Add to list
  Add(pRes);
  return pRes;
}

C4GameRes *C4GameResList::CreateByNetRes(C4Network2Res::Ref pNetRes) {
  // Create & set
  C4GameRes *pRes = new C4GameRes();
  pRes->SetNetRes(pNetRes);
  // Add to list
  Add(pRes);
  return pRes;
}

bool C4GameResList::InitNetwork(C4Network2ResList *pNetResList) {
  // Check all resources without attached network resource object
  for (int i = 0; i < iResCount; i++)
    if (!pResList[i]->InitNetwork(pNetResList)) return false;
  // Success
  return true;
}

void C4GameResList::CalcHashes() {
  for (int32_t i = 0; i < iResCount; i++) pResList[i]->CalcHash();
}

bool C4GameResList::RetrieveFiles() {
  // wait for all resources
  for (int32_t i = 0; i < iResCount; i++) {
    const C4Network2ResCore &Core = *pResList[i]->getResCore();
    StdStrBuf ResNameBuf =
        FormatString("%s: %s", LoadResStr("IDS_DLG_DEFINITION"),
                     GetFilename(Core.getFileName()));
    if (!Game.Network.RetrieveRes(Core, C4NetResRetrieveTimeout,
                                  ResNameBuf.getData()))
      return false;
  }
  return true;
}

void C4GameResList::Add(C4GameRes *pRes) {
  // Enlarge
  if (iResCount >= iResCapacity) {
    iResCapacity += 10;
    C4GameRes **pnResList = new C4GameRes *[iResCapacity];
    for (int i = 0; i < iResCount; i++) pnResList[i] = pResList[i];
    pResList = pnResList;
  }
  // Add
  pResList[iResCount++] = pRes;
}

void C4GameResList::CompileFunc(StdCompiler *pComp) {
  bool fCompiler = pComp->isCompiler();
  // Clear previous data
  if (fCompiler) Clear();
  // Compile resource count
  pComp->Value(mkNamingCountAdapt(iResCount, "Resource"));
  // Create list
  if (fCompiler) {
    pResList = new C4GameRes *[iResCapacity = iResCount];
    ZeroMem(pResList, sizeof(*pResList) * iResCount);
  }
  // Compile list
  pComp->Value(mkNamingAdapt(
      mkArrayAdaptMap(pResList, iResCount,
                      /*(C4GameRes *)NULL, */ mkPtrAdaptNoNull<C4GameRes>),
      "Resource"));
  mkPtrAdaptNoNull<C4GameRes>(*pResList);
}

// *** C4GameParameters

C4GameParameters::C4GameParameters() {}

C4GameParameters::~C4GameParameters() {}

void C4GameParameters::Clear() {
  League.Clear();
  LeagueAddress.Clear();
  Rules.Clear();
  Goals.Clear();
  Scenario.Clear();
  GameRes.Clear();
  Clients.Clear();
  PlayerInfos.Clear();
  RestorePlayerInfos.Clear();
  Teams.Clear();
}

BOOL C4GameParameters::Load(C4Group &hGroup, C4Scenario *pScenario,
                            const char *szGameText, C4LangStringTable *pLang,
                            const char *DefinitionFilenames) {
  // Clear previous data
  Clear();

  // Scenario
  Scenario.SetFile(NRT_Scenario, hGroup.GetFullName().getData());

  // Additional game resources
  if (!GameRes.Load(DefinitionFilenames)) return false;

  // Player infos (replays only)
  if (pScenario->Head.Replay)
    if (hGroup.FindEntry(C4CFN_PlayerInfos))
      PlayerInfos.Load(hGroup, C4CFN_PlayerInfos);

  // Savegame restore infos: Used for savegames to rejoin joined players
  if (hGroup.FindEntry(C4CFN_SavePlayerInfos)) {
    // load to savegame info list
    RestorePlayerInfos.Load(hGroup, C4CFN_SavePlayerInfos, pLang);
    // transfer counter to allow for additional player joins in savegame resumes
    PlayerInfos.SetIDCounter(RestorePlayerInfos.GetIDCounter());
    // in network mode, savegame players may be reassigned in the lobby
    // in any mode, the final player restoration will be done in InitPlayers()
    // dropping any players that could not be restored
  } else if (pScenario->Head.SaveGame) {
    // maybe there should be a player info file? (old-style savegame)
    if (szGameText) {
      // then recreate the player infos to be restored from game text
      RestorePlayerInfos.LoadFromGameText(szGameText);
      // transfer counter
      PlayerInfos.SetIDCounter(RestorePlayerInfos.GetIDCounter());
    }
  }

  // Load teams
  if (!Teams.Load(hGroup, pScenario, pLang)) {
    LogFatal(LoadResStr("IDS_PRC_ERRORLOADINGTEAMS"));
    return FALSE;
  }

  // Compile data
  StdStrBuf Buf;
  if (hGroup.LoadEntryString(C4CFN_Parameters, Buf)) {
    if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
            mkNamingAdapt(mkParAdapt(*this, pScenario), "Parameters"), Buf,
            C4CFN_Parameters))
      return FALSE;
  } else {
    // Set default values
    StdCompilerNull DefaultCompiler;
    DefaultCompiler.Compile(mkParAdapt(*this, pScenario));

    // Set random seed
    RandomSeed = time(NULL);

    // Set control rate default
    if (ControlRate < 0) ControlRate = Config.Network.ControlRate;

    // network game?
    IsNetworkGame = Game.NetworkActive;

    // FairCrew-flag by command line
    if (!FairCrewForced) UseFairCrew = !!Config.General.FairCrew;
    if (!FairCrewStrength && UseFairCrew)
      FairCrewStrength = Config.General.FairCrewStrength;

    // Auto frame skip by options
    AutoFrameSkip = ::Config.Graphics.AutoFrameSkip;
  }

  // enforce league settings
  if (isLeague()) EnforceLeagueRules(pScenario);

  // Done
  return TRUE;
}

void C4GameParameters::EnforceLeagueRules(C4Scenario *pScenario) {
  Scenario.CalcHash();
  GameRes.CalcHashes();
  Teams.EnforceLeagueRules();
  AllowDebug = false;
  // Fair crew enabled in league, if not explicitely disabled by scenario
  // Fair crew strengt to a moderately high value
  if (!Game.Parameters.FairCrewForced) {
    Game.Parameters.UseFairCrew = true;
    Game.Parameters.FairCrewForced = true;
    Game.Parameters.FairCrewStrength = 20000;
  }
  if (pScenario) MaxPlayers = pScenario->Head.MaxPlayerLeague;
}

bool C4GameParameters::CheckLeagueRulesStart(bool fFixIt) {
  // Additional checks for start parameters that are illegal in league games.

  if (!isLeague()) return true;

  bool fError = false;
  StdStrBuf Error;

  // league games: enforce one team per client
  C4ClientPlayerInfos *pClient;
  C4PlayerInfo *pInfo;
  for (int iClient = 0; pClient = Game.PlayerInfos.GetIndexedInfo(iClient);
       iClient++) {
    bool fHaveTeam = false;
    int32_t iClientTeam;
    const char *szFirstPlayer = nullptr;
    for (int iInfo = 0; pInfo = pClient->GetPlayerInfo(iInfo); iInfo++) {
      // Actual human players only
      if (pInfo->GetType() != C4PT_User) continue;

      int32_t iTeam = pInfo->GetTeam();
      if (!fHaveTeam) {
        iClientTeam = iTeam;
        szFirstPlayer = pInfo->GetName();
        fHaveTeam = true;
      } else if ((!Teams.IsCustom() && Game.C4S.Game.IsMelee()) ||
                 iTeam != iClientTeam) {
        Error.Format(LoadResStr("IDS_MSG_NOSPLITSCREENINLEAGUE"), szFirstPlayer,
                     pInfo->GetName());
        if (!fFixIt) {
          fError = true;
        } else {
          C4Client *pClient2 =
              Game.Clients.getClientByID(pClient->GetClientID());
          if (!pClient2 || pClient2->isHost())
            fError = true;
          else
            Game.Clients.CtrlRemove(pClient2, Error.getData());
        }
      }
    }
  }

  // Error?
  if (fError) {
    if (Game.pGUI)
      Game.pGUI->ShowMessageModal(
          Error.getData(), LoadResStr("IDS_NET_ERR_LEAGUE"),
          C4GUI::MessageDialog::btnOK, C4GUI::Ico_MeleeLeague);
    else
      Log(Error.getData());
    return false;
  }
  // All okay
  return true;
}

BOOL C4GameParameters::Save(C4Group &hGroup, C4Scenario *pScenario) {
  // Write Parameters.txt
  StdStrBuf ParData = DecompileToBuf<StdCompilerINIWrite>(
      mkNamingAdapt(mkParAdapt(*this, pScenario), "Parameters"));
  if (!hGroup.Add(C4CFN_Parameters, ParData, FALSE, TRUE)) return FALSE;

  // Done
  return TRUE;
}

BOOL C4GameParameters::InitNetwork(C4Network2ResList *pResList) {
  // Scenario & material resource
  if (!Scenario.InitNetwork(pResList)) return FALSE;

  // Other game resources
  if (!GameRes.InitNetwork(pResList)) return FALSE;

  // Done
  return TRUE;
}

void C4GameParameters::CompileFunc(StdCompiler *pComp, C4Scenario *pScenario) {
  pComp->Value(mkNamingAdapt(RandomSeed, "RandomSeed",
                             !pScenario ? 0 : pScenario->Head.RandomSeed));
  pComp->Value(mkNamingAdapt(StartupPlayerCount, "StartupPlayerCount", 0));
  pComp->Value(mkNamingAdapt(MaxPlayers, "MaxPlayers",
                             !pScenario ? 0 : pScenario->Head.MaxPlayer));
  pComp->Value(mkNamingAdapt(
      UseFairCrew, "UseFairCrew",
      !pScenario ? false
                 : (pScenario->Head.ForcedFairCrew == C4SFairCrew_FairCrew)));
  pComp->Value(mkNamingAdapt(
      FairCrewForced, "FairCrewForced",
      !pScenario ? false
                 : (pScenario->Head.ForcedFairCrew != C4SFairCrew_Free)));
  pComp->Value(
      mkNamingAdapt(FairCrewStrength, "FairCrewStrength",
                    !pScenario ? 0 : pScenario->Head.FairCrewStrength));
  pComp->Value(mkNamingAdapt(AllowDebug, "AllowDebug", true));
  pComp->Value(mkNamingAdapt(IsNetworkGame, "IsNetworkGame", false));
  pComp->Value(mkNamingAdapt(ControlRate, "ControlRate", -1));
  pComp->Value(mkNamingAdapt(AutoFrameSkip, "AutoFrameSkip", false));
  pComp->Value(mkNamingAdapt(Rules, "Rules",
                             !pScenario ? C4IDList() : pScenario->Game.Rules));
  pComp->Value(mkNamingAdapt(Goals, "Goals",
                             !pScenario ? C4IDList() : pScenario->Game.Goals));
  pComp->Value(mkNamingAdapt(League, "League", StdStrBuf()));

  // These values are either stored seperately (see Load/Save) or
  // don't make sense for savegames.
  if (!pScenario) {
    pComp->Value(mkNamingAdapt(LeagueAddress, "LeagueAddress", ""));

    pComp->Value(mkNamingAdapt(Scenario, "Scenario"));
    pComp->Value(GameRes);

    pComp->Value(mkNamingAdapt(PlayerInfos, "PlayerInfos"));
    pComp->Value(mkNamingAdapt(RestorePlayerInfos, "RestorePlayerInfos"));
    pComp->Value(mkNamingAdapt(Teams, "Teams"));
  }

  pComp->Value(Clients);
}

StdStrBuf C4GameParameters::GetGameGoalString() {
  // getting game goals from the ID list
  // unfortunately, names cannot be deduced before object definitions are loaded
  StdStrBuf sResult;
  C4ID idGoal;
  for (int32_t i = 0; i < Goals.GetNumberOfIDs(); ++i)
    if (idGoal = Goals.GetID(i))
      if (idGoal != C4ID_None) {
        if (Game.IsRunning) {
          C4Def *pDef = C4Id2Def(idGoal);
          if (pDef) {
            if (sResult.getLength()) sResult.Append(", ");
            sResult.Append(pDef->GetName());
          }
        } else {
          if (sResult.getLength()) sResult.Append(", ");
          sResult.Append(C4IdText(idGoal));
        }
      }
  // Max length safety
  if (sResult.getLength() > C4MaxTitle) sResult.SetLength(C4MaxTitle);
  // Compose desc string
  if (sResult.getLength())
    return FormatString("%s: %s", LoadResStr("IDS_MENU_CPGOALS"),
                        sResult.getData());
  else
    return StdCopyStrBuf(LoadResStr("IDS_CTL_NOGOAL"), true);
}
