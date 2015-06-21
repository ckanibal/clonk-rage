/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Handles the sound bank and plays effects using DSoundX */

#include "C4Include.h"
#include "platform/C4SoundSystem.h"

#ifndef BIG_C4INCLUDE
#include "C4Random.h"
#include "object/C4Object.h"
#include "game/C4Game.h"
#include "config/C4Config.h"
#include "game/C4Application.h"
#endif

C4SoundEffect::C4SoundEffect()
    : UsageTime(0),
      Instances(0),
#if defined C4SOUND_USE_FMOD || defined HAVE_LIBSDL_MIXER
      pSample(NULL),
#endif
      Static(FALSE),
      Next(NULL),
      FirstInst(NULL) {
  Name[0] = 0;
}

C4SoundEffect::~C4SoundEffect() { Clear(); }

void C4SoundEffect::Clear() {
  while (FirstInst) RemoveInst(FirstInst);
#ifdef C4SOUND_USE_FMOD
  if (pSample) FSOUND_Sample_Free(pSample);
#endif
#ifdef HAVE_LIBSDL_MIXER
  if (pSample) Mix_FreeChunk(pSample);
#endif
#if defined C4SOUND_USE_FMOD || defined HAVE_LIBSDL_MIXER
  pSample = NULL;
#endif
}

BOOL C4SoundEffect::Load(const char *szFileName, C4Group &hGroup,
                         BOOL fStatic) {
  // Sound check
  if (!Config.Sound.RXSound) return FALSE;
  // Locate sound in file
  StdBuf WaveBuffer;
  if (!hGroup.LoadEntry(szFileName, WaveBuffer)) return FALSE;
  // load it from mem
  if (!Load((BYTE *)WaveBuffer.getData(), WaveBuffer.getSize(), fStatic))
    return FALSE;
  // Set name
  SCopy(szFileName, Name, C4MaxSoundName);
  return TRUE;
}

BOOL C4SoundEffect::Load(BYTE *pData, size_t iDataLen, BOOL fStatic,
                         bool fRaw) {
  // Sound check
  if (!Config.Sound.RXSound) return FALSE;
// load directly from memory
#ifdef C4SOUND_USE_FMOD
  int32_t iOptions = FSOUND_NORMAL | FSOUND_2D | FSOUND_LOADMEMORY;
  if (fRaw) iOptions |= FSOUND_LOADRAW;
  if (!(pSample = FSOUND_Sample_Load(FSOUND_UNMANAGED, (const char *)pData,
                                     iOptions, 0, iDataLen))) {
    Clear();
    return FALSE;
  }
  // get length
  int32_t iSamples = FSOUND_Sample_GetLength(pSample);
  int iSampleRate = SampleRate;
  if (!iSamples || !FSOUND_Sample_GetDefaults(pSample, &iSampleRate, 0, 0, 0))
    return FALSE;
  SampleRate = iSampleRate;
  Length = iSamples * 10 / (SampleRate / 100);
#endif
#ifdef HAVE_LIBSDL_MIXER
  // Be paranoid about SDL_Mixer initialisation
  if (!Application.MusicSystem.MODInitialized) {
    Clear();
    return FALSE;
  }
  if (!(pSample = Mix_LoadWAV_RW(SDL_RWFromConstMem(pData, iDataLen), 1))) {
    Clear();
    return FALSE;
  }
  // FIXME: Is this actually correct?
  Length = 1000 * pSample->alen / (44100 * 2);
  SampleRate = 0;
#endif
  *Name = '\0';
  // Set usage time
  UsageTime = Game.Time;
  Static = fStatic;
  return TRUE;
}

void C4SoundEffect::Execute() {
  // check for instances that have stopped and volume changes
  for (C4SoundInstance *pInst = FirstInst; pInst;) {
    C4SoundInstance *pNext = pInst->pNext;
    if (!pInst->Playing())
      RemoveInst(pInst);
    else
      pInst->Execute();
    pInst = pNext;
  }
}

C4SoundInstance *C4SoundEffect::New(bool fLoop, int32_t iVolume, C4Object *pObj,
                                    int32_t iCustomFalloffDistance) {
  // check: too many instances?
  if (!fLoop && Instances >= C4MaxSoundInstances) return NULL;
  // create & init sound instance
  C4SoundInstance *pInst = new C4SoundInstance();
  if (!pInst->Create(this, fLoop, iVolume, pObj, 0, iCustomFalloffDistance)) {
    delete pInst;
    return NULL;
  }
  // add to list
  AddInst(pInst);
  // return
  return pInst;
}

C4SoundInstance *C4SoundEffect::GetInstance(C4Object *pObj) {
  for (C4SoundInstance *pInst = FirstInst; pInst; pInst = pInst->pNext)
    if (pInst->getObj() == pObj) return pInst;
  return NULL;
}

void C4SoundEffect::ClearPointers(C4Object *pObj) {
  for (C4SoundInstance *pInst = FirstInst; pInst; pInst = pInst->pNext)
    pInst->ClearPointers(pObj);
}

int32_t C4SoundEffect::GetStartedInstanceCount(int32_t iX, int32_t iY,
                                               int32_t iRad) {
  int32_t cnt = 0;
  for (C4SoundInstance *pInst = FirstInst; pInst; pInst = pInst->pNext)
    if (pInst->isStarted() && pInst->getObj() && pInst->Inside(iX, iY, iRad))
      cnt++;
  return cnt;
}

int32_t C4SoundEffect::GetStartedInstanceCount() {
  int32_t cnt = 0;
  for (C4SoundInstance *pInst = FirstInst; pInst; pInst = pInst->pNext)
    if (pInst->isStarted() && pInst->Playing() && !pInst->getObj()) cnt++;
  return cnt;
}

void C4SoundEffect::AddInst(C4SoundInstance *pInst) {
  pInst->pNext = FirstInst;
  FirstInst = pInst;
  Instances++;
}
void C4SoundEffect::RemoveInst(C4SoundInstance *pInst) {
  if (pInst == FirstInst)
    FirstInst = pInst->pNext;
  else {
    C4SoundInstance *pPos = FirstInst;
    while (pPos && pPos->pNext != pInst) pPos = pPos->pNext;
    if (pPos) pPos->pNext = pInst->pNext;
  }
  delete pInst;
  Instances--;
}

C4SoundInstance::C4SoundInstance()
    : pEffect(NULL), pNext(NULL), iChannel(-1), iPan(0), iVolume(0) {}

C4SoundInstance::~C4SoundInstance() { Clear(); }

void C4SoundInstance::Clear() {
  Stop();
  iChannel = -1;
}

BOOL C4SoundInstance::Create(C4SoundEffect *pnEffect, bool fLoop,
                             int32_t inVolume, C4Object *pnObj,
                             int32_t inNearInstanceMax,
                             int32_t iFalloffDistance) {
  // Sound check
  if (!Config.Sound.RXSound || !pnEffect) return FALSE;
  // Already playing? Stop
  if (Playing()) {
    Stop();
    return FALSE;
  }
  // Set effect
  pEffect = pnEffect;
  // Set
  iStarted = timeGetTime();
  iVolume = inVolume;
  iPan = 0;
  iChannel = -1;
  iNearInstanceMax = inNearInstanceMax;
  this->iFalloffDistance = iFalloffDistance;
  pObj = pnObj;
  fLooping = fLoop;
  // Start
  Execute();
  // Safe usage
  pEffect->UsageTime = Game.Time;
  return TRUE;
}

BOOL C4SoundInstance::CheckStart() {
  // already started?
  if (isStarted()) return TRUE;
  // don't bother if half the time is up and the sound is not looping
  if (timeGetTime() > iStarted + pEffect->Length / 2 && !fLooping) return FALSE;
  // do near-instances check
  int32_t iNearInstances = pObj
                               ? pEffect->GetStartedInstanceCount(
                                     pObj->x, pObj->y, C4NearSoundRadius)
                               : pEffect->GetStartedInstanceCount();
  // over maximum?
  if (iNearInstances > iNearInstanceMax) return FALSE;
  // Start
  return Start();
}

BOOL C4SoundInstance::Start() {
#ifdef C4SOUND_USE_FMOD
  // Start
  if ((iChannel = FSOUND_PlaySound(FSOUND_FREE, pEffect->pSample)) == -1)
    return FALSE;
  if (!FSOUND_SetLoopMode(iChannel,
                          fLooping ? FSOUND_LOOP_NORMAL : FSOUND_LOOP_OFF)) {
    Stop();
    return FALSE;
  }
  // set position
  if (timeGetTime() > iStarted + 20) {
    int32_t iTime = (timeGetTime() - iStarted) % pEffect->Length;
    FSOUND_SetCurrentPosition(iChannel, iTime / 10 * pEffect->SampleRate / 100);
  }
#elif defined HAVE_LIBSDL_MIXER
  // Be paranoid about SDL_Mixer initialisation
  if (!Application.MusicSystem.MODInitialized) return FALSE;
  if ((iChannel = Mix_PlayChannel(-1, pEffect->pSample, fLooping ? -1 : 0)) ==
      -1)
    return FALSE;
#else
  return FALSE;
#endif
  // Update volume
  Execute();
  return TRUE;
}

BOOL C4SoundInstance::Stop() {
  if (!pEffect) return FALSE;
  // Stop sound
  BOOL fRet = TRUE;
#ifdef C4SOUND_USE_FMOD
  if (Playing()) fRet = !!FSOUND_StopSound(iChannel);
#endif
#ifdef HAVE_LIBSDL_MIXER
  // iChannel == -1 will halt all channels. Is that right?
  if (Playing()) Mix_HaltChannel(iChannel);
#endif
  iChannel = -1;
  iStarted = 0;
  fLooping = false;
  return fRet;
}

BOOL C4SoundInstance::Playing() {
  if (!pEffect) return FALSE;
#ifdef C4SOUND_USE_FMOD
  if (fLooping) return TRUE;
  return isStarted() ? FSOUND_GetCurrentSample(iChannel) == pEffect->pSample
                     : timeGetTime() < iStarted + pEffect->Length;
#endif
#ifdef HAVE_LIBSDL_MIXER
  return Application.MusicSystem.MODInitialized && (iChannel != -1) &&
         Mix_Playing(iChannel);
#endif
  return false;
}

void C4SoundInstance::Execute() {
  // Object deleted?
  if (pObj && !pObj->Status) ClearPointers(pObj);
  // initial values
  int32_t iVol = iVolume * 256 * Config.Sound.SoundVolume / 100,
          iPan = C4SoundInstance::iPan;
  // bound to an object?
  if (pObj) {
    int iAudibility = pObj->Audible;
    // apply custom falloff distance
    if (iFalloffDistance) {
      iAudibility = BoundBy<int32_t>(
          100 + (iAudibility - 100) * C4AudibilityRadius / iFalloffDistance, 0,
          100);
    }
    iVol = iVol * iAudibility / 100;
    iPan += pObj->AudiblePan;
  }
  // sound off?
  if (!iVol) {
    // stop, if started
    if (isStarted()) {
#ifdef C4SOUND_USE_FMOD
      FSOUND_StopSound(iChannel);
#endif
#ifdef HAVE_LIBSDL_MIXER
      Mix_HaltChannel(iChannel);
#endif
      iChannel = -1;
    }
  } else {
    // start
    if (!isStarted())
      if (!CheckStart()) return;
// set volume & panning
#ifdef C4SOUND_USE_FMOD
    FSOUND_SetVolume(iChannel, BoundBy(iVol / 100, 0, 255));
    FSOUND_SetPan(iChannel, BoundBy(256 * (iPan + 100) / 200, 0, 255));
#endif
#ifdef HAVE_LIBSDL_MIXER
    Mix_Volume(iChannel, (iVol * MIX_MAX_VOLUME) / (100 * 256));
    // Mix_SetPanning(iChannel, ((100 + iPan) * 256) / 200, ((100 - iPan) * 256)
    // / 200);
    Mix_SetPanning(iChannel, BoundBy((100 - iPan) * 256 / 100, 0, 255),
                   BoundBy((100 + iPan) * 256 / 100, 0, 255));
#endif
  }
}

void C4SoundInstance::SetVolumeByPos(int32_t x, int32_t y) {
  iVolume = Game.GraphicsSystem.GetAudibility(x, y, &iPan);
}

void C4SoundInstance::ClearPointers(C4Object *pDelete) {
  if (!Playing()) {
    Stop();
    return;
  }
  if (pObj == pDelete) {
    // stop if looping (would most likely loop forever)
    if (fLooping) Stop();
    // otherwise: set volume by last position
    else
      SetVolumeByPos(pObj->x, pObj->y);
    pObj = NULL;
  }
}

bool C4SoundInstance::Inside(int32_t iX, int32_t iY, int32_t iRad) {
  return pObj &&
         (pObj->x - iX) * (pObj->x - iX) + (pObj->y - iY) * (pObj->y - iY) <=
             iRad * iRad;
}

C4SoundSystem::C4SoundSystem() : FirstSound(NULL) {}

C4SoundSystem::~C4SoundSystem() {}

BOOL C4SoundSystem::Init() {
  if (!Application.MusicSystem.MODInitialized &&
      !Application.MusicSystem.InitializeMOD())
    return false;

  // Might be reinitialisation
  ClearEffects();
  // Open sound file
  if (!SoundFile.IsOpen())
    if (!SoundFile.Open(Config.AtExePath(C4CFN_Sound))) return false;
#ifdef HAVE_LIBSDL_MIXER
  Mix_AllocateChannels(C4MaxSoundInstances);
#endif
  return true;
}

void C4SoundSystem::Clear() {
  ClearEffects();
  // Close sound file
  SoundFile.Close();
}

void C4SoundSystem::ClearEffects() {
  // Clear sound bank
  C4SoundEffect *csfx, *next;
  for (csfx = FirstSound; csfx; csfx = next) {
    next = csfx->Next;
    delete csfx;
  }
  FirstSound = NULL;
}

void C4SoundSystem::Execute() {
  // Sound effect statistics & unload check
  C4SoundEffect *csfx, *next = NULL, *prev = NULL;
  for (csfx = FirstSound; csfx; csfx = next) {
    next = csfx->Next;
    // Instance removal check
    csfx->Execute();
    // Unload check
    if (!csfx->Static && Game.Time - csfx->UsageTime > SoundUnloadTime &&
        !csfx->FirstInst) {
      if (prev)
        prev->Next = next;
      else
        FirstSound = next;
      delete csfx;
    } else
      prev = csfx;
  }
}

int32_t C4SoundSystem::EffectInBank(const char *szSound) {
  int32_t iResult = 0;
  C4SoundEffect *pSfx;
  char szName[C4MaxSoundName + 4 + 1];
  // Compose name (with extension)
  SCopy(szSound, szName, C4MaxSoundName);
  DefaultExtension(szName, "wav");
  // Count all matching sounds in bank
  for (pSfx = FirstSound; pSfx; pSfx = pSfx->Next)
    if (WildcardMatch(szName, pSfx->Name)) iResult++;
  return iResult;
}

C4SoundEffect *C4SoundSystem::AddEffect(const char *szSoundName) {
  C4SoundEffect *nsfx;
  // Allocate new bank entry
  if (!(nsfx = new C4SoundEffect)) return NULL;
  // Load sound to entry
  C4GRP_DISABLE_REWINDWARN  // dynamic load; must rewind here :(
      if (!nsfx->Load(szSoundName, SoundFile,
                      FALSE)) if (!nsfx->Load(szSoundName, Game.ScenarioFile,
                                              FALSE)) {
    C4GRP_ENABLE_REWINDWARN delete nsfx;
    return NULL;
  }
  C4GRP_ENABLE_REWINDWARN
  // Add sound to bank
  nsfx->Next = FirstSound;
  FirstSound = nsfx;
  return nsfx;
}

C4SoundEffect *C4SoundSystem::GetEffect(const char *szSndName) {
  C4SoundEffect *pSfx;
  char szName[C4MaxSoundName + 4 + 1];
  int32_t iNumber;
  // Evaluate sound name
  SCopy(szSndName, szName, C4MaxSoundName);
  // Default extension
  DefaultExtension(szName, "wav");
  // Convert old style '*' wildcard to correct '?' wildcard
  // For sound effects, '*' is supposed to match single digits only
  SReplaceChar(szName, '*', '?');
  // Sound with a wildcard: determine number of available matches
  if (SCharCount('?', szName)) {
    // Search global sound file
    if (!(iNumber = SoundFile.EntryCount(szName)))
      // Search scenario local files
      if (!(iNumber = Game.ScenarioFile.EntryCount(szName)))
        // Search bank loaded sounds
        if (!(iNumber = EffectInBank(szName)))
          // None found: failure
          return NULL;
    // Insert index to name
    iNumber = BoundBy(1 + SafeRandom(iNumber), 1, 9);
    SReplaceChar(szName, '?', '0' + iNumber);
  }
  // Find requested sound effect in bank
  for (pSfx = FirstSound; pSfx; pSfx = pSfx->Next)
    if (SEqualNoCase(szName, pSfx->Name)) break;
  // Sound not in bank, try add
  if (!pSfx)
    if (!(pSfx = AddEffect(szName))) return NULL;
  return pSfx;
}

C4SoundInstance *C4SoundSystem::NewEffect(const char *szSndName, bool fLoop,
                                          int32_t iVolume, C4Object *pObj,
                                          int32_t iCustomFalloffDistance) {
  // Sound not active
  if (!Config.Sound.RXSound) return NULL;
  // Get sound
  C4SoundEffect *csfx;
  if (!(csfx = GetEffect(szSndName))) return NULL;
  // Play
  return csfx->New(fLoop, iVolume, pObj, iCustomFalloffDistance);
}

C4SoundInstance *C4SoundSystem::FindInstance(const char *szSndName,
                                             C4Object *pObj) {
  char szName[C4MaxSoundName + 4 + 1];
  // Evaluate sound name (see GetEffect)
  SCopy(szSndName, szName, C4MaxSoundName);
  DefaultExtension(szName, "wav");
  SReplaceChar(szName, '*', '?');
  // Find an effect with a matching instance
  for (C4SoundEffect *csfx = FirstSound; csfx; csfx = csfx->Next)
    if (WildcardMatch(szName, csfx->Name)) {
      C4SoundInstance *pInst = csfx->GetInstance(pObj);
      if (pInst) return pInst;
    }
  return NULL;
}

// LoadEffects will load all sound effects of all known sound types (i.e. *.wav
// and *.ogg)
// To play an ogg effect, however, you will have to specify the extension in the
// sound
// command (e.g. Sound("Hello.ogg")), because all playback functions will
// default to wav only.
// LoadEffects is currently used for static loading from object definitions
// only.

int32_t C4SoundSystem::LoadEffects(C4Group &hGroup, BOOL fStatic) {
  int32_t iNum = 0;
  char szFilename[_MAX_FNAME + 1];
  char szFileType[_MAX_FNAME + 1];
  C4SoundEffect *nsfx;
  // Process segmented list of file types
  for (int32_t i = 0;
       SCopySegment(C4CFN_SoundFiles, i, szFileType, '|', _MAX_FNAME); i++) {
    // Search all sound files in group
    hGroup.ResetSearch();
    while (hGroup.FindNextEntry(szFileType, szFilename))
      // Create and load effect
      if (nsfx = new C4SoundEffect)
        if (nsfx->Load(szFilename, hGroup, fStatic)) {
          // Overload same name effects
          RemoveEffect(szFilename);
          // Add effect
          nsfx->Next = FirstSound;
          FirstSound = nsfx;
          iNum++;
        } else
          delete nsfx;
  }
  return iNum;
}

int32_t C4SoundSystem::RemoveEffect(const char *szFilename) {
  int32_t iResult = 0;
  C4SoundEffect *pNext, *pPrev = NULL;
  for (C4SoundEffect *pSfx = FirstSound; pSfx; pSfx = pNext) {
    pNext = pSfx->Next;
    if (WildcardMatch(szFilename, pSfx->Name)) {
      delete pSfx;
      if (pPrev)
        pPrev->Next = pNext;
      else
        FirstSound = pNext;
      iResult++;
    } else
      pPrev = pSfx;
  }
  return iResult;
}

void C4SoundSystem::ClearPointers(C4Object *pObj) {
  for (C4SoundEffect *pEff = FirstSound; pEff; pEff = pEff->Next)
    pEff->ClearPointers(pObj);
}
