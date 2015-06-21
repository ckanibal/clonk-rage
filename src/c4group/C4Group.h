/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Handles group files */

#ifndef INC_C4Group
#define INC_C4Group

#ifdef HAVE_IO_H
#include <io.h>
#include <unistd.h>
#endif
#include <CStdFile.h>
#include <StdBuf.h>
#include <StdCompiler.h>

// C4Group-Rewind-warning:
// The current C4Group-implementation cannot handle random file access very
// well,
// because all files are written within a single zlib-stream.
// For every out-of-order-file accessed a group-rewind must be performed, and
// every
// single file up to the accessed file unpacked. As a workaround, all C4Groups
// are
// packed in a file order matching the reading order of the engine.
// If the reading order doesn't match the packing order, and a rewind has to be
// performed,
// a warning is issued in Debug-builds of the engine. But since some components
// require
// random access because they are loaded on-demand at runtime (e.g. global
// sounds), the
// warning may be temp disabled for those files using C4GRP_DISABLE_REWINDWARN
// and
// C4GRP_ENABLE_REWINDWARN. A ref counter keeps track of nested calls to those
// functions.
//
// If you add any new components to scenario or definition files, remember to
// adjust the
// sort order lists in C4Components.h accordingly, and enforce a reading order
// for that
// component.
//
// Maybe some day, someone will write a C4Group-implementation that is probably
// capable of
// random access...
#ifdef _DEBUG
extern int iC4GroupRewindFilePtrNoWarn;
#define C4GRP_DISABLE_REWINDWARN ++iC4GroupRewindFilePtrNoWarn;
#define C4GRP_ENABLE_REWINDWARN --iC4GroupRewindFilePtrNoWarn;
#else
#define C4GRP_DISABLE_REWINDWARN ;
#define C4GRP_ENABLE_REWINDWARN ;
#endif

const int C4GroupFileVer1 = 1, C4GroupFileVer2 = 2;

const int C4GroupMaxMaker = 30, C4GroupMaxPassword = 30, C4GroupMaxError = 100;

#define C4GroupFileID "RedWolf Design GrpFolder"

void C4Group_SetMaker(const char *szMaker);
void C4Group_SetPasswords(const char *szPassword);
void C4Group_SetTempPath(const char *szPath);
const char *C4Group_GetTempPath();
void C4Group_SetSortList(const char **ppSortList);
void C4Group_SetProcessCallback(BOOL (*fnCallback)(const char *, int));
BOOL C4Group_IsGroup(const char *szFilename);
BOOL C4Group_CopyItem(const char *szSource, const char *szTarget,
                      bool fNoSort = false, bool fResetAttributes = false);
BOOL C4Group_MoveItem(const char *szSource, const char *szTarget,
                      bool fNoSort = false);
BOOL C4Group_DeleteItem(const char *szItem, bool fRecycle = false);
bool C4Group_PackDirectoryTo(const char *szFilename, const char *szFilenameTo);
bool C4Group_PackDirectory(const char *szFilename);
BOOL C4Group_UnpackDirectory(const char *szFilename);
bool C4Group_ExplodeDirectory(const char *szFilename);
int C4Group_GetCreation(const char *szFilename);
BOOL C4Group_SetOriginal(const char *szFilename, BOOL fOriginal);
bool C4Group_ReadFile(const char *szFilename, char **pData, size_t *iSize);
bool C4Group_GetFileCRC(const char *szFilename, uint32_t *pCRC32);
bool C4Group_GetFileSHA1(const char *szFilename, BYTE *pSHA1);

BOOL EraseItemSafe(const char *szFilename);

extern const char *C4CFN_FLS[];

extern time_t C4Group_AssumeTimeOffset;

#pragma pack(push, 1)

class C4GroupHeader {
 public:
  C4GroupHeader();

 public:
  char id[24 + 4];
  int Ver1, Ver2;
  int Entries;
  char Maker[C4GroupMaxMaker + 2];
  char Password[C4GroupMaxPassword + 2];
  int Creation, Original;
  BYTE fbuf[92];

 public:
  void Init();
};

const char C4GECS_None = 0, C4GECS_Old = 1, C4GECS_New = 2;

class C4GroupEntryCore {
 public:
  C4GroupEntryCore();

 public:
  char FileName[260];
  BOOL Packed, ChildGroup;
  int32_t Size, __Unused, Offset;
  uint32_t Time;
  char HasCRC;
  unsigned int CRC;
  char Executable;
  BYTE fbuf[26];
};

#pragma pack(pop)

const int C4GRES_InGroup = 0, C4GRES_OnDisk = 1, C4GRES_InMemory = 2,
          C4GRES_Deleted = 3;

class C4GroupEntry : public C4GroupEntryCore {
 public:
  C4GroupEntry();
  ~C4GroupEntry();

 public:
  char DiskPath[_MAX_PATH + 1];
  int Status;
  BOOL DeleteOnDisk;
  BOOL HoldBuffer;
  bool BufferIsStdbuf;
  BOOL NoSort;
  BYTE *bpMemBuf;
  C4GroupEntry *Next;

 public:
  void Set(const DirectoryIterator &iter, const char *szPath);
};

const int GRPF_Inactive = 0, GRPF_File = 1, GRPF_Folder = 2;

class C4Group : public CStdStream {
 public:
  C4Group();
  ~C4Group();

 protected:
  int Status;
  char FileName[_MAX_PATH + 1];
  // Parent status
  C4Group *Mother;
  BOOL ExclusiveChild;
  // File & Folder
  C4GroupEntry *SearchPtr;
  CStdFile StdFile;
  size_t iCurrFileSize;  // size of last accessed file
  // File only
  int FilePtr;
  int MotherOffset;
  int EntryOffset;
  BOOL Modified;
  C4GroupHeader Head;
  C4GroupEntry *FirstEntry;
  // Folder only
  // struct _finddata_t Fdt;
  // long hFdt;
  DirectoryIterator FolderSearch;
  C4GroupEntry FolderSearchEntry;
  C4GroupEntry LastFolderSearchEntry;

  BOOL StdOutput;
  BOOL (*fnProcessCallback)(const char *, int);
  char ErrorString[C4GroupMaxError + 1];
  BOOL MadeOriginal;

  BOOL NoSort;  // If this flag is set, all entries will be marked NoSort in
                // AddEntry

 public:
  bool Open(const char *szGroupName, BOOL fCreate = FALSE);
  bool Close();
  bool Save(BOOL fReOpen);
  bool OpenAsChild(C4Group *pMother, const char *szEntryName,
                   BOOL fExclusive = FALSE);
  bool OpenChild(const char *strEntry);
  bool OpenMother();
  bool Add(const char *szFiles);
  bool Add(const char *szFile, const char *szAddAs);
  bool Add(const char *szName, void *pBuffer, int iSize, bool fChild = false,
           bool fHoldBuffer = false, int iTime = 0, bool fExecutable = false);
  bool Add(const char *szName, StdBuf &pBuffer, bool fChild = false,
           bool fHoldBuffer = false, int iTime = 0, bool fExecutable = false);
  bool Add(const char *szName, StdStrBuf &pBuffer, bool fChild = false,
           bool fHoldBuffer = false, int iTime = 0, bool fExecutable = false);
  bool Add(const char *szEntryname, C4Group &hSource);
  bool Merge(const char *szFolders);
  bool Move(const char *szFiles);
  bool Move(const char *szFile, const char *szAddAs);
  bool Extract(const char *szFiles, const char *szExtractTo = NULL,
               const char *szExclude = NULL);
  bool ExtractEntry(const char *szFilename, const char *szExtractTo = NULL);
  bool Delete(const char *szFiles, bool fRecursive = false);
  bool DeleteEntry(const char *szFilename, bool fRecycle = false);
  bool Rename(const char *szFile, const char *szNewName);
  bool Sort(const char *szSortList);
  bool SortByList(const char **ppSortList, const char *szFilename = NULL);
  bool View(const char *szFiles);
  bool GetOriginal();
  bool AccessEntry(const char *szWildCard, size_t *iSize = NULL,
                   char *sFileName = NULL, bool *fChild = NULL);
  bool AccessNextEntry(const char *szWildCard, size_t *iSize = NULL,
                       char *sFileName = NULL, bool *fChild = NULL);
  bool LoadEntry(const char *szEntryName, char **lpbpBuf, size_t *ipSize = NULL,
                 int iAppendZeros = 0);
  bool LoadEntry(const char *szEntryName, StdBuf &Buf);
  bool LoadEntryString(const char *szEntryName, StdStrBuf &Buf);
  bool FindEntry(const char *szWildCard, char *sFileName = NULL,
                 size_t *iSize = NULL, bool *fChild = NULL);
  bool FindNextEntry(const char *szWildCard, char *sFileName = NULL,
                     size_t *iSize = NULL, bool *fChild = NULL,
                     bool fStartAtFilename = false);
  bool Read(void *pBuffer, size_t iSize);
  bool Advance(int iOffset);
#ifdef C4FRONTEND
  // Stuff for the FE
  bool LoadIcon(const char *szEntryname, HICON *lphIcon);
  bool ReadDDB(HBITMAP *lphBitmap, HDC hdc = NULL);
  bool ReadDDBSection(HBITMAP *lphBitmap, HDC hdc, int iSecX, int iSecY,
                      int iSecWdt, int iSecHgt, int iImgWdt = -1,
                      int iImgHgt = -1, BOOL fTransCol = FALSE);
  bool ReadPNGSection(HBITMAP *lphBitmap, HDC hdc, int iSecX, int iSecY,
                      int iSecWdt, int iSecHgt, int iImgWdt = -1,
                      int iImgHgt = -1);
#endif
  void SetMaker(const char *szMaker);
  void SetPassword(const char *szPassword);
  void SetStdOutput(BOOL fStatus);
  void SetProcessCallback(BOOL (*fnCallback)(const char *, int));
  void MakeOriginal(BOOL fOriginal);
  void ResetSearch();
  const char *GetError();
  const char *GetMaker();
  const char *GetPassword();
  const char *GetName();
  StdStrBuf GetFullName() const;
  int EntryCount(const char *szWildCard = NULL);
  int EntrySize(const char *szWildCard = NULL);
  int AccessedEntrySize() {
    return iCurrFileSize;
  }  // retrieve size of last accessed entry
  int EntryTime(const char *szFilename);
  unsigned int EntryCRC32(const char *szWildCard = NULL);
  int GetVersion();
  int GetCreation();
  int GetStatus();
  inline bool IsOpen() { return Status != GRPF_Inactive; }
  C4Group *GetMother();
  inline bool IsPacked() { return Status == GRPF_File; }
  inline bool HasPackedMother() {
    if (!Mother) return false;
    return Mother->IsPacked();
  }
  inline bool SetNoSort(BOOL fNoSort) {
    NoSort = fNoSort;
    return true;
  }
#ifdef _DEBUG
  void PrintInternals(const char *szIndent = NULL);
#endif

 protected:
  void Init();
  void Default();
  void Clear();
  void ProcessOut(const char *szMessage, int iProcess = 0);
  bool EnsureChildFilePtr(C4Group *pChild);
  bool CloseExclusiveMother();
  bool Error(const char *szStatus);
  bool OpenReal(const char *szGroupName);
  bool OpenRealGrpFile();
  bool SetFilePtr(int iOffset);
  bool RewindFilePtr();
  bool AdvanceFilePtr(int iOffset, C4Group *pByChild = NULL);
  bool AddEntry(int status, bool childgroup, const char *fname, long size,
                time_t time, char cCRC, unsigned int iCRC,
                const char *entryname = NULL, BYTE *membuf = NULL,
                bool fDeleteOnDisk = false, bool fHoldBuffer = false,
                bool fExecutable = false, bool fBufferIsStdbuf = false);
  bool AddEntryOnDisk(const char *szFilename, const char *szAddAs = NULL,
                      bool fMove = FALSE);
  bool SetFilePtr2Entry(const char *szName, C4Group *pByChild = NULL);
  bool AppendEntry2StdFile(C4GroupEntry *centry, CStdFile &stdfile);
  C4GroupEntry *GetEntry(const char *szName);
  C4GroupEntry *SearchNextEntry(const char *szName);
  C4GroupEntry *GetNextFolderEntry();
#ifdef C4FRONTEND
  HBITMAP SubReadDDB(HDC hdc, int sx = -1, int sy = -1, int swdt = -1,
                     int shgt = -1, int twdt = -1, int thgt = -1,
                     BOOL transcol = FALSE);
  HBITMAP SubReadPNG(HDC hdc, int sx = -1, int sy = -1, int swdt = -1,
                     int shgt = -1, int twdt = -1, int thgt = -1);
#endif
  bool CalcCRC32(C4GroupEntry *pEntry);
};

#endif
