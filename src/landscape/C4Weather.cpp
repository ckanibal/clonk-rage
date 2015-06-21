/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Controls temperature, wind, and natural disasters */

#include "C4Include.h"
#include "landscape/C4Weather.h"

#ifndef BIG_C4INCLUDE
#include "object/C4Object.h"
#include "C4Random.h"
#include "C4Wrappers.h"
#endif

C4Weather::C4Weather() { Default(); }

C4Weather::~C4Weather() { Clear(); }

void C4Weather::Init(BOOL fScenario) {
  if (fScenario) {
    // Season
    Season = Game.C4S.Weather.StartSeason.Evaluate();
    YearSpeed = Game.C4S.Weather.YearSpeed.Evaluate();
    // Temperature
    Climate = 100 - Game.C4S.Weather.Climate.Evaluate() - 50;
    Temperature = Climate;
    // Wind
    Wind = TargetWind = Game.C4S.Weather.Wind.Evaluate();
    // Precipitation
    if (!Game.C4S.Head.NoInitialize)
      if (Game.C4S.Weather.Rain.Evaluate())
        for (int32_t iClouds = Min(GBackWdt / 500, 5); iClouds > 0; iClouds--) {
          volatile int iWidth = GBackWdt / 15 + Random(320);
          volatile int iX = Random(GBackWdt);
          LaunchCloud(iX, -1, iWidth, Game.C4S.Weather.Rain.Evaluate(),
                      Game.C4S.Weather.Precipitation);
        }
    // Lightning
    LightningLevel = Game.C4S.Weather.Lightning.Evaluate();
    // Disasters
    MeteoriteLevel = Game.C4S.Disasters.Meteorite.Evaluate();
    VolcanoLevel = Game.C4S.Disasters.Volcano.Evaluate();
    EarthquakeLevel = Game.C4S.Disasters.Earthquake.Evaluate();
    // gamma?
    NoGamma = Game.C4S.Weather.NoGamma;
  }
  // set gamma
  SetSeasonGamma();
}

void C4Weather::Execute() {
  // Season
  if (!Tick35) {
    SeasonDelay += YearSpeed;
    if (SeasonDelay >= 200) {
      SeasonDelay = 0;
      Season++;
      if (Season > Game.C4S.Weather.StartSeason.Max)
        Season = Game.C4S.Weather.StartSeason.Min;
      SetSeasonGamma();
    }
  }
  // Temperature
  if (!Tick35) {
    int32_t iTemperature =
        Climate -
        (int32_t)(TemperatureRange * cos(6.28 * (float)Season / 100.0));
    if (Temperature < iTemperature)
      Temperature++;
    else if (Temperature > iTemperature)
      Temperature--;
  }
  // Wind
  if (!Tick1000) TargetWind = Game.C4S.Weather.Wind.Evaluate();
  if (!Tick10)
    Wind =
        BoundBy<int32_t>(Wind + Sign(TargetWind - Wind),
                         Game.C4S.Weather.Wind.Min, Game.C4S.Weather.Wind.Max);
  if (!Tick10) SoundLevel("Wind", NULL, Max(Abs(Wind) - 30, 0) * 2);
  // Disaster launch
  if (!Tick10) {
    // Meteorite
    if (!Random(60))
      if (Random(100) < MeteoriteLevel) {
        C4Object *meto;
        // In cave landscapes, meteors must be created a bit lower so they don't
        // hit the ceiling
        // (who activates meteors in cave landscapes anyway?)
        meto = Game.CreateObject(C4ID_Meteor, NULL, NO_OWNER, Random(GBackWdt),
                                 Game.Landscape.TopOpen ? -20 : 5, 0,
                                 itofix(Random(100 + 1) - 50) / 10,
                                 Game.Landscape.TopOpen ? Fix0 : itofix(2),
                                 itofix(1) / 5);
      }
    // Lightning
    if (!Random(35))
      if (Random(100) < LightningLevel)
        LaunchLightning(Random(GBackWdt), 0, -20, 41, +5, 15, TRUE);
    // Earthquake
    if (!Random(50))
      if (Random(100) < EarthquakeLevel)
        LaunchEarthquake(Random(GBackWdt), Random(GBackHgt));
    // Volcano
    if (!Random(60))
      if (Random(100) < VolcanoLevel)
        LaunchVolcano(Game.Material.Get("Lava"), Random(GBackWdt), GBackHgt - 1,
                      BoundBy(15 * GBackHgt / 500 + Random(10), 10, 60));
  }
}

void C4Weather::Clear() {}

BOOL C4Weather::LaunchLightning(int32_t x, int32_t y, int32_t xdir,
                                int32_t xrange, int32_t ydir, int32_t yrange,
                                BOOL fDoGamma) {
  C4Object *pObj;
  if (pObj = Game.CreateObject(C4Id("FXL1"), NULL))
    pObj->Call(PSF_Activate,
               &C4AulParSet(C4VInt(x), C4VInt(y), C4VInt(xdir), C4VInt(xrange),
                            C4VInt(ydir), C4VInt(yrange), C4VBool(!!fDoGamma)));
  return TRUE;
}

int32_t C4Weather::GetWind(int32_t x, int32_t y) {
  if (GBackIFT(x, y)) return 0;
  return Wind;
}

int32_t C4Weather::GetTemperature() { return Temperature; }

BOOL C4Weather::LaunchVolcano(int32_t mat, int32_t x, int32_t y, int32_t size) {
  C4Object *pObj;
  if (pObj = Game.CreateObject(C4Id("FXV1"), NULL))
    pObj->Call(PSF_Activate,
               &C4AulParSet(C4VInt(x), C4VInt(y), C4VInt(size), C4VInt(mat)));
  return TRUE;
}

void C4Weather::Default() {
  Season = 0;
  YearSpeed = 0;
  SeasonDelay = 0;
  Wind = TargetWind = 0;
  Temperature = Climate = 0;
  TemperatureRange = 30;
  MeteoriteLevel = VolcanoLevel = EarthquakeLevel = LightningLevel = 0;
  NoGamma = TRUE;
}

BOOL C4Weather::LaunchEarthquake(int32_t iX, int32_t iY) {
  C4Object *pObj;
  if (pObj = Game.CreateObject(C4Id("FXQ1"), NULL, NO_OWNER, iX, iY))
    if (!!pObj->Call(PSF_Activate)) return TRUE;
  return FALSE;
}

BOOL C4Weather::LaunchCloud(int32_t iX, int32_t iY, int32_t iWidth,
                            int32_t iStrength, const char *szPrecipitation) {
  if (Game.Material.Get(szPrecipitation) == MNone) return FALSE;
  C4Object *pObj;
  if (pObj = Game.CreateObject(C4Id("FXP1"), NULL, NO_OWNER, iX, iY))
    if (!!pObj->Call(PSF_Activate,
                     &C4AulParSet(C4VInt(Game.Material.Get(szPrecipitation)),
                                  C4VInt(iWidth), C4VInt(iStrength))))
      return TRUE;
  return FALSE;
}

void C4Weather::SetWind(int32_t iWind) {
  Wind = BoundBy<int32_t>(iWind, -100, +100);
  TargetWind = BoundBy<int32_t>(iWind, -100, +100);
}

void C4Weather::SetTemperature(int32_t iTemperature) {
  Temperature = BoundBy<int32_t>(iTemperature, -100, 100);
  SetSeasonGamma();
}

void C4Weather::SetSeason(int32_t iSeason) {
  Season = BoundBy<int32_t>(iSeason, 0, 100);
  SetSeasonGamma();
}

int32_t C4Weather::GetSeason() { return Season; }

void C4Weather::SetClimate(int32_t iClimate) {
  Climate = BoundBy<int32_t>(iClimate, -50, +50);
  SetSeasonGamma();
}

int32_t C4Weather::GetClimate() { return Climate; }

static DWORD SeasonColors[4][3] = {
    {0x000000, 0x7f7f90,
     0xefefff},  // winter: slightly blue; blued out by temperature
    {0x070f00, 0x90a07f, 0xffffdf},  // spring: green to yellow
    {0x000000, 0x808080, 0xffffff},  // summer: regular ramp
    {0x0f0700, 0xa08067, 0xffffdf}   // fall:   dark, brown ramp
};

void C4Weather::SetSeasonGamma() {
  if (NoGamma) return;
  // get season num and offset
  int32_t iSeason1 = (Season / 25) % 4;
  int32_t iSeason2 = (iSeason1 + 1) % 4;
  int32_t iSeasonOff1 = BoundBy(Season % 25, 5, 19) - 5;
  int32_t iSeasonOff2 = 15 - iSeasonOff1;
  DWORD dwClr[3];
  ZeroMemory(dwClr, sizeof(DWORD) * 3);
  // interpolate between season colors
  for (int32_t i = 0; i < 3; ++i)
    for (int32_t iChan = 0; iChan < 24; iChan += 8) {
      BYTE byC1 = BYTE(SeasonColors[iSeason1][i] >> iChan);
      BYTE byC2 = BYTE(SeasonColors[iSeason2][i] >> iChan);
      int32_t iChanVal = (byC1 * iSeasonOff2 + byC2 * iSeasonOff1) / 15;
      // red+green: reduce in winter
      if (Temperature < 0)
        if (iChan)
          iChanVal += Temperature / 2;
        else
          // blue channel: emphasize in winter
          iChanVal -= Temperature / 2;
      // set channel
      dwClr[i] |= BoundBy<int32_t>(iChanVal, 0, 255) << iChan;
    }
  // apply gamma ramp
  Game.GraphicsSystem.SetGamma(dwClr[0], dwClr[1], dwClr[2], C4GRI_SEASON);
}

void C4Weather::CompileFunc(StdCompiler *pComp) {
  pComp->Value(mkNamingAdapt(Season, "Season", 0));
  pComp->Value(mkNamingAdapt(YearSpeed, "YearSpeed", 0));
  pComp->Value(mkNamingAdapt(SeasonDelay, "SeasonDelay", 0));
  pComp->Value(mkNamingAdapt(Wind, "Wind", 0));
  pComp->Value(mkNamingAdapt(TargetWind, "TargetWind", 0));
  pComp->Value(mkNamingAdapt(Temperature, "Temperature", 0));
  pComp->Value(mkNamingAdapt(TemperatureRange, "TemperatureRange", 30));
  pComp->Value(mkNamingAdapt(Climate, "Climate", 0));
  pComp->Value(mkNamingAdapt(MeteoriteLevel, "MeteoriteLevel", 0));
  pComp->Value(mkNamingAdapt(VolcanoLevel, "VolcanoLevel", 0));
  pComp->Value(mkNamingAdapt(EarthquakeLevel, "EarthquakeLevel", 0));
  pComp->Value(mkNamingAdapt(LightningLevel, "LightningLevel", 0));
  pComp->Value(mkNamingAdapt(NoGamma, "NoGamma", TRUE));
  uint32_t dwGammaDefaults[C4MaxGammaRamps * 3];
  for (int32_t i = 0; i < C4MaxGammaRamps; ++i) {
    dwGammaDefaults[i * 3 + 0] = 0x000000;
    dwGammaDefaults[i * 3 + 1] = 0x808080;
    dwGammaDefaults[i * 3 + 2] = 0xffffff;
  }
  pComp->Value(mkNamingAdapt(mkArrayAdaptM(Game.GraphicsSystem.dwGamma),
                             "Gamma", dwGammaDefaults));
}
