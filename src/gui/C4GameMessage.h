/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Text messages drawn inside the viewport */

#ifndef INC_C4GameMessage
#define INC_C4GameMessage

#include "graphics/C4Surface.h"
#include <gui/C4Gui.h>

const int32_t C4GM_MaxText = 256, C4GM_MinDelay = 20;

const int32_t C4GM_Global = 1, C4GM_GlobalPlayer = 2, C4GM_Target = 3,
              C4GM_TargetPlayer = 4;

const int32_t C4GM_NoBreak = 1 << 0,
              C4GM_Bottom = 1 << 1,  // message placed at bottom of screen
    C4GM_Multiple = 1 << 2, C4GM_Top = 1 << 3, C4GM_Left = 1 << 4,
              C4GM_Right = 1 << 5, C4GM_HCenter = 1 << 6, C4GM_VCenter = 1 << 7,
              C4GM_DropSpeech = 1 << 8,  // cut any text after '$'
    C4GM_WidthRel = 1 << 9, C4GM_XRel = 1 << 10, C4GM_YRel = 1 << 11;

const int32_t C4GM_PositioningFlags = C4GM_Bottom | C4GM_Top | C4GM_Left |
                                      C4GM_Right | C4GM_HCenter | C4GM_VCenter;

class C4GameMessage {
  friend class C4GameMessageList;

 public:
  void Draw(C4FacetEx &cgo, int32_t iPlayer);
  C4GameMessage();
  ~C4GameMessage();

 protected:
  int32_t X, Y, Wdt;
  int32_t Delay;
  DWORD ColorDw;
  int32_t Player;
  int32_t Type;
  C4Object *Target;
  StdCopyStrBuf Text;
  C4GameMessage *Next;
  C4ID DecoID;
  StdStrBuf PortraitDef;
  C4GUI::FrameDecoration *pFrameDeco;
  uint32_t dwFlags;

 protected:
  void Init(int32_t iType, const StdStrBuf &Text, C4Object *pTarget,
            int32_t iPlayer, int32_t iX, int32_t iY, uint32_t dwCol,
            C4ID idDecoID, const char *szPortraitDef, uint32_t dwFlags,
            int width);
  void Append(const char *szText, BOOL fNoDuplicates = FALSE);
  BOOL Execute();
  const char *WordWrap(int32_t iMaxWidth);
  void UpdateDef(C4ID idUpdDef);

 public:
  int32_t GetPositioningFlags() const {
    return dwFlags & C4GM_PositioningFlags;
  }
};

class C4GameMessageList {
 public:
  C4GameMessageList();
  ~C4GameMessageList();

 protected:
  C4GameMessage *First;

 public:
  void Default();
  void Clear();
  void Execute();
  void Draw(C4FacetEx &cgo, int32_t iPlayer);
  void ClearPlayers(int32_t iPlayer, int32_t dwPositioningFlags);
  void ClearPointers(C4Object *pObj);
  void UpdateDef(C4ID idUpdDef);  // called after reloaddef
  bool New(int32_t iType, const StdStrBuf &Text, C4Object *pTarget,
           int32_t iPlayer, int32_t iX = -1, int32_t iY = -1,
           uint32_t dwClr = 0xffFFFFFF, C4ID idDecoID = C4ID_None,
           const char *szPortraitDef = NULL, uint32_t dwFlags = 0u,
           int32_t width = 0);
  bool New(int32_t iType, const char *szText, C4Object *pTarget,
           int32_t iPlayer, int32_t iX, int32_t iY, uint8_t bCol);
  bool New(int32_t iType, const char *szText, C4Object *pTarget,
           int32_t iPlayer, int32_t iX, int32_t iY, uint32_t dwClr,
           C4ID idDecoID = C4ID_None, const char *szPortraitDef = NULL,
           uint32_t dwFlags = 0u, int32_t width = 0);
  bool Append(int32_t iType, const char *szText, C4Object *pTarget,
              int32_t iPlayer, int32_t iX, int32_t iY, uint8_t bCol,
              bool fNoDuplicates = false);
};

void GameMsgObject(const char *szText, C4Object *pTarget,
                   int32_t iFCol = FWhite);
void GameMsgObjectPlayer(const char *szText, C4Object *pTarget, int32_t iPlayer,
                         int32_t iFCol = FWhite);
void GameMsgGlobal(const char *szText, int32_t iFCol = FWhite);
void GameMsgPlayer(const char *szText, int32_t iPlayer, int32_t iFCol = FWhite);

void GameMsgObjectDw(const char *szText, C4Object *pTarget, uint32_t dwClr);

#endif
