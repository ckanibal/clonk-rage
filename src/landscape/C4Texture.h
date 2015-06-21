/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Textures used by the landscape */

#ifndef INC_C4Texture
#define INC_C4Texture

#include "C4Material.h"
#include "graphics/C4Surface.h"

class C4Texture {
  friend class C4TextureMap;

 public:
  C4Texture();
  ~C4Texture();
  CSurface *Surface32;
  CSurface8 *Surface8;

 protected:
  char Name[C4M_MaxName + 1];
  C4Texture *Next;
};

class C4TexMapEntry {
  friend class C4TextureMap;

 public:
  C4TexMapEntry();

 private:
  StdCopyStrBuf Material, Texture;
  int32_t iMaterialIndex;
  C4Material *pMaterial;
  CPattern MatPattern;

 public:
  bool isNull() const { return Material.isNull(); }
  const char *GetMaterialName() const { return Material.getData(); }
  const char *GetTextureName() const { return Texture.getData(); }
  int32_t GetMaterialIndex() const { return iMaterialIndex; }
  C4Material *GetMaterial() const { return pMaterial; }
  const CPattern &getPattern() const { return MatPattern; }
  void Clear();
  bool Create(const char *szMaterial, const char *szTexture);
  bool Init();
};

class C4TextureMap {
 public:
  C4TextureMap();
  ~C4TextureMap();

 protected:
  C4TexMapEntry Entry[C4M_MaxTexIndex];
  C4Texture *FirstTexture;
  bool fOverloadMaterials;
  bool fOverloadTextures;
  bool fInitialized;  // Set after Init() - newly added entries initialized
                      // automatically
 public:
  bool fEntriesAdded;

 public:
  const C4TexMapEntry *GetEntry(int32_t iIndex) const {
    return Inside<int32_t>(iIndex, 0, C4M_MaxTexIndex - 1) ? &Entry[iIndex]
                                                           : NULL;
  }
  void RemoveEntry(int32_t iIndex) {
    if (Inside<int32_t>(iIndex, 1, C4M_MaxTexIndex - 1)) Entry[iIndex].Clear();
  }
  void Default();
  void Clear();
  void StoreMapPalette(BYTE *bypPalette, C4MaterialMap &rMaterials);
  static BOOL LoadFlags(C4Group &hGroup, const char *szEntryName,
                        BOOL *pOverloadMaterials, BOOL *pOverloadTextures);
  int32_t LoadMap(C4Group &hGroup, const char *szEntryName,
                  BOOL *pOverloadMaterials, BOOL *pOverloadTextures);
  int32_t Init();
  bool SaveMap(C4Group &hGroup, const char *szEntryName);
  int32_t LoadTextures(C4Group &hGroup, C4Group *OverloadFile = 0);
  bool HasTextures(C4Group &hGroup);
  const char *GetTexture(int32_t iIndex);
  void MoveIndex(BYTE byOldIndex, BYTE byNewIndex);  // change index of texture
  int32_t GetIndex(const char *szMaterial, const char *szTexture,
                   BOOL fAddIfNotExist = TRUE,
                   const char *szErrorIfFailed = NULL);
  int32_t GetIndexMatTex(const char *szMaterialTexture,
                         const char *szDefaultTexture = NULL,
                         BOOL fAddIfNotExist = TRUE,
                         const char *szErrorIfFailed = NULL);
  C4Texture *GetTexture(const char *szTexture);
  bool CheckTexture(const char *szTexture);  // return whether texture exists
  BOOL AddEntry(BYTE byIndex, const char *szMaterial, const char *szTexture);

 protected:
  BOOL AddTexture(const char *szTexture, CSurface *sfcSurface);
  BOOL AddTexture(const char *szTexture, CSurface8 *sfcSurface);
};

#endif
