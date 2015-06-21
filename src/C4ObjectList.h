/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Dynamic object list */

#ifndef INC_C4ObjectList
#define INC_C4ObjectList

#include "object/C4Id.h"
#include "object/C4Def.h"

class C4Object;
class C4FacetEx;

class C4ObjectLink {
 public:
  C4Object *Obj;
  C4ObjectLink *Prev, *Next;
};

class C4ObjectListChangeListener {
 public:
  virtual void OnObjectRemove(C4ObjectList *pList, C4ObjectLink *pLnk) = 0;
  virtual void OnObjectAdded(C4ObjectList *pList, C4ObjectLink *pLnk) = 0;
  virtual void OnObjectRename(C4ObjectList *pList, C4ObjectLink *pLnk) = 0;
  // virtual void OnObjectReorder(C4ObjectLink * pLnk1, C4ObjectLink * pLnk2) =
  // 0;
  virtual ~C4ObjectListChangeListener() {}
};

extern C4ObjectListChangeListener &ObjectListChangeListener;

class C4ObjectList {
 public:
  C4ObjectList();
  C4ObjectList(const C4ObjectList &List);
  virtual ~C4ObjectList();

  C4ObjectLink *First, *Last;
  int Mass;
  std::list<int32_t> *pEnumerated;

  enum SortType {
    stNone = 0,
    stMain,
    stContents,
    stReverse,
  };

  // An iterator which survives if an object is removed from the list
  class iterator {
   public:
    ~iterator();
    iterator &operator++();    // prefix ++
    iterator operator++(int);  // postfix ++
    iterator(const iterator &iter);
    C4Object *operator*();
    bool operator==(const iterator &iter) const;
    bool operator!=(const iterator &iter) const;

    iterator &operator=(const iterator &iter);

   private:
    explicit iterator(C4ObjectList &List);
    iterator(C4ObjectList &List, C4ObjectLink *pLink);
    C4ObjectList &List;
    C4ObjectLink *pLink;
    iterator *Next;
    friend class C4ObjectList;
  };
  iterator begin();
  const iterator end();

  void SortByCategory();
  void Default();
  void Clear();
  void Sort();
  void Enumerate();
  void Denumerate();
  void Copy(const C4ObjectList &rList);
  void DrawAll(C4FacetEx &cgo,
               int iPlayer = -1);  // draw all objects, including bg
  void DrawIfCategory(C4FacetEx &cgo, int iPlayer, uint32_t dwCat,
                      bool fInvert);  // draw all objects that match dwCat (or
                                      // don't match if fInvert)
  void Draw(C4FacetEx &cgo, int iPlayer = -1);  // draw all objects
  void DrawList(C4Facet &cgo, int iSelection = -1, DWORD dwCategory = C4D_All);
  void DrawIDList(C4Facet &cgo, int iSelection, C4DefList &rDefs,
                  int32_t dwCategory, C4RegionList *pRegions = NULL,
                  int iRegionCom = COM_None, BOOL fDrawOneCounts = TRUE);
  void DrawSelectMark(C4FacetEx &cgo);
  void CloseMenus();
  void UpdateGraphics(bool fGraphicsChanged);
  void UpdateFaces(bool bUpdateShape);
  void SyncClearance();
  void ResetAudibility();
  void UpdateTransferZones();
  void SetOCF();
  void GetIDList(C4IDList &rList, int32_t dwCategory = C4D_All);
  void ClearInfo(C4ObjectInfo *pInfo);
  void ClearDefPointers(C4Def *pDef);  // clear all pointers into definition
  void UpdateDefPointers(
      C4Def *pDef);  // restore any cleared pointers after def reload

  typedef int SortProc(C4Object *, C4Object *);

  BOOL Add(C4Object *nObj, SortType eSort, C4ObjectList *pLstSorted = NULL);
  bool AddSortCustom(C4Object *nObj, SortProc &pSortProc);
  BOOL Remove(C4Object *pObj);

  BOOL AssignInfo();
  BOOL ValidateOwners();
  BOOL AssignPlrViewRange();
  StdStrBuf GetNameList(C4DefList &rDefs, DWORD dwCategory = C4D_All);
  BOOL IsClear() const;
  BOOL ReadEnumerated(const char *szSource);
  BOOL DenumerateRead();
  BOOL Write(char *szTarget);
  void CompileFunc(StdCompiler *pComp, bool fSaveRefs = true,
                   bool fSkipPlayerObjects = false);

  long ObjectNumber(C4Object *pObj);
  bool IsContained(C4Object *pObj);
  int ClearPointers(C4Object *pObj);
  int ObjectCount(C4ID id = C4ID_None, int32_t dwCategory = C4D_All) const;
  int MassCount();
  int ListIDCount(int32_t dwCategory);

  C4Object *Denumerated(C4Object *pObj);
  C4Object *Enumerated(C4Object *pObj);
  virtual C4Object *ObjectPointer(int32_t iNumber);
  C4Object *SafeObjectPointer(int32_t iNumber);
  C4Object *GetObject(int Index = 0);
  C4Object *Find(C4ID id, int iOwner = ANY_OWNER, DWORD dwOCF = OCF_All);
  C4Object *FindOther(C4ID id, int iOwner = ANY_OWNER);

  C4ObjectLink *GetLink(C4Object *pObj);

  C4ID GetListID(int32_t dwCategory, int Index);

  BOOL OrderObjectBefore(C4Object *pObj1,
                         C4Object *pObj2);  // order pObj1 before pObj2
  BOOL OrderObjectAfter(C4Object *pObj1,
                        C4Object *pObj2);  // order pObj1 after pObj2

  BOOL ShiftContents(
      C4Object *pNewFirst);  // cycle list so pNewFirst is at front

  void DeleteObjects();  // delete all objects and links

  void UpdateScriptPointers();  // update pointers to C4AulScript *

  BOOL CheckSort(C4ObjectList *pList);  // check that all objects of this list
                                        // appear in the other list in the same
                                        // order
  void CheckCategorySort();  // assertwhether sorting by category is done right

 protected:
  virtual void InsertLinkBefore(C4ObjectLink *pLink, C4ObjectLink *pBefore);
  virtual void InsertLink(C4ObjectLink *pLink, C4ObjectLink *pAfter);
  virtual void RemoveLink(C4ObjectLink *pLnk);
  iterator *FirstIter;
  iterator *AddIter(iterator *iter);
  void RemoveIter(iterator *iter);

  friend class iterator;
  friend class C4ObjResort;
};

class C4NotifyingObjectList : public C4ObjectList {
 public:
  C4NotifyingObjectList() {}
  C4NotifyingObjectList(const C4NotifyingObjectList &List)
      : C4ObjectList(List) {}
  C4NotifyingObjectList(const C4ObjectList &List) : C4ObjectList(List) {}
  virtual ~C4NotifyingObjectList() {}

 protected:
  virtual void InsertLinkBefore(C4ObjectLink *pLink, C4ObjectLink *pBefore);
  virtual void InsertLink(C4ObjectLink *pLink, C4ObjectLink *pAfter);
  virtual void RemoveLink(C4ObjectLink *pLnk);
  friend class C4ObjResort;
};

// This iterator is used to return objects of same ID and picture as grouped.
// It's taking advantage of the fact that object lists are sorted by ID.
// Used by functions such as C4ObjectList::DrawIDList, or the menu-filling of
// activation/selling menus
class C4ObjectListIterator {
 private:
  C4ObjectList &rList;           // iterated list
  C4ObjectList::iterator pCurr;  // link to last returned object
  // C4ObjectLink *pCurr;
  C4ObjectList::iterator pCurrID;  // link to head of link group with same ID

  C4ObjectListIterator(const C4ObjectListIterator &rCopy);  // no copy ctor
 public:
  C4ObjectListIterator(C4ObjectList &rList)
      : rList(rList), pCurr(rList.end()), pCurrID(rList.begin()) {}  // ctor
  C4Object *GetNext(int32_t *piCount,
                    uint32_t dwCategory =
                        0);  // get next object; return NULL if end is reached
};

#endif
