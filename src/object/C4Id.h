/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* 32-bit value to identify object definitions */

#pragma once

#include <StdAdaptors.h>

// Use 64 Bit for C4ID (on x86_64) to pass 64 bit for each script function
// parameter
typedef unsigned long C4ID;

const C4ID C4ID_None = 0x0,
           C4ID_Clonk = 0x4B4E4C43,  // CLNK
    C4ID_Flag = 0x47414C46,          // FLAG
    C4ID_Conkit = 0x544B4E43,        // CNKT
    C4ID_Gold = 0x444C4F47,          // GOLD
    C4ID_Lorry = 0x59524F4C,         // LORY
    C4ID_Meteor = 0x4F54454D,        // METO
    C4ID_Linekit = 0x544B4E4C,       // LNKT
    C4ID_PowerLine = 0x4C525750,     // PWRL
    C4ID_SourcePipe = 0x50495053,    // SPIP
    C4ID_DrainPipe = 0x50495044,     // DPIP
    C4ID_Energy = 0x47524E45,        // ENRG
    C4ID_CnMaterial = 0x544D4E43,    // CNMT
    C4ID_FlagRemvbl = 0x56524746,    // FGRV
    C4ID_TeamHomebase = 0x41424854,  // THBA
    C4ID_Contents = 0x00002710;      // 10001

C4ID C4Id(const char *szId);
void GetC4IdText(C4ID id, char *sBuf);
const char *C4IdText(C4ID id);
BOOL LooksLikeID(const char *szText);
bool LooksLikeID(C4ID id);

// * C4ID Adaptor
struct C4IDAdapt {
  C4ID &rValue;
  explicit C4IDAdapt(C4ID &rValue) : rValue(rValue) {}
  inline void CompileFunc(StdCompiler *pComp) const {
    char cC4ID[5];
    if (pComp->isDecompiler()) GetC4IdText(rValue, cC4ID);

    pComp->Value(mkStringAdapt(cC4ID, 4, StdCompiler::RCT_ID));

    if (pComp->isCompiler()) {
      if (strlen(cC4ID) != 4)
        rValue = 0;
      else
        rValue = C4Id(cC4ID);
    }
  }
  // Operators for default checking/setting
  inline bool operator==(const C4ID &nValue) const { return rValue == nValue; }
  inline C4IDAdapt &operator=(const C4ID &nValue) {
    rValue = nValue;
    return *this;
  }
  // trick g++
  ALLOW_TEMP_TO_REF(C4IDAdapt)
};
inline C4IDAdapt mkC4IDAdapt(C4ID &rValue) { return C4IDAdapt(rValue); }