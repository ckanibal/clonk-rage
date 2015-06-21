/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Handles the sound bank and plays effects using FMOD */

#ifndef INC_C4SoundSystem
#define INC_C4SoundSystem

#include <c4group/C4Group.h>

#ifdef C4SOUND_USE_FMOD
#include "../fmod/fmod.h"
#endif
#ifdef HAVE_LIBSDL_MIXER
#define USE_RWOPS
#include <SDL_mixer.h>
#undef USE_RWOPS
#endif

const int32_t C4MaxSoundName = 100, C4MaxSoundInstances = 20,
              C4NearSoundRadius = 50, C4AudibilityRadius = 700;

class C4SoundInstance;

class C4SoundEffect {
  friend class C4SoundInstance;

 public:
  C4SoundEffect();
  ~C4SoundEffect();

 public:
  char Name[C4MaxSoundName + 1];
  int32_t UsageTime, Instances;
  int32_t SampleRate, Length;
  BOOL Static;
#ifdef C4SOUND_USE_FMOD
  FSOUND_SAMPLE *pSample;
#endif
#ifdef HAVE_LIBSDL_MIXER
  Mix_Chunk *pSample;
#endif
  C4SoundInstance *FirstInst;
  C4SoundEffect *Next;

 public:
  void Clear();
  BOOL Load(const char *szFileName, C4Group &hGroup, BOOL fStatic);
  BOOL Load(BYTE *pData, size_t iDataLen, BOOL fStatic,
            bool fRaw = false);  // load directly from memory
  void Execute();
  C4SoundInstance *New(bool fLoop = false, int32_t iVolume = 100,
                       C4Object *pObj = NULL,
                       int32_t iCustomFalloffDistance = 0);
  C4SoundInstance *GetInstance(C4Object *pObj);
  void ClearPointers(C4Object *pObj);
  int32_t GetStartedInstanceCount(int32_t iX, int32_t iY,
                                  int32_t iRad);  // local
  int32_t GetStartedInstanceCount();              // global
 protected:
  void AddInst(C4SoundInstance *pInst);
  void RemoveInst(C4SoundInstance *pInst);
};

class C4SoundInstance {
  friend class C4SoundEffect;

 protected:
  C4SoundInstance();

 public:
  ~C4SoundInstance();

 protected:
  C4SoundEffect *pEffect;
  int32_t iVolume, iPan, iChannel;
  unsigned long iStarted;
  int32_t iNearInstanceMax;
  bool fLooping;
  C4Object *pObj;
  int32_t iFalloffDistance;
  C4SoundInstance *pNext;

 public:
  C4Object *getObj() const { return pObj; }
  bool isStarted() const { return iChannel != -1; }
  void Clear();
  BOOL Create(C4SoundEffect *pEffect, bool fLoop = false, int32_t iVolume = 100,
              C4Object *pObj = NULL, int32_t iNearInstanceMax = 0,
              int32_t iFalloffDistance = 0);
  BOOL CheckStart();
  BOOL Start();
  BOOL Stop();
  BOOL Playing();
  void Execute();
  void SetVolume(int32_t inVolume) { iVolume = inVolume; }
  void SetPan(int32_t inPan) { iPan = inPan; }
  void SetVolumeByPos(int32_t x, int32_t y);
  void SetObj(C4Object *pnObj) { pObj = pnObj; }
  void ClearPointers(C4Object *pObj);
  bool Inside(int32_t iX, int32_t iY, int32_t iRad);
};

const int32_t SoundUnloadTime = 60, SoundMaxUnloadSize = 100000;

class C4SoundSystem {
 public:
  C4SoundSystem();
  ~C4SoundSystem();
  void Clear();
  void Execute();
  int32_t LoadEffects(C4Group &hGroup, BOOL fStatic = TRUE);
  C4SoundInstance *NewEffect(const char *szSound, bool fLoop = false,
                             int32_t iVolume = 100, C4Object *pObj = NULL,
                             int32_t iCustomFalloffDistance = 0);
  C4SoundInstance *FindInstance(const char *szSound, C4Object *pObj);
  BOOL Init();
  void ClearPointers(C4Object *pObj);

 protected:
  C4Group SoundFile;
  C4SoundEffect *FirstSound;
  void ClearEffects();
  C4SoundEffect *GetEffect(const char *szSound);
  C4SoundEffect *AddEffect(const char *szSound);
  int32_t RemoveEffect(const char *szFilename);
  int32_t EffectInBank(const char *szSound);
};

#endif
