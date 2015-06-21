/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Small member of the landscape class to handle the sky background */

#include "C4Include.h"
#include "landscape/C4Sky.h"

#ifndef BIG_C4INCLUDE
#include "game/C4Game.h"
#include "C4Random.h"
#include "graphics/C4SurfaceFile.h"
#include "C4Components.h"
#include "C4Wrappers.h"
#endif

static BOOL SurfaceEnsureSize(C4Surface **ppSfc, int iMinWdt, int iMinHgt) {
  // safety
  if (!ppSfc) return FALSE;
  if (!*ppSfc) return FALSE;
  // get size
  int iWdt = (*ppSfc)->Wdt, iHgt = (*ppSfc)->Hgt;
  int iDstWdt = iWdt, iDstHgt = iHgt;
  // check if it must be enlarged
  while (iDstWdt < iMinWdt) iDstWdt += iWdt;
  while (iDstHgt < iMinHgt) iDstHgt += iHgt;
  if (iDstWdt == iWdt && iDstHgt == iHgt) return TRUE;
  // create new surface
  C4Surface *pNewSfc = new C4Surface();
  if (!pNewSfc->Create(iDstWdt, iDstHgt, false)) {
    delete pNewSfc;
    return FALSE;
  }
  // blit tiled into dest surface
  lpDDraw->BlitSurfaceTile2(*ppSfc, pNewSfc, 0, 0, iDstWdt, iDstHgt, 0, 0,
                            FALSE);
  // destroy old surface, assign new
  delete *ppSfc;
  *ppSfc = pNewSfc;
  // success
  return TRUE;
}

void C4Sky::SetFadePalette(int32_t *ipColors) {
  // If colors all zero, use game palette default blue
  if (ipColors[0] + ipColors[1] + ipColors[2] + ipColors[3] + ipColors[4] +
          ipColors[5] ==
      0) {
    BYTE *pClr = Game.GraphicsResource.GamePalette + 3 * CSkyDef1;
    FadeClr1 = C4RGB(pClr[0], pClr[1], pClr[2]);
    FadeClr2 = C4RGB(pClr[3 * 19 + 0], pClr[3 * 19 + 1], pClr[3 * 19 + 2]);
  } else {
    // set colors
    FadeClr1 = C4RGB(ipColors[0], ipColors[1], ipColors[2]);
    FadeClr2 = C4RGB(ipColors[3], ipColors[4], ipColors[5]);
  }
}

BOOL C4Sky::Init(bool fSavegame) {
  int32_t skylistn;

  // reset scrolling pos+speed
  // not in savegame, because it will have been loaded from game data there
  if (!fSavegame) {
    x = y = xdir = ydir = 0;
    ParX = ParY = 10;
    ParallaxMode = 0;
  }

  // Check for sky bitmap in scenario file
  Surface = new C4Surface();
  bool loaded = !!Surface->LoadAny(Game.ScenarioFile, C4CFN_Sky, true, true);

  // Else, evaluate scenario core landscape sky default list
  if (!loaded) {
    // Scan list sections
    SReplaceChar(Game.C4S.Landscape.SkyDef, ',',
                 ';');  // modifying the C4S here...!
    skylistn = SCharCount(';', Game.C4S.Landscape.SkyDef) + 1;
    SCopySegment(Game.C4S.Landscape.SkyDef,
                 SeededRandom(Game.Parameters.RandomSeed, skylistn), OSTR, ';',
                 sizeof(OSTR) - 1);
    SClearFrontBack(OSTR);
    // Sky tile specified, try load
    if (OSTR[0] && !SEqual(OSTR, "Default")) {
      // Check for sky tile in scenario file
      loaded = !!Surface->LoadAny(Game.ScenarioFile, OSTR, true, true);
      if (!loaded) {
        loaded = !!Surface->LoadAny(Game.GraphicsResource.Files, OSTR, true);
      }
    }
  }

  if (loaded) {
    // surface loaded, store first color index
    /*if (Surface->HasOwnPal())
            {
            FadeClr1=Surface->pPal->GetClr(0);
            FadeClr2=Surface->pPal->GetClr(19);
            }
    else*/
    FadeClr1 = FadeClr2 = 0xffffff;
    // enlarge surface to avoid slow 1*1-px-skies
    if (!SurfaceEnsureSize(&Surface, 128, 128)) return FALSE;

    // set parallax scroll mode
    switch (Game.C4S.Landscape.SkyScrollMode) {
      case 0:  // default: no scrolling
        break;
      case 1:  // go with the wind in xdir, and do some parallax scrolling in
               // ydir
        ParallaxMode = C4SkyPM_Wind;
        ParY = 20;
        break;
      case 2:  // parallax in both directions
        ParX = ParY = 20;
        break;
    }
  }

  // Else, try creating default Surface
  if (!loaded) {
    SetFadePalette(Game.C4S.Landscape.SkyDefFade);
    delete Surface;
    Surface = 0;
  }

  // no sky - using fade in newgfx
  if (!Surface) return TRUE;

  // Store size
  if (Surface) {
    int iWdt, iHgt;
    if (Surface->GetSurfaceSize(iWdt, iHgt)) {
      Width = iWdt;
      Height = iHgt;
    }
  }

  // Success
  return TRUE;
}

void C4Sky::Default() {
  Width = Height = 0;
  Surface = NULL;
  x = y = xdir = ydir = 0;
  Modulation = 0x00ffffff;
  ParX = ParY = 10;
  ParallaxMode = C4SkyPM_Fixed;
  BackClr = 0;
  BackClrEnabled = FALSE;
}

C4Sky::~C4Sky() { Clear(); }

void C4Sky::Clear() {
  delete Surface;
  Surface = NULL;
  Modulation = 0x00ffffff;
}

BOOL C4Sky::Save(C4Group &hGroup) {
  // Sky-saving disabled by scenario core
  // (With this option enabled, script-defined changes to sky palette will not
  // be saved!)
  if (Game.C4S.Landscape.NoSky) {
    hGroup.Delete(C4CFN_Sky);
    return TRUE;
  }
  // no sky?
  if (!Surface) return TRUE;
  // FIXME?
  // Success
  return TRUE;
}

void C4Sky::Execute() {
  // surface exists?
  if (!Surface) return;
  // advance pos
  x += xdir;
  y += ydir;
  // clip by bounds
  if (x >= itofix(Width)) x -= itofix(Width);
  if (y >= itofix(Height)) y -= itofix(Height);
  // update speed
  if (ParallaxMode == C4SkyPM_Wind) xdir = FIXED100(Game.Weather.Wind);
}

void C4Sky::Draw(C4FacetEx &cgo) {
  // background color?
  if (BackClrEnabled)
    Application.DDraw->DrawBoxDw(cgo.Surface, cgo.X, cgo.Y, cgo.X + cgo.Wdt,
                                 cgo.Y + cgo.Hgt, BackClr);
  // sky surface?
  if (Modulation != 0xffffff)
    Application.DDraw->ActivateBlitModulation(Modulation);
  if (Surface) {
    // blit parallax sky
    int iParX = cgo.TargetX * 10 / ParX - fixtoi(x);
    int iParY = cgo.TargetY * 10 / ParY - fixtoi(y);
    Application.DDraw->BlitSurfaceTile2(Surface, cgo.Surface, cgo.X, cgo.Y,
                                        cgo.Wdt, cgo.Hgt, iParX, iParY, FALSE);
  } else {
    // no sky surface: blit sky fade
    DWORD dwClr1 = GetSkyFadeClr(cgo.TargetY);
    DWORD dwClr2 = GetSkyFadeClr(cgo.TargetY + cgo.Hgt);
    Application.DDraw->DrawBoxFade(cgo.Surface, cgo.X, cgo.Y, cgo.Wdt, cgo.Hgt,
                                   dwClr1, dwClr1, dwClr2, dwClr2, cgo.TargetX,
                                   cgo.TargetY);
  }
  if (Modulation != 0xffffff) Application.DDraw->DeactivateBlitModulation();
  // done
}

DWORD C4Sky::GetSkyFadeClr(int32_t iY) {
  int32_t iPos2 = (iY * 256) / GBackHgt;
  int32_t iPos1 = 256 - iPos2;
  return (((((FadeClr1 & 0xff00ff) * iPos1 + (FadeClr2 & 0xff00ff) * iPos2) &
            0xff00ff00) |
           (((FadeClr1 & 0x00ff00) * iPos1 + (FadeClr2 & 0x00ff00) * iPos2) &
            0x00ff0000)) >>
          8) |
         (FadeClr1 & 0xff000000);
}

bool C4Sky::SetModulation(DWORD dwWithClr, DWORD dwBackClr) {
  Modulation = dwWithClr;
  BackClr = dwBackClr;
  BackClrEnabled = (Modulation >> 24) ? TRUE : FALSE;
  return true;
}

void C4Sky::CompileFunc(StdCompiler *pComp) {
  pComp->Value(mkNamingAdapt(mkCastIntAdapt(x), "X", Fix0));
  pComp->Value(mkNamingAdapt(mkCastIntAdapt(y), "Y", Fix0));
  pComp->Value(mkNamingAdapt(mkCastIntAdapt(xdir), "XDir", Fix0));
  pComp->Value(mkNamingAdapt(mkCastIntAdapt(ydir), "YDir", Fix0));
  pComp->Value(mkNamingAdapt(Modulation, "Modulation", 0x00ffffffU));
  pComp->Value(mkNamingAdapt(ParX, "ParX", 10));
  pComp->Value(mkNamingAdapt(ParY, "ParY", 10));
  pComp->Value(mkNamingAdapt(ParallaxMode, "ParMode", C4SkyPM_Fixed));
  pComp->Value(mkNamingAdapt(BackClr, "BackClr", 0));
  pComp->Value(mkNamingAdapt(BackClrEnabled, "BackClrEnabled", false));
}
