/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Create map from dynamic landscape data in scenario */

#include "C4Include.h"
#include "C4Map.h"

#ifndef BIG_C4INCLUDE
#include "C4Random.h"
#include "landscape/C4Texture.h"
#include "c4group/C4Group.h"
#endif

#include <Bitmap256.h>

C4MapCreator::C4MapCreator() { Reset(); }

void C4MapCreator::Reset() {
  MapIFT = 128;
  MapBuf = NULL;
  Exclusive = -1;
}

void C4MapCreator::SetPix(int32_t x, int32_t y, BYTE col) {
  // Safety
  if (!Inside<int32_t>(x, 0, MapWdt - 1) || !Inside<int32_t>(y, 0, MapHgt - 1))
    return;
  // Exclusive
  if (Exclusive > -1)
    if (GetPix(x, y) != Exclusive) return;
  // Set pix
  MapBuf->SetPix(x, y, col);
}

void C4MapCreator::SetSpot(int32_t x, int32_t y, int32_t rad, BYTE col) {
  int32_t ycnt, xcnt, lwdt, dpy;
  for (ycnt = -rad; ycnt <= rad; ycnt++) {
    lwdt = (int32_t)sqrt(double(rad * rad - ycnt * ycnt));
    dpy = y + ycnt;
    for (xcnt = -lwdt; xcnt < lwdt + (lwdt == 0); xcnt++)
      SetPix(x + xcnt, dpy, col);
  }
}

void C4MapCreator::DrawLayer(int32_t x, int32_t y, int32_t size, BYTE col) {
  int32_t cnt, cnt2;
  for (cnt = 0; cnt < size; cnt++) {
    x += Random(9) - 4;
    y += Random(3) - 1;
    for (cnt2 = Random(3); cnt2 < 5; cnt2++) {
      SetPix(x + cnt2, y, col);
      SetPix(x + cnt2 + 1, y + 1, col);
    }
  }
}

BYTE C4MapCreator::GetPix(int32_t x, int32_t y) {
  // Safety
  if (!Inside<int32_t>(x, 0, MapWdt - 1) || !Inside<int32_t>(y, 0, MapHgt - 1))
    return 0;
  // Get pix
  return MapBuf->GetPix(x, y);
}

void C4MapCreator::Create(CSurface8 *sfcMap, C4SLandscape &rLScape,
                          C4TextureMap &rTexMap, BOOL fLayers,
                          int32_t iPlayerNum) {
  double fullperiod = 20.0 * pi;
  BYTE ccol;
  int32_t cx, cy;

  // Safeties
  if (!sfcMap) return;
  iPlayerNum = BoundBy<int32_t>(iPlayerNum, 1, C4S_MaxPlayer);

  // Set creator variables
  MapBuf = sfcMap;
  MapWdt = MapBuf->Wdt;
  MapHgt = MapBuf->Hgt;

  // Reset map (0 is sky)
  MapBuf->ClearBox8Only(0, 0, MapBuf->Wdt, MapBuf->Hgt);

  // Surface
  ccol = rTexMap.GetIndexMatTex(rLScape.Material, "Smooth") + MapIFT;
  float amplitude = (float)rLScape.Amplitude.Evaluate();
  float phase = (float)rLScape.Phase.Evaluate();
  float period = (float)rLScape.Period.Evaluate();
  if (rLScape.MapPlayerExtend)
    period *= Min(iPlayerNum, C4S_MaxMapPlayerExtend);
  float natural = (float)rLScape.Random.Evaluate();
  int32_t level0 = Min(MapWdt, MapHgt) / 2;
  int32_t maxrange = level0 * 3 / 4;
  double cy_curve, cy_natural;  // -1.0 - +1.0 !

  double rnd_cy, rnd_tend;  // -1.0 - +1.0 !
  rnd_cy = (double)(Random(2000 + 1) - 1000) / 1000.0;
  rnd_tend = (double)(Random(200 + 1) - 100) / 20000.0;

  for (cx = 0; cx < MapWdt; cx++) {
    rnd_cy += rnd_tend;
    rnd_tend += (double)(Random(100 + 1) - 50) / 10000;
    if (rnd_tend > +0.05) rnd_tend = +0.05;
    if (rnd_tend < -0.05) rnd_tend = -0.05;
    if (rnd_cy < -0.5) rnd_tend += 0.01;
    if (rnd_cy > +0.5) rnd_tend -= 0.01;

    cy_natural = rnd_cy * natural / 100.0;
    cy_curve = sin(fullperiod * period / 100.0 * (float)cx / (float)MapWdt +
                   2.0 * pi * phase / 100.0) *
               amplitude / 100.0;

    cy = level0 + BoundBy((int32_t)((float)maxrange * (cy_curve + cy_natural)),
                          -maxrange, +maxrange);

    SetPix(cx, cy, ccol);
  }

  // Raise bottom to surface
  for (cx = 0; cx < MapWdt; cx++)
    for (cy = MapHgt - 1; (cy >= 0) && !GetPix(cx, cy); cy--)
      SetPix(cx, cy, ccol);
  // Raise liquid level
  Exclusive = 0;
  ccol = rTexMap.GetIndexMatTex(rLScape.Liquid, "Smooth");
  int32_t wtr_level = rLScape.LiquidLevel.Evaluate();
  for (cx = 0; cx < MapWdt; cx++)
    for (cy = MapHgt * (100 - wtr_level) / 100; cy < MapHgt; cy++)
      SetPix(cx, cy, ccol);
  Exclusive = -1;

  // Layers
  if (fLayers) {
    // Base material
    Exclusive = rTexMap.GetIndexMatTex(rLScape.Material, "Smooth") + MapIFT;

    int32_t cnt, clayer, layer_num, sptx, spty;

    // Process layer name list
    for (clayer = 0; clayer < C4MaxNameList; clayer++)
      if (rLScape.Layers.Name[clayer][0]) {
        // Draw layers
        ccol = rTexMap.GetIndexMatTex(rLScape.Layers.Name[clayer], "Rough") +
               MapIFT;
        layer_num = rLScape.Layers.Count[clayer];
        layer_num = layer_num * MapWdt * MapHgt / 15000;
        for (cnt = 0; cnt < layer_num; cnt++) {
          // Place layer
          sptx = Random(MapWdt);
          for (spty = 0; (spty < MapHgt) && (GetPix(sptx, spty) != Exclusive);
               spty++)
            ;
          spty += 5 + Random((MapHgt - spty) - 10);
          DrawLayer(sptx, spty, Random(15), ccol);
        }
      }

    Exclusive = -1;
  }
}

/*BOOL C4MapCreator::Load(
                                BYTE **pbypBuffer,
                                int32_t &rBufWdt, int32_t &rMapWdt, int32_t
   &rMapHgt,
                                C4Group &hGroup, const char *szEntryName,
                                C4TextureMap &rTexMap)
        {
        BOOL fOwnBuf=FALSE;

        CBitmap256Info Bmp;

        // Access entry in group, read bitmap info
        if (!hGroup.AccessEntry(szEntryName)) return FALSE;
        if (!hGroup.Read(&Bmp,sizeof(Bmp))) return FALSE;
        if (!Bmp.Valid()) return FALSE;
        if (!hGroup.Advance(Bmp.FileBitsOffset())) return FALSE;

        // If buffer is present, check for sufficient size
        if (*pbypBuffer)
                {
                if ((Bmp.Info.biWidth>rMapWdt)
                 || (Bmp.Info.biHeight>rMapHgt) ) return FALSE;
                }
        // Else, allocate buffer, set sizes
        else
                {
                rMapWdt = Bmp.Info.biWidth;
                rMapHgt = Bmp.Info.biHeight;
                rBufWdt = rMapWdt; int dwBufWdt = rBufWdt; DWordAlign(dwBufWdt);
   rBufWdt = dwBufWdt;
                if (!(*pbypBuffer = new BYTE [rBufWdt*rMapHgt]))
                        return FALSE;
                fOwnBuf=TRUE;
                }

        // Read bits to buffer
        for (int32_t cline=Bmp.Info.biHeight-1; cline>=0; cline--)
                if (!hGroup.Read(*pbypBuffer+rBufWdt*cline,rBufWdt))
                        { if (fOwnBuf) delete [] *pbypBuffer; return FALSE; }

        // Validate texture indices
        MapBuf=*pbypBuffer;
        MapBufWdt=rBufWdt;
        MapWdt=rMapWdt; MapHgt=rMapHgt;
        ValidateTextureIndices(rTexMap);

        return TRUE;
        }*/

void C4MapCreator::ValidateTextureIndices(C4TextureMap &rTextureMap) {
  int32_t iX, iY;
  for (iY = 0; iY < MapHgt; iY++)
    for (iX = 0; iX < MapWdt; iX++)
      if (!rTextureMap.GetEntry(GetPix(iX, iY))) SetPix(iX, iY, 0);
}
