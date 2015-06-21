/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Text messages drawn inside the viewport */

#include "C4Include.h"
#include "gui/C4GameMessage.h"

#ifndef BIG_C4INCLUDE
#include "object/C4Object.h"
#include "C4FogOfWar.h"
#include "game/C4Application.h"
#include "game/C4Game.h"
#include "player/C4Player.h"
#endif

const int32_t TextMsgDelayFactor = 2;  // frames per char message display time

C4GameMessage::C4GameMessage() : pFrameDeco(NULL) {}

C4GameMessage::~C4GameMessage() { delete pFrameDeco; }

void C4GameMessage::Init(int32_t iType, const StdStrBuf &sText,
                         C4Object *pTarget, int32_t iPlayer, int32_t iX,
                         int32_t iY, uint32_t dwClr, C4ID idDecoID,
                         const char *szPortraitDef, uint32_t dwFlags,
                         int width) {
  // safety!
  if (pTarget && !pTarget->Status) pTarget = NULL;
  // Set data
  Text.Copy(sText);
  Target = pTarget;
  X = iX;
  Y = iY;
  Wdt = width;
  Player = iPlayer;
  ColorDw = dwClr;
  Type = iType;
  Delay = Max<int32_t>(C4GM_MinDelay, Text.getLength() * TextMsgDelayFactor);
  DecoID = idDecoID;
  this->dwFlags = dwFlags;
  if (szPortraitDef && *szPortraitDef)
    PortraitDef.Copy(szPortraitDef);
  else
    PortraitDef.Clear();
  // Permanent message
  if ('@' == Text[0]) {
    Delay = -1;
    Text.Move(1, Text.getLength());
    Text.Shrink(1);
  }
  // frame decoration
  delete pFrameDeco;
  pFrameDeco = NULL;
  if (DecoID) {
    pFrameDeco = new C4GUI::FrameDecoration();
    if (!pFrameDeco->SetByDef(DecoID)) {
      delete pFrameDeco;
      pFrameDeco = NULL;
    }
  }
}

void C4GameMessage::Append(const char *szText, BOOL fNoDuplicates) {
  // Check for duplicates
  if (fNoDuplicates)
    for (const char *pPos = Text.getData(); *pPos;
         pPos = SAdvancePast(pPos, '|'))
      if (SEqual2(pPos, szText)) return;
  // Append new line
  Text.AppendFormat("|%s", szText);
  Delay += SLen(szText) * TextMsgDelayFactor;
}

BOOL C4GameMessage::Execute() {
  // Position by target
  // currently done in C4GameMessage::Draw for parallaxity
  /*if (Target)
          { X=Target->x; Y=Target->y-Target->Def->Shape.Hgt/2-5; }*/
  // Delay / removal
  if (Delay > 0) Delay--;
  if (Delay == 0) return FALSE;
  // Done
  return TRUE;
}

int32_t DrawMessageOffset = -35;  // For finding the optimum place to draw
                                  // startup info & player messages...
int32_t PortraitWidth = 64;
int32_t PortraitIndent = 10;

void C4GameMessage::Draw(C4FacetEx &cgo, int32_t iPlayer) {
  // Globals or player
  if (Type == C4GM_Global ||
      ((Type == C4GM_GlobalPlayer) && (iPlayer == Player))) {
    int32_t iTextWdt, iTextHgt;
    StdStrBuf sText;
    int32_t x, y, wdt;
    if (dwFlags & C4GM_XRel)
      x = X * cgo.Wdt / 100;
    else
      x = X;
    if (dwFlags & C4GM_YRel)
      y = Y * cgo.Hgt / 100;
    else
      y = Y;
    if (dwFlags & C4GM_WidthRel)
      wdt = Wdt * cgo.Wdt / 100;
    else
      wdt = Wdt;
    if (~dwFlags & C4GM_NoBreak) {
      // Word wrap to cgo width
      if (PortraitDef) {
        if (!wdt)
          wdt = BoundBy<int32_t>(cgo.Wdt / 2, 50,
                                 Min<int32_t>(500, cgo.Wdt - 10));
        int32_t iUnbrokenTextWidth =
            Game.GraphicsResource.FontRegular.GetTextWidth(Text.getData(),
                                                           true);
        wdt = Min<int32_t>(wdt, iUnbrokenTextWidth + 10);
      } else {
        if (!wdt)
          wdt = BoundBy<int32_t>(cgo.Wdt - 50, 50, 500);
        else
          wdt = BoundBy<int32_t>(wdt, 10, cgo.Wdt - 10);
      }
      iTextWdt = wdt;
      iTextHgt = Game.GraphicsResource.FontRegular.BreakMessage(
          Text.getData(), iTextWdt, &sText, true);
    } else {
      Game.GraphicsResource.FontRegular.GetTextExtent(Text.getData(), iTextWdt,
                                                      iTextHgt, true);
      sText.Ref(Text);
    }
    int32_t iDrawX = cgo.X + x;
    int32_t iDrawY = cgo.Y + y;
    // draw message
    if (PortraitDef) {
      // message with portrait
      // bottom-placed portrait message: Y-Positioning 0 refers to bottom of
      // viewport
      if (dwFlags & C4GM_Bottom)
        iDrawY += cgo.Hgt;
      else if (dwFlags & C4GM_VCenter)
        iDrawY += cgo.Hgt / 2;
      if (dwFlags & C4GM_Right)
        iDrawX += cgo.Wdt;
      else if (dwFlags & C4GM_HCenter)
        iDrawX += cgo.Wdt / 2;
      // draw decoration
      if (pFrameDeco) {
        C4Rect rect(iDrawX - cgo.TargetX, iDrawY - cgo.TargetY,
                    iTextWdt + PortraitWidth + PortraitIndent +
                        pFrameDeco->iBorderLeft + pFrameDeco->iBorderRight,
                    Max(iTextHgt, PortraitWidth) + pFrameDeco->iBorderTop +
                        pFrameDeco->iBorderBottom);
        if (dwFlags & C4GM_Bottom) {
          rect.y -= rect.Hgt;
          iDrawY -= rect.Hgt;
        } else if (dwFlags & C4GM_VCenter) {
          rect.y -= rect.Hgt / 2;
          iDrawY -= rect.Hgt / 2;
        }
        if (dwFlags & C4GM_Right) {
          rect.x -= rect.Wdt;
          iDrawX -= rect.Wdt;
        } else if (dwFlags & C4GM_HCenter) {
          rect.x -= rect.Wdt / 2;
          iDrawX -= rect.Wdt / 2;
        }
        pFrameDeco->Draw(cgo, rect);
        iDrawX += pFrameDeco->iBorderLeft;
        iDrawY += pFrameDeco->iBorderTop;
      } else
        iDrawY -= iTextHgt;
      // draw portrait
      C4FacetExSurface fctPortrait;
      Game.DrawTextSpecImage(fctPortrait, PortraitDef.getData());
      C4Facet facet(cgo.Surface, iDrawX, iDrawY, PortraitWidth, PortraitWidth);
      fctPortrait.Draw(facet);
      // draw message
      Application.DDraw->TextOut(
          sText.getData(), Game.GraphicsResource.FontRegular, 1.0, cgo.Surface,
          iDrawX + PortraitWidth + PortraitIndent, iDrawY, ColorDw, ALeft);
    } else {
      // message without portrait
      iDrawX += cgo.Wdt / 2;
      iDrawY += 2 * cgo.Hgt / 3 + 50;
      if (!(dwFlags & C4GM_Bottom)) iDrawY += DrawMessageOffset;
      Application.DDraw->TextOut(sText.getData(),
                                 Game.GraphicsResource.FontRegular, 1.0,
                                 cgo.Surface, iDrawX, iDrawY, ColorDw, ACenter);
    }
  }
  // Positioned
  else if (Type == C4GM_Target ||
           ((Type == C4GM_TargetPlayer) && (iPlayer == Player))) {
    // adjust position by object; care about parallaxity
    int32_t iMsgX, iMsgY;
    if (Type == C4GM_Target || Type == C4GM_TargetPlayer) {
      Target->GetViewPos(iMsgX, iMsgY, cgo.TargetX, cgo.TargetY, cgo);
      iMsgY -= Target->Def->Shape.Hgt / 2 + 5;
      iMsgX += X;
      iMsgY += Y;
    } else {
      iMsgX = X;
      iMsgY = Y;
    }
    // check output bounds
    if (Inside(iMsgX - cgo.TargetX, 0, cgo.Wdt - 1))
      if (Inside(iMsgY - cgo.TargetY, 0, cgo.Hgt - 1)) {
        // if the message is attached to an object and the object
        // is invisible for that player, the message won't be drawn
        if (Type == C4GM_Target)
          if (!Target->IsVisible(iPlayer, false)) return;
        // check fog of war
        C4Player *pPlr = Game.Players.Get(iPlayer);
        if (pPlr && pPlr->fFogOfWar)
          if (!pPlr->FoWIsVisible(iMsgX, iMsgY)) {
            // special: Target objects that ignore FoW should display the
            // message even if within FoW
            if (Type != C4GM_Target && Type != C4GM_TargetPlayer) return;
            if (~Target->Category & C4D_IgnoreFoW) return;
          }
        // Word wrap to cgo width
        StdStrBuf sText;
        if (~dwFlags & C4GM_NoBreak)
          Game.GraphicsResource.FontRegular.BreakMessage(
              Text.getData(), BoundBy<int32_t>(cgo.Wdt, 50, 200), &sText, true);
        else
          sText.Ref(Text);
        // Adjust position by output boundaries
        int32_t iTX, iTY, iTWdt, iTHgt;
        Game.GraphicsResource.FontRegular.GetTextExtent(sText.getData(), iTWdt,
                                                        iTHgt, true);
        iTX = BoundBy<int32_t>(iMsgX - cgo.TargetX, iTWdt / 2,
                               cgo.Wdt - iTWdt / 2);
        iTY = BoundBy<int32_t>(iMsgY - cgo.TargetY - iTHgt, 0, cgo.Hgt - iTHgt);
        // Draw
        Application.DDraw->TextOut(
            sText.getData(), Game.GraphicsResource.FontRegular, 1.0,
            cgo.Surface, cgo.X + iTX, cgo.Y + iTY, ColorDw, ACenter);
      }
  }
}

void C4GameMessage::UpdateDef(C4ID idUpdDef) {
  // frame deco might be using updated/deleted def
  if (pFrameDeco) {
    if (!pFrameDeco->UpdateGfx()) {
      delete pFrameDeco;
      pFrameDeco = NULL;
    }
  }
}

C4GameMessageList::C4GameMessageList() { Default(); }

C4GameMessageList::~C4GameMessageList() { Clear(); }

void C4GameMessageList::Default() { First = NULL; }

void C4GameMessageList::ClearPointers(C4Object *pObj) {
  C4GameMessage *cmsg, *next, *prev = NULL;
  for (cmsg = First; cmsg; cmsg = next) {
    next = cmsg->Next;
    if (cmsg->Target == pObj) {
      delete cmsg;
      if (prev)
        prev->Next = next;
      else
        First = next;
    } else
      prev = cmsg;
  }
}

void C4GameMessageList::Clear() {
  C4GameMessage *cmsg, *next;
  for (cmsg = First; cmsg; cmsg = next) {
    next = cmsg->Next;
    delete cmsg;
  }
  First = NULL;
}

void C4GameMessageList::Execute() {
  C4GameMessage *cmsg, *next, *prev = NULL;
  for (cmsg = First; cmsg; cmsg = next) {
    next = cmsg->Next;
    if (!cmsg->Execute()) {
      delete cmsg;
      if (prev)
        prev->Next = next;
      else
        First = next;
    } else
      prev = cmsg;
  }
}

bool C4GameMessageList::New(int32_t iType, const char *szText,
                            C4Object *pTarget, int32_t iPlayer, int32_t iX,
                            int32_t iY, uint8_t bCol) {
  return New(iType, StdStrBuf(szText), pTarget, iPlayer, iX, iY,
             0xff000000 | Application.DDraw->Pal.GetClr(FColors[bCol]));
}

bool C4GameMessageList::New(int32_t iType, const char *szText,
                            C4Object *pTarget, int32_t iPlayer, int32_t iX,
                            int32_t iY, uint32_t dwClr, C4ID idDecoID,
                            const char *szPortraitDef, uint32_t dwFlags,
                            int32_t width) {
  return New(iType, StdStrBuf(szText), pTarget, iPlayer, iX, iY, dwClr,
             idDecoID, szPortraitDef, dwFlags, width);
}

bool C4GameMessageList::New(int32_t iType, const StdStrBuf &sText,
                            C4Object *pTarget, int32_t iPlayer, int32_t iX,
                            int32_t iY, uint32_t dwClr, C4ID idDecoID,
                            const char *szPortraitDef, uint32_t dwFlags,
                            int32_t width) {
  if (!(dwFlags & C4GM_Multiple)) {
    // Clear messages with same target
    if (pTarget) ClearPointers(pTarget);

    // Clear other player messages
    if (iType == C4GM_Global || iType == C4GM_GlobalPlayer)
      ClearPlayers(iPlayer, dwFlags & C4GM_PositioningFlags);
  }

  // Object deleted?
  if (pTarget && !pTarget->Status) return false;

  // Empty message? (only deleting old message)
  if (!sText.getLength()) return TRUE;

  // Add new message
  C4GameMessage *msgNew = new C4GameMessage;
  msgNew->Init(iType, sText, pTarget, iPlayer, iX, iY, dwClr, idDecoID,
               szPortraitDef, dwFlags, width);
  msgNew->Next = First;
  First = msgNew;

  return TRUE;
}

bool C4GameMessageList::Append(int32_t iType, const char *szText,
                               C4Object *pTarget, int32_t iPlayer, int32_t iX,
                               int32_t iY, uint8_t bCol, bool fNoDuplicates) {
  C4GameMessage *cmsg = NULL;
  if (iType == C4GM_Target) {
    for (cmsg = Game.Messages.First; cmsg; cmsg = cmsg->Next)
      if (pTarget == cmsg->Target) break;
  }
  if (iType == C4GM_Global || iType == C4GM_GlobalPlayer) {
    for (cmsg = Game.Messages.First; cmsg; cmsg = cmsg->Next)
      if (iPlayer == cmsg->Player) break;
  }
  if (!cmsg || pTarget != cmsg->Target) {
    New(iType, szText, pTarget, iPlayer, iX, iY, bCol);
  } else {
    cmsg->Append(szText, fNoDuplicates);
  }
  return TRUE;
}

void C4GameMessageList::ClearPlayers(int32_t iPlayer,
                                     int32_t dwPositioningFlags) {
  C4GameMessage *cmsg, *next, *prev = NULL;
  for (cmsg = First; cmsg; cmsg = next) {
    next = cmsg->Next;
    if (cmsg->Player == iPlayer &&
        cmsg->GetPositioningFlags() == dwPositioningFlags) {
      delete cmsg;
      if (prev)
        prev->Next = next;
      else
        First = next;
    } else
      prev = cmsg;
  }
}

void C4GameMessageList::UpdateDef(C4ID idUpdDef) {
  C4GameMessage *cmsg;
  for (cmsg = First; cmsg; cmsg = cmsg->Next) cmsg->UpdateDef(idUpdDef);
}

void C4GameMessageList::Draw(C4FacetEx &cgo, int32_t iPlayer) {
  C4GameMessage *cmsg;
  for (cmsg = First; cmsg; cmsg = cmsg->Next) cmsg->Draw(cgo, iPlayer);
}
