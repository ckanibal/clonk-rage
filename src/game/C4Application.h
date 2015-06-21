/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

#ifndef INC_C4Application
#define INC_C4Application

#include "config/C4Config.h"
#include "c4group/C4Group.h"
#include "platform/C4MusicSystem.h"
#include "platform/C4SoundSystem.h"
#include "C4Components.h"
#include "network/C4InteractiveThread.h"
#include "network/C4Network2IRC.h"
#include "StdWindow.h"

#include "C4Include.h"

class CStdDDraw;

class C4Sec1Callback {
 public:
  virtual void OnSec1Timer() = 0;
  void StartSec1Timer();
  virtual ~C4Sec1Callback() {}
};

// callback interface for sec1timers
class C4Sec1TimerCallbackBase {
 protected:
  C4Sec1TimerCallbackBase *pNext;
  int iRefs;

 public:
  C4Sec1TimerCallbackBase();  // ctor - ref set to 2
  virtual ~C4Sec1TimerCallbackBase() {}
  void Release() {
    if (!--iRefs) delete this;
  }  // release: destruct in next cycle

 protected:
  virtual void OnSec1Timer() = 0;
  bool IsReleased() { return iRefs <= 1; }
  friend class C4Application;
};

template <class T>
class C4Sec1TimerCallback : public C4Sec1TimerCallbackBase {
 private:
  T *pCallback;

 protected:
  virtual void OnSec1Timer() { pCallback->OnSec1Timer(); }

 public:
  C4Sec1TimerCallback(T *pCB) : pCallback(pCB) {}
};

/* Main class to initialize configuration and execute the game */

class C4Application : public CStdApp {
 private:
  // if set, this mission will be launched next
  StdCopyStrBuf NextMission;

 public:
  C4Application();
  ~C4Application();
  // set by ParseCommandLine
  bool isFullScreen;
  // set by ParseCommandLine, if neither scenario nor direct join adress has
  // been specified
  bool UseStartupDialog;
  // set by ParseCommandLine, for installing registration keys
  StdStrBuf IncomingKeyfile;
  // set by ParseCommandLine, for manually applying downloaded update packs
  StdStrBuf IncomingUpdate;
  // set by ParseCommandLine, for manually invoking an update check by command
  // line or url
  bool CheckForUpdates;
  // set by ParseCommandLine, only pertains to this program start - independent
  // of Config.Startup.NoSplash
  bool NoSplash;
  // Flag for launching editor on quit
  bool launchEditor;
  // Flag for restarting the engine at the end
  bool restartAtEnd;
  // main System.c4g in working folder
  C4Group SystemGroup;
  C4MusicSystem MusicSystem;
  C4SoundSystem SoundSystem;
  C4GamePadControl *pGamePadControl;
  // Thread for interactive processes (automatically starts as needed)
  C4InteractiveThread InteractiveThread;
#ifdef NETWORK
  // IRC client for global chat
  C4Network2IRCClient IRCClient;
#endif
  // Tick timing
  unsigned int iLastGameTick, iGameTickDelay, iExtraGameTickDelay;
  class CStdDDraw *DDraw;
  virtual int32_t &ScreenWidth() { return Config.Graphics.ResX; }
  virtual int32_t &ScreenHeight() { return Config.Graphics.ResY; }
  void Clear();
  void Execute();
  // System.c4g helper funcs
  bool OpenSystemGroup() {
    return SystemGroup.IsOpen() || SystemGroup.Open(C4CFN_System);
  }
  void CloseSystemGroup() { SystemGroup.Close(); }
  void DoSec1Timers();
  void SetGameTickDelay(int iDelay);
  bool SetResolution(int32_t iNewResX, int32_t iNewResY);
  bool SetGameFont(const char *szFontFace, int32_t iFontSize);

 protected:
  enum State {
    C4AS_None,
    C4AS_PreInit,
    C4AS_Startup,
    C4AS_StartGame,
    C4AS_Game,
    C4AS_Quit,
  } AppState;

 protected:
  virtual bool DoInit();
  bool OpenGame();
  bool PreInit();
  virtual void OnNetworkEvents();
  static BOOL ProcessCallback(const char *szMessage, int iProcess);
  C4Sec1TimerCallbackBase *pSec1TimerCallback;
  friend class C4Sec1TimerCallbackBase;

  virtual void OnCommand(const char *szCmd);

 public:
  virtual void Quit();
  void QuitGame();  // quit game only, but restart application if in fullscreen
                    // startup menu mode
  void Activate();  // activate app to gain full focus in OS
  void SetNextMission(const char *szMissionFilename);
};

extern C4Application Application;

#endif
