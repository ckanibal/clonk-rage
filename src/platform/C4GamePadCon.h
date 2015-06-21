/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Gamepad control - forwards gamepad events of opened gamepads to
 * Game.KeyboardInput */

#ifndef INC_C4GamePadCon
#define INC_C4GamePadCon

#ifdef _WIN32
#include <StdJoystick.h>
#endif

#ifdef HAVE_SDL
#include "gui/C4KeyboardInput.h"
#include <set>
#endif

struct _SDL_Joystick;
typedef struct _SDL_Joystick SDL_Joystick;

union SDL_Event;
typedef union SDL_Event SDL_Event;

class C4GamePadControl {
#ifdef _WIN32
 private:
  struct Pad {
    CStdGamePad *pGamepad;
    int iRefCount;
    uint32_t Buttons;
    CStdGamePad::AxisPos AxisPosis[CStdGamepad_MaxAxis];
  };
  Pad Gamepads[CStdGamepad_MaxGamePad];
  int iNumGamepads;

 public:
  void OpenGamepad(int id);            // add gamepad ref
  void CloseGamepad(int id);           // del gamepad ref
  static C4GamePadControl *pInstance;  // singleton
#elif defined HAVE_SDL
 public:
  void FeedEvent(SDL_Event& e);

 private:
  std::set<C4KeyCode> PressedAxis;
#endif
 public:
  C4GamePadControl();
  ~C4GamePadControl();
  void Clear();
  int GetGamePadCount();
  void Execute();
  static bool AnyButtonDown();
};

class C4GamePadOpener {
#ifdef _WIN32
  int iGamePad;
#endif
 public:
  C4GamePadOpener(int iGamePad);
  ~C4GamePadOpener();
  void SetGamePad(int iNewGamePad);
#ifdef HAVE_SDL
  SDL_Joystick *Joy;
#endif
};

#endif
