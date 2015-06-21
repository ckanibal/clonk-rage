/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Main class to execute the game fullscreen mode */

#ifndef INC_C4FullScreen
#define INC_C4FullScreen

#include "gui/C4MainMenu.h"
#include <StdWindow.h>

#ifndef BIG_C4INCLUDE
#include "game/C4Game.h"
#endif

bool IsKeyDown(int iKey);

class C4FullScreen : public CStdWindow {
 public:
  C4MainMenu *pMenu;

 public:
  C4FullScreen();
  ~C4FullScreen();
  void Execute(bool fRecursiveCall);
  BOOL Restart();
  BOOL ViewportCheck();
  BOOL OpenGame(const char *szCmdLine);
  void Sec1Timer() {
    Game.Sec1Timer();
    Application.DoSec1Timers();
  }
  bool ShowAbortDlg();  // show game abort dialog (Escape pressed)
  bool ActivateMenuMain();
  BOOL MenuCommand(const char *szCommand);
  void CloseMenu();
  bool MenuKeyControl(BYTE byCom);  // direct keyboard callback
  // User requests close
  virtual void Close();
  virtual void CharIn(const char *c);
#ifdef USE_X11
  virtual void HandleMessage(XEvent &e);
#elif USE_SDL_MAINLOOP
  virtual void HandleMessage(SDL_Event &e);
#endif
};

extern C4FullScreen FullScreen;

#endif
