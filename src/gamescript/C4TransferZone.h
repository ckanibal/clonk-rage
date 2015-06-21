/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Special regions to extend the pathfinder */

#ifndef INC_C4TransferZone
#define INC_C4TransferZone

class C4TransferZones;

class C4TransferZone {
  friend class C4TransferZones;

 public:
  C4TransferZone();
  ~C4TransferZone();

 public:
  C4Object *Object;
  int32_t X, Y, Wdt, Hgt;
  BOOL Used;

 protected:
  C4TransferZone *Next;

 public:
  BOOL GetEntryPoint(int32_t &rX, int32_t &rY, int32_t iToX, int32_t iToY);
  void Draw(C4FacetEx &cgo, BOOL fHighlight = false);
  void Default();
  void Clear();
  BOOL At(int32_t iX, int32_t iY);
};

class C4TransferZones {
 public:
  C4TransferZones();
  ~C4TransferZones();

 protected:
  int32_t RemoveNullZones();
  C4TransferZone *First;

 public:
  void Default();
  void Clear();
  void ClearUsed();
  void ClearPointers(C4Object *pObj);
  void Draw(C4FacetEx &cgo);
  void Synchronize();
  C4TransferZone *Find(C4Object *pObj);
  C4TransferZone *Find(int32_t iX, int32_t iY);
  BOOL Add(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, C4Object *pObj);
  BOOL Set(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, C4Object *pObj);
};

#endif
