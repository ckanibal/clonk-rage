/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Main class to initialize configuration and execute the game */

#include "C4Include.h"
#include "game/C4Application.h"
#include "C4Version.h"
#ifdef _WIN32
#include <StdRegistry.h>
#include "gui/C4UpdateDlg.h"
#endif

#ifndef BIG_C4INCLUDE
#include "C4FileClasses.h"
#include "game/C4FullScreen.h"
#include "c4group/C4Language.h"
#include "C4Console.h"
#include "gui/C4Startup.h"
#include "C4Log.h"
#include "platform/C4GamePadCon.h"
#include "gui/C4GameLobby.h"
#endif

#include <StdRegistry.h>  // For DDraw emulation warning

C4Sec1TimerCallbackBase::C4Sec1TimerCallbackBase() : pNext(NULL), iRefs(2) {
  // register into engine callback stack
  pNext = Application.pSec1TimerCallback;
  Application.pSec1TimerCallback = this;
}

C4Application::C4Application()
    : isFullScreen(true),
      UseStartupDialog(true),
      launchEditor(false),
      restartAtEnd(false),
      DDraw(NULL),
      AppState(C4AS_None),
      pSec1TimerCallback(NULL),
      iLastGameTick(0),
      iGameTickDelay(28),
      iExtraGameTickDelay(0),
      pGamePadControl(NULL),
      CheckForUpdates(false),
      NoSplash(false) {}

C4Application::~C4Application() {
  // clear gamepad
  if (pGamePadControl) delete pGamePadControl;
  // Close log
  CloseLog();
  // Launch editor
  if (launchEditor) {
#ifdef _WIN32
    char strCommandLine[_MAX_PATH + 1];
    SCopy(Config.AtExePath(C4CFN_Editor), strCommandLine);
    STARTUPINFO StartupInfo;
    ZeroMemory(&StartupInfo, sizeof StartupInfo);
    StartupInfo.cb = sizeof StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    ZeroMemory(&ProcessInfo, sizeof ProcessInfo);
    CreateProcess(NULL, strCommandLine, NULL, NULL, TRUE, 0, NULL, NULL,
                  &StartupInfo, &ProcessInfo);
#endif
  }
}

bool C4Application::DoInit() {
  assert(AppState == C4AS_None);
  // Config overwrite by parameter
  StdStrBuf sConfigFilename;
  char szParameter[_MAX_PATH + 1];
  for (int32_t iPar = 0;
       SGetParameter(GetCommandLine(), iPar, szParameter, _MAX_PATH); iPar++)
    if (SEqual2NoCase(szParameter, "/config:"))
      sConfigFilename.Copy(szParameter + 8);
  // Config check
  Config.Init();
  Config.Load(true, sConfigFilename.getData());
  Config.Save();
  // sometimes, the configuration can become corrupted due to loading errors or
  // w/e
  // check this and reset defaults if necessary
  if (Config.IsCorrupted()) {
    if (sConfigFilename) {
      // custom config corrupted: Fail
      Log("Warning: Custom configuration corrupted - program abort!\n");
      return false;
    } else {
      // default config corrupted: Restore default
      Log("Warning: Configuration corrupted - restoring default!\n");
      Config.Default();
      Config.Save();
      Config.Load();
    }
  }
  MMTimer = Config.General.MMTimer != 0;
  // Init C4Group
  C4Group_SetMaker(Config.General.Name);
  C4Group_SetProcessCallback(&ProcessCallback);
  C4Group_SetTempPath(Config.General.TempPath);
  C4Group_SetSortList(C4CFN_FLS);

  // Open log
  if (!OpenLog()) return false;

  // init system group
  if (!SystemGroup.Open(C4CFN_System)) {
    // Error opening system group - no LogFatal, because it needs language
    // table.
    // This will *not* use the FatalErrors stack, but this will cause the game
    // to instantly halt, anyway.
    Log("Error opening system group file (System.c4g)!");
    return false;
  }

  // Language override by parameter
  const char *pLanguage;
  if (pLanguage = SSearchNoCase(GetCommandLine(), "/Language:"))
    SCopyUntil(pLanguage, Config.General.LanguageEx, ' ', CFG_MaxString);

  // Init external language packs
  Languages.Init();
  // Load language string table
  if (!Languages.LoadLanguage(Config.General.LanguageEx))
    // No language table was loaded - bad luck...
    if (!IsResStrTableLoaded())
      Log("WARNING: No language string table loaded!");

  // Set unregistered user name
  C4Group_SetMaker(LoadResStr("IDS_PRC_UNREGUSER"));

  // Parse command line
  Game.ParseCommandLine(GetCommandLine());

#ifdef WIN32
  // Windows: handle incoming updates directly, even before starting up the gui
  //          because updates will be applied in the console anyway.
  if (Application.IncomingUpdate)
    if (C4UpdateDlg::ApplyUpdate(Application.IncomingUpdate.getData(), false,
                                 NULL))
      return true;
#endif

  // activate
  Active = TRUE;

  // Init carrier window
  if (isFullScreen) {
    if (!(pWindow = FullScreen.Init(this))) {
      Clear();
      return false;
    }
  } else {
    if (!(pWindow = Console.Init(this))) {
      Clear();
      return false;
    }
  }

  // init timers (needs window)
  if (!InitTimer()) {
    LogFatal(LoadResStr("IDS_ERR_TIMER"));
    Clear();
    return false;
  }

  // Engine header message
  Log(C4ENGINEINFOLONG);
  LogF("Version: %s %s", C4VERSION, C4_OS);

#if defined(USE_DIRECTX) && defined(_WIN32)
  // DDraw emulation warning
  DWORD DDrawEmulationState;
  if (GetRegistryDWord(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DirectDraw",
                       "EmulationOnly", &DDrawEmulationState))
    if (DDrawEmulationState)
      Log("WARNING: DDraw Software emulation is activated!");
#endif
  // Initialize D3D/OpenGL
  DDraw = DDrawInit(this, isFullScreen, FALSE, Config.Graphics.BitDepth,
                    Config.Graphics.Engine, Config.Graphics.Monitor);
  if (!DDraw) {
    LogFatal(LoadResStr("IDS_ERR_DDRAW"));
    Clear();
    return false;
  }

#if defined(_WIN32) && !defined(USE_CONSOLE)
  // Register clonk file classes - notice: under Vista this will only work if we
  // have administrator rights
  char szModule[_MAX_PATH + 1];
  GetModuleFileName(NULL, szModule, _MAX_PATH);
  SetC4FileClasses(szModule);
#endif

  // Initialize gamepad
  if (!pGamePadControl && Config.General.GamepadEnabled)
    pGamePadControl = new C4GamePadControl();

  AppState = C4AS_PreInit;

  return true;
}

bool C4Application::PreInit() {
  if (!Game.PreInit()) return false;

  // startup dialog: Only use if no next mission has been provided
  bool fDoUseStartupDialog = UseStartupDialog && !*Game.ScenarioFilename;

  // init loader: Black screen for first start if a video is to be shown;
  // otherwise default spec
  if (fDoUseStartupDialog) {
    // Log(LoadResStr("IDS_PRC_INITLOADER"));
    bool fUseBlackScreenLoader =
        UseStartupDialog && !C4Startup::WasFirstRun() &&
        !Config.Startup.NoSplash && !NoSplash && FileExists(C4CFN_Splash);
    if (!Game.GraphicsSystem.InitLoaderScreen(C4CFN_StartupBackgroundMain,
                                              fUseBlackScreenLoader)) {
      LogFatal(LoadResStr("IDS_PRC_ERRLOADER"));
      return false;
    }
  }

  Game.SetInitProgress(fDoUseStartupDialog ? 10.0f : 1.0f);

  // Music
  if (!MusicSystem.Init("Frontend.*")) Log(LoadResStr("IDS_PRC_NOMUSIC"));

  Game.SetInitProgress(fDoUseStartupDialog ? 20.0f : 2.0f);

  // Sound
  if (!SoundSystem.Init()) Log(LoadResStr("IDS_PRC_NOSND"));

  Game.SetInitProgress(fDoUseStartupDialog ? 30.0f : 3.0f);

  AppState = fDoUseStartupDialog ? C4AS_Startup : C4AS_StartGame;

  return true;
}

BOOL C4Application::ProcessCallback(const char *szMessage, int iProcess) {
  Console.Out(szMessage);
  return TRUE;
}

void C4Application::Clear() {
  Game.Clear();
  NextMission.Clear();
  // close system group (System.c4g)
  SystemGroup.Close();
  // Close timers
  C4Sec1TimerCallbackBase *pSec1Timer, *pNextSec1Timer = pSec1TimerCallback;
  pSec1TimerCallback = NULL;
  while (pSec1Timer = pNextSec1Timer) {
    pNextSec1Timer = pSec1Timer->pNext;
    delete pSec1Timer;
  }
  // Log
  if (IsResStrTableLoaded())  // Avoid (double and undefined) message on
                              // (second?) shutdown...
    Log(LoadResStr("IDS_PRC_DEINIT"));
  // Clear external language packs and string table
  Languages.Clear();
  Languages.ClearLanguage();
  // gamepad clear
  if (pGamePadControl) {
    delete pGamePadControl;
    pGamePadControl = NULL;
  }
  // music system clear
  MusicSystem.Clear();
  SoundSystem.Clear();
  // Clear direct draw (late, because it's needed for e.g. Log)
  if (DDraw) {
    delete DDraw;
    DDraw = NULL;
  }
  // Close window
  FullScreen.Clear();
  Console.Clear();
  // The very final stuff
  CStdApp::Clear();
}

bool C4Application::OpenGame() {
  if (isFullScreen) {
    // Open game
    return Game.Init();
  } else {
    // Check registration
    /*if ( !Config.Registered() )	FREEWARE
            { LogFatal(LoadResStr("IDS_CNS_REGONLY")); Clear(); return FALSE; }
       */
    // Execute command line
    if (Game.ScenarioFilename[0] || Game.DirectJoinAddress[0])
      return Console.OpenGame(szCmdLine);
  }
  // done; success
  return true;
}

void C4Application::Quit() {
  // Clear definitions passed by frontend for this round
  Config.General.Definitions[0] = 0;
  // Participants should not be cleared for usual startup dialog
  // Config.General.Participants[0] = 0;
  // Save config if there was no loading error
  if (Config.fConfigLoaded) Config.Save();
  // quit app
  CStdApp::Quit();
  AppState = C4AS_Quit;
}

void C4Application::QuitGame() {
  // reinit desired? Do restart
  if (UseStartupDialog || NextMission) {
    // backup last start params
    bool fWasNetworkActive = Game.NetworkActive;
    // stop game
    Game.Clear();
    Game.Default();
    AppState = C4AS_PreInit;
    // if a next mission is desired, set to start it
    if (NextMission) {
      SCopy(NextMission.getData(), Game.ScenarioFilename, _MAX_PATH);
      SReplaceChar(
          Game.ScenarioFilename, '\\',
          DirSep[0]);  // linux/mac: make sure we are using forward slashes
#ifdef NETWORK
      Game.fLobby = Game.NetworkActive = fWasNetworkActive;
#endif
      Game.fObserve = false;
      Game.Record = !!Config.General.Record;
      NextMission.Clear();
    }
  } else {
    Quit();
  }
}

void C4Application::Execute() {
  CStdApp::Execute();
  // Recursive execution check
  static int32_t iRecursionCount = 0;
  ++iRecursionCount;
  // Exec depending on game state
  assert(AppState != C4AS_None);
  switch (AppState) {
    case C4AS_Quit:
      // Do nothing, HandleMessage will return HR_Failure soon
      break;
    case C4AS_PreInit:
      if (!PreInit()) Quit();
      break;
    case C4AS_Startup:
#ifdef USE_CONSOLE
// Console engines just stay in this state until aborted or new commands arrive
// on stdin
#else
      AppState = C4AS_Game;
      // if no scenario or direct join has been specified, get game startup
      // parameters by startup dialog
      Game.ScenarioTitle.Copy(LoadResStr("IDS_PRC_INITIALIZE"));
      if (!C4Startup::Execute()) {
        Quit();
        --iRecursionCount;
        return;
      }
      AppState = C4AS_StartGame;
#endif
      break;
    case C4AS_StartGame:
      // immediate progress to next state; OpenGame will enter
      // HandleMessage-loops
      // in startup and lobby!
      AppState = C4AS_Game;
      // first-time game initialization
      if (!OpenGame()) {
        // set error flag (unless this was a lobby user abort)
        if (!C4GameLobby::UserAbort) Game.fQuitWithError = true;
        // no start: Regular QuitGame; this may reset the engine to startup mode
        // if desired
        QuitGame();
      }
      break;
    case C4AS_Game: {
      uint32_t iThisGameTick = timeGetTime();
      // Game (do additional timing check)
      if (Game.IsRunning && iRecursionCount <= 1)
        if (Game.GameGo || !iExtraGameTickDelay ||
            (iThisGameTick > iLastGameTick + iExtraGameTickDelay)) {
          // Execute
          Game.Execute();
          // Save back time
          iLastGameTick = iThisGameTick;
        }
      // Graphics
      if (!Game.DoSkipFrame) {
        uint32_t iPreGfxTime = timeGetTime();
        // Fullscreen mode
        if (isFullScreen) FullScreen.Execute(iRecursionCount > 1);
        // Console mode
        else
          Console.Execute();
        // Automatic frame skip if graphics are slowing down the game (skip max.
        // every 2nd frame)
        Game.DoSkipFrame = Game.Parameters.AutoFrameSkip &&
                           ((iPreGfxTime + iGameTickDelay) < timeGetTime());
      } else
        Game.DoSkipFrame = false;
      // Sound
      SoundSystem.Execute();
      // Gamepad
      if (pGamePadControl) pGamePadControl->Execute();
      break;
    }
  }

  --iRecursionCount;
}

void C4Application::OnNetworkEvents() { InteractiveThread.ProcessEvents(); }

void C4Application::DoSec1Timers() {
  C4Sec1TimerCallbackBase *pCBNext, *pCB, **ppCBCurr = &pSec1TimerCallback;
  while ((pCB = pCBNext = *ppCBCurr)) {
    pCBNext = pCB->pNext;
    if (pCB->IsReleased()) {
      delete pCB;
      *ppCBCurr = pCBNext;
    } else {
      pCB->OnSec1Timer();
      ppCBCurr = &(pCB->pNext);
    }
  }
}

void C4Application::SetGameTickDelay(int iDelay) {
  // Remember delay
  iGameTickDelay = iDelay;
  // Smaller than minimum refresh delay?
  if (iDelay < Config.Graphics.MaxRefreshDelay) {
    // Set critical timer
    ResetTimer(iDelay);
    // No additional breaking needed
    iExtraGameTickDelay = 0;
  } else {
    // Do some magic to get as near as possible to the requested delay
    int iGraphDelay = Max(1, iDelay);
    iGraphDelay /= (iGraphDelay + Config.Graphics.MaxRefreshDelay - 1) /
                   Config.Graphics.MaxRefreshDelay;
    // Set critical timer
    ResetTimer(iGraphDelay);
    // Slow down game tick
    iExtraGameTickDelay = iDelay - iGraphDelay / 2;
  }
}

bool C4Application::SetResolution(int32_t iNewResX, int32_t iNewResY) {
  // set in config
  int32_t iOldResX = Config.Graphics.ResX, iOldResY = Config.Graphics.ResY;
  Config.Graphics.ResX = iNewResX;
  Config.Graphics.ResY = iNewResY;
  // ask graphics system to change it
  if (lpDDraw) {
    if (!lpDDraw->OnResolutionChanged()) {
      // gfx system could not reinit: Failure
      Config.Graphics.ResX = iOldResX;
      Config.Graphics.ResY = iOldResY;
      return false;
    }
  }
  // resolution change success!
  // notify game
  Game.OnResolutionChanged();
  return true;
}

bool C4Application::SetGameFont(const char *szFontFace, int32_t iFontSize) {
#ifndef USE_CONSOLE
  // safety
  if (!szFontFace || !*szFontFace || iFontSize < 1 ||
      SLen(szFontFace) >= static_cast<int>(sizeof Config.General.RXFontName))
    return false;
  // first, check if the selected font can be created at all
  // check regular font only - there's no reason why the other fonts couldn't be
  // created
  CStdFont TestFont;
  if (!Game.FontLoader.InitFont(TestFont, szFontFace, C4FontLoader::C4FT_Main,
                                iFontSize, &Game.GraphicsResource.Files))
    return false;
  // OK; reinit all fonts
  StdStrBuf sOldFont;
  sOldFont.Copy(Config.General.RXFontName);
  int32_t iOldFontSize = Config.General.RXFontSize;
  SCopy(szFontFace, Config.General.RXFontName);
  Config.General.RXFontSize = iFontSize;
  if (!Game.GraphicsResource.InitFonts() ||
      !C4Startup::Get()->Graphics.InitFonts()) {
    // failed :o
    // shouldn't happen. Better restore config.
    SCopy(sOldFont.getData(), Config.General.RXFontName);
    Config.General.RXFontSize = iOldFontSize;
    return false;
  }
#endif
  // save changes
  return true;
}

void C4Application::OnCommand(const char *szCmd) {
  // Find parameters
  const char *szPar = strchr(szCmd, ' ');
  while (szPar && *szPar == ' ') szPar++;

  if (SEqual(szCmd, "/quit")) {
    Quit();
    return;
  }

  switch (AppState) {
    case C4AS_Startup:

      // Open a new game
      if (SEqual2(szCmd, "/open ")) {
        // Try to start the game with given parameters
        AppState = C4AS_Game;
        Game.ParseCommandLine(szPar);
        UseStartupDialog = true;
        if (!Game.Init()) AppState = C4AS_Startup;
      }

      break;

    case C4AS_Game:

      // Clear running game
      if (SEqual(szCmd, "/close")) {
        Game.Clear();
        Game.Default();
        AppState = C4AS_PreInit;
        UseStartupDialog = true;
        return;
      }

      // Lobby commands
      if (Game.Network.isLobbyActive()) {
        if (SEqual2(szCmd, "/start")) {
          // timeout given?
          int32_t iTimeout = Config.Lobby.CountdownTime;
          if (!Game.Network.isHost())
            Log(LoadResStr("IDS_MSG_CMD_HOSTONLY"));
          else if (szPar && (!sscanf(szPar, "%d", &iTimeout) || iTimeout < 0))
            Log(LoadResStr("IDS_MSG_CMD_START_USAGE"));
          else
            // start new countdown (aborts previous if necessary)
            Game.Network.StartLobbyCountdown(iTimeout);
          break;
        }
      }

      // Normal commands
      Game.MessageInput.ProcessInput(szCmd);
      break;
  }
}

void C4Application::Activate() {
#ifdef WIN32
  // Activate the application to regain focus if it has been lost during
  // loading.
  // As this is officially not possible any more in new versions of Windows
  // (BringWindowTopTop alone won't have any effect if the calling process is
  // not in the foreground itself), we are using an ugly OS hack.
  DWORD nForeThread = GetWindowThreadProcessId(GetForegroundWindow(), 0);
  DWORD nAppThread = GetCurrentThreadId();
  if (nForeThread != nAppThread) {
    AttachThreadInput(nForeThread, nAppThread, TRUE);
    BringWindowToTop(FullScreen.hWindow);
    ShowWindow(FullScreen.hWindow, SW_SHOW);
    AttachThreadInput(nForeThread, nAppThread, FALSE);
  } else {
    BringWindowToTop(FullScreen.hWindow);
    ShowWindow(FullScreen.hWindow, SW_SHOW);
  }
#endif
}

void C4Application::SetNextMission(const char *szMissionFilename) {
  // set next mission if any is desired
  if (szMissionFilename)
    NextMission.Copy(szMissionFilename);
  else
    NextMission.Clear();
}
