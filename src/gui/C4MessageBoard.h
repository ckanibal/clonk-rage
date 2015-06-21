/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Fullscreen startup log and chat type-in */

#ifndef INC_C4MessageBoard
#define INC_C4MessageBoard

const int C4MSGB_MaxMsgFading = 6;

#include "graphics/C4Facet.h"
#include "C4LogBuf.h"
#include "player/C4Player.h"

class C4MessageBoard {
 public:
  C4MessageBoard();
  ~C4MessageBoard();

 public:
  C4Facet Output;

  BOOL Active;

 protected:
  int ScreenFader;
  BOOL Startup;
  int iMode;  // 0 = one line (std), 1 = > 1 lines, 2 = invisible
  // mode 0:
  int Delay;   // how long the curr msg will stay
  int Fader;   // =0: hold curr msg until Delay == 0
               // <0: fade curr msg out
               // >0: fade curr msg in
  int Speed;   // fade/delay speed
  bool Empty;  // msgboard empty?
  // mode 1:
  int iLines;       // how many lines are visible? (+ one line for typin! )
  int iBackScroll;  // how many lines scrolled back?
  // all modes:
  int iLineHgt;  // line height

  C4LogBuffer LogBuffer;  // backbuffer for log
 public:
  void Default();
  void Clear();
  void Init(C4Facet &cgo, BOOL fStartup);
  void Execute();
  void DrawLoader(C4Facet &cgo);
  void Draw(C4Facet &cgo);
  void AddLog(const char *szMessage);
  void ClearLog();
  void LogNotify();
  void EnsureLastMessage();
  bool ControlScrollUp();
  bool ControlScrollDown();
  bool ControlChangeMode();
  C4Player *GetMessagePlayer(const char *szMessage);
  void ChangeMode(int inMode);

  friend class C4MessageInput;
};

#endif
