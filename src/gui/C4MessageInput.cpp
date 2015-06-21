/* by Sven2, 2005 */
// handles input dialogs, last-message-buffer, MessageBoard-commands

#include "C4Include.h"
#include "gui/C4MessageInput.h"

#ifndef BIG_C4INCLUDE
#include "game/C4Game.h"
#include "object/C4Object.h"
#include "script/C4Script.h"
#include "gui/C4Gui.h"
#include "C4Console.h"
#include "game/C4Application.h"
#include "network/C4Network2Dialogs.h"
#include "C4Log.h"
#include "player/C4Player.h"
#include "gui/C4GameLobby.h"
#endif

// --------------------------------------------------
// C4ChatInputDialog

// singleton
C4ChatInputDialog *C4ChatInputDialog::pInstance = NULL;

// helper func: Determine whether input text is good for a chat-style-layout
// dialog
bool IsSmallInputQuery(const char *szInputQuery) {
  if (!szInputQuery) return true;
  int32_t w, h;
  if (SCharCount('|', szInputQuery)) return false;
  if (!C4GUI::GetRes()->TextFont.GetTextExtent(szInputQuery, w, h, true))
    return false;  // ???
  return w < C4GUI::GetScreenWdt() / 5;
}

C4ChatInputDialog::C4ChatInputDialog(bool fObjInput, C4Object *pScriptTarget,
                                     bool fUppercase, bool fTeam, int32_t iPlr,
                                     const StdStrBuf &rsInputQuery)
    : C4GUI::InputDialog(
          fObjInput ? rsInputQuery.getData() : LoadResStrNoAmp("IDS_CTL_CHAT"),
          NULL, C4GUI::Ico_None, NULL,
          !fObjInput || IsSmallInputQuery(rsInputQuery.getData())),
      fObjInput(fObjInput),
      fUppercase(fUppercase),
      pTarget(pScriptTarget),
      BackIndex(-1),
      iPlr(iPlr),
      fProcessed(false) {
  // singleton-var
  pInstance = this;
  // set custom edit control
  SetCustomEdit(new C4GUI::CallbackEdit<C4ChatInputDialog>(
      C4Rect(0, 0, 10, 10), this, &C4ChatInputDialog::OnChatInput,
      &C4ChatInputDialog::OnChatCancel));
  // key bindings
  pKeyHistoryUp =
      new C4KeyBinding(C4KeyCodeEx(K_UP), "ChatHistoryUp", KEYSCOPE_Gui,
                       new C4GUI::DlgKeyCBEx<C4ChatInputDialog, bool>(
                           *this, true, &C4ChatInputDialog::KeyHistoryUpDown),
                       C4CustomKey::PRIO_CtrlOverride);
  pKeyHistoryDown =
      new C4KeyBinding(C4KeyCodeEx(K_DOWN), "ChatHistoryDown", KEYSCOPE_Gui,
                       new C4GUI::DlgKeyCBEx<C4ChatInputDialog, bool>(
                           *this, false, &C4ChatInputDialog::KeyHistoryUpDown),
                       C4CustomKey::PRIO_CtrlOverride);
  pKeyAbort = new C4KeyBinding(
      C4KeyCodeEx(K_F2), "ChatAbort", KEYSCOPE_Gui,
      new C4GUI::DlgKeyCB<C4GUI::Dialog>(*this, &C4GUI::Dialog::KeyEscape),
      C4CustomKey::PRIO_CtrlOverride);
  pKeyNickComplete =
      new C4KeyBinding(C4KeyCodeEx(K_TAB), "ChatNickComplete", KEYSCOPE_Gui,
                       new C4GUI::DlgKeyCB<C4ChatInputDialog>(
                           *this, &C4ChatInputDialog::KeyCompleteNick),
                       C4CustomKey::PRIO_CtrlOverride);
  pKeyPlrControl = new C4KeyBinding(
      C4KeyCodeEx(KEY_Any, KEYS_Control), "ChatForwardPlrCtrl", KEYSCOPE_Gui,
      new C4GUI::DlgKeyCBPassKey<C4ChatInputDialog>(
          *this, &C4ChatInputDialog::KeyPlrControl),
      C4CustomKey::PRIO_Dlg);
  pKeyGamepadControl = new C4KeyBinding(
      C4KeyCodeEx(KEY_Any), "ChatForwardGamepadCtrl", KEYSCOPE_Gui,
      new C4GUI::DlgKeyCBPassKey<C4ChatInputDialog>(
          *this, &C4ChatInputDialog::KeyGamepadControlDown,
          &C4ChatInputDialog::KeyGamepadControlUp,
          &C4ChatInputDialog::KeyGamepadControlPressed),
      C4CustomKey::PRIO_PlrControl);
  pKeyBackClose =
      new C4KeyBinding(C4KeyCodeEx(K_BACK), "ChatBackspaceClose", KEYSCOPE_Gui,
                       new C4GUI::DlgKeyCB<C4ChatInputDialog>(
                           *this, &C4ChatInputDialog::KeyBackspaceClose),
                       C4CustomKey::PRIO_CtrlOverride);
  // free when closed...
  SetDelOnClose();
  // initial team text
  if (fTeam) pEdit->InsertText("/team ", true);
}

C4ChatInputDialog::~C4ChatInputDialog() {
  delete pKeyHistoryUp;
  delete pKeyHistoryDown;
  delete pKeyAbort;
  delete pKeyNickComplete;
  delete pKeyPlrControl;
  delete pKeyGamepadControl;
  delete pKeyBackClose;
  if (this == pInstance) pInstance = NULL;
}

void C4ChatInputDialog::OnChatCancel() {
  // abort chat: Make sure msg board query is aborted
  fProcessed = true;
  if (fObjInput) {
    // check if the target input is still valid
    C4Player *pPlr = Game.Players.Get(iPlr);
    if (!pPlr) return;
    if (pPlr->MarkMessageBoardQueryAnswered(pTarget)) {
      // there was an associated query - it must be removed on all clients
      // synchronized via queue
      // do this by calling OnMessageBoardAnswer without an answer
      Game.Control.DoInput(
          CID_Script,
          new C4ControlScript(
              FormatString("OnMessageBoardAnswer(Object(%d), %d, 0)",
                           pTarget ? pTarget->Number : 0, iPlr).getData()),
          CDT_Decide);
    }
  }
}

void C4ChatInputDialog::OnClosed(bool fOK) {
  // make sure chat input is processed, even if closed by other means than Enter
  // on edit
  if (!fProcessed)
    if (fOK)
      OnChatInput(pEdit, false, false);
    else
      OnChatCancel();
  else
    OnChatCancel();
  typedef C4GUI::InputDialog BaseDlg;
  BaseDlg::OnClosed(fOK);
}

C4GUI::Edit::InputResult C4ChatInputDialog::OnChatInput(C4GUI::Edit *edt,
                                                        bool fPasting,
                                                        bool fPastingMore) {
  // no double processing
  if (fProcessed) return C4GUI::Edit::IR_CloseDlg;
  // get edit text
  C4GUI::Edit *pEdt = reinterpret_cast<C4GUI::Edit *>(edt);
  char *szInputText = const_cast<char *>(pEdt->GetText());
  // Store to back buffer
  Game.MessageInput.StoreBackBuffer(szInputText);
  // script queried input?
  if (fObjInput) {
    fProcessed = true;
    // check if the target input is still valid
    C4Player *pPlr = Game.Players.Get(iPlr);
    if (!pPlr) return C4GUI::Edit::IR_CloseDlg;
    if (!pPlr->MarkMessageBoardQueryAnswered(pTarget)) {
      // there was no associated query!
      return C4GUI::Edit::IR_CloseDlg;
    }
    // then do a script callback, incorporating the input into the answer
    if (fUppercase) SCapitalize(szInputText);
    StdStrBuf sInput;
    sInput.Copy(szInputText);
    sInput.EscapeString();
    Game.Control.DoInput(
        CID_Script,
        new C4ControlScript(
            FormatString("OnMessageBoardAnswer(Object(%d), %d, \"%s\")",
                         pTarget ? pTarget->Number : 0, iPlr,
                         sInput.getData()).getData()),
        CDT_Decide);
    return C4GUI::Edit::IR_CloseDlg;
  } else
    // reroute to message input class
    Game.MessageInput.ProcessInput(szInputText);
  // safety: message board commands may do strange things
  if (!C4GUI::IsGUIValid() || this != pInstance) return C4GUI::Edit::IR_Abort;
  // select all text to be removed with next keypress
  // just for pasting mode; usually the dlg will be closed now anyway
  pEdt->SelectAll();
  // avoid dlg close, if more content is to be pasted
  if (fPastingMore) return C4GUI::Edit::IR_None;
  fProcessed = true;
  return C4GUI::Edit::IR_CloseDlg;
}

bool C4ChatInputDialog::KeyHistoryUpDown(bool fUp) {
  // browse chat history
  pEdit->SelectAll();
  pEdit->DeleteSelection();
  const char *szPrevInput =
      Game.MessageInput.GetBackBuffer(fUp ? (++BackIndex) : (--BackIndex));
  if (!szPrevInput || !*szPrevInput)
    BackIndex = -1;
  else {
    pEdit->InsertText(szPrevInput, true);
    pEdit->SelectAll();
  }
  return true;
}

bool C4ChatInputDialog::KeyPlrControl(C4KeyCodeEx key) {
  // Control pressed while doing this key: Reroute this key as a player-control
  Game.DoKeyboardInput(WORD(key.Key), KEYEV_Down, !!(key.dwShift & KEYS_Alt),
                       false, !!(key.dwShift & KEYS_Shift), key.IsRepeated(),
                       NULL, true);
  // mark as processed, so it won't get any double processing
  return true;
}

bool C4ChatInputDialog::KeyGamepadControlDown(C4KeyCodeEx key) {
  // filter gamepad control
  if (!Key_IsGamepad(key.Key)) return false;
  // forward it
  Game.DoKeyboardInput(key.Key, KEYEV_Down, false, false, false,
                       key.IsRepeated(), NULL, true);
  return true;
}

bool C4ChatInputDialog::KeyGamepadControlUp(C4KeyCodeEx key) {
  // filter gamepad control
  if (!Key_IsGamepad(key.Key)) return false;
  // forward it
  Game.DoKeyboardInput(key.Key, KEYEV_Up, false, false, false, key.IsRepeated(),
                       NULL, true);
  return true;
}

bool C4ChatInputDialog::KeyGamepadControlPressed(C4KeyCodeEx key) {
  // filter gamepad control
  if (!Key_IsGamepad(key.Key)) return false;
  // forward it
  Game.DoKeyboardInput(key.Key, KEYEV_Pressed, false, false, false,
                       key.IsRepeated(), NULL, true);
  return true;
}

bool C4ChatInputDialog::KeyBackspaceClose() {
  // close if chat text box is empty (on backspace)
  if (pEdit->GetText() && *pEdit->GetText()) return false;
  Close(false);
  return true;
}

bool C4ChatInputDialog::KeyCompleteNick() {
  if (!pEdit) return false;
  char IncompleteNick[256 + 1];
  // get current word in edit
  if (!pEdit->GetCurrentWord(IncompleteNick, 256)) return false;
  if (!*IncompleteNick) return false;
  C4Player *plr = Game.Players.First;
  while (plr) {
    // Compare name and input
    if (SEqualNoCase(plr->GetName(), IncompleteNick, SLen(IncompleteNick))) {
      pEdit->InsertText(plr->GetName() + SLen(IncompleteNick), true);
      return true;
    } else
      plr = plr->Next;
  }
  // no match found
  return false;
}

// --------------------------------------------------
// C4MessageInput

bool C4MessageInput::Init() {
  // add default commands
  if (!pCommands) {
    AddCommand("speed", "SetGameSpeed(%d)");
  }
  return true;
}

void C4MessageInput::Default() {
  // clear backlog
  for (int32_t cnt = 0; cnt < C4MSGB_BackBufferMax; cnt++)
    BackBuffer[cnt][0] = 0;
}

void C4MessageInput::Clear() {
  // close any dialog
  CloseTypeIn();
  // free messageboard-commands
  C4MessageBoardCommand *pCmd;
  while (pCmd = pCommands) {
    pCommands = pCmd->Next;
    delete pCmd;
  }
}

bool C4MessageInput::CloseTypeIn() {
  // close dialog if present and valid
  C4ChatInputDialog *pDlg = GetTypeIn();
  if (!pDlg || !C4GUI::IsGUIValid()) return false;
  pDlg->Close(false);
  return true;
}

bool C4MessageInput::StartTypeIn(bool fObjInput, C4Object *pObj,
                                 bool fUpperCase, bool fTeam, int32_t iPlr,
                                 const StdStrBuf &rsInputQuery) {
  if (!C4GUI::IsGUIValid()) return false;
  // if (!Application.isFullScreen) return false;
  // close any previous
  if (IsTypeIn()) CloseTypeIn();
  // start new
  return Game.pGUI->ShowRemoveDlg(new C4ChatInputDialog(
      fObjInput, pObj, fUpperCase, fTeam, iPlr, rsInputQuery));
}

bool C4MessageInput::KeyStartTypeIn(bool fTeam) {
  // fullscreen only
  if (!Application.isFullScreen) return false;
  // OK, start typing
  return StartTypeIn(false, NULL, false, fTeam);
}

bool C4MessageInput::ToggleTypeIn() {
  // safety
  if (!C4GUI::IsGUIValid()) return false;
  // toggle off?
  if (IsTypeIn()) {
    // no accidental close of script queried dlgs by chat request
    if (GetTypeIn()->IsScriptQueried()) return false;
    return CloseTypeIn();
  } else
    // toggle on!
    return StartTypeIn();
}

bool C4MessageInput::IsTypeIn() {
  // check GUI and dialog
  return C4GUI::IsGUIValid() && C4ChatInputDialog::IsShown();
}

bool C4MessageInput::ProcessInput(const char *szText) {
  // helper variables
  C4ControlMessageType eMsgType;
  const char *szMsg = NULL;
  int32_t iToPlayer = -1;

  // Starts with '^', "team:" or "/team ": Team message
  if (szText[0] == '^' || SEqual2NoCase(szText, "team:") ||
      SEqual2NoCase(szText, "/team ")) {
    if (!Game.Teams.IsTeamVisible()) {
      // team not known; can't send!
      Log(LoadResStr("IDS_MSG_CANTSENDTEAMMESSAGETEAMSN"));
      return FALSE;
    } else {
      eMsgType = C4CMT_Team;
      szMsg = szText[0] == '^' ? szText + 1 : szText[0] == '/' ? szText + 6
                                                               : szText + 5;
    }
  }
  // Starts with "/private ": Private message (running game only)
  else if (Game.IsRunning && SEqual2NoCase(szText, "/private ")) {
    // get target name
    char szTargetPlr[C4MaxName + 1];
    SCopyUntil(szText + 9, szTargetPlr, ' ', C4MaxName);
    // search player
    C4Player *pToPlr = Game.Players.GetByName(szTargetPlr);
    if (!pToPlr) return FALSE;
    // set
    eMsgType = C4CMT_Private;
    iToPlayer = pToPlr->Number;
    szMsg = szText + 10 + SLen(szTargetPlr);
    if (szMsg > szText + SLen(szText)) return FALSE;
  }
  // Starts with "/me ": Me-Message
  else if (SEqual2NoCase(szText, "/me ")) {
    eMsgType = C4CMT_Me;
    szMsg = szText + 4;
  }
  // Starts with "/sound ": Sound-Message
  else if (SEqual2NoCase(szText, "/sound ")) {
    eMsgType = C4CMT_Sound;
    szMsg = szText + 7;
  }
  // Starts with "/alert": Taskbar flash (message optional)
  else if (SEqual2NoCase(szText, "/alert ") || SEqualNoCase(szText, "/alert")) {
    eMsgType = C4CMT_Alert;
    szMsg = szText + 6;
    if (*szMsg) ++szMsg;
  }
  // Starts with '"': Let the clonk say it
  else if (Game.IsRunning && szText[0] == '"') {
    eMsgType = C4CMT_Say;
    // Append '"', if neccessary
    SCopy(szText, OSTR, 400);
    char *pEnd = OSTR + SLen(OSTR) - 1;
    if (*pEnd != '"') {
      *++pEnd = '"';
      *++pEnd = 0;
    }
    szMsg = OSTR;
  }
  // Starts with '/': Command
  else if (szText[0] == '/')
    return ProcessCommand(szText);
  // Regular message
  else {
    eMsgType = C4CMT_Normal;
    szMsg = szText;
  }

  // message?
  if (szMsg) {
    char szMessage[C4MaxMessage + 1];
    // go over whitespaces, check empty message
    while (IsWhiteSpace(*szMsg)) szMsg++;
    if (!*szMsg) {
      if (eMsgType != C4CMT_Alert) return TRUE;
      *szMessage = '\0';
    } else {
      // trim right
      const char *szEnd = szMsg + SLen(szMsg) - 1;
      while (IsWhiteSpace(*szEnd) && szEnd >= szMsg) szEnd--;
      // Say: Strip quotation marks in cinematic film mode
      if (Game.C4S.Head.Film == C4SFilm_Cinematic) {
        if (eMsgType == C4CMT_Say) {
          ++szMsg;
          szEnd--;
        }
      }
      // get message
      SCopy(szMsg, szMessage,
            Min<unsigned long>(C4MaxMessage, szEnd - szMsg + 1));
    }
    // get sending player (if any)
    C4Player *pPlr = Game.IsRunning ? Game.Players.GetLocalByIndex(0) : NULL;
    // send
    Game.Control.DoInput(
        CID_Message, new C4ControlMessage(eMsgType, szMessage,
                                          pPlr ? pPlr->Number : -1, iToPlayer),
        CDT_Private);
  }

  return TRUE;
}

bool C4MessageInput::ProcessCommand(const char *szCommand) {
  C4GameLobby::MainDlg *pLobby = Game.Network.GetLobby();
  // command
  char szCmdName[C4MaxName + 1];
  SCopyUntil(szCommand + 1, szCmdName, ' ', C4MaxName);
  // parameter
  const char *pCmdPar = SSearch(szCommand, " ");
  if (!pCmdPar) pCmdPar = "";

  // dev-scripts
  if (SEqual(szCmdName, "help")) {
    LogF(LoadResStr("IDS_TEXT_COMMANDSAVAILABLEDURINGGA"));
    LogF("/private [player] [message] - %s",
         LoadResStr("IDS_MSG_SENDAPRIVATEMESSAGETOTHES"));
    LogF("/team [message] - %s",
         LoadResStr("IDS_MSG_SENDAPRIVATEMESSAGETOYOUR"));
    LogF("/me [action] - %s", LoadResStr("IDS_TEXT_PERFORMANACTIONINYOURNAME"));
    LogF("/sound [sound] - %s",
         LoadResStr("IDS_TEXT_PLAYASOUNDFROMTHEGLOBALSO"));
    LogF("/kick [client] - %s", LoadResStr("IDS_TEXT_KICKTHESPECIFIEDCLIENT"));
    LogF("/observer [client] - %s",
         LoadResStr("IDS_TEXT_SETTHESPECIFIEDCLIENTTOOB"));
    LogF("/fast [x] - %s", LoadResStr("IDS_TEXT_SETTOFASTMODESKIPPINGXFRA"));
    LogF("/slow - %s", LoadResStr("IDS_TEXT_SETTONORMALSPEEDMODE"));
    LogF("/chart - %s", LoadResStr("IDS_TEXT_DISPLAYNETWORKSTATISTICS"));
    LogF("/nodebug - %s", LoadResStr("IDS_TEXT_PREVENTDEBUGMODEINTHISROU"));
    LogF("/set comment [comment] - %s",
         LoadResStr("IDS_TEXT_SETANEWNETWORKCOMMENT"));
    LogF("/set password [password] - %s",
         LoadResStr("IDS_TEXT_SETANEWNETWORKPASSWORD"));
    LogF("/set faircrew [on/off] - %s",
         LoadResStr("IDS_TEXT_ENABLEORDISABLEFAIRCREW"));
    LogF("/set maxplayer [4] - %s",
         LoadResStr("IDS_TEXT_SETANEWMAXIMUMNUMBEROFPLA"));
    LogF("/script [script] - %s", LoadResStr("IDS_TEXT_EXECUTEASCRIPTCOMMAND"));
    LogF("/clear - %s", LoadResStr("IDS_MSG_CLEARTHEMESSAGEBOARD"));
    return TRUE;
  }
  // dev-scripts
  if (SEqual(szCmdName, "script")) {
    if (!Game.IsRunning) return FALSE;
    if (!Game.DebugMode) return FALSE;
    if (!Game.Network.isEnabled() &&
        !SEqual(Game.ScenarioFile.GetMaker(), Config.General.Name) &&
        Game.ScenarioFile.GetStatus() != GRPF_Folder)
      return FALSE;
    if (Game.Network.isEnabled() && !Game.Network.isHost()) return FALSE;

    Game.Control.DoInput(
        CID_Script,
        new C4ControlScript(pCmdPar, C4ControlScript::SCOPE_Console, false),
        CDT_Decide);
    return TRUE;
  }
  // set runtimte properties
  if (SEqual(szCmdName, "set")) {
    if (SEqual2(pCmdPar, "maxplayer ")) {
      if (Game.Control.isCtrlHost()) {
        if (atoi(pCmdPar + 10) == 0 && !SEqual(pCmdPar + 10, "0")) {
          Log("Syntax: /set maxplayer count");
          return FALSE;
        }
        Game.Control.DoInput(
            CID_Set, new C4ControlSet(C4CVT_MaxPlayer, atoi(pCmdPar + 10)),
            CDT_Decide);
        return TRUE;
      }
    }
    if (SEqual2(pCmdPar, "comment ") || SEqual(pCmdPar, "comment")) {
      if (!Game.Network.isEnabled() || !Game.Network.isHost()) return FALSE;
      // Set in configuration, update reference
      Config.Network.Comment.CopyValidated(pCmdPar[7] ? (pCmdPar + 8) : "");
      Game.Network.InvalidateReference();
      Log(LoadResStr("IDS_NET_COMMENTCHANGED"));
      return TRUE;
    }
    if (SEqual2(pCmdPar, "password ") || SEqual(pCmdPar, "password")) {
      if (!Game.Network.isEnabled() || !Game.Network.isHost()) return FALSE;
      Game.Network.SetPassword(pCmdPar[8] ? (pCmdPar + 9) : NULL);
      if (pLobby) pLobby->UpdatePassword();
      return TRUE;
    }
    if (SEqual2(pCmdPar, "faircrew ")) {
      if (!Game.Control.isCtrlHost() || Game.Parameters.isLeague())
        return FALSE;
      C4ControlSet *pSet = NULL;
      if (SEqual(pCmdPar + 9, "on"))
        pSet =
            new C4ControlSet(C4CVT_FairCrew, Config.General.FairCrewStrength);
      else if (SEqual(pCmdPar + 9, "off"))
        pSet = new C4ControlSet(C4CVT_FairCrew, -1);
      else if (isdigit((unsigned char)pCmdPar[9]))
        pSet = new C4ControlSet(C4CVT_FairCrew, atoi(pCmdPar + 9));
      else
        return FALSE;
      Game.Control.DoInput(CID_Set, pSet, CDT_Decide);
      return TRUE;
    }
    // unknown property
    return FALSE;
  }
  // get szen from network folder - not in lobby; use res tab there
  if (SEqual(szCmdName, "netgetscen")) {
    if (Game.Network.isEnabled() && !Game.Network.isHost() && !pLobby) {
      const C4Network2ResCore *pResCoreScen =
          Game.Parameters.Scenario.getResCore();
      if (pResCoreScen) {
        C4Network2Res::Ref pScenario =
            Game.Network.ResList.getRefRes(pResCoreScen->getID());
        if (pScenario)
          if (C4Group_CopyItem(
                  pScenario->getFile(),
                  Config.AtExePath(GetFilename(Game.ScenarioFilename)))) {
            LogF(LoadResStr("IDS_MSG_CMD_NETGETSCEN_SAVED"),
                 Config.AtExePath(GetFilename(Game.ScenarioFilename)));
            return TRUE;
          }
      }
    }
    return FALSE;
  }
  // clear message board
  if (SEqual(szCmdName, "clear")) {
    // lobby
    if (pLobby) {
      pLobby->ClearLog();
    }
    // fullscreen
    else if (Game.GraphicsSystem.MessageBoard.Active)
      Game.GraphicsSystem.MessageBoard.ClearLog();
    else {
      // EM mode
      Console.ClearLog();
    }
    return TRUE;
  }
  // kick client
  if (SEqual(szCmdName, "kick")) {
    if (Game.Network.isEnabled() && Game.Network.isHost()) {
      // find client
      C4Client *pClient = Game.Clients.getClientByName(pCmdPar);
      if (!pClient) {
        LogF(LoadResStr("IDS_MSG_CMD_NOCLIENT"), pCmdPar);
        return FALSE;
      }
      // league: Kick needs voting
      if (Game.Parameters.isLeague() &&
          Game.Players.GetAtClient(pClient->getID()))
        Game.Network.Vote(VT_Kick, true, pClient->getID());
      else
        // add control
        Game.Clients.CtrlRemove(pClient,
                                LoadResStr("IDS_MSG_KICKFROMMSGBOARD"));
    }
    return TRUE;
  }
  // set fast mode
  if (SEqual(szCmdName, "fast")) {
    if (!Game.IsRunning) return FALSE;
    if (Game.Parameters.isLeague()) {
      Log(LoadResStr("IDS_LOG_COMMANDNOTALLOWEDINLEAGUE"));
      return FALSE;
    }
    int32_t iFS;
    if ((iFS = atoi(pCmdPar)) == 0) return FALSE;
    // set frameskip and fullspeed flag
    Game.FrameSkip = BoundBy<int32_t>(iFS, 1, 500);
    Game.FullSpeed = TRUE;
    // start calculation immediatly
    Application.NextTick(false);
    return TRUE;
  }
  // reset fast mode
  if (SEqual(szCmdName, "slow")) {
    if (!Game.IsRunning) return FALSE;
    Game.FullSpeed = FALSE;
    Game.FrameSkip = 1;
    return TRUE;
  }

  if (SEqual(szCmdName, "nodebug")) {
    if (!Game.IsRunning) return FALSE;
    Game.Control.DoInput(CID_Set, new C4ControlSet(C4CVT_AllowDebug, false),
                         CDT_Decide);
    return TRUE;
  }

  if (SEqual(szCmdName, "msgboard")) {
    if (!Game.IsRunning) return FALSE;
    // get line cnt
    int32_t iLineCnt = BoundBy(atoi(pCmdPar), 0, 20);
    if (iLineCnt == 0)
      Game.GraphicsSystem.MessageBoard.ChangeMode(2);
    else if (iLineCnt == 1)
      Game.GraphicsSystem.MessageBoard.ChangeMode(0);
    else {
      Game.GraphicsSystem.MessageBoard.iLines = iLineCnt;
      Game.GraphicsSystem.MessageBoard.ChangeMode(1);
    }
    return TRUE;
  }

  // kick/activate/deactivate/observer
  if (SEqual(szCmdName, "activate") || SEqual(szCmdName, "deactivate") ||
      SEqual(szCmdName, "observer")) {
    if (!Game.Network.isEnabled() || !Game.Network.isHost()) {
      Log(LoadResStr("IDS_MSG_CMD_HOSTONLY"));
      return FALSE;
    }
    // search for client
    C4Client *pClient = Game.Clients.getClientByName(pCmdPar);
    if (!pClient) {
      LogF(LoadResStr("IDS_MSG_CMD_NOCLIENT"), pCmdPar);
      return FALSE;
    }
    // what to do?
    C4ControlClientUpdate *pCtrl = NULL;
    if (szCmdName[0] == 'a')  // activate
      pCtrl = new C4ControlClientUpdate(pClient->getID(), CUT_Activate, true);
    else if (szCmdName[0] == 'd' && !Game.Parameters.isLeague())  // deactivate
      pCtrl = new C4ControlClientUpdate(pClient->getID(), CUT_Activate, false);
    else if (szCmdName[0] == 'o' && !Game.Parameters.isLeague())  // observer
      pCtrl = new C4ControlClientUpdate(pClient->getID(), CUT_SetObserver);
    // perform it
    if (pCtrl)
      Game.Control.DoInput(CID_ClientUpdate, pCtrl, CDT_Sync);
    else
      Log(LoadResStr("IDS_LOG_COMMANDNOTALLOWEDINLEAGUE"));
    return TRUE;
  }

  // control mode
  if (SEqual(szCmdName, "centralctrl") || SEqual(szCmdName, "decentralctrl") ||
      SEqual(szCmdName, "asyncctrl")) {
    if (!Game.Network.isEnabled() || !Game.Network.isHost()) {
      Log(LoadResStr("IDS_MSG_CMD_HOSTONLY"));
      return FALSE;
    }
    if (Game.Parameters.isLeague() && *szCmdName == 'a') {
      Log(LoadResStr("IDS_LOG_COMMANDNOTALLOWEDINLEAGUE"));
      return FALSE;
    }
    Game.Network.SetCtrlMode(
        *szCmdName == 'c' ? CNM_Central : *szCmdName == 'd' ? CNM_Decentral
                                                            : CNM_Async);
    return TRUE;
  }

  // show chart
  if (Game.IsRunning)
    if (SEqual(szCmdName, "chart")) return Game.ToggleChart();

  // custom command
  C4MessageBoardCommand *pCmd;
  if (Game.IsRunning)
    if (pCmd = GetCommand(szCmdName)) {
      StdStrBuf Script, CmdScript;
      // replace %player% by calling player number
      if (SSearch(pCmd->Script, "%player%")) {
        int32_t iLocalPlr = NO_OWNER;
        C4Player *pLocalPlr = Game.Players.GetLocalByIndex(0);
        if (pLocalPlr) iLocalPlr = pLocalPlr->Number;
        StdStrBuf sLocalPlr;
        sLocalPlr.Format("%d", iLocalPlr);
        CmdScript.Copy(pCmd->Script);
        CmdScript.Replace("%player%", sLocalPlr.getData());
      } else {
        CmdScript.Ref(pCmd->Script);
      }
      // insert parameters
      if (SSearch(CmdScript.getData(), "%d")) {
        // make sure it's a number by converting
        Script.Format(CmdScript.getData(), (int)atoi(pCmdPar));
      } else if (SSearch(CmdScript.getData(), "%s")) {
        // Unrestricted parameters?
        // That's kind of a security risk as it will allow anyone to execute
        // code
        switch (pCmd->eRestriction) {
          case C4MessageBoardCommand::C4MSGCMDR_Escaped: {
            // escape strings
            StdStrBuf Par;
            Par.Copy(pCmdPar);
            Par.EscapeString();
            // compose script
            Script.Format(CmdScript.getData(), Par.getData());
          } break;

          case C4MessageBoardCommand::C4MSGCMDR_Plain:
            // unescaped
            Script.Format(CmdScript.getData(), pCmdPar);
            break;

          case C4MessageBoardCommand::C4MSGCMDR_Identifier: {
            // only allow identifier-characters
            StdStrBuf Par;
            while (IsIdentifier(*pCmdPar) || isspace((unsigned char)*pCmdPar))
              Par.AppendChar(*pCmdPar++);
            // compose script
            Script.Format(CmdScript.getData(), Par.getData());
          } break;
        }
      } else
        Script = CmdScript.getData();
      // add script
      Game.Control.DoInput(CID_Script, new C4ControlScript(Script.getData()),
                           CDT_Decide);
      // ok
      return TRUE;
    }

  // unknown command
  StdStrBuf sErr;
  sErr.Format(LoadResStr("IDS_ERR_UNKNOWNCMD"), szCmdName);
  if (pLobby)
    pLobby->OnError(sErr.getData());
  else
    Log(sErr.getData());
  return FALSE;
}

void C4MessageInput::AddCommand(
    const char *strCommand, const char *strScript,
    C4MessageBoardCommand::Restriction eRestriction) {
  if (GetCommand(strCommand)) return;
  // create entry
  C4MessageBoardCommand *pCmd = new C4MessageBoardCommand();
  SCopy(strCommand, pCmd->Name, C4MaxName);
  SCopy(strScript, pCmd->Script, _MAX_FNAME + 30);
  pCmd->eRestriction = eRestriction;
  // add to list
  pCmd->Next = pCommands;
  pCommands = pCmd;
}

C4MessageBoardCommand *C4MessageInput::GetCommand(const char *strName) {
  for (C4MessageBoardCommand *pCmd = pCommands; pCmd; pCmd = pCmd->Next)
    if (SEqual(pCmd->Name, strName)) return pCmd;
  return NULL;
}

void C4MessageInput::ClearPointers(C4Object *pObj) {
  // target object loose? stop input
  C4ChatInputDialog *pDlg = GetTypeIn();
  if (pDlg && pDlg->GetScriptTargetObject() == pObj) CloseTypeIn();
}

void C4MessageInput::AbortMsgBoardQuery(C4Object *pObj, int32_t iPlr) {
  // close typein if it is used for the given parameters
  C4ChatInputDialog *pDlg = GetTypeIn();
  if (pDlg && pDlg->IsScriptQueried() &&
      pDlg->GetScriptTargetObject() == pObj &&
      pDlg->GetScriptTargetPlayer() == iPlr)
    CloseTypeIn();
}

void C4MessageInput::StoreBackBuffer(const char *szMessage) {
  if (!szMessage || !szMessage[0]) return;
  int32_t i, cnt;
  // Check: Remove doubled buffer
  for (i = 0; i < C4MSGB_BackBufferMax - 1; ++i)
    if (SEqual(BackBuffer[i], szMessage)) break;
  // Move up buffers
  for (cnt = i; cnt > 0; cnt--) SCopy(BackBuffer[cnt - 1], BackBuffer[cnt]);
  // Add message
  SCopy(szMessage, BackBuffer[0], C4MaxMessage);
}

const char *C4MessageInput::GetBackBuffer(int32_t iIndex) {
  if (!Inside<int32_t>(iIndex, 0, C4MSGB_BackBufferMax - 1)) return NULL;
  return BackBuffer[iIndex];
}

C4MessageBoardCommand::C4MessageBoardCommand() {
  Name[0] = '\0';
  Script[0] = '\0';
  Next = NULL;
}

void C4MessageBoardQuery::CompileFunc(StdCompiler *pComp) {
  // note that this CompileFunc does not save the fAnswered-flag, so pending
  // message board queries will be re-asked when resuming SaveGames
  pComp->Seperator(StdCompiler::SEP_START);  // '('
  // callback object number
  pComp->Value(nCallbackObj);
  pComp->Seperator();
  // input query string
  pComp->Value(sInputQuery);
  pComp->Seperator();
  // options
  pComp->Value(fIsUppercase);
  // list end
  pComp->Seperator(StdCompiler::SEP_END);  // ')'
}
