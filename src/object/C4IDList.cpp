/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* At static list of C4IDs */

#include "C4Include.h"
#include "object/C4IDList.h"

#ifndef BIG_C4INCLUDE
#include "object/C4Def.h"
#include "game/C4Application.h"
#include "game/C4Game.h"
#endif

C4IDListChunk::C4IDListChunk() {
  // prepare list
  pNext = NULL;
}

C4IDListChunk::~C4IDListChunk() {
  // empty the list
  Clear();
}

void C4IDListChunk::Clear() {
  // kill all chunks
  C4IDListChunk *pChunk = pNext, *pChunk2;
  while (pChunk) {
    pChunk2 = pChunk->pNext;
    pChunk->pNext = NULL;
    delete pChunk;
    pChunk = pChunk2;
  }
  pNext = NULL;
}

C4IDList::C4IDList() : C4IDListChunk() { Default(); }

C4IDList::C4IDList(const C4IDList &rCopy) : C4IDListChunk() {
  Default();
  *this = rCopy;
}

C4IDList &C4IDList::operator=(const C4IDList &rCopy) {
  // clear previous list
  Clear();
  // copy primary
  memcpy(this, &rCopy, sizeof(C4IDList));
  // copy all chunks
  C4IDListChunk *pTo = this;
  for (C4IDListChunk *pFrom = rCopy.pNext; pFrom; pFrom = pFrom->pNext) {
    C4IDListChunk *pNew = new C4IDListChunk(*pFrom);
    pTo->pNext = pNew;
    pTo = pNew;
  }
  // finalize
  pTo->pNext = NULL;
  return *this;
}

C4IDList::~C4IDList() {
  // destruction is done in chunk
}

void C4IDList::Clear() {
  // inherited
  C4IDListChunk::Clear();
  // reset count
  Count = 0;
}

bool C4IDList::IsClear() const { return !Count; }

C4ID C4IDList::GetID(size_t index, int32_t *ipCount) const {
  // outside list?
  if (!Inside<int32_t>(index, 0, Count - 1)) return C4ID_None;
  // get chunk to query
  const C4IDListChunk *pQueryChunk = this;
  while (index >= C4IDListChunkSize) {
    pQueryChunk = pQueryChunk->pNext;
    index -= C4IDListChunkSize;
  }
  // query it
  if (ipCount) *ipCount = pQueryChunk->Count[index];
  return pQueryChunk->id[index];
}

int32_t C4IDList::GetCount(size_t index) const {
  // outside list?
  if (!Inside<int32_t>(index, 0, Count - 1)) return 0;
  // get chunk to query
  const C4IDListChunk *pQueryChunk = this;
  while (index >= C4IDListChunkSize) {
    pQueryChunk = pQueryChunk->pNext;
    index -= C4IDListChunkSize;
  }
  // query it
  return pQueryChunk->Count[index];
}

bool C4IDList::SetCount(size_t index, int32_t iCount) {
  // outside list?
  if (!Inside<int32_t>(index, 0, Count - 1)) return false;
  // get chunk to set in
  C4IDListChunk *pQueryChunk = this;
  while (index >= C4IDListChunkSize) {
    pQueryChunk = pQueryChunk->pNext;
    index -= C4IDListChunkSize;
  }
  // set it
  pQueryChunk->Count[index] = iCount;
  // success
  return true;
}

int32_t C4IDList::GetIDCount(C4ID c_id, int32_t iZeroDefVal) const {
  // find id
  const C4IDListChunk *pQueryChunk = this;
  size_t cnt = Count, cntl = 0;
  while (cnt--) {
    if (pQueryChunk->id[cntl] == c_id) {
      int32_t iCount = pQueryChunk->Count[cntl];
      return iCount ? iCount : iZeroDefVal;
    }
    if (++cntl == C4IDListChunkSize) {
      pQueryChunk = pQueryChunk->pNext;
      cntl = 0;
    }
  }
  // none found
  return 0;
}

bool C4IDList::SetIDCount(C4ID c_id, int32_t iCount, bool fAddNewID) {
  // find id
  C4IDListChunk *pQueryChunk = this;
  size_t cnt = Count, cntl = 0;
  while (cnt--) {
    if (pQueryChunk->id[cntl] == c_id) {
      pQueryChunk->Count[cntl] = iCount;
      return true;
    }
    if (++cntl == C4IDListChunkSize) {
      pQueryChunk = pQueryChunk->pNext;
      cntl = 0;
    }
  }
  // none found: add new
  if (fAddNewID) {
    // if end was reached, add new chunk
    if (!pQueryChunk) {
      C4IDListChunk *pLast = this;
      while (pLast->pNext) pLast = pLast->pNext;
      pQueryChunk = new C4IDListChunk();
      pLast->pNext = pQueryChunk;
    }
    // set id
    pQueryChunk->id[cntl] = c_id;
    pQueryChunk->Count[cntl] = iCount;
    // count id!
    ++Count;
    // success
    return true;
  }
  // failure
  return false;
}

int32_t C4IDList::GetNumberOfIDs() const { return Count; }

int32_t C4IDList::GetIndex(C4ID c_id) const {
  // find id in list
  const C4IDListChunk *pQueryChunk = this;
  size_t cnt = Count, cntl = 0;
  while (cnt--) {
    if (pQueryChunk->id[cntl] == c_id) return Count - cnt - 1;
    if (++cntl == C4IDListChunkSize) {
      pQueryChunk = pQueryChunk->pNext;
      cntl = 0;
    }
  }
  // not found
  return -1;
}

bool C4IDList::IncreaseIDCount(C4ID c_id, bool fAddNewID, int32_t IncreaseBy,
                               bool fRemoveEmpty) {
  // find id in list
  C4IDListChunk *pQueryChunk = this;
  size_t cnt = Count, cntl = 0;
  while (cnt--) {
    if (pQueryChunk->id[cntl] == c_id) {
      // increase count
      pQueryChunk->Count[cntl] += IncreaseBy;
      // check count
      if (fRemoveEmpty && !pQueryChunk->Count[cntl])
        DeleteItem(Count - cnt - 1);
      // success
      return true;
    }
    if (++cntl == C4IDListChunkSize) {
      pQueryChunk = pQueryChunk->pNext;
      cntl = 0;
    }
  }
  // add desired?
  if (!fAddNewID) return true;
  // add it
  // if end was reached, add new chunk
  if (!pQueryChunk) {
    C4IDListChunk *pLast = this;
    while (pLast->pNext) pLast = pLast->pNext;
    pQueryChunk = new C4IDListChunk();
    pLast->pNext = pQueryChunk;
  }
  // set id
  pQueryChunk->id[cntl] = c_id;
  pQueryChunk->Count[cntl] = IncreaseBy;
  ++Count;
  // success
  return true;
}

// Access by category-sorted index
#ifdef C4ENGINE
C4ID C4IDList::GetID(C4DefList &rDefs, int32_t dwCategory, int32_t index,
                     int32_t *ipCount) const {
  int32_t cindex = -1;
  C4Def *cDef;
  if (ipCount) *ipCount = 0;
  // find id
  const C4IDListChunk *pQueryChunk = this;
  size_t cnt = Count, cntl = 0;
  while (cnt--) {
    if ((dwCategory == C4D_All) ||
        ((cDef = rDefs.ID2Def(pQueryChunk->id[cntl])) &&
         (cDef->Category & dwCategory))) {
      cindex++;
      if (cindex == index) {
        if (ipCount) *ipCount = pQueryChunk->Count[cntl];
        return pQueryChunk->id[cntl];
      }
    }
    if (++cntl == C4IDListChunkSize) {
      pQueryChunk = pQueryChunk->pNext;
      cntl = 0;
    }
  }
  return C4ID_None;
}

int32_t C4IDList::GetCount(C4DefList &rDefs, int32_t dwCategory,
                           int32_t index) const {
  int32_t cindex = -1;
  C4Def *cDef;
  const C4IDListChunk *pQueryChunk = this;
  size_t cnt = Count, cntl = 0;
  while (cnt--) {
    if ((dwCategory == C4D_All) ||
        ((cDef = rDefs.ID2Def(pQueryChunk->id[cntl])) &&
         (cDef->Category & dwCategory))) {
      cindex++;
      if (cindex == index) return pQueryChunk->Count[cntl];
    }
    if (++cntl == C4IDListChunkSize) {
      pQueryChunk = pQueryChunk->pNext;
      cntl = 0;
    }
  }
  return 0;
}

bool C4IDList::SetCount(C4DefList &rDefs, int32_t dwCategory, int32_t index,
                        int32_t iCount) {
  int32_t cindex = -1;
  C4Def *cDef;
  C4IDListChunk *pQueryChunk = this;
  size_t cnt = Count, cntl = 0;
  while (cnt--) {
    if ((dwCategory == C4D_All) ||
        ((cDef = rDefs.ID2Def(pQueryChunk->id[cntl])) &&
         (cDef->Category & dwCategory))) {
      cindex++;
      if (cindex == index) {
        pQueryChunk->Count[cntl] = iCount;
        return true;
      }
    }
    if (++cntl == C4IDListChunkSize) {
      pQueryChunk = pQueryChunk->pNext;
      cntl = 0;
    }
  }
  return false;
}

int32_t C4IDList::GetNumberOfIDs(C4DefList &rDefs, int32_t dwCategory) const {
  int32_t idnum = 0;
  C4Def *cDef;
  const C4IDListChunk *pQueryChunk = this;
  size_t cnt = Count, cntl = 0;
  while (cnt--) {
    if ((dwCategory == C4D_All) ||
        ((cDef = rDefs.ID2Def(pQueryChunk->id[cntl])) &&
         (cDef->Category & dwCategory)))
      idnum++;
    if (++cntl == C4IDListChunkSize) {
      pQueryChunk = pQueryChunk->pNext;
      cntl = 0;
    }
  }
  return idnum;
}
#endif
// IDList merge

bool C4IDList::Add(C4IDList &rList) {
  C4IDListChunk *pQueryChunk = &rList;
  size_t cnt = rList.Count, cntl = 0;
  while (cnt--) {
    IncreaseIDCount(pQueryChunk->id[cntl], true, pQueryChunk->Count[cntl]);
    if (++cntl == C4IDListChunkSize) {
      pQueryChunk = pQueryChunk->pNext;
      cntl = 0;
    }
  }
  return true;
}

// Removes all empty id gaps from the list.

bool C4IDList::Consolidate() {
  // however, there ain't be any of those crappy gaps!
  return false;
}

bool C4IDList::ConsolidateValids(C4DefList &rDefs, int32_t dwCategory) {
  bool fIDsRemoved = false;
  C4IDListChunk *pQueryChunk = this;
  size_t cnt = Count, cntl = 0;
  C4Def *pDef;
  while (cnt--) {
    // ID does not resolve to a valid def or category is specified and def does
    // not match category
    if (!(pDef = rDefs.ID2Def(pQueryChunk->id[cntl])) ||
        (dwCategory && !(pDef->Category & dwCategory))) {
      // delete it
      DeleteItem(Count - cnt - 1);
      // handle this list index again!
      --cntl;
      // something was done
      fIDsRemoved = true;
    }
    if (++cntl == C4IDListChunkSize) {
      pQueryChunk = pQueryChunk->pNext;
      cntl = 0;
    }
  }
  return fIDsRemoved;
}

void C4IDList::SortByCategory(C4DefList &rDefs) {
  bool fBubble;
  size_t cnt;
  C4Def *cdef1, *cdef2;
  do {
    fBubble = false;
    for (cnt = 0; cnt + 1 < Count; cnt++)
      if ((cdef1 = rDefs.ID2Def(GetID(cnt))) &&
          (cdef2 = rDefs.ID2Def(GetID(cnt + 1))))
        if ((cdef1->Category & C4D_SortLimit) <
            (cdef2->Category & C4D_SortLimit)) {
          SwapItems(cnt, cnt + 1);
          fBubble = true;
        }
  } while (fBubble);
}

void C4IDList::SortByValue(C4DefList &rDefs) {
  bool fBubble;
  size_t cnt;
  C4Def *cdef1, *cdef2;
  do {
    fBubble = false;
    for (cnt = 0; cnt + 1 < Count; cnt++)
      if ((cdef1 = rDefs.ID2Def(GetID(cnt))) &&
          (cdef2 = rDefs.ID2Def(GetID(cnt + 1))))
        // FIXME: Should call GetValue here
        if (cdef1->Value > cdef2->Value) {
          SwapItems(cnt, cnt + 1);
          fBubble = true;
        }
  } while (fBubble);
}

void C4IDList::Load(C4DefList &rDefs, int32_t dwCategory) {
  // (deprecated, use StdCompiler instead)
  C4Def *cdef;
  size_t cntl = 0, cnt = 0;
  // clear list
  Clear();
  // add all IDs of def list
  C4IDListChunk *pChunk = this;
  while (cdef = rDefs.GetDef(cnt++, dwCategory)) {
    // add new chunk if necessary
    if (cntl == C4IDListChunkSize) {
      C4IDListChunk *pLast = this;
      while (pLast->pNext) pLast = pLast->pNext;
      pChunk = new C4IDListChunk();
      pLast->pNext = pChunk;
      cntl = 0;
    }
    // set def
    pChunk->id[cntl] = cdef->id;
    pChunk->Count[cntl] = 0;
    // advance in own list
    ++cntl;
    ++Count;
  }
}

bool C4IDList::Write(char *szTarget, bool fValues) const {
  // (deprecated, use StdCompiler instead)
  char buf[50], buf2[5];
  if (!szTarget) return false;
  szTarget[0] = 0;
  const C4IDListChunk *pQueryChunk = this;
  size_t cnt = Count, cntl = 0;
  while (cnt--) {
    GetC4IdText(pQueryChunk->id[cntl], buf2);
    if (fValues)
      sprintf(buf, "%s=%d;", buf2, pQueryChunk->Count[cntl]);
    else
      sprintf(buf, "%s;", buf2);
    SAppend(buf, szTarget);
    if (++cntl == C4IDListChunkSize) {
      pQueryChunk = pQueryChunk->pNext;
      cntl = 0;
    }
  }
  return true;
}

bool C4IDList::Read(const char *szSource, int32_t iDefValue) {
  char buf[50];
  if (!szSource) return false;
  Clear();
  for (int32_t cseg = 0; SCopySegment(szSource, cseg, buf, ';', 50); cseg++) {
    SClearFrontBack(buf);

    int32_t value = iDefValue;
    if (SCharCount('=', buf)) {
      value = strtol(buf + SCharPos('=', buf) + 1, NULL, 10);
      buf[SCharPos('=', buf)] = 0;
      SClearFrontBack(buf);
    }

    if (SLen(buf) == 4)
      if (!SetIDCount(C4Id(buf), value, true)) return false;
  }
  return true;
}

void C4IDList::Draw(C4Facet &cgo, int32_t iSelection, C4DefList &rDefs,
                    DWORD dwCategory, bool fCounts, int32_t iAlign) const {
#ifdef C4ENGINE  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                 // - - - - - - - - - -

  int32_t sections = cgo.GetSectionCount();
  int32_t idnum = GetNumberOfIDs(rDefs, dwCategory);
  int32_t firstid = BoundBy<int32_t>(iSelection - sections / 2, 0,
                                     Max<int32_t>(idnum - sections, 0));
  int32_t idcount;
  C4ID c_id;
  C4Facet cgo2;
  char buf[10];
  for (int32_t cnt = 0;
       (cnt < sections) &&
           (c_id = GetID(rDefs, dwCategory, firstid + cnt, &idcount));
       cnt++) {
    cgo2 = cgo.TruncateSection(iAlign);
    rDefs.Draw(c_id, cgo2, (firstid + cnt == iSelection), 0);
    sprintf(buf, "%dx", idcount);
    if (fCounts)
      Application.DDraw->TextOut(
          buf, Game.GraphicsResource.FontRegular, 1.0, cgo2.Surface,
          cgo2.X + cgo2.Wdt - 1,
          cgo2.Y + cgo2.Hgt - 1 - Game.GraphicsResource.FontRegular.iLineHgt,
          CStdDDraw::DEFAULT_MESSAGE_COLOR, ARight);
  }

#endif  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // - - - -
}

void C4IDList::Default() { Clear(); }

bool C4IDList::ConsolidateCounts() {
  bool fIDsRemoved = false;
  C4IDListChunk *pQueryChunk = this;
  size_t cnt = Count, cntl = 0;
  while (cnt--) {
    if (!pQueryChunk->Count[cntl]) {
      // delete it
      DeleteItem(Count - cnt - 1);
      // handle this list index again!
      --cntl;
      // something was done
      fIDsRemoved = true;
    }
    if (++cntl == C4IDListChunkSize) {
      pQueryChunk = pQueryChunk->pNext;
      cntl = 0;
    }
  }
  return fIDsRemoved;
}

bool C4IDList::SwapItems(size_t iIndex1, size_t iIndex2) {
  // Invalid index
  if (!Inside<int32_t>(iIndex1, 0, Count - 1)) return FALSE;
  if (!Inside<int32_t>(iIndex2, 0, Count - 1)) return FALSE;
  // get first+second chunk and index
  C4IDListChunk *pChunk1 = this;
  while (iIndex1 >= C4IDListChunkSize) {
    pChunk1 = pChunk1->pNext;
    iIndex1 -= C4IDListChunkSize;
  }
  C4IDListChunk *pChunk2 = this;
  while (iIndex2 >= C4IDListChunkSize) {
    pChunk2 = pChunk2->pNext;
    iIndex2 -= C4IDListChunkSize;
  }
  // swap id & count
  int32_t iTemp = pChunk1->Count[iIndex1];
  C4ID idTemp = pChunk1->id[iIndex1];
  pChunk1->Count[iIndex1] = pChunk2->Count[iIndex2];
  pChunk1->id[iIndex1] = pChunk2->id[iIndex2];
  pChunk2->Count[iIndex2] = iTemp;
  pChunk2->id[iIndex2] = idTemp;
  // done
  return true;
}

// Clear index entry and shift all entries behind down by one.

bool C4IDList::DeleteItem(size_t iIndex) {
  // invalid index
  if (!Inside<size_t>(iIndex, 0, Count - 1)) return FALSE;
  // get chunk to delete of
  size_t index = iIndex;
  C4IDListChunk *pQueryChunk = this;
  while (index >= C4IDListChunkSize) {
    pQueryChunk = pQueryChunk->pNext;
    index -= C4IDListChunkSize;
  }
  // shift down all entries behind
  size_t cnt = --Count - iIndex, cntl = index, cntl2 = cntl;
  C4IDListChunk *pNextChunk = pQueryChunk;
  while (cnt--) {
    // check for list overlap
    if (++cntl2 == C4IDListChunkSize) {
      pNextChunk = pQueryChunk->pNext;
      cntl2 = 0;
    }
    // move down
    pQueryChunk->id[cntl] = pNextChunk->id[cntl2];
    pQueryChunk->Count[cntl] = pNextChunk->Count[cntl2];
    // next item
    pQueryChunk = pNextChunk;
    cntl = cntl2;
  }
  // done
  return true;
}

bool C4IDList::operator==(const C4IDList &rhs) const {
  // compare counts
  if (Count != rhs.Count) return false;
  // compare all chunks
  const C4IDListChunk *pChunk1 = this;
  const C4IDListChunk *pChunk2 = &rhs;
  int32_t cnt = Count;
  while (pChunk1 && pChunk2) {
    if (memcmp(pChunk1->id, pChunk2->id,
               sizeof(C4ID) * Min<int32_t>(cnt, C4IDListChunkSize)))
      return false;
    if (memcmp(pChunk1->Count, pChunk2->Count,
               sizeof(int32_t) * Min<int32_t>(cnt, C4IDListChunkSize)))
      return false;
    pChunk1 = pChunk1->pNext;
    pChunk2 = pChunk2->pNext;
    cnt -= C4IDListChunkSize;
  }
  // equal!
  return true;
}

void C4IDList::CompileFunc(StdCompiler *pComp, bool fValues) {
  // Get compiler characteristics
  bool fCompiler = pComp->isCompiler();
  bool fNaming = pComp->hasNaming();
  // Compiling: Clear existing data first
  if (fCompiler) Clear();
  // Start
  C4IDListChunk *pChunk = this;
  size_t iNr = 0, iCNr = 0;
  // Without naming: Compile Count
  int32_t iCount = Count;
  if (!fNaming) pComp->Value(iCount);
  Count = iCount;
  // Read
  for (;;) {
    // Prepare compiling of single mapping
    if (!fCompiler) {
      // End of list?
      if (iNr >= Count) break;
      // End of chunk?
      if (iCNr >= C4IDListChunkSize) {
        pChunk = pChunk->pNext;
        iCNr = 0;
      }
    } else {
      // End of list?
      if (!fNaming)
        if (iNr >= Count) break;
      // End of chunk?
      if (iCNr >= C4IDListChunkSize) {
        pChunk = pChunk->pNext = new C4IDListChunk();
        iCNr = 0;
      }
    }
    // Seperator (';')
    if (iNr > 0)
      if (!pComp->Seperator(StdCompiler::SEP_SEP2)) break;
    // ID
    pComp->Value(mkDefaultAdapt(mkC4IDAdapt(pChunk->id[iCNr]), C4ID_None));
    // ID not valid? Note that C4ID_None is invalid.
    if (!LooksLikeID(pChunk->id[iCNr])) break;
    // Value: Skip this part if requested
    if (fValues) {
      // Seperator ('=')
      if (pComp->Seperator(StdCompiler::SEP_SET))
        // Count
        pComp->Value(mkDefaultAdapt(pChunk->Count[iCNr], 0));
    } else if (fCompiler)
      pChunk->Count[iCNr] = 0;
    // Goto next entry
    iNr++;
    iCNr++;
    // Save back count
    if (fCompiler && fNaming) Count = iNr;
  }
}
