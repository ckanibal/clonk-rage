/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Buffered fast and network-safe random */

#ifndef INC_C4Random
#define INC_C4Random

#ifndef C4ENGINE
#include <StdRandom.h>
#endif

#ifdef DEBUGREC
#include "control/C4Record.h"
#endif

extern int RandomCount;
extern unsigned int RandomHold;
#ifdef C4ENGINE

inline void FixedRandom(DWORD dwSeed) {
  // for SafeRandom
  srand((unsigned)time(NULL));
  RandomHold = dwSeed;  // srand(dwSeed);
  RandomCount = 0;
}

inline void Randomize() {
  FixedRandom((unsigned)time(NULL));
  // for SafeRandom
  srand((unsigned)time(NULL));
}

inline int Random(int iRange) {
  // next pseudorandom value
  RandomCount++;
#ifdef DEBUGREC
  C4RCRandom rc;
  rc.Cnt = RandomCount;
  rc.Range = iRange;
  if (iRange == 0)
    rc.Val = 0;
  else {
    RandomHold = RandomHold * 214013L + 2531011L;
    rc.Val = (RandomHold >> 16) % iRange;
  }
  AddDbgRec(RCT_Random, &rc, sizeof(rc));
  return rc.Val;
#else
  if (iRange == 0) return 0;
  RandomHold = RandomHold * 214013L + 2531011L;
  return (RandomHold >> 16) % iRange;
#endif
}

inline unsigned int SeededRandom(unsigned int iSeed, unsigned int iRange) {
  if (!iRange) return 0;
  iSeed = iSeed * 214013L + 2531011L;
  return (iSeed >> 16) % iRange;
}

#endif

inline int SafeRandom(int range) {
  if (!range) return 0;
  return rand() % range;
}

void Randomize3();
int Rnd3();

#endif  // INC_C4Random
