/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Core component of a scenario file */

#ifndef INC_C4Scenario
#define INC_C4Scenario

#include "C4NameList.h"
#include <object/C4IDList.h>

class C4SVal {
 public:
  C4SVal(int32_t std = 0, int32_t rnd = 0, int32_t min = 0, int32_t max = 100);

 public:
  int32_t Std, Rnd, Min, Max;

 public:
  void Default();
  void Set(int32_t std = 0, int32_t rnd = 0, int32_t min = 0,
           int32_t max = 100);
  int32_t Evaluate();
  void CompileFunc(StdCompiler *pComp);
};

inline bool operator==(C4SVal Val1, C4SVal Val2) {
  return MemEqual((void *)&Val1, (void *)&Val2, sizeof(C4SVal));
}

#define C4SGFXMODE_NEWGFX 1
#define C4SGFXMODE_OLDGFX 2

#define C4S_SECTIONLOAD \
  1 /* parts of the C4S that are modifyable for different landcape sections */

// flags for section reloading
#define C4S_SAVE_LANDSCAPE 1
#define C4S_SAVE_OBJECTS 2
#define C4S_KEEP_EFFECTS 4

enum C4SForceFairCrew {
  C4SFairCrew_Free = 0,
  C4SFairCrew_FairCrew = 1,
  C4SFairCrew_NormalCrew = 2,
};

enum C4SFilmMode {
  C4SFilm_None = 0,
  C4SFilm_Normal = 1,
  C4SFilm_Cinematic = 2,
};

class C4SHead {
 public:
  int32_t C4XVer[4];
  char Title[C4MaxTitle + 1];
  char Loader[C4MaxTitle + 1];
  char Font[C4MaxTitle + 1];  // scenario specific font; may be 0
  int32_t Difficulty;
  int32_t EnableUnregisteredAccess;
  int32_t Icon;
  int32_t NoInitialize;
  int32_t MaxPlayer, MinPlayer, MaxPlayerLeague;
  int32_t SaveGame;
  int32_t Replay;
  int32_t Film;
  int32_t DisableMouse;
  int32_t IgnoreSyncChecks;
  int32_t RandomSeed;
  char Engine[C4MaxTitle +
              1];  // Relative filename of engine to be used for this scenario
  char MissionAccess[C4MaxTitle + 1];
  bool NetworkGame;
  bool NetworkRuntimeJoin;
  int32_t ForcedGfxMode;   // 0: free; 1/2: newgfx/oldgfx
  int32_t ForcedFairCrew;  // 0: free; 1: force FairCrew; 2: force normal Crew
                           // (C4SForceFairCrew)
  int32_t FairCrewStrength;
  int32_t ForcedAutoContextMenu;  // -1: Not forced; 0: Force off;  1: Force on
  int32_t ForcedControlStyle;     // -1: Not forced; 0: Force off;  1: Force on
  StdCopyStrBuf Origin;  // original oath and filename to scenario (for records
                         // and savegames)
 public:
  void Default();
  void CompileFunc(StdCompiler *pComp, bool fSection);
};

const int32_t C4S_MaxDefinitions = 10;

class C4SDefinitions {
 public:
  int32_t LocalOnly;
  int32_t AllowUserChange;
  char Definition[C4S_MaxDefinitions][_MAX_PATH + 1];
  C4IDList SkipDefs;

 public:
  void SetModules(const char *szList, const char *szRelativeToPath = NULL,
                  const char *szRelativeToPath2 = NULL);
  bool GetModules(StdStrBuf *psOutModules) const;
  BOOL AssertModules(const char *szPath = NULL, char *sMissing = NULL);
  void Default();
  void CompileFunc(StdCompiler *pComp);
};

class C4SRealism {
 public:
  int32_t ConstructionNeedsMaterial;
  int32_t StructuresNeedEnergy;
  C4IDList ValueOverloads;
  int32_t LandscapePushPull;      // Use new experimental push-pull-algorithms
  int32_t LandscapeInsertThrust;  // Inserted material may thrust material of
                                  // lower density aside
  int32_t
      BaseFunctionality;  // functions a base can fulfill (BASEFUNC_*-constants)
  int32_t BaseRegenerateEnergyPrice;  // price for 100 points of energy

 public:
  void Default();
};

class C4SGame {
 public:
  int32_t Mode;
  int32_t Elimination;
  int32_t EnableRemoveFlag;

  // Player winning - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - - -
  int32_t ValueGain;  // If nonzero and value gain is equal or higher
  // Cooperative game over - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - -
  C4IDList CreateObjects;    // If all these objects exist
  C4IDList ClearObjects;     // If fewer than these objects exist
  C4NameList ClearMaterial;  // If less than these materials exist
  int32_t CooperativeGoal;   // Master selection for Frontend

  C4IDList Goals;
  C4IDList Rules;

  uint32_t FoWColor;  // color of FoW; may contain transparency

  C4SRealism Realism;

 public:
  BOOL IsMelee();
  void ConvertGoals(C4SRealism &rRealism);
  void Default();
  void ClearCooperativeGoals();
  void CompileFunc(StdCompiler *pComp, bool fSection);

 protected:
  void ClearOldGoals();
};

// Game mode

const int32_t C4S_Cooperative = 0, C4S_Melee = 1, C4S_MeleeTeamwork = 2;

// Player elimination

const int32_t C4S_KillTheCaptain = 0, C4S_EliminateCrew = 1,
              C4S_CaptureTheFlag = 2;

// Cooperative goals

const int32_t C4S_NoGoal = 0, C4S_Goldmine = 1, C4S_Monsterkill = 2,
              C4S_ValueGain = 3, C4S_Extended = 4;

// Maximum map player extend factor

const int32_t C4S_MaxMapPlayerExtend = 4;

class C4SPlrStart {
 public:
  C4ID NativeCrew;  // Obsolete
  C4SVal Crew;      // Obsolete
  C4SVal Wealth;
  int32_t Position[2];
  int32_t EnforcePosition;
  C4IDList ReadyCrew;
  C4IDList ReadyBase;
  C4IDList ReadyVehic;
  C4IDList ReadyMaterial;
  C4IDList BuildKnowledge;
  C4IDList HomeBaseMaterial;
  C4IDList HomeBaseProduction;
  C4IDList Magic;

 public:
  void Default();
  bool EquipmentEqual(C4SPlrStart &rhs);
  bool operator==(const C4SPlrStart &rhs);
  void CompileFunc(StdCompiler *pComp);
};

class C4SLandscape {
 public:
  int32_t ExactLandscape;
  C4SVal VegLevel;
  C4IDList Vegetation;
  C4SVal InEarthLevel;
  C4IDList InEarth;
  int32_t BottomOpen, TopOpen;
  int32_t LeftOpen, RightOpen;
  int32_t AutoScanSideOpen;
  char SkyDef[C4MaxDefString + 1];
  int32_t SkyDefFade[6];
  int32_t NoSky;
  int32_t NoScan;
  C4SVal Gravity;
  // Dynamic map
  C4SVal MapWdt, MapHgt, MapZoom;
  C4SVal Amplitude, Phase, Period, Random;
  C4SVal LiquidLevel;
  int32_t MapPlayerExtend;
  C4NameList Layers;
  char Material[C4M_MaxDefName + 1];
  char Liquid[C4M_MaxDefName + 1];
  int32_t KeepMapCreator;     // set if the mapcreator will be needed in the
                              // scenario (for DrawDefMap)
  int32_t SkyScrollMode;      // sky scrolling mode for newgfx
  int32_t NewStyleLandscape;  // if set to 2, the landscape uses up to 125
                              // mat/texture pairs
  int32_t FoWRes;             // chunk size of FoGOfWar
 public:
  void Default();
  void GetMapSize(int32_t &rWdt, int32_t &rHgt, int32_t iPlayerNum);
  void CompileFunc(StdCompiler *pComp);
};

class C4SWeather {
 public:
  C4SVal Climate;
  C4SVal StartSeason, YearSpeed;
  C4SVal Rain, Lightning, Wind;
  char Precipitation[C4M_MaxName + 1];
  int32_t NoGamma;

 public:
  void Default();
  void CompileFunc(StdCompiler *pComp);
};

class C4SAnimals {
 public:
  C4IDList FreeLife;
  C4IDList EarthNest;

 public:
  void Default();
  void CompileFunc(StdCompiler *pComp);
};

class C4SEnvironment {
 public:
  C4IDList Objects;

 public:
  void Default();
  void CompileFunc(StdCompiler *pComp);
};

class C4SDisasters {
 public:
  C4SVal Volcano;
  C4SVal Earthquake;
  C4SVal Meteorite;

 public:
  void Default();
  void CompileFunc(StdCompiler *pComp);
};

class C4Scenario {
 public:
  C4Scenario();

 public:
  C4SHead Head;
  C4SDefinitions Definitions;
  C4SGame Game;
  C4SPlrStart PlrStart[C4S_MaxPlayer];
  C4SLandscape Landscape;
  C4SAnimals Animals;
  C4SWeather Weather;
  C4SDisasters Disasters;
  C4SEnvironment Environment;

 public:
  void SetExactLandscape();
  void Clear();
  void Default();
  BOOL Load(C4Group &hGroup, bool fLoadSection = false);
  BOOL Save(C4Group &hGroup, bool fSaveSection = false);
  void CompileFunc(StdCompiler *pComp, bool fSection);
  int32_t GetMinPlayer();  // will try to determine the minimum player count for
                           // this scenario
 protected:
  BOOL Compile(const char *szSource, bool fLoadSection = false);
  BOOL Decompile(char **ppOutput, int32_t *ipSize, bool fSaveSection = false);
};

class C4ScenarioSection;

#ifdef C4ENGINE  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

extern const char *C4ScenSect_Main;

// ref to one scenario section
class C4ScenarioSection {
 public:
  C4ScenarioSection(char *szName);  // ctor
  ~C4ScenarioSection();             // dtor

 public:
  char *szName;          // section name
  char *szTempFilename;  // filename of data file if in temp dir
  char *szFilename;      // filename of section in scenario file
  bool fModified;  // if set, the file is temp and contains runtime landscape
                   // and/or object data

  C4ScenarioSection *pNext;  // next member of linked list

 public:
  bool ScenarioLoad(char *szFilename);   // called when scenario is loaded:
                                         // extract to temp store
  C4Group *GetGroupfile(C4Group &rGrp);  // get group at section file (returns
                                         // temp group, scenario subgroup or
                                         // scenario group itself)
  bool EnsureTempStore(bool fExtractLandscape,
                       bool fExtractObjects);  // make sure that a temp file is
                                               // created, and nothing is
                                               // modified within the main
                                               // scenario file
};

#endif  // C4ENGINE - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif  // INC_C4Scenario
