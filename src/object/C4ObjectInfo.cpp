/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Holds crew member information */

#include "C4Include.h"
#include "object/C4ObjectInfo.h"

#ifndef BIG_C4INCLUDE
#include "C4Wrappers.h"
#include "C4Random.h"
#include "C4Components.h"
#include "game/C4Game.h"
#include "config/C4Config.h"
#include "game/C4Application.h"
#include "player/C4RankSystem.h"
#include "C4Log.h"
#include "player/C4Player.h"
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

C4ObjectInfo::C4ObjectInfo() { Default(); }

C4ObjectInfo::~C4ObjectInfo() { Clear(); }

void C4ObjectInfo::Default() {
  WasInAction = FALSE;
  InAction = FALSE;
  InActionTime = 0;
  HasDied = FALSE;
  ControlCount = 0;
  Filename[0] = 0;
  Next = NULL;
  pDef = NULL;
#ifdef C4ENGINE  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                 // - - - - -
  Portrait.Default();
  pNewPortrait = NULL;
  pCustomPortrait = NULL;
#endif  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}

BOOL C4ObjectInfo::Load(C4Group &hMother, const char *szEntryname,
                        bool fLoadPortrait) {
  // New version
  if (SEqualNoCase(GetExtension(szEntryname), "c4i")) {
    C4Group hChild;
    if (hChild.OpenAsChild(&hMother, szEntryname)) {
      if (!C4ObjectInfo::Load(hChild, fLoadPortrait)) {
        hChild.Close();
        return FALSE;
      }
// resolve definition, if possible
// only works in game, but is not needed in frontend or startup editing anyway
#ifdef C4ENGINE
      pDef = C4Id2Def(id);
#endif
      hChild.Close();
      return TRUE;
    }
  }

  return FALSE;
}

BOOL C4ObjectInfo::Load(C4Group &hGroup, bool fLoadPortrait) {
  // Store group file name
  SCopy(GetFilename(hGroup.GetName()), Filename, _MAX_FNAME);
  // Load core
  if (!C4ObjectInfoCore::Load(hGroup)) return FALSE;
#ifdef C4ENGINE  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                 // - - - - -
  // Load portrait - always try linking, even if fLoadPortrait is false (doesn't
  // cost mem anyway)
  // evaluate portrait string in info
  bool fPortraitFileChecked = false;
  if (*PortraitFile) {
    // custom portrait?
    if (SEqual(PortraitFile, C4Portrait_Custom)) {
      // try to load it
      if (pCustomPortrait) delete pCustomPortrait;
      pCustomPortrait = new C4Portrait();
      if (pCustomPortrait->Load(hGroup, C4CFN_Portrait_Old, C4CFN_Portrait,
                                C4CFN_PortraitOverlay)) {
        // link portrait to custom portrait
        Portrait.Link(pCustomPortrait->GetGfx());
      } else {
        // load failure: reset portrait info
        *PortraitFile = 0;
        delete pCustomPortrait;
        pCustomPortrait = NULL;
        // do not try to load a custom portrait again
        fPortraitFileChecked = true;
      }
    } else if (SEqual(PortraitFile, C4Portrait_None)) {
      // no portrait
      // nothing to be done here :D
    } else {
      // get portrait by info string
      const char *szPortraitName;
      C4ID idPortraitID;
      szPortraitName = C4Portrait::EvaluatePortraitString(
          PortraitFile, idPortraitID, id, NULL);
      // get portrait def
      C4Def *pPortraitDef = Game.Defs.ID2Def(idPortraitID);
      // def found?
      if (pPortraitDef && pPortraitDef->Portraits) {
        // find portrait by name
        C4DefGraphics *pDefPortraitGfx =
            pPortraitDef->Portraits->Get(szPortraitName);
        C4PortraitGraphics *pPortraitGfx =
            pDefPortraitGfx ? pDefPortraitGfx->IsPortrait() : NULL;
        // link if found
        if (pPortraitGfx)
          Portrait.Link(pPortraitGfx);
        else {
          // portrait not found? Either the specification has been deleted
          // (bad), or this is some scenario with custom portraits
          // assume the latter, and temp-assign a random portrait for the
          // duration of this round
          if (Config.Graphics.AddNewCrewPortraits)
            SetRandomPortrait(0, false, false);
        }
      }
    }
  }
  // portrait not defined or invalid (custom w/o file or invalid file)
  // assign a new one (local players only)
  if (!*PortraitFile && fLoadPortrait)
    // try to load a custom portrait
    if (!fPortraitFileChecked &&
        Portrait.Load(hGroup, C4CFN_Portrait_Old, C4CFN_Portrait,
                      C4CFN_PortraitOverlay))
      // assign it as custom portrait
      SCopy(C4Portrait_Custom, PortraitFile);
    else if (Config.Graphics.AddNewCrewPortraits)
      // assign a new random crew portrait
      SetRandomPortrait(0, true, false);
#endif  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // - - - -
  return TRUE;
}

BOOL C4ObjectInfo::Save(C4Group &hGroup, bool fStoreTiny, C4DefList *pDefs) {
  // Set group file name; rename if necessary
  char szTempGroup[_MAX_PATH + 1];
  SCopy(Name, szTempGroup, _MAX_PATH);
  MakeFilenameFromTitle(szTempGroup);
  SAppend(".c4i", szTempGroup, _MAX_PATH);
  if (!SEqualNoCase(Filename, szTempGroup)) {
    if (!Filename[0]) {
      // first time creation of file - make sure it's not a duplicate
      SCopy(szTempGroup, Filename, _MAX_PATH);
      while (hGroup.FindEntry(Filename)) {
        // if a crew info of that name exists already, rename!
        RemoveExtension(Filename);
        int32_t iFinNum = GetTrailingNumber(Filename), iLen = SLen(Filename);
        while (iLen && Inside(Filename[iLen - 1], '0', '9')) --iLen;
        if (iLen > _MAX_PATH - 22) {
          LogF("Error generating unique filename for %s(%s): Path overflow",
               Name, hGroup.GetFullName().getData());
          break;
        }
        snprintf(Filename + iLen, 22, "%d", iFinNum + 1);
        EnforceExtension(Filename, "c4i");
      }
    } else {
      // Crew was renamed; file rename necessary, if the name is not blocked by
      // another crew info
      if (!hGroup.FindEntry(szTempGroup))
        if (hGroup.Rename(Filename, szTempGroup))
          SCopy(szTempGroup, Filename, _MAX_PATH);
        else {
          // could not rename. Not fatal; just use old file
          LogF(
              "Error adjusting crew info for %s into %s: Rename error from %s "
              "to %s!",
              Name, hGroup.GetFullName().getData(), Filename, szTempGroup);
        }
    }
  }
  // Set temp group file name
  SCopy(Filename, szTempGroup);
  MakeTempFilename(szTempGroup);
  // If object info group exists, copy to temp group file
  BOOL fextract = hGroup.Extract(Filename, szTempGroup);
  const char *ferr = hGroup.GetError();
  // Open temp group
  C4Group hTemp;
  if (!hTemp.Open(szTempGroup, TRUE)) return FALSE;
#ifdef C4ENGINE  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                 // - - - - -
  // New portrait present, or old portrait not saved yet (old player begin
  // updated)?
  if (!fStoreTiny && Config.Graphics.SaveDefaultPortraits)
    if (pNewPortrait ||
        (Config.Graphics.AddNewCrewPortraits && Portrait.GetGfx() &&
         !hTemp.FindEntry(C4CFN_Portrait))) {
      C4Portrait *pSavePortrait = pNewPortrait ? pNewPortrait : &Portrait;
      C4DefGraphics *pPortraitGfx;
      // erase any old-style portrait
      hGroup.Delete(C4CFN_Portrait_Old, false);
      // save new
      if (pSavePortrait->GetGfx())
        pSavePortrait->SavePNG(hTemp, C4CFN_Portrait, C4CFN_PortraitOverlay);
      // save spec
      if (pNewPortrait) {
        // owned portrait?
        if (pNewPortrait->IsOwnedGfx())
          // use saved portrait
          SCopy(C4Portrait_Custom, PortraitFile);
        else {
          if ((pPortraitGfx = pNewPortrait->GetGfx()) && pPortraitGfx->pDef) {
            // same ID: save portrait name only
            if (pPortraitGfx->pDef->id == id)
              SCopy(pPortraitGfx->GetName(), PortraitFile);
            else {
              // different ID (crosslinked portrait)
              SCopy(C4IdText(pPortraitGfx->pDef->id), PortraitFile);
              SAppend("::", PortraitFile);
              SAppend(pPortraitGfx->GetName(), PortraitFile);
            }
          } else
            // No portrait
            SCopy(C4Portrait_None, PortraitFile);
        }
      }
      // portrait synced
    }

  // delete default portraits if they are not desired
  if (!fStoreTiny && !Config.Graphics.SaveDefaultPortraits &&
      hTemp.FindEntry(C4CFN_Portrait)) {
    if (!SEqual(PortraitFile, C4Portrait_Custom)) {
      hTemp.Delete(C4CFN_Portrait);
      hTemp.Delete(C4CFN_PortraitOverlay);
    }
  }

  // custom rank image present?
  if (pDefs && !fStoreTiny) {
    C4Def *pDef = pDefs->ID2Def(id);
    if (pDef) {
      if (pDef->pRankSymbols) {
        C4FacetExSurface fctRankSymbol;
        if (C4RankSystem::DrawRankSymbol(&fctRankSymbol, Rank,
                                         pDef->pRankSymbols,
                                         pDef->iNumRankSymbols, true)) {
          fctRankSymbol.GetFace().SavePNG(hTemp, C4CFN_ClonkRank);
        }
      } else {
        // definition does not have custom rank symbols: Remove any rank image
        // from Clonk
        hTemp.Delete(C4CFN_ClonkRank);
      }
    }
  }

#endif  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // - - - -
  // Save info to temp group
  if (!C4ObjectInfoCore::Save(hTemp, pDefs)) {
    hTemp.Close();
    return FALSE;
  }
  // Close temp group
  hTemp.Sort(C4FLS_Object);
  if (!hTemp.Close()) return FALSE;
  // Move temp group to mother group
  if (!hGroup.Move(szTempGroup, Filename)) return FALSE;
  // Success
  return TRUE;
}

void C4ObjectInfo::Evaluate() {
  Retire();
  if (WasInAction) Rounds++;
}

void C4ObjectInfo::Clear() {
#ifdef C4ENGINE  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                 // - - - - -
  Portrait.Clear();
  if (pNewPortrait) {
    delete pNewPortrait;
    pNewPortrait = NULL;
  }
  if (pCustomPortrait) {
    delete pCustomPortrait;
    pCustomPortrait = NULL;
  }
  pDef = NULL;
#endif  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // - - -
}

void C4ObjectInfo::Draw(C4Facet &cgo, BOOL fShowPortrait, BOOL fCaptain,
                        C4Object *pOfObj) {
#ifdef C4ENGINE  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                 // - - - - -

  int iX = 0;

  // Portrait
  if (fShowPortrait) {
    C4DefGraphics *pPortraitGfx;
    if (pPortraitGfx = Portrait.GetGfx())
      if (pPortraitGfx->Bitmap->Wdt) {
        // C4Facet fctPortrait; fctPortrait.Set(Portrait);
        C4Facet ccgo;
        ccgo.Set(cgo.Surface, cgo.X + iX, cgo.Y, 4 * cgo.Hgt / 3 + 10,
                 cgo.Hgt + 10);
        DWORD dwColor = 0xFFFFFFFF;
        if (pOfObj && Game.Players.Get(pOfObj->Owner))
          dwColor = Game.Players.Get(pOfObj->Owner)->ColorDw;
        pPortraitGfx->DrawClr(ccgo, TRUE, dwColor);
        iX += 4 * cgo.Hgt / 3;
      }
  }

  // Captain symbol
  if (fCaptain) {
    Game.GraphicsResource.fctCaptain.Draw(cgo.Surface, cgo.X + iX, cgo.Y, 0, 0);
    iX += Game.GraphicsResource.fctCaptain.Wdt;
  }

  // Rank symbol
  C4RankSystem *pRankSys = &Game.Rank;
  C4FacetEx *pRankRes = &Game.GraphicsResource.fctRank;
  int iRankCnt = Game.GraphicsResource.iNumRanks;
  if (pOfObj) {
    C4Def *pDef = pOfObj->Def;
    if (pDef->pRankSymbols) {
      pRankRes = pDef->pRankSymbols;
      iRankCnt = pDef->iNumRankSymbols;
    }
    if (pDef->pRankNames) {
      pRankSys = pDef->pRankNames;
    }
  }
  pRankSys->DrawRankSymbol(NULL, Rank, pRankRes, iRankCnt, false, iX, &cgo);
  iX += Game.GraphicsResource.fctRank.Wdt;
  // Rank & Name
  if (Rank > 0)
    sprintf(OSTR, "%s|%s", sRankName.getData(), pOfObj->GetName());
  else
    sprintf(OSTR, "%s", pOfObj->GetName());
  Application.DDraw->TextOut(OSTR, Game.GraphicsResource.FontRegular, 1.0,
                             cgo.Surface, cgo.X + iX, cgo.Y,
                             CStdDDraw::DEFAULT_MESSAGE_COLOR, ALeft);

#endif  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // - - - - -
}

void C4ObjectInfo::Recruit() {
  // already recruited?
  if (InAction) return;
  WasInAction = TRUE;
  InAction = TRUE;
#ifdef C4ENGINE  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                 // - - - - -
  InActionTime = Game.Time;
  // rank name overload by def?
  C4Def *pDef = Game.Defs.ID2Def(id);
  if (pDef)
    if (pDef->pRankNames) {
      StdStrBuf sRank(pDef->pRankNames->GetRankName(Rank, true));
      if (sRank) sRankName.Copy(sRank);
    }
#endif  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // - - - - -
}

void C4ObjectInfo::Retire() {
  // not recruited?
  if (!InAction) return;
  // retire
  InAction = FALSE;
#ifdef C4ENGINE  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                 // - - - - -
  TotalPlayingTime += (Game.Time - InActionTime);
#endif  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // - - - - -
}

void C4ObjectInfo::SetBirthday() { Birthday = time(NULL); }

#ifdef C4ENGINE  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                 // - - - - -

bool C4ObjectInfo::SetRandomPortrait(C4ID idSourceDef, bool fAssignPermanently,
                                     bool fCopyFile) {
  // No source def specified: use own def
  if (!idSourceDef) idSourceDef = id;
  // Get source def
  C4Def *pPortraitDef = Game.Defs.ID2Def(idSourceDef);
  // Portrait source def not loaded: do not assign a portrait now, so the clonk
  // can get
  // the correct portrait later when the source def is available (assuming this
  // clonk is
  // not going to be used in this round anyway)
  if (!pPortraitDef) return false;
  // Portrait def is loaded but does not have any portraits
  if (!pPortraitDef->PortraitCount) {
    // Then use CLNK portraits (2do: base on include chains in latter case)?
    pPortraitDef = Game.Defs.ID2Def(C4ID_Clonk);
    // Assign permanently, assuming it is some kind of normal clonk here...
    fAssignPermanently = true;
    fCopyFile = false;
    // No CLNK loaded or no portraits present? forget it, then
    if (!pPortraitDef || !pPortraitDef->PortraitCount) return false;
  }
  // shouldn't happen if PortraitCount is != 0
  if (!pPortraitDef->Portraits) return false;
  // set a random portrait (note: not net synced!)
  return SetPortrait(pPortraitDef->Portraits->GetByIndex(
                         SafeRandom(pPortraitDef->PortraitCount)),
                     fAssignPermanently, fCopyFile);
}

bool C4ObjectInfo::SetPortrait(const char *szPortraitName, C4Def *pSourceDef,
                               bool fAssignPermanently, bool fCopyFile) {
  // safety
  if (!szPortraitName || !pSourceDef || !pSourceDef->Portraits) return false;
  // Special case: custom portrait
  if (SEqual(szPortraitName, C4Portrait_Custom))
    // does the info really have a custom portrait?
    if (pCustomPortrait)
      // set the custom portrait - we're just re-linking the custom portrait
      // (fAssignPermanently or fCopyFile have nothing to do here)
      return Portrait.Link(pCustomPortrait->GetGfx());
  // set desired portrait
  return SetPortrait(pSourceDef->Portraits->Get(szPortraitName),
                     fAssignPermanently, fCopyFile);
}

bool C4ObjectInfo::SetPortrait(C4PortraitGraphics *pNewPortraitGfx,
                               bool fAssignPermanently, bool fCopyFile) {
  // safety
  if (!pNewPortraitGfx) return false;
  // assign portrait
  if (fCopyFile)
    Portrait.CopyFrom(*pNewPortraitGfx);
  else
    Portrait.Link(pNewPortraitGfx);
  // store permanently?
  if (fAssignPermanently) {
    if (!pNewPortrait) pNewPortrait = new C4Portrait();
    pNewPortrait->CopyFrom(Portrait);
  }
  // done, success
  return true;
}

bool C4ObjectInfo::ClearPortrait(bool fPermanently) {
  // no portrait
  Portrait.Clear();
  // clear new portrait; do not delete class (because empty class means
  // no-portrait-as-new-setting)
  if (fPermanently)
    if (pNewPortrait)
      pNewPortrait->Clear();
    else
      pNewPortrait = new C4Portrait();
  // done, success
  return true;
}

#endif
