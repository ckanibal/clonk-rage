/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* A facet that can hold its own surface and also target coordinates */

#include "C4Include.h"
#include "graphics/C4FacetEx.h"

#ifndef BIG_C4INCLUDE
#include "C4Random.h"
#include "object/C4Shape.h"
#include "c4group/C4Group.h"
#endif

#ifdef C4ENGINE

void C4FacetEx::Set(SURFACE nsfc, int nx, int ny, int nwdt, int nhgt, int ntx,
                    int nty) {
  C4Facet::Set(nsfc, nx, ny, nwdt, nhgt);
  TargetX = ntx;
  TargetY = nty;
}

C4FacetEx C4FacetEx::GetSection(int iSection) {
  C4FacetEx fctResult;
  fctResult.Set(Surface, X + Hgt * iSection, Y, Hgt, Hgt, 0, 0);
  return fctResult;
}

C4FacetEx C4FacetEx::GetPhase(int iPhaseX, int iPhaseY) {
  C4FacetEx fctResult;
  fctResult.Set(Surface, X + Wdt * iPhaseX, Y + Hgt * iPhaseY, Wdt, Hgt, 0, 0);
  return fctResult;
}

void C4FacetEx::SetRect(C4TargetRect &rSrc) {
  X = rSrc.x;
  Y = rSrc.y;
  Wdt = rSrc.Wdt;
  Hgt = rSrc.Hgt;
  TargetX = rSrc.tx;
  TargetY = rSrc.ty;
}

void C4FacetEx::DrawLine(int iX1, int iY1, int iX2, int iY2, BYTE bCol1,
                         BYTE bCol2) {
  if (!lpDDraw || !Surface || !Wdt || !Hgt) return;
  // Scroll position
  iX1 -= TargetX;
  iY1 -= TargetY;
  iX2 -= TargetX;
  iY2 -= TargetY;
  // No clipping is done here, because clipping will be done by gfx wrapper
  // anyway
  // Draw line
  lpDDraw->DrawLine(Surface, X + iX1, Y + iY1, X + iX2, Y + iY2, bCol1);
  lpDDraw->DrawPix(Surface, (float)(X + iX1), (float)(Y + iY1),
                   lpDDraw->Pal.GetClr(bCol2));
}

// bolt random size
#define DrawBoltR1 7
#define DrawBoltR2 3

void C4FacetEx::DrawBolt(int iX1, int iY1, int iX2, int iY2, BYTE bCol,
                         BYTE bCol2) {
  if (!lpDDraw || !Surface || !Wdt || !Hgt) return;
  // Scroll position
  iX1 -= TargetX;
  iY1 -= TargetY;
  iX2 -= TargetX;
  iY2 -= TargetY;
  // Facet bounds
  if (!Inside(iX1, 0, Wdt - 1) && !Inside(iX2, 0, Wdt - 1)) return;
  if (!Inside(iY1, 0, Hgt - 1) && !Inside(iY2, 0, Hgt - 1)) return;
  iX1 += X;
  iX2 += X;
  iY1 += Y;
  iY2 += Y;
  // Draw bolt
  int pvtx[2 * 4];
  pvtx[0] = iX1;
  pvtx[1] = iY1;
  pvtx[2] = iX2;
  pvtx[3] = iY2;
#ifdef C4ENGINE
  pvtx[4] = iX2 + SafeRandom(DrawBoltR1) - DrawBoltR2;
  pvtx[5] = iY2 + SafeRandom(DrawBoltR1) - DrawBoltR2;
  pvtx[6] = iX1 + SafeRandom(DrawBoltR1) - DrawBoltR2;
  pvtx[7] = iY1 + SafeRandom(DrawBoltR1) - DrawBoltR2;
#else
  pvtx[4] = iX2 + X % 3 - 1;
  pvtx[5] = iY2 + X % 3 - 1;
  pvtx[6] = iX1 + Y % 3 - 1;
  pvtx[7] = iY1 + Y % 3 - 1;
#endif
  // Draw in surface
  DWORD dwClr1 = lpDDraw->Pal.GetClr(bCol), dwClr2;
  DWORD dwClr3 = lpDDraw->Pal.GetClr(bCol2), dwClr4;
  /*if (DDrawCfg.NoBoxFades)
          {*/
  dwClr2 = dwClr1;
  dwClr4 = dwClr3;
  /*}
else
  {
  DWORD dwClr2=dwClr1|0xff000000; ...this leads to black and white lightning
bolts. Who wants that?
  DWORD dwClr4=dwClr3|0xff000000;
  }*/
  lpDDraw->DrawQuadDw(Surface, pvtx, dwClr1, dwClr3, dwClr4, dwClr2);
}

// ------------------------
// C4FacetExSurface

BOOL C4FacetExSurface::Create(int iWdt, int iHgt, int iWdt2, int iHgt2) {
  Clear();
  // Create surface
  Face.Default();
  if (!Face.Create(iWdt, iHgt)) return FALSE;
  // Set facet
  if (iWdt2 == C4FCT_Full) iWdt2 = Face.Wdt;
  if (iWdt2 == C4FCT_Height) iWdt2 = Face.Hgt;
  if (iWdt2 == C4FCT_Width) iWdt2 = Face.Wdt;
  if (iHgt2 == C4FCT_Full) iHgt2 = Face.Hgt;
  if (iHgt2 == C4FCT_Height) iHgt2 = Face.Hgt;
  if (iHgt2 == C4FCT_Width) iHgt2 = Face.Wdt;
  Set(&Face, 0, 0, iWdt2, iHgt2, 0, 0);
  return TRUE;
}

BOOL C4FacetExSurface::CreateClrByOwner(CSurface *pBySurface) {
  Clear();
  // create surface
  if (!Face.CreateColorByOwner(pBySurface)) return FALSE;
  // set facet
  Set(&Face, 0, 0, Face.Wdt, Face.Hgt, 0, 0);
  // success
  return TRUE;
}

BOOL C4FacetExSurface::EnsureSize(int iMinWdt, int iMinHgt) {
  // safety
  if (!Surface) return FALSE;
  // check size
  int iWdt = Face.Wdt, iHgt = Face.Hgt;
  if (iWdt >= iMinWdt && iHgt >= iMinHgt) return TRUE;
  // create temp surface
  CSurface *sfcDup = new CSurface(iWdt, iHgt);
  if (!sfcDup) return FALSE;
  if (!lpDDraw->BlitSurface(&Face, sfcDup, 0, 0, false)) {
    delete sfcDup;
    return FALSE;
  }
  // calc needed size
  int iDstWdt = Surface->Wdt, iDstHgt = iHgt;
  while (iDstWdt < iMinWdt) iDstWdt += iWdt;
  while (iDstHgt < iMinHgt) iDstHgt += iHgt;
  // recreate this one
  if (!Face.Create(iDstWdt, iDstHgt)) {
    delete sfcDup;
    Clear();
    return FALSE;
  }
  // blit tiled into it
  BOOL fSuccess = lpDDraw->BlitSurfaceTile(sfcDup, &Face, 0, 0, iDstWdt,
                                           iDstHgt, 0, 0, FALSE);
  // del temp surface
  delete sfcDup;
  // done
  return fSuccess;
}

/*BOOL C4FacetExSurface::Save(C4Group &hGroup, const char *szName)
        {
        // Empty
        if (!Wdt || !Hgt) return FALSE;
        // Full surface
        if ((Wdt==Face.Wdt) && (Hgt==Face.Hgt))
                {
                if (!Face.Save(hGroup,szName)) return FALSE;
                }
        // Surface section
        else
                {
                C4Surface sfcFacet;
                if (!sfcFacet.Create(Wdt,Hgt)) return FALSE;
                Draw(&sfcFacet,0,0);
                if (!sfcFacet.Save(hGroup,szName)) return FALSE;
                }
        // Success
        return TRUE;
        }*/

BOOL C4FacetExSurface::Load(C4Group &hGroup, const char *szName, int iWdt,
                            int iHgt, bool fOwnPal, bool fNoErrIfNotFound) {
  Clear();
  // Entry name
  char szFilename[_MAX_FNAME + 1];
  SCopy(szName, szFilename, _MAX_FNAME);
  char *szExt = GetExtension(szFilename);
  if (!*szExt) {
    // no extension: Default to extension that is found as file in group
    const char *const extensions[] = {"png", "bmp", "jpeg", "jpg", NULL};
    int i = 0;
    const char *szExt;
    while (szExt = extensions[i++]) {
      EnforceExtension(szFilename, szExt);
      if (hGroup.FindEntry(szFilename)) break;
    }
  }
  // Load surface
  if (!Face.Load(hGroup, szFilename, fOwnPal, fNoErrIfNotFound)) return FALSE;
  // Set facet
  if (iWdt == C4FCT_Full) iWdt = Face.Wdt;
  if (iWdt == C4FCT_Height) iWdt = Face.Hgt;
  if (iWdt == C4FCT_Width) iWdt = Face.Wdt;
  if (iHgt == C4FCT_Full) iHgt = Face.Hgt;
  if (iHgt == C4FCT_Height) iHgt = Face.Hgt;
  if (iHgt == C4FCT_Width) iHgt = Face.Wdt;
  Set(&Face, 0, 0, iWdt, iHgt, 0, 0);
  return TRUE;
}

bool C4FacetExSurface::CopyFromSfcMaxSize(C4Surface &srcSfc, int32_t iMaxSize,
                                          uint32_t dwColor) {
  // safety
  if (!srcSfc.Wdt || !srcSfc.Hgt) return false;
  Clear();
  // no scale?
  bool fNeedsScale = !(srcSfc.Wdt <= iMaxSize && srcSfc.Hgt <= iMaxSize);
  if (!fNeedsScale && !dwColor) {
    // no change necessary; just copy then
    Face.Copy(srcSfc);
  } else {
    // must scale down or colorize. Just blit.
    C4Facet fctSource;
    fctSource.Set(&srcSfc, 0, 0, srcSfc.Wdt, srcSfc.Hgt);
    int32_t iTargetWdt, iTargetHgt;
    if (fNeedsScale) {
      if (fctSource.Wdt > fctSource.Hgt) {
        iTargetWdt = iMaxSize;
        iTargetHgt = fctSource.Hgt * iTargetWdt / fctSource.Wdt;
      } else {
        iTargetHgt = iMaxSize;
        iTargetWdt = fctSource.Wdt * iTargetHgt / fctSource.Hgt;
      }
    } else {
      iTargetWdt = fctSource.Wdt;
      iTargetHgt = fctSource.Hgt;
    }
    if (dwColor) srcSfc.SetClr(dwColor);
    Create(iTargetWdt, iTargetHgt);
    lpDDraw->Blit(&srcSfc, 0.0f, 0.0f, float(fctSource.Wdt),
                  float(fctSource.Hgt), &Face, 0, 0, iTargetWdt, iTargetHgt);
  }
  Set(&Face, 0, 0, Face.Wdt, Face.Hgt);
  return true;
}

void C4FacetExSurface::Grayscale(int32_t iOffset) {
  if (!lpDDraw || !Surface || !Wdt || !Hgt) return;
  lpDDraw->Grayscale(Surface, iOffset);
}

bool C4FacetExSurface::EnsureOwnSurface() {
  // is it a link?
  if (Surface != &Face) {
    // then recreate in same size
    C4Facet fctOld = *this;
    if (!Create(fctOld.Wdt, fctOld.Hgt)) return false;
    fctOld.Draw(*this);
  }
  return true;
}

#endif  // C4ENGINE
