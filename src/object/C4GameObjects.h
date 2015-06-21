/* by Sven2, 2001 */
// game object lists

#ifndef INC_C4GameObjects
#define INC_C4GameObjects

#include "C4ObjectList.h"
#include <gamescript/C4FindObject.h>
#include <object/C4Sector.h>

class C4ObjResort;

// main object list class
class C4GameObjects : public C4NotifyingObjectList {
 public:
  C4GameObjects();   // constructor
  ~C4GameObjects();  // destructor
  void Default();
  void Init(int32_t iWidth, int32_t iHeight);
  void Clear(bool fClearInactive = true);  // clear objects

 private:
  uint32_t LastUsedMarker;  // last used value for C4Object::Marker

 public:
  C4LSectors Sectors;            // section object lists
  C4ObjectList InactiveObjects;  // inactive objects (Status=2)
  C4ObjResort *ResortProc;       // current sheduled user resorts

  BOOL Add(C4Object *nObj);     // add object
  BOOL Remove(C4Object *pObj);  // clear pointers to object

  C4ObjectList &ObjectsAt(int ix, int iy);  // get object list for map pos

  void CrossCheck();  // various collision-checks
  C4Object *AtObject(int ctx, int cty, DWORD &ocf,
                     C4Object *exclude = NULL);  // find object at ctx/cty
  void Synchronize();                            // network synchronization
  uint32_t GetNextMarker();

  C4Object *FindInternal(C4ID id);  // find object in first sector
  virtual C4Object *ObjectPointer(int32_t iNumber);  // object pointer by number
  long ObjectNumber(C4Object *pObj);                 // object number by pointer

  C4ObjectList &ObjectsInt();  // return object list containing system objects

  void PutSolidMasks();
  void RemoveSolidMasks();
  void RecheckSolidMasks();

  int Load(C4Group &hGroup, bool fKeepInactive);
  BOOL Save(const char *szFilename, BOOL fSaveGame, bool fSaveInactive);
  BOOL Save(C4Group &hGroup, BOOL fSaveGame, bool fSaveInactive);

  void UpdateScriptPointers();  // update pointers to C4AulScript *

  void UpdatePos(C4Object *pObj);
  void UpdatePosResort(C4Object *pObj);

  BOOL OrderObjectBefore(C4Object *pObj1,
                         C4Object *pObj2);  // order pObj1 before pObj2
  BOOL OrderObjectAfter(C4Object *pObj1,
                        C4Object *pObj2);  // order pObj1 after pObj2
  void FixObjectOrder();  // Called after loading: Resort any objects that are
                          // out of order
  void
  ResortUnsorted();  // resort any objects with unsorted-flag set into lists
  void ExecuteResorts();  // execute custom resort procs

  void DeleteObjects();  // delete all objects and links

  void ClearDefPointers(C4Def *pDef);  // clear all pointers into definition
  void UpdateDefPointers(
      C4Def *pDef);  // restore any cleared pointers after def reload

  bool ValidateOwners();
  bool AssignInfo();
};

class C4AulFunc;

// sheduled resort holder
class C4ObjResort {
 public:
  C4ObjResort();   // constructor
  ~C4ObjResort();  // destructor

  void Execute();  // do the resort!
  void Sort(C4ObjectLink *pFirst,
            C4ObjectLink *pLast);  // sort list between pFirst and pLast
  void SortObject();               // sort single object within its category

  int Category;          // object category mask to be sorted
  C4AulFunc *OrderFunc;  // function determining new sort order
  C4ObjResort *Next;     // next resort holder
  C4Object *pSortObj,
      *pObjBefore;  // objects that are swapped if no OrderFunc is given
  BOOL fSortAfter;  // if set, the sort object is sorted
};

#endif
