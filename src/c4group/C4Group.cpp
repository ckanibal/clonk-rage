/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Handles group files */

/* Needs to be compiles as Objective C++ on OS X */

#include "C4Include.h"
#include "c4group/C4Group.h"

#include "C4Components.h"
#include "C4InputValidation.h"

#ifdef _WIN32
#include <sys/utime.h>
#include <shellapi.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <StdPNG.h>
#include "zlib.h"
#include <fcntl.h>
#include <cryptopp/sha.h>

//------------------------------ File Sort Lists
//-------------------------------------------

const char *C4CFN_FLS[] = {
    C4CFN_System,        C4FLS_System,           C4CFN_Mouse,
    C4FLS_Mouse,         C4CFN_Keyboard,         C4FLS_Keyboard,
    C4CFN_Easy,          C4FLS_Easy,             C4CFN_Material,
    C4FLS_Material,      C4CFN_Graphics,         C4FLS_Graphics,
    "Western.c4f",       C4FLS_Western,  // hardcoded stuff for foldermap
    C4CFN_DefFiles,      C4FLS_Def,              C4CFN_PlayerFiles,
    C4FLS_Player,        C4CFN_ObjectInfoFiles,  C4FLS_Object,
    C4CFN_ScenarioFiles, C4FLS_Scenario,         C4CFN_FolderFiles,
    C4FLS_Folder,        C4CFN_ScenarioSections, C4FLS_Section,
    C4CFN_Music,         C4FLS_Music,            NULL,
    NULL};

#ifdef _DEBUG
char *szCurrAccessedEntry = NULL;
int iC4GroupRewindFilePtrNoWarn = 0;
#endif

#ifdef C4ENGINE
#ifdef _DEBUG
//#define C4GROUP_DUMP_ACCESS
#endif
#endif

//---------------------------- Global C4Group_Functions
//-------------------------------------------

char C4Group_Maker[C4GroupMaxMaker + 1] = "";
char C4Group_Passwords[CFG_MaxString + 1] = "";
char C4Group_TempPath[_MAX_PATH + 1] = "";
char C4Group_Ignore[_MAX_PATH + 1] = "cvs;Thumbs.db";
const char **C4Group_SortList = NULL;
time_t C4Group_AssumeTimeOffset = 0;
BOOL (*C4Group_ProcessCallback)(const char *, int) = NULL;

void C4Group_SetProcessCallback(BOOL (*fnCallback)(const char *, int)) {
  C4Group_ProcessCallback = fnCallback;
}

void C4Group_SetSortList(const char **ppSortList) {
  C4Group_SortList = ppSortList;
}

void C4Group_SetMaker(const char *szMaker) {
  if (!szMaker)
    C4Group_Maker[0] = 0;
  else
    SCopy(szMaker, C4Group_Maker, C4GroupMaxMaker);
}

void C4Group_SetPasswords(const char *szPassword) {
  if (!szPassword)
    C4Group_Passwords[0] = 0;
  else
    SCopy(szPassword, C4Group_Passwords, CFG_MaxString);
}

void C4Group_SetTempPath(const char *szPath) {
  if (!szPath || !szPath[0])
    C4Group_TempPath[0] = 0;
  else {
    SCopy(szPath, C4Group_TempPath, _MAX_PATH);
    AppendBackslash(C4Group_TempPath);
  }
}

const char *C4Group_GetTempPath() { return C4Group_TempPath; }

bool C4Group_TestIgnore(const char *szFilename) {
  return *GetFilename(szFilename) == '.' ||
         SIsModule(C4Group_Ignore, GetFilename(szFilename));
}

BOOL C4Group_IsGroup(const char *szFilename) {
  C4Group hGroup;
  if (hGroup.Open(szFilename)) {
    hGroup.Close();
    return TRUE;
  }
  return FALSE;
}

int C4Group_GetCreation(const char *szFilename) {
  int iResult = -1;
  C4Group hGroup;
  if (hGroup.Open(szFilename)) {
    iResult = hGroup.GetCreation();
    hGroup.Close();
  }
  return iResult;
}

BOOL C4Group_SetOriginal(const char *szFilename, BOOL fOriginal) {
  C4Group hGroup;
  if (!hGroup.Open(szFilename)) return FALSE;
  hGroup.MakeOriginal(fOriginal);
  if (!hGroup.Close()) return FALSE;
  return TRUE;
}

BOOL C4Group_CopyItem(const char *szSource, const char *szTarget1, bool fNoSort,
                      bool fResetAttributes) {
  // Parameter check
  char szTarget[_MAX_PATH + 1];
  SCopy(szTarget1, szTarget, _MAX_PATH);
  if (!szSource || !szTarget || !szSource[0] || !szTarget[0]) return FALSE;

  // Backslash terminator indicates target is a path only (append filename)
  if (szTarget[SLen(szTarget) - 1] == DirectorySeparator)
    SAppend(GetFilename(szSource), szTarget);

  // Check for identical source and target
  // Note that attributes aren't reset here
  if (ItemIdentical(szSource, szTarget)) return TRUE;

  // Source and target are simple items
  if (ItemExists(szSource) && CreateItem(szTarget))
    return CopyItem(szSource, szTarget, fResetAttributes);

  // For items within groups, attribute resetting isn't needed, because
  // packing/unpacking will kill all
  // attributes anyway

  // Source & target
  C4Group hSourceParent, hTargetParent;
  char szSourceParentPath[_MAX_PATH + 1], szTargetParentPath[_MAX_PATH + 1];
  GetParentPath(szSource, szSourceParentPath);
  GetParentPath(szTarget, szTargetParentPath);

  // Temp filename
  char szTempFilename[_MAX_PATH + 1];
  SCopy(C4Group_TempPath, szTempFilename, _MAX_PATH);
  SAppend(GetFilename(szSource), szTempFilename);
  MakeTempFilename(szTempFilename);

  // Extract source to temp file
  if (!hSourceParent.Open(szSourceParentPath) ||
      !hSourceParent.Extract(GetFilename(szSource), szTempFilename) ||
      !hSourceParent.Close())
    return FALSE;

  // Move temp file to target
  if (!hTargetParent.Open(szTargetParentPath) ||
      !hTargetParent.SetNoSort(fNoSort) ||
      !hTargetParent.Move(szTempFilename, GetFilename(szTarget)) ||
      !hTargetParent.Close()) {
    EraseItem(szTempFilename);
    return FALSE;
  }

  return TRUE;
}

BOOL C4Group_MoveItem(const char *szSource, const char *szTarget1,
                      bool fNoSort) {
  // Parameter check
  char szTarget[_MAX_PATH + 1];
  SCopy(szTarget1, szTarget, _MAX_PATH);
  if (!szSource || !szTarget || !szSource[0] || !szTarget[0]) return FALSE;

  // Backslash terminator indicates target is a path only (append filename)
  if (szTarget[SLen(szTarget) - 1] == DirectorySeparator)
    SAppend(GetFilename(szSource), szTarget);

  // Check for identical source and target
  if (ItemIdentical(szSource, szTarget)) return TRUE;

  // Source and target are simple items
  if (ItemExists(szSource) && CreateItem(szTarget)) {
    // erase test file, because it may block moving a directory
    EraseItem(szTarget);
    return MoveItem(szSource, szTarget);
  }

  // Source & target
  C4Group hSourceParent, hTargetParent;
  char szSourceParentPath[_MAX_PATH + 1], szTargetParentPath[_MAX_PATH + 1];
  GetParentPath(szSource, szSourceParentPath);
  GetParentPath(szTarget, szTargetParentPath);

  // Temp filename
  char szTempFilename[_MAX_PATH + 1];
  SCopy(C4Group_TempPath, szTempFilename, _MAX_PATH);
  SAppend(GetFilename(szSource), szTempFilename);
  MakeTempFilename(szTempFilename);

  // Extract source to temp file
  if (!hSourceParent.Open(szSourceParentPath) ||
      !hSourceParent.Extract(GetFilename(szSource), szTempFilename) ||
      !hSourceParent.Close())
    return FALSE;

  // Move temp file to target
  if (!hTargetParent.Open(szTargetParentPath) ||
      !hTargetParent.SetNoSort(fNoSort) ||
      !hTargetParent.Move(szTempFilename, GetFilename(szTarget)) ||
      !hTargetParent.Close()) {
    EraseItem(szTempFilename);
    return FALSE;
  }

  // Delete original file
  if (!hSourceParent.Open(szSourceParentPath) ||
      !hSourceParent.DeleteEntry(GetFilename(szSource)) ||
      !hSourceParent.Close())
    return FALSE;

  return TRUE;
}

BOOL C4Group_DeleteItem(const char *szItem, bool fRecycle) {
  // Parameter check
  if (!szItem || !szItem[0]) return FALSE;

  // simple item?
  if (ItemExists(szItem)) {
    if (fRecycle)
      return EraseItemSafe(szItem);
    else
      return EraseItem(szItem);
  }

  // delete from parent
  C4Group hParent;
  char szParentPath[_MAX_PATH + 1];
  GetParentPath(szItem, szParentPath);

  // Delete original file
  if (!hParent.Open(szParentPath) ||
      !hParent.DeleteEntry(GetFilename(szItem), fRecycle) || !hParent.Close())
    return FALSE;

  return TRUE;
}

bool C4Group_PackDirectoryTo(const char *szFilename, const char *szFilenameTo) {
  // Check file type
  if (!DirectoryExists(szFilename)) return false;
  // Target mustn't exist
  if (FileExists(szFilenameTo)) return false;
  // Ignore
  if (C4Group_TestIgnore(szFilename)) return true;
  // Process message
  if (C4Group_ProcessCallback) C4Group_ProcessCallback(szFilename, 0);
  // Create group file
  C4Group hGroup;
  if (!hGroup.Open(szFilenameTo, TRUE)) return false;
  // Add folder contents to group
  DirectoryIterator i(szFilename);
  for (; *i; i++) {
    // Ignore
    if (C4Group_TestIgnore(*i)) continue;
    // Must pack?
    if (DirectoryExists(*i)) {
      // Find temporary filename
      char szTempFilename[_MAX_PATH + 1];
      // At C4Group temp path
      SCopy(C4Group_TempPath, szTempFilename, _MAX_PATH);
      SAppend(GetFilename(*i), szTempFilename, _MAX_PATH);
      // Make temporary filename
      MakeTempFilename(szTempFilename);
      // Pack and move into group
      if (!C4Group_PackDirectoryTo(*i, szTempFilename)) break;
      if (!hGroup.Move(szTempFilename, GetFilename(*i))) {
        EraseFile(szTempFilename);
        break;
      }
    }
    // Add normally otherwise
    else if (!hGroup.Add(*i, NULL))
      break;
  }
  // Something went wrong?
  if (*i) {
    // Close group and remove temporary file
    hGroup.Close();
    EraseItem(szFilenameTo);
    return false;
  }
  // Reset iterator
  i.Reset();
  // Close group
  hGroup.SortByList(C4Group_SortList, szFilename);
  if (!hGroup.Close()) return false;
  // Done
  return true;
}

bool C4Group_PackDirectory(const char *szFilename) {
  // Make temporary filename
  char szTempFilename[_MAX_PATH + 1];
  SCopy(szFilename, szTempFilename, _MAX_PATH);
  MakeTempFilename(szTempFilename);
  // Pack directory
  if (!C4Group_PackDirectoryTo(szFilename, szTempFilename)) return false;
  // Rename folder
  char szTempFilename2[_MAX_PATH + 1];
  SCopy(szFilename, szTempFilename2, _MAX_PATH);
  MakeTempFilename(szTempFilename2);
  if (!RenameFile(szFilename, szTempFilename2)) return false;
  // Name group file
  if (!RenameFile(szTempFilename, szFilename)) return false;
  // Last: Delete folder
  return EraseDirectory(szTempFilename2);
}

BOOL C4Group_UnpackDirectory(const char *szFilename) {
  // Already unpacked: success
  if (DirectoryExists(szFilename)) return TRUE;

  // Not a real file: unpack parent directory first
  char szParentFilename[_MAX_PATH + 1];
  if (!FileExists(szFilename))
    if (GetParentPath(szFilename, szParentFilename))
      if (!C4Group_UnpackDirectory(szParentFilename)) return FALSE;

  // Open group
  C4Group hGroup;
  if (!hGroup.Open(szFilename)) return FALSE;

  // Process message
  if (C4Group_ProcessCallback) C4Group_ProcessCallback(szFilename, 0);

  // Create target directory
  char szFoldername[_MAX_PATH + 1];
  SCopy(szFilename, szFoldername, _MAX_PATH);
  MakeTempFilename(szFoldername);
  if (!CreateDirectory(szFoldername, NULL)) {
    hGroup.Close();
    return FALSE;
  }

  // Extract files to folder
  if (!hGroup.Extract("*", szFoldername)) {
    hGroup.Close();
    return FALSE;
  }

  // Close group
  hGroup.Close();

  // Rename group file
  char szTempFilename[_MAX_PATH + 1];
  SCopy(szFilename, szTempFilename, _MAX_PATH);
  MakeTempFilename(szTempFilename);
  if (!RenameFile(szFilename, szTempFilename)) return FALSE;

  // Rename target directory
  if (!RenameFile(szFoldername, szFilename)) return FALSE;

  // Delete renamed group file
  return EraseItem(szTempFilename);
}

bool C4Group_ExplodeDirectory(const char *szFilename) {
  // Ignore
  if (C4Group_TestIgnore(szFilename)) return TRUE;

  // Unpack this directory
  if (!C4Group_UnpackDirectory(szFilename)) return FALSE;

  // Explode all children
  ForEachFile(szFilename, C4Group_ExplodeDirectory);

  // Success
  return TRUE;
}

bool C4Group_ReadFile(const char *szFile, char **pData, size_t *iSize) {
  // security
  if (!szFile || !pData) return false;
  // get mother path & file name
  char szPath[_MAX_PATH + 1];
  GetParentPath(szFile, szPath);
  const char *pFileName = GetFilename(szFile);
  // open mother group
  C4Group MotherGroup;
  if (!MotherGroup.Open(szPath)) return false;
  // access the file
  size_t iFileSize;
  if (!MotherGroup.AccessEntry(pFileName, &iFileSize)) return false;
  // create buffer
  *pData = new char[iFileSize];
  // read it
  if (!MotherGroup.Read(*pData, iFileSize)) {
    delete[] * pData;
    *pData = NULL;
    return false;
  }
  // ok
  MotherGroup.Close();
  if (iSize) *iSize = iFileSize;
  return true;
}

bool C4Group_GetFileCRC(const char *szFilename, uint32_t *pCRC32) {
  if (!pCRC32) return false;
  // doesn't exist physically?
  char szPath[_MAX_PATH + 1];
  bool fTemporary = false;
  if (FileExists(szFilename))
    SCopy(szFilename, szPath, _MAX_PATH);
  else {
    // Expect file to be packed: Extract to temporary
    SCopy(GetFilename(szFilename), szPath, _MAX_PATH);
    MakeTempFilename(szPath);
    if (!C4Group_CopyItem(szFilename, szPath)) return false;
    fTemporary = true;
  }
  // open file
  CStdFile File;
  if (!File.Open(szFilename)) return false;
  // calculcate CRC
  uint32_t iCRC32 = 0;
  for (;;) {
    // read a chunk of data
    BYTE szData[CStdFileBufSize];
    size_t iSize = 0;
    if (!File.Read(szData, CStdFileBufSize, &iSize))
      if (!iSize) break;
    // update CRC
    iCRC32 = crc32(iCRC32, szData, iSize);
  }
  // close file
  File.Close();
  // okay
  *pCRC32 = iCRC32;
  return true;
}

bool C4Group_GetFileSHA1(const char *szFilename, BYTE *pSHA1) {
  if (!pSHA1) return false;
  // doesn't exist physically?
  char szPath[_MAX_PATH + 1];
  bool fTemporary = false;
  if (FileExists(szFilename))
    SCopy(szFilename, szPath, _MAX_PATH);
  else {
    // Expect file to be packed: Extract to temporary
    SCopy(GetFilename(szFilename), szPath, _MAX_PATH);
    MakeTempFilename(szPath);
    if (!C4Group_CopyItem(szFilename, szPath)) return false;
    fTemporary = true;
  }
  // open file
  CStdFile File;
  if (!File.Open(szFilename)) return false;
  // calculcate CRC
  CryptoPP::SHA hash;
  for (;;) {
    // read a chunk of data
    BYTE szData[CStdFileBufSize];
    size_t iSize = 0;
    if (!File.Read(szData, CStdFileBufSize, &iSize)) {
      if (!iSize) {
        break;
      }
    }
    // update CRC
    hash.Update(szData, iSize);
  }
  // close file
  File.Close();
  // finish calculation
  hash.Final(pSHA1);
  return true;
}

void MemScramble(BYTE *bypBuffer, int iSize) {
  int cnt;
  BYTE temp;
  // XOR deface
  for (cnt = 0; cnt < iSize; cnt++) bypBuffer[cnt] ^= 237;
  // BYTE swap
  for (cnt = 0; cnt + 2 < iSize; cnt += 3) {
    temp = bypBuffer[cnt];
    bypBuffer[cnt] = bypBuffer[cnt + 2];
    bypBuffer[cnt + 2] = temp;
  }
}

//---------------------------------- C4Group
//---------------------------------------------

C4GroupHeader::C4GroupHeader() { ZeroMem(this, sizeof(C4GroupHeader)); }

void C4GroupHeader::Init() {
  SCopy(C4GroupFileID, id, sizeof(id) - 1);
  Ver1 = C4GroupFileVer1;
  Ver2 = C4GroupFileVer2;
  Entries = 0;
  SCopy("New C4Group", Maker, C4GroupMaxMaker);
  Password[0] = 0;
}

C4GroupEntryCore::C4GroupEntryCore() {
  ZeroMem(this, sizeof(C4GroupEntryCore));
}

C4GroupEntry::C4GroupEntry() { ZeroMem(this, sizeof(C4GroupEntry)); }

C4GroupEntry::~C4GroupEntry() {
  if (HoldBuffer)
    if (bpMemBuf) {
      if (BufferIsStdbuf)
        StdBuf::DeletePointer(bpMemBuf);
      else
        delete[] bpMemBuf;
    }
}
#ifdef _WIN32
void C4GroupEntry::Set(const DirectoryIterator &iter, const char *szPath) {
  ZeroMem(this, sizeof(C4GroupEntry));
  SCopy(iter.fdt.name, FileName, _MAX_FNAME);
  Size = iter.fdt.size;
  Time = iter.fdt.time_create;
  // SCopy(szPath,DiskPath,_MAX_PATH-1); AppendBackslash(DiskPath);
  // SAppend(FileName,DiskPath,_MAX_PATH);
  SCopy(*iter, DiskPath, _MAX_PATH - 1);
  Status = C4GRES_OnDisk;
  Packed = FALSE;
  ChildGroup = FALSE;  // FileGroupCheck(DiskPath);
  // Notice folder entries are not checked for ChildGroup status.
  // This would cause extreme performance loss and be good for
  // use in entry list display only.
}
#else
void C4GroupEntry::Set(const DirectoryIterator &iter, const char *path) {
  ZeroMem(this, sizeof(C4GroupEntry));
  SCopy(GetFilename(*iter), FileName, _MAX_FNAME);
  SCopy(*iter, DiskPath, _MAX_PATH - 1);
  struct stat buf;
  if (!stat(DiskPath, &buf)) {
    Size = buf.st_size;
    Time = buf.st_mtime;
  } else
    Size = 0;
  // SCopy(path,DiskPath,_MAX_PATH-1); AppendBackslash(DiskPath);
  // SAppend(FileName,DiskPath,_MAX_PATH);
  Status = C4GRES_OnDisk;
  Packed = FALSE;
  ChildGroup = FALSE;  // FileGroupCheck(DiskPath);
  // Notice folder entries are not checked for ChildGroup status.
  // This would cause extreme performance loss and be good for
  // use in entry list display only.
}
#endif
C4Group::C4Group() {
  Init();
  StdOutput = FALSE;
  fnProcessCallback = NULL;
  MadeOriginal = FALSE;
  NoSort = FALSE;
}

void C4Group::Init() {
  // General
  Status = GRPF_Inactive;
  FileName[0] = 0;
  // Child status
  Mother = NULL;
  ExclusiveChild = FALSE;
  // File only
  FilePtr = 0;
  EntryOffset = 0;
  Modified = FALSE;
  Head.Init();
  FirstEntry = NULL;
  SearchPtr = NULL;
  // Folder only
  // hFdt=-1;
  FolderSearch.Reset();
  // Error status
  SCopy("No Error", ErrorString, C4GroupMaxError);
}

C4Group::~C4Group() { Clear(); }

bool C4Group::Error(const char *szStatus) {
  SCopy(szStatus, ErrorString, C4GroupMaxError);
  return FALSE;
}

const char *C4Group::GetError() { return ErrorString; }

void C4Group::SetStdOutput(BOOL fStatus) { StdOutput = fStatus; }

bool C4Group::Open(const char *szGroupName, BOOL fCreate) {
  if (!szGroupName) return Error("Open: Null filename");
  if (!szGroupName[0]) return Error("Open: Empty filename");

  char szGroupNameN[_MAX_FNAME];
  SCopy(szGroupName, szGroupNameN, _MAX_FNAME);
  // Convert to native path
  SReplaceChar(szGroupNameN, '\\', DirectorySeparator);

  // Real reference
  if (FileExists(szGroupNameN)) {
    // Init
    Init();
    // Open group or folder
    return OpenReal(szGroupNameN);
  }

  // If requested, try creating a new group file
  if (fCreate) {
    CStdFile temp;
    if (temp.Create(szGroupNameN, FALSE)) {
      // Temporary file has been created
      temp.Close();
      // Init
      Init();
      Status = GRPF_File;
      Modified = TRUE;
      SCopy(szGroupNameN, FileName, _MAX_FNAME);
      return TRUE;
    }
  }

  // While not a real reference (child group), trace back to mother group or
  // folder.
  // Open mother and child in exclusive mode.
  char szRealGroup[_MAX_FNAME];
  SCopy(szGroupNameN, szRealGroup, _MAX_FNAME);
  do {
    if (!TruncatePath(szRealGroup)) return Error("Open: File not found");
  } while (!FileExists(szRealGroup));

  // Open mother and child in exclusive mode
  C4Group *pMother;
  if (!(pMother = new C4Group)) return Error("Open: mem");
  pMother->SetStdOutput(StdOutput);
  if (!pMother->Open(szRealGroup)) {
    Clear();
    return Error("Open: Cannot open mother");
  }
  if (!OpenAsChild(pMother, szGroupNameN + SLen(szRealGroup) + 1, TRUE)) {
    Clear();
    return Error("Open:: Cannot open as child");
  }

  // Success
  return true;
}

bool C4Group::OpenReal(const char *szFilename) {
  // Get original filename
  if (!szFilename) return FALSE;
  SCopy(szFilename, FileName, _MAX_FNAME);
  MakeOriginalFilename(FileName);

  // Folder
  if (DirectoryExists(FileName)) {
    // Ignore
    if (C4Group_TestIgnore(szFilename)) return FALSE;
    // OpenReal: Simply set status and return
    Status = GRPF_Folder;
    SCopy("Open directory", Head.Maker, C4GroupMaxMaker);
    ResetSearch();
    // Success
    return TRUE;
  }

  // File: Try reading header and entries
  if (OpenRealGrpFile()) {
    Status = GRPF_File;
    ResetSearch();
    return TRUE;
  } else
    return FALSE;

  return Error("OpenReal: Not a valid group");
}

bool C4Group::OpenRealGrpFile() {
  int cnt, file_entries;
  C4GroupEntryCore corebuf;

  // Open StdFile
  if (!StdFile.Open(FileName, TRUE))
    return Error("OpenRealGrpFile: Cannot open standard file");

  // Read header
  if (!StdFile.Read((BYTE *)&Head, sizeof(C4GroupHeader)))
    return Error("OpenRealGrpFile: Error reading header");
  MemScramble((BYTE *)&Head, sizeof(C4GroupHeader));
  EntryOffset += sizeof(C4GroupHeader);

  // Check Header
  if (!SEqual(Head.id, C4GroupFileID) || (Head.Ver1 != C4GroupFileVer1) ||
      (Head.Ver2 > C4GroupFileVer2))
    return Error("OpenRealGrpFile: Invalid header");

  // Read Entries
  file_entries = Head.Entries;
  Head.Entries = 0;  // Reset, will be recounted by AddEntry
  for (cnt = 0; cnt < file_entries; cnt++) {
    if (!StdFile.Read((BYTE *)&corebuf, sizeof(C4GroupEntryCore)))
      return Error("OpenRealGrpFile: Error reading entries");
    C4InVal::ValidateFilename(corebuf.FileName);  // filename validation:
                                                  // Prevent overwriting of user
                                                  // stuff by malicuous groups
    EntryOffset += sizeof(C4GroupEntryCore);
    if (!AddEntry(C4GRES_InGroup, !!corebuf.ChildGroup, corebuf.FileName,
                  corebuf.Size, corebuf.Time, corebuf.HasCRC, corebuf.CRC,
                  corebuf.FileName, NULL, FALSE, FALSE, !!corebuf.Executable))
      return Error("OpenRealGrpFile: Cannot add entry");
  }

  return true;
}

bool C4Group::AddEntry(int status, bool childgroup, const char *fname,
                       long size, time_t time, char cCRC, unsigned int iCRC,
                       const char *entryname, BYTE *membuf, bool fDeleteOnDisk,
                       bool fHoldBuffer, bool fExecutable,
                       bool fBufferIsStdbuf) {
  // Folder: add file to folder immediately
  if (Status == GRPF_Folder) {
    // Close open StdFile
    StdFile.Close();

    // Get path to target folder file
    char tfname[_MAX_FNAME];
    SCopy(FileName, tfname, _MAX_FNAME);
    AppendBackslash(tfname);
    if (entryname)
      SAppend(entryname, tfname);
    else
      SAppend(GetFilename(fname), tfname);

    switch (status) {
      case C4GRES_OnDisk:  // Copy/move file to folder
        return (CopyItem(fname, tfname) &&
                (!fDeleteOnDisk || EraseItem(fname)));

      case C4GRES_InMemory:  // Save buffer to file in folder
        CStdFile hFile;
        bool fOkay = false;
        if (hFile.Create(tfname, !!childgroup))
          fOkay = !!hFile.Write(membuf, size);
        hFile.Close();

        if (fHoldBuffer)
          if (fBufferIsStdbuf)
            StdBuf::DeletePointer(membuf);
          else
            delete[] membuf;

        return fOkay;

        // InGrp & Deleted ignored
    }

    return Error("Add to folder: Invalid request");
  }

  // Group file: add to virtual entry list

  C4GroupEntry *nentry, *lentry, *centry;

  // Delete existing entries of same name
  centry = GetEntry(GetFilename(entryname ? entryname : fname));
  if (centry) {
    centry->Status = C4GRES_Deleted;
    Head.Entries--;
  }

  // Allocate memory for new entry
  if (!(nentry = new C4GroupEntry))
    return FALSE;  //...theoretically, delete Hold buffer here

  // Find end of list
  for (lentry = FirstEntry; lentry && lentry->Next; lentry = lentry->Next)
    ;

  // Init entry core data
  if (entryname)
    SCopy(entryname, nentry->FileName, _MAX_FNAME);
  else
    SCopy(GetFilename(fname), nentry->FileName, _MAX_FNAME);
  nentry->Size = size;
  nentry->Time = time + C4Group_AssumeTimeOffset;
  nentry->ChildGroup = childgroup;
  nentry->Offset = 0;
  nentry->HasCRC = cCRC;
  nentry->CRC = iCRC;
  nentry->Executable = fExecutable;
  nentry->DeleteOnDisk = fDeleteOnDisk;
  nentry->HoldBuffer = fHoldBuffer;
  nentry->BufferIsStdbuf = fBufferIsStdbuf;
  if (lentry) nentry->Offset = lentry->Offset + lentry->Size;

  // Init list entry data
  SCopy(fname, nentry->DiskPath, _MAX_FNAME);
  nentry->Status = status;
  nentry->bpMemBuf = membuf;
  nentry->Next = NULL;
  nentry->NoSort = NoSort;

  // Append entry to list
  if (lentry)
    lentry->Next = nentry;
  else
    FirstEntry = nentry;

  // Increase virtual file count of group
  Head.Entries++;

  return TRUE;
}

C4GroupEntry *C4Group::GetEntry(const char *szName) {
  if (Status == GRPF_Folder) return NULL;
  C4GroupEntry *centry;
  for (centry = FirstEntry; centry; centry = centry->Next)
    if (centry->Status != C4GRES_Deleted)
      if (WildcardMatch(szName, centry->FileName)) return centry;
  return NULL;
}

bool C4Group::Close() {
  C4GroupEntry *centry;
  BOOL fRewrite = FALSE;

  if (Status == GRPF_Inactive) return FALSE;

  // Folder: just close
  if (Status == GRPF_Folder) {
    CloseExclusiveMother();
    Clear();
    return TRUE;
  }

  // Rewrite check
  for (centry = FirstEntry; centry; centry = centry->Next)
    if (centry->Status != C4GRES_InGroup) fRewrite = TRUE;
  if (Modified) fRewrite = TRUE;
  // if (Head.Entries==0) fRewrite=TRUE;

  // No rewrite: just close
  if (!fRewrite) {
    CloseExclusiveMother();
    Clear();
    return TRUE;
  }

  if (StdOutput) printf("Writing group file...\n");

  // Set new version
  Head.Ver1 = C4GroupFileVer1;
  Head.Ver2 = C4GroupFileVer2;

  // Creation stamp
  Head.Creation = time(NULL);

  // Lose original on any save unless made in this session
  if (!MadeOriginal) Head.Original = 0;

  // Automatic maker
  if (C4Group_Maker[0]) SCopy(C4Group_Maker, Head.Maker, C4GroupMaxMaker);

  // Automatic sort
  SortByList(C4Group_SortList);

  // Calculate all missing checksums
  EntryCRC32(NULL);

  // Save group contents to disk
  BOOL fSuccess = Save(FALSE);

  // Close exclusive mother
  CloseExclusiveMother();

  // Close file
  Clear();

  return !!fSuccess;
}

bool C4Group::Save(BOOL fReOpen) {
  int cscore;
  C4GroupEntryCore *save_core;
  C4GroupEntry *centry;
  char szTempFileName[_MAX_FNAME + 1], szGrpFileName[_MAX_FNAME + 1];

  // Create temporary core list with new actual offsets to be saved
  save_core = new C4GroupEntryCore[Head.Entries];
  cscore = 0;
  for (centry = FirstEntry; centry; centry = centry->Next)
    if (centry->Status != C4GRES_Deleted) {
      save_core[cscore] = (C4GroupEntryCore)*centry;
      // Make actual offset
      save_core[cscore].Offset = 0;
      if (cscore > 0)
        save_core[cscore].Offset =
            save_core[cscore - 1].Offset + save_core[cscore - 1].Size;
      cscore++;
    }

  // Create target temp file (in working directory!)
  SCopy(FileName, szGrpFileName, _MAX_FNAME);
  SCopy(GetFilename(FileName), szTempFileName, _MAX_FNAME);
  MakeTempFilename(szTempFileName);
  // (Temp file must not have the same name as the group.)
  if (SEqual(szTempFileName, szGrpFileName)) {
    SAppend(".tmp", szTempFileName);  // Add a second temp extension
    MakeTempFilename(szTempFileName);
  }

  // Create the new (temp) group file
  CStdFile tfile;
  if (!tfile.Create(szTempFileName, TRUE)) {
    delete[] save_core;
    return Error("Close: ...");
  }

  // Save header and core list
  C4GroupHeader headbuf = Head;
  MemScramble((BYTE *)&headbuf, sizeof(C4GroupHeader));
  if (!tfile.Write((BYTE *)&headbuf, sizeof(C4GroupHeader)) ||
      !tfile.Write((BYTE *)save_core,
                   Head.Entries * sizeof(C4GroupEntryCore))) {
    tfile.Close();
    delete[] save_core;
    return Error("Close: ...");
  }
  delete[] save_core;

  // Save Entries to temp file
  int iTotalSize = 0, iSizeDone = 0;
  for (centry = FirstEntry; centry; centry = centry->Next)
    iTotalSize += centry->Size;
  for (centry = FirstEntry; centry; centry = centry->Next)
    if (AppendEntry2StdFile(centry, tfile)) {
      iSizeDone += centry->Size;
      if (iTotalSize && fnProcessCallback)
        fnProcessCallback(centry->FileName, 100 * iSizeDone / iTotalSize);
    } else {
      tfile.Close();
      return FALSE;
    }
  tfile.Close();

  // Child: move temp file to mother
  if (Mother) {
    if (!Mother->Move(szTempFileName, GetFilename(FileName))) {
      CloseExclusiveMother();
      Clear();
      return Error("Close: Cannot move rewritten child temp file to mother");
    }
    StdFile.Close();
    return TRUE;
  }

  // Clear (close file)
  Clear();

  // Delete old group file, rename new file
  if (!EraseFile(szGrpFileName)) return Error("Close: Cannot erase temp file");
  if (!RenameFile(szTempFileName, szGrpFileName))
    return Error("Close: Cannot rename group file");

  // Should reopen the file?
  if (fReOpen) OpenReal(szGrpFileName);

  return TRUE;
}

void C4Group::Default() {
  FirstEntry = NULL;
  StdFile.Default();
  Mother = NULL;
  ExclusiveChild = 0;
  // hFdt = -1;
  Init();
}

void C4Group::Clear() {
  // Delete entries
  C4GroupEntry *next;
  while (FirstEntry) {
    next = FirstEntry->Next;
    delete FirstEntry;
    FirstEntry = next;
  }
  // Close std file
  StdFile.Close();
  // Delete mother
  if (Mother && ExclusiveChild) {
    delete Mother;
    Mother = NULL;
  }
  // done in init
  // FolderSearch.Reset();
  // Reset
  Init();
}

bool C4Group::AppendEntry2StdFile(C4GroupEntry *centry, CStdFile &hTarget) {
  CStdFile hSource;
  long csize;
  BYTE fbuf;

  switch (centry->Status) {
    case C4GRES_InGroup:  // Copy from group to std file
      if (!SetFilePtr(centry->Offset))
        return Error("AE2S: Cannot set file pointer");
      for (csize = centry->Size; csize > 0; csize--) {
        if (!Read(&fbuf, 1))
          return Error("AE2S: Cannot read entry from group file");
        if (!hTarget.Write(&fbuf, 1))
          return Error("AE2S: Cannot write to target file");
      }
      break;

    case C4GRES_OnDisk:  // Copy/move from disk item to std file
    {
      char szFileSource[_MAX_FNAME + 1];
      SCopy(centry->DiskPath, szFileSource, _MAX_FNAME);

      // Disk item is a directory
      if (DirectoryExists(centry->DiskPath))
        return Error("AE2S: Cannot add directory to group file");
      /*{
      if (StdOutput) printf("Adding directory %s to group
      file...\n",centry->FileName);
      // Temporary file name
      MakeTempFilename(szFileSource);
      // Create temporary copy of directory
      if (!CopyItem(centry->DiskPath,szFileSource)) return Error("Cannot create
      temporary directory");
      // Convert directory to temporary group file
      if (!Folder2Group(szFileSource)) return Error("Cannot convert directory to
      group file");
      }*/

      // Resort group if neccessary
      // (The group might be renamed by adding, forcing a resort)
      bool fTempFile = false;
      if (centry->ChildGroup)
        if (!centry->NoSort)
          if (!SEqual(GetFilename(szFileSource), centry->FileName)) {
            // copy group
            MakeTempFilename(szFileSource);
            if (!CopyItem(centry->DiskPath, szFileSource))
              return Error("AE2S: Cannot copy item");
            // open group and resort
            C4Group SortGrp;
            if (!SortGrp.Open(szFileSource))
              return Error("AE2S: Cannot open group");
            if (!SortGrp.SortByList(C4Group_SortList, centry->FileName))
              return Error("AE2S: Cannot resort group");
            fTempFile = true;
            // close group (won't be saved if the sort didn't change)
            SortGrp.Close();
          }

      // Append disk source to target file
      if (!hSource.Open(szFileSource, !!centry->ChildGroup))
        return Error("AE2S: Cannot open on-disk file");
      for (csize = centry->Size; csize > 0; csize--) {
        if (!hSource.Read(&fbuf, 1)) {
          hSource.Close();
          return Error("AE2S: Cannot read on-disk file");
        }
        if (!hTarget.Write(&fbuf, 1)) {
          hSource.Close();
          return Error("AE2S: Cannot write to target file");
        }
      }
      hSource.Close();

      // Erase temp file
      if (fTempFile) EraseItem(szFileSource);
      // Erase disk source if requested
      if (centry->DeleteOnDisk) EraseItem(centry->DiskPath);

      break;
    }

    case C4GRES_InMemory:  // Copy from mem to std file
      // if (StdOutput) printf("Saving InMem entry %d...\n",centry->Size);
      if (!centry->bpMemBuf) return Error("AE2S: no buffer");
      if (!hTarget.Write(centry->bpMemBuf, centry->Size))
        return Error("AE2S: writing error");
      break;

    case C4GRES_Deleted:  // Don't save
      break;

    default:  // Unknown file status
      return Error("AE2S: Unknown file status");
  }

  return TRUE;
}

void C4Group::ResetSearch() {
  switch (Status) {
    case GRPF_Folder:
      SearchPtr = NULL;
      FolderSearch.Reset(FileName);
      if (*FolderSearch) {
        FolderSearchEntry.Set(FolderSearch, FileName);
        SearchPtr = &FolderSearchEntry;
      }
      break;
    case GRPF_File:
      SearchPtr = FirstEntry;
      break;
  }
}

C4GroupEntry *C4Group::GetNextFolderEntry() {
  if (*++FolderSearch) {
    FolderSearchEntry.Set(FolderSearch, FileName);
    return &FolderSearchEntry;
  } else {
    return NULL;
  }
}

C4GroupEntry *C4Group::SearchNextEntry(const char *szName) {
  // Wildcard "*.*" is expected to find all files: substitute correct wildcard
  // "*"
  if (SEqual(szName, "*.*")) szName = "*";
  // Search by group type
  C4GroupEntry *pEntry;
  switch (Status) {
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - -
    case GRPF_File:
      for (pEntry = SearchPtr; pEntry; pEntry = pEntry->Next)
        if (pEntry->Status != C4GRES_Deleted)
          if (WildcardMatch(szName, pEntry->FileName)) {
            SearchPtr = pEntry->Next;
            return pEntry;
          }
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - -
    case GRPF_Folder:
      for (pEntry = SearchPtr; pEntry; pEntry = GetNextFolderEntry())
        if (WildcardMatch(szName, pEntry->FileName))
          if (!C4Group_TestIgnore(pEntry->FileName)) {
            LastFolderSearchEntry = (*pEntry);
            pEntry = &LastFolderSearchEntry;
            SearchPtr = GetNextFolderEntry();
            return pEntry;
          }
      break;
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // - - - - - - - - - - - - - - - -
  }
  // No entry found: reset search pointer
  SearchPtr = NULL;
  return NULL;
}

bool C4Group::SetFilePtr(int iOffset) {
  if (Status == GRPF_Folder)
    return Error("SetFilePtr not implemented for Folders");

  // ensure mother is at correct pos
  if (Mother) Mother->EnsureChildFilePtr(this);

  // Rewind if necessary
  if (FilePtr > iOffset)
    if (!RewindFilePtr()) return FALSE;

  // Advance to target pointer
  if (FilePtr < iOffset)
    if (!AdvanceFilePtr(iOffset - FilePtr)) return FALSE;

  return TRUE;
}

bool C4Group::Advance(int iOffset) {
  if (Status == GRPF_Folder) return !!StdFile.Advance(iOffset);
  // FIXME: reading the file one byte at a time sounds just slow.
  BYTE buf;
  for (; iOffset > 0; iOffset--)
    if (!Read(&buf, 1)) return FALSE;
  return TRUE;
}

bool C4Group::Read(void *pBuffer, size_t iSize) {
  switch (Status) {
    case GRPF_File:
      // Child group: read from mother group
      if (Mother) {
        if (!Mother->Read(pBuffer, iSize)) {
          RewindFilePtr();
          return Error("Read:");
        }
      }
      // Regular group: read from standard file
      else {
        if (!StdFile.Read(pBuffer, iSize)) {
          RewindFilePtr();
          return Error("Read:");
        }
      }
      FilePtr += iSize;
      break;
    case GRPF_Folder:
      if (!StdFile.Read(pBuffer, iSize))
        return Error("Read: Error reading from folder contents");
      break;
  }

  return TRUE;
}

bool C4Group::AdvanceFilePtr(int iOffset, C4Group *pByChild) {
  // Child group file: pass command to mother
  if ((Status == GRPF_File) && Mother) {
    // Ensure mother file ptr for it may have been moved by foreign access to
    // mother
    if (!Mother->EnsureChildFilePtr(this)) return FALSE;

    if (!Mother->AdvanceFilePtr(iOffset, this)) return FALSE;

  }
  // Regular group
  else if (Status == GRPF_File) {
    if (!StdFile.Advance(iOffset)) return FALSE;
  }
  // Open folder
  else {
    if (!StdFile.Advance(iOffset)) return FALSE;
  }

  // Advanced
  FilePtr += iOffset;

  return TRUE;
}

bool C4Group::RewindFilePtr() {
#ifdef _DEBUG
#ifdef C4ENGINE
  if (szCurrAccessedEntry && !iC4GroupRewindFilePtrNoWarn) {
    sprintf(OSTR, "C4Group::RewindFilePtr() for %s (%s)",
            szCurrAccessedEntry ? szCurrAccessedEntry : "???", FileName);
    szCurrAccessedEntry = NULL;
    Log(OSTR);
  }
#endif
#endif

  // Child group file: pass command to mother
  if ((Status == GRPF_File) && Mother) {
    if (!Mother->SetFilePtr2Entry(FileName, this))  // Set to group file start
      return FALSE;
    if (!Mother->AdvanceFilePtr(EntryOffset, this))  // Advance data offset
      return FALSE;
  }
  // Regular group or open folder: rewind standard file
  else {
    if (!StdFile.Rewind())  // Set to group file start
      return FALSE;
    if (!StdFile.Advance(EntryOffset))  // Advance data offset
      return FALSE;
  }

  FilePtr = 0;

  return TRUE;
}

bool C4Group::View(const char *szFiles) {
  char oformat[100];
  C4GroupEntry *centry;
  int fcount = 0, bcount = 0;  // Virtual counts
  int maxfnlen = 0;

  if (!StdOutput) return FALSE;

  // Calculate group file crc
  uint32_t crc = 0;
  C4Group_GetFileCRC(GetFullName().getData(), &crc);

  // Display list
  ResetSearch();
  while (centry = SearchNextEntry(szFiles)) {
    fcount++;
    bcount += centry->Size;
    maxfnlen = Max(maxfnlen, SLen(centry->FileName));
  }
  sprintf(
      oformat,
      "%%%ds %%8ld Bytes %%02d.%%02d.%%02d %%02d:%%02d:%%02d %%s%%08X %%s\n",
      maxfnlen);

  printf("Maker: %s  Creation: %i  %s\n\rVersion: %d.%d  CRC: %u (%X)\n",
         GetMaker(), Head.Creation, GetOriginal() ? "Original" : "", Head.Ver1,
         Head.Ver2, crc, crc);
  ResetSearch();
  while (centry = SearchNextEntry(szFiles)) {
    // convert centry->Time into time_t for localtime
    time_t cur_time = centry->Time;
    tm *pcoretm = localtime(&cur_time);
    tm coretm;
    if (pcoretm)
      coretm = *pcoretm;
    else
      printf("(invalid timestamp) ");
    centry->Time = cur_time;

    printf(oformat, centry->FileName, centry->Size, coretm.tm_mday,
           coretm.tm_mon + 1, coretm.tm_year % 100, coretm.tm_hour,
           coretm.tm_min, coretm.tm_sec,
           centry->HasCRC ? ((centry->HasCRC == C4GECS_New) ? "!" : "~") : " ",
           centry->HasCRC ? centry->CRC : 0,
           centry->ChildGroup ? "(Group)"
                              : (centry->Executable ? "(Executable)" : ""));
  }
  printf("%d Entries, %d Bytes\n", fcount, bcount);

  return TRUE;
}

bool C4Group::Merge(const char *szFolders) {
  bool fMove = true;

  if (StdOutput) printf("%s...\n", fMove ? "Moving" : "Adding");

  // Add files & directories
  char szFileName[_MAX_FNAME + 1];
  int iFileCount = 0;
  DirectoryIterator i;

  // Process segmented path & search wildcards
  char cSeparator = (SCharCount(';', szFolders) ? ';' : '|');
  for (int cseg = 0;
       SCopySegment(szFolders, cseg, szFileName, cSeparator, _MAX_FNAME);
       cseg++) {
    i.Reset(szFileName);
    while (*i) {
      // File count
      iFileCount++;
      // Process output & callback
      if (StdOutput) printf("%s\n", GetFilename(*i));
      if (fnProcessCallback)
        fnProcessCallback(GetFilename(*i), 0);  // cbytes/tbytes
      // AddEntryOnDisk
      AddEntryOnDisk(*i, NULL, fMove);
      ++i;
    }
  }

  if (StdOutput)
    printf("%d file(s) %s.\n", iFileCount, fMove ? "moved" : "added");

  return TRUE;
}

bool C4Group::AddEntryOnDisk(const char *szFilename, const char *szAddAs,
                             bool fMove) {
  // Do not process yourself
  if (ItemIdentical(szFilename, FileName)) return TRUE;

  // File is a directory: copy to temp path, pack, and add packed file
  if (DirectoryExists(szFilename)) {
    // Ignore
    if (C4Group_TestIgnore(szFilename)) return TRUE;
    // Temp filename
    char szTempFilename[_MAX_PATH + 1];
    if (C4Group_TempPath[0]) {
      SCopy(C4Group_TempPath, szTempFilename, _MAX_PATH);
      SAppend(GetFilename(szFilename), szTempFilename, _MAX_PATH);
    } else
      SCopy(szFilename, szTempFilename, _MAX_PATH);
    MakeTempFilename(szTempFilename);
    // Copy or move item to temp file (moved items might be killed if later
    // process fails)
    if (fMove) {
      if (!MoveItem(szFilename, szTempFilename))
        return Error("AddEntryOnDisk: Move failure");
    } else {
      if (!CopyItem(szFilename, szTempFilename))
        return Error("AddEntryOnDisk: Copy failure");
    }
    // Pack temp file
    if (!C4Group_PackDirectory(szTempFilename))
      return Error("AddEntryOnDisk: Pack directory failure");
    // Add temp file
    if (!szAddAs) szAddAs = GetFilename(szFilename);
    szFilename = szTempFilename;
    fMove = TRUE;
  }

  // Determine size
  bool fIsGroup = !!C4Group_IsGroup(szFilename);
  int iSize =
      fIsGroup ? UncompressedFileSize(szFilename) : FileSize(szFilename);

  // Determine executable bit (linux only)
  bool fExecutable = false;
#ifdef __linux__
  fExecutable = (access(szFilename, X_OK) == 0);
#endif

  // AddEntry
  return AddEntry(C4GRES_OnDisk, fIsGroup, szFilename, iSize,
                  FileTime(szFilename), false, 0, szAddAs, NULL, fMove, false,
                  fExecutable);
}

bool C4Group::Add(const char *szFile, const char *szAddAs) {
  bool fMove = false;

  if (StdOutput)
    printf("%s %s as %s...\n", fMove ? "Moving" : "Adding", GetFilename(szFile),
           szAddAs);

  return AddEntryOnDisk(szFile, szAddAs, fMove);
}

bool C4Group::Move(const char *szFile, const char *szAddAs) {
  bool fMove = true;

  if (StdOutput)
    printf("%s %s as %s...\n", fMove ? "Moving" : "Adding", GetFilename(szFile),
           szAddAs);

  return AddEntryOnDisk(szFile, szAddAs, fMove);
}

bool C4Group::Delete(const char *szFiles, bool fRecursive) {
  int fcount = 0;
  C4GroupEntry *tentry;

  // Segmented file specs
  if (SCharCount(';', szFiles) || SCharCount('|', szFiles)) {
    char cSeparator = (SCharCount(';', szFiles) ? ';' : '|');
    bool success = true;
    char filespec[_MAX_FNAME + 1];
    for (int cseg = 0;
         SCopySegment(szFiles, cseg, filespec, cSeparator, _MAX_FNAME); cseg++)
      if (!Delete(filespec, fRecursive)) success = false;
    return success;  // Would be nicer to return the file count and add up all
                     // counts from recursive actions...
  }

  // Delete all matching Entries
  ResetSearch();
  while ((tentry = SearchNextEntry(szFiles))) {
    // StdOutput
    if (StdOutput) printf("%s\n", tentry->FileName);
    if (!DeleteEntry(tentry->FileName))
      return Error("Delete: Could not delete entry");
    fcount++;
  }

  // Recursive: process sub groups
  if (fRecursive) {
    C4Group hChild;
    ResetSearch();
    while ((tentry = SearchNextEntry("*")))
      if (tentry->ChildGroup)
        if (hChild.OpenAsChild(this, tentry->FileName)) {
          hChild.SetStdOutput(StdOutput);
          hChild.Delete(szFiles, fRecursive);
          hChild.Close();
        }
  }

  // StdOutput
  if (StdOutput) printf("%d file(s) deleted.\n", fcount);

  return TRUE;  // Would be nicer to return the file count and add up all counts
                // from recursive actions...
}

// delete item to the recycle bin
BOOL EraseItemSafe(const char *szFilename) {
#ifdef _WIN32
  char Filename[_MAX_PATH + 1];
  SCopy(szFilename, Filename, _MAX_PATH);
  Filename[SLen(Filename) + 1] = 0;
  SHFILEOPSTRUCT shs;
  shs.hwnd = 0;
  shs.wFunc = FO_DELETE;
  shs.pFrom = Filename;
  shs.pTo = NULL;
  shs.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
  shs.fAnyOperationsAborted = FALSE;
  shs.hNameMappings = 0;
  shs.lpszProgressTitle = NULL;
  return !SHFileOperation(&shs);
#elif defined(USE_SDL_MAINLOOP) && defined(C4ENGINE) && defined(__APPLE__)
  bool sendFileToTrash(const char *filename);
  sendFileToTrash(szFilename);
#else
  return false;
#endif
}

bool C4Group::DeleteEntry(const char *szFilename, bool fRecycle) {
  switch (Status) {
    case GRPF_File:
      // Get entry
      C4GroupEntry *pEntry;
      if (!(pEntry = GetEntry(szFilename))) return FALSE;
      // Delete moved source files
      if (pEntry->Status == C4GRES_OnDisk)
        if (pEntry->DeleteOnDisk) {
          EraseItem(pEntry->DiskPath);
        }
      // (moved buffers are deleted by ~C4GroupEntry)
      // Delete status and update virtual file count
      pEntry->Status = C4GRES_Deleted;
      Head.Entries--;
      break;
    case GRPF_Folder:
      StdFile.Close();
      char szPath[_MAX_FNAME + 1];
      sprintf(szPath, "%s%c%s", FileName, DirectorySeparator, szFilename);

      if (fRecycle) {
        if (!EraseItemSafe(szPath)) return FALSE;
      } else {
        if (!EraseItem(szPath)) return FALSE;
      }
      break;
  }
  return TRUE;
}

bool C4Group::Rename(const char *szFile, const char *szNewName) {
  if (StdOutput) printf("Renaming %s to %s...\n", szFile, szNewName);

  switch (Status) {
    case GRPF_File:
      // Get entry
      C4GroupEntry *pEntry;
      if (!(pEntry = GetEntry(szFile))) return Error("Rename: File not found");
      // Check double name
      if (GetEntry(szNewName) && !SEqualNoCase(szNewName, szFile))
        return Error("Rename: File exists already");
      // Rename
      SCopy(szNewName, pEntry->FileName, _MAX_FNAME);
      Modified = TRUE;
      break;
    case GRPF_Folder:
      StdFile.Close();
      char path[_MAX_FNAME + 1];
      SCopy(FileName, path, _MAX_PATH - 1);
      AppendBackslash(path);
      SAppend(szFile, path, _MAX_PATH);
      char path2[_MAX_FNAME + 1];
      SCopy(FileName, path2, _MAX_PATH - 1);
      AppendBackslash(path2);
      SAppend(szNewName, path2, _MAX_PATH);
      if (!RenameFile(path, path2)) return Error("Rename: Failure");
      break;
  }

  return TRUE;
}

bool C4Group_IsExcluded(const char *szFile, const char *szExcludeList) {
  // No file or no exclude list
  if (!szFile || !szFile[0] || !szExcludeList || !szExcludeList[0])
    return FALSE;
  // Process segmented exclude list
  char cSeparator = (SCharCount(';', szExcludeList) ? ';' : '|');
  char szSegment[_MAX_PATH + 1];
  for (int i = 0;
       SCopySegment(szExcludeList, i, szSegment, cSeparator, _MAX_PATH); i++)
    if (WildcardMatch(szSegment, GetFilename(szFile))) return TRUE;
  // No match
  return FALSE;
}

bool C4Group::Extract(const char *szFiles, const char *szExtractTo,
                      const char *szExclude) {
  // StdOutput
  if (StdOutput) {
    printf("Extracting");
    if (szExtractTo) printf(" to %s", szExtractTo);
    printf("...\n");
  }

  int fcount = 0;
  int cbytes, tbytes;
  C4GroupEntry *tentry;

  cbytes = 0;
  tbytes = EntrySize();

  // Process segmented list
  char cSeparator = (SCharCount(';', szFiles) ? ';' : '|');
  char szFileName[_MAX_PATH + 1];
  for (int cseg = 0;
       SCopySegment(szFiles, cseg, szFileName, cSeparator, _MAX_PATH); cseg++) {
    // Search all entries
    ResetSearch();
    while (tentry = SearchNextEntry(szFileName)) {
      // skip?
      if (C4Group_IsExcluded(tentry->FileName, szExclude)) continue;
      // Process data & output
      if (StdOutput) printf("%s\n", tentry->FileName);
      cbytes += tentry->Size;
      if (fnProcessCallback)
        fnProcessCallback(tentry->FileName, 100 * cbytes / Max(tbytes, 1));

      // Extract
      if (!ExtractEntry(tentry->FileName, szExtractTo))
        return Error("Extract: Could not extract entry");

      fcount++;
    }
  }

  if (StdOutput) printf("%d file(s) extracted.\n", fcount);

  return TRUE;
}

bool C4Group::ExtractEntry(const char *szFilename, const char *szExtractTo) {
  CStdFile tfile;
  CStdFile hDummy;
  char szTempFName[_MAX_FNAME + 1], szTargetFName[_MAX_FNAME + 1];

  // Target file name
  if (szExtractTo) {
    SCopy(szExtractTo, szTargetFName, _MAX_FNAME - 1);
    if (DirectoryExists(szTargetFName)) {
      AppendBackslash(szTargetFName);
      SAppend(szFilename, szTargetFName, _MAX_FNAME);
    }
  } else
    SCopy(szFilename, szTargetFName, _MAX_FNAME);

  // Extract
  switch (Status) {
    case GRPF_File:  // Copy entry to target
      // Get entry
      C4GroupEntry *pEntry;
      if (!(pEntry = GetEntry(szFilename))) return FALSE;
      // Create dummy file to reserve target file name
      hDummy.Save(szTargetFName,
                  reinterpret_cast<const unsigned char *>("Dummy"), 5);
      // Make temp target file name
      SCopy(szTargetFName, szTempFName, _MAX_FNAME);
      MakeTempFilename(szTempFName);
      // Create temp target file
      if (!tfile.Create(szTempFName, !!pEntry->ChildGroup,
                        !!pEntry->Executable))
        return Error("Extract: Cannot create target file");
      // Write entry file to temp target file
      if (!AppendEntry2StdFile(pEntry, tfile)) {
        // Failure: close and erase temp target file
        tfile.Close();
        EraseItem(szTempFName);
        // Also erase reservation target file
        EraseItem(szTargetFName);
        // Failure
        return FALSE;
      }
      // Close target file
      tfile.Close();
      // Make temp file to original file
      if (!EraseItem(szTargetFName))
        return Error("Extract: Cannot erase temporary file");
      if (!RenameItem(szTempFName, szTargetFName))
        return Error("Extract: Cannot rename temporary file");
// Set output file time
#ifdef _WIN32
      _utimbuf tftime;
      tftime.actime = pEntry->Time;
      tftime.modtime = pEntry->Time;
      _utime(szTargetFName, &tftime);
#else
      utimbuf tftime;
      tftime.actime = pEntry->Time;
      tftime.modtime = pEntry->Time;
      utime(szTargetFName, &tftime);
#endif
      break;
    case GRPF_Folder:  // Copy item from folder to target
      char szPath[_MAX_FNAME + 1];
      sprintf(szPath, "%s%c%s", FileName, DirectorySeparator, szFilename);
      if (!CopyItem(szPath, szTargetFName))
        return Error("ExtractEntry: Cannot copy item");
      break;
  }
  return TRUE;
}

bool C4Group::OpenAsChild(C4Group *pMother, const char *szEntryName,
                          BOOL fExclusive) {
  if (!pMother) return Error("OpenAsChild: No mother specified");

  if (SCharCount('*', szEntryName))
    return Error("OpenAsChild: No wildcards allowed");

  // Open nested child group check: If szEntryName is a reference to
  // a nested group, open the first mother (in specified mode), then open the
  // child
  // in exclusive mode

  if (SCharCount(DirectorySeparator, szEntryName)) {
    char mothername[_MAX_FNAME + 1];
    SCopyUntil(szEntryName, mothername, DirectorySeparator, _MAX_FNAME);

    C4Group *pMother2;
    pMother2 = new C4Group;
    pMother2->SetStdOutput(StdOutput);
    if (!pMother2->OpenAsChild(pMother, mothername, fExclusive)) {
      delete pMother2;
      return Error("OpenAsChild: Cannot open mother");
    }
    return OpenAsChild(pMother2, szEntryName + SLen(mothername) + 1, TRUE);
  }

  // Init
  SCopy(szEntryName, FileName, _MAX_FNAME);
  Mother = pMother;
  ExclusiveChild = fExclusive;

  // Folder: Simply set status and return
  char path[_MAX_FNAME + 1];
  SCopy(GetFullName().getData(), path, _MAX_FNAME);
  if (DirectoryExists(path)) {
    SCopy(path, FileName, _MAX_FNAME);
    SCopy("Open directory", Head.Maker, C4GroupMaxMaker);
    Status = GRPF_Folder;
    ResetSearch();
    return TRUE;
  }

  // Get original entry name
  C4GroupEntry *centry;
  if (centry = Mother->GetEntry(FileName))
    SCopy(centry->FileName, FileName, _MAX_PATH);

  // Access entry in mother group
  size_t iSize;
  if (!Mother->AccessEntry(FileName, &iSize)) {
    CloseExclusiveMother();
    Clear();
    return Error("OpenAsChild: Entry not in mother group");
  }

  // Child Group?
  if (centry && !centry->ChildGroup) {
    CloseExclusiveMother();
    Clear();
    return Error("OpenAsChild: Is not a child group");
  }

  // Read header
  // Do not do size checks for packed subgroups of unpacked groups (there will
  // be no entry),
  //  because that would be the PACKED size which can actually be smaller than
  //  sizeof(C4GroupHeader)!
  if (iSize < sizeof(C4GroupHeader) && centry) {
    CloseExclusiveMother();
    Clear();
    return Error("OpenAsChild: Entry too small");
  }
  if (!Mother->Read(&Head, sizeof(C4GroupHeader))) {
    CloseExclusiveMother();
    Clear();
    return Error("OpenAsChild: Entry reading error");
  }
  MemScramble((BYTE *)&Head, sizeof(C4GroupHeader));
  EntryOffset += sizeof(C4GroupHeader);

  // Check Header
  if (!SEqual(Head.id, C4GroupFileID) || (Head.Ver1 != C4GroupFileVer1) ||
      (Head.Ver2 > C4GroupFileVer2)) {
    CloseExclusiveMother();
    Clear();
    return Error("OpenAsChild: Invalid Header");
  }

  // Read Entries
  C4GroupEntryCore corebuf;
  int file_entries = Head.Entries;
  Head.Entries = 0;  // Reset, will be recounted by AddEntry
  for (int cnt = 0; cnt < file_entries; cnt++) {
    if (!Mother->Read(&corebuf, sizeof(C4GroupEntryCore))) {
      CloseExclusiveMother();
      Clear();
      return Error("OpenAsChild: Entry reading error");
    }
    EntryOffset += sizeof(C4GroupEntryCore);
    if (!AddEntry(C4GRES_InGroup, !!corebuf.ChildGroup, corebuf.FileName,
                  corebuf.Size, corebuf.Time, corebuf.HasCRC, corebuf.CRC, NULL,
                  NULL, FALSE, FALSE, !!corebuf.Executable)) {
      CloseExclusiveMother();
      Clear();
      return Error("OpenAsChild: Insufficient memory");
    }
  }

  ResetSearch();

  // File
  Status = GRPF_File;

  // save position in mother group
  if (centry) MotherOffset = centry->Offset;

  return TRUE;
}

bool C4Group::AccessEntry(const char *szWildCard, size_t *iSize,
                          char *sFileName, bool *fChild) {
#ifdef C4GROUP_DUMP_ACCESS
  LogF("Group access in %s: %s", GetFullName().getData(), szWildCard);
#endif
  char fname[_MAX_FNAME + 1];
  if (!FindEntry(szWildCard, fname, &iCurrFileSize, fChild)) return FALSE;
#ifdef _DEBUG
  szCurrAccessedEntry = fname;
#endif
  BOOL fResult = SetFilePtr2Entry(fname);
#ifdef _DEBUG
  szCurrAccessedEntry = NULL;
#endif
  if (!fResult) return FALSE;
  if (sFileName) SCopy(fname, sFileName);
  if (iSize) *iSize = iCurrFileSize;
  return TRUE;
}

bool C4Group::AccessNextEntry(const char *szWildCard, size_t *iSize,
                              char *sFileName, bool *fChild) {
  char fname[_MAX_FNAME + 1];
  if (!FindNextEntry(szWildCard, fname, &iCurrFileSize, fChild)) return FALSE;
#ifdef _DEBUG
  szCurrAccessedEntry = fname;
#endif
  BOOL fResult = SetFilePtr2Entry(fname);
#ifdef _DEBUG
  szCurrAccessedEntry = NULL;
#endif
  if (!fResult) return FALSE;
  if (sFileName) SCopy(fname, sFileName);
  if (iSize) *iSize = iCurrFileSize;
  return TRUE;
}

bool C4Group::SetFilePtr2Entry(const char *szName, C4Group *pByChild) {
  switch (Status) {
    case GRPF_File:
      C4GroupEntry *centry;
      if (!(centry = GetEntry(szName))) return FALSE;
      if (centry->Status != C4GRES_InGroup) return FALSE;
      return SetFilePtr(centry->Offset);

    case GRPF_Folder:
      StdFile.Close();
      char path[_MAX_FNAME + 1];
      SCopy(FileName, path, _MAX_FNAME);
      AppendBackslash(path);
      SAppend(szName, path);
      BOOL childgroup = C4Group_IsGroup(path);
      bool fSuccess = StdFile.Open(path, !!childgroup);
      return fSuccess;
  }
  return FALSE;
}

bool C4Group::FindEntry(const char *szWildCard, char *sFileName, size_t *iSize,
                        bool *fChild) {
  ResetSearch();
  return FindNextEntry(szWildCard, sFileName, iSize, fChild);
}

bool C4Group::FindNextEntry(const char *szWildCard, char *sFileName,
                            size_t *iSize, bool *fChild,
                            bool fStartAtFilename) {
  C4GroupEntry *centry;
  if (!szWildCard) return FALSE;

  // Reset search to specified position
  if (fStartAtFilename) FindEntry(sFileName);

  if (!(centry = SearchNextEntry(szWildCard))) return FALSE;
  if (sFileName) SCopy(centry->FileName, sFileName);
  if (iSize) *iSize = centry->Size;
  if (fChild) *fChild = !!centry->ChildGroup;
  return TRUE;
}
#ifdef _WIN32
bool C4Group::Add(const char *szFiles) {
  bool fMove = false;

  if (StdOutput) printf("%s...\n", fMove ? "Moving" : "Adding");

  // Add files & directories
  char szFileName[_MAX_FNAME + 1];
  int iFileCount = 0;
  long lAttrib = 0x037;  // _A_ALL
  struct _finddata_t fdt;
  long fdthnd;

  // Process segmented path & search wildcards
  char cSeparator = (SCharCount(';', szFiles) ? ';' : '|');
  for (int cseg = 0;
       SCopySegment(szFiles, cseg, szFileName, cSeparator, _MAX_FNAME); cseg++)
    if ((fdthnd = _findfirst((char *)szFileName, &fdt)) >= 0) {
      do {
        if (fdt.attrib & lAttrib) {
          // ignore
          if (fdt.name[0] == '.') continue;
          // Compose item path
          SCopy(szFiles, szFileName, _MAX_FNAME);
          *GetFilename(szFileName) = 0;
          SAppend(fdt.name, szFileName, _MAX_FNAME);
          // File count
          iFileCount++;
          // Process output & callback
          if (StdOutput) printf("%s\n", GetFilename(szFileName));
          if (fnProcessCallback)
            fnProcessCallback(GetFilename(szFileName), 0);  // cbytes/tbytes
                                                            // AddEntryOnDisk
          AddEntryOnDisk(szFileName, NULL, fMove);
        }
      } while (_findnext(fdthnd, &fdt) == 0);
      _findclose(fdthnd);
    }

  if (StdOutput)
    printf("%d file(s) %s.\n", iFileCount, fMove ? "moved" : "added");

  return TRUE;
}

bool C4Group::Move(const char *szFiles) {
  bool fMove = true;

  if (StdOutput) printf("%s...\n", fMove ? "Moving" : "Adding");

  // Add files & directories
  char szFileName[_MAX_FNAME + 1];
  int iFileCount = 0;
  long lAttrib = 0x037;  // _A_ALL
  struct _finddata_t fdt;
  long fdthnd;

  // Process segmented path & search wildcards
  char cSeparator = (SCharCount(';', szFiles) ? ';' : '|');
  for (int cseg = 0;
       SCopySegment(szFiles, cseg, szFileName, cSeparator, _MAX_FNAME); cseg++)
    if ((fdthnd = _findfirst((char *)szFileName, &fdt)) >= 0) {
      do {
        if (fdt.attrib & lAttrib) {
          // ignore
          if (fdt.name[0] == '.') continue;
          // Compose item path
          SCopy(szFiles, szFileName, _MAX_FNAME);
          *GetFilename(szFileName) = 0;
          SAppend(fdt.name, szFileName, _MAX_FNAME);
          // File count
          iFileCount++;
          // Process output & callback
          if (StdOutput) printf("%s\n", GetFilename(szFileName));
          if (fnProcessCallback)
            fnProcessCallback(GetFilename(szFileName), 0);  // cbytes/tbytes
                                                            // AddEntryOnDisk
          AddEntryOnDisk(szFileName, NULL, fMove);
        }
      } while (_findnext(fdthnd, &fdt) == 0);
      _findclose(fdthnd);
    }

  if (StdOutput)
    printf("%d file(s) %s.\n", iFileCount, fMove ? "moved" : "added");

  return TRUE;
}
#endif
bool C4Group::Add(const char *szName, void *pBuffer, int iSize, bool fChild,
                  bool fHoldBuffer, int iTime, bool fExecutable) {
  return AddEntry(C4GRES_InMemory, fChild, szName, iSize,
                  iTime ? iTime : time(NULL), false, 0, szName, (BYTE *)pBuffer,
                  FALSE, fHoldBuffer, fExecutable);
}

bool C4Group::Add(const char *szName, StdBuf &pBuffer, bool fChild,
                  bool fHoldBuffer, int iTime, bool fExecutable) {
  if (!AddEntry(C4GRES_InMemory, fChild, szName, pBuffer.getSize(),
                iTime ? iTime : time(NULL), false, 0, szName,
                (BYTE *)pBuffer.getData(), FALSE, fHoldBuffer, fExecutable,
                true))
    return false;
  // Pointer is now owned and released by C4Group!
  if (fHoldBuffer) pBuffer.GrabPointer();
  return true;
}

bool C4Group::Add(const char *szName, StdStrBuf &pBuffer, bool fChild,
                  bool fHoldBuffer, int iTime, bool fExecutable) {
  if (!AddEntry(C4GRES_InMemory, fChild, szName, pBuffer.getLength(),
                iTime ? iTime : time(NULL), false, 0, szName,
                (BYTE *)pBuffer.getData(), FALSE, fHoldBuffer, fExecutable,
                true))
    return false;
  // Pointer is now owned and released by C4Group!
  if (fHoldBuffer) pBuffer.GrabPointer();
  return true;
}

#ifdef C4FRONTEND
HBITMAP C4Group::SubReadDDB(HDC hdc, int sx, int sy, int swdt, int shgt,
                            int twdt, int thgt, BOOL transcol) {
  HBITMAP hbmp;
  BITMAPFILEHEADER fhead;
  BITMAPINFO *pbmi =
      (BITMAPINFO *)new BYTE[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
  long bfoffs;
  BYTE fbuf;
  uint8_t *bmpbits;
  const BOOL f256Only = TRUE;  // Format accept flag

  // Read and check file header
  if (!Read(&fhead, sizeof(fhead))) {
    delete pbmi;
    return NULL;
  }
  if (fhead.bfType != *((WORD *)"BM")) {
    delete pbmi;
    return NULL;
  }

  // Read and check bitmap info header
  if (!Read(&(pbmi->bmiHeader), sizeof(BITMAPINFOHEADER))) {
    delete pbmi;
    return NULL;
  }
  if (f256Only)
    if ((pbmi->bmiHeader.biBitCount != 8) ||
        (pbmi->bmiHeader.biCompression != 0)) {
      delete pbmi;
      return NULL;
    }
  if (!pbmi->bmiHeader.biSizeImage) {
    delete pbmi;
    return NULL;
  }

  // Read colors
  if (!Read(pbmi->bmiColors, 256 * sizeof(RGBQUAD))) {
    delete pbmi;
    return NULL;
  }

  if (transcol) {
    pbmi->bmiColors[0].rgbRed = 0xFF;
    pbmi->bmiColors[0].rgbBlue = 0xFF;
    pbmi->bmiColors[0].rgbGreen = 0x00;
  }

  // Read offset to pixels
  for (bfoffs = fhead.bfOffBits - sizeof(BITMAPFILEHEADER) -
                sizeof(BITMAPINFOHEADER) - 256 * sizeof(RGBQUAD);
       bfoffs > 0; bfoffs--)
    if (!Read(&fbuf, 1)) {
      delete pbmi;
      return NULL;
    }

  // Read the pixels
  int iBufferSize =
      pbmi->bmiHeader.biHeight * DWordAligned(pbmi->bmiHeader.biWidth);
  if (!(bmpbits = new uint8_t[iBufferSize])) {
    delete pbmi;
    return NULL;
  }
  if (!Read(bmpbits, iBufferSize)) {
    delete[] bmpbits;
    delete pbmi;
    return NULL;
  }

  // Cut bitmap to section if desired
  if ((sx > -1) && (sy > -1) && (swdt > -1) && (shgt > -1)) {
    if (twdt == -1) twdt = swdt;
    if (thgt == -1) thgt = shgt;

    if (sx + swdt > pbmi->bmiHeader.biWidth) {
      delete pbmi;
      return NULL;
    }
    if (sy + shgt > pbmi->bmiHeader.biHeight) {
      delete pbmi;
      return NULL;
    }

    int tbufwdt = twdt;
    DWordAlign(tbufwdt);
    int tbufhgt = thgt;
    int sbufwdt = pbmi->bmiHeader.biWidth;
    DWordAlign(sbufwdt);
    int sbufhgt = pbmi->bmiHeader.biHeight;
    uint8_t *tbuf;
    if (tbuf = new uint8_t[tbufhgt * tbufwdt]) {
      ZeroMem(tbuf, tbufhgt * tbufwdt);
      BufferBlitAspect(bmpbits, sbufwdt, sbufhgt, sx, sy, swdt, shgt, tbuf,
                       tbufwdt, tbufhgt, 0, 0, twdt, thgt);
      delete[] bmpbits;
      bmpbits = tbuf;
      pbmi->bmiHeader.biSizeImage = tbufwdt * tbufhgt;
      pbmi->bmiHeader.biWidth = twdt;
      pbmi->bmiHeader.biHeight = thgt;
    }
  }

  // Create the bitmap
  hbmp = (HBITMAP)CreateDIBitmap(hdc, (BITMAPINFOHEADER *)&(pbmi->bmiHeader),
                                 CBM_INIT, bmpbits, (BITMAPINFO *)pbmi,
                                 DIB_RGB_COLORS);

  delete[] bmpbits;
  delete pbmi;

  return hbmp;
}

bool C4Group::ReadDDB(HBITMAP *lphBitmap, HDC hdc) {
  BOOL fOwnHDC = FALSE;
  if (!lphBitmap) return FALSE;
  // Check DC
  if (!hdc) {
    hdc = (HDC)GetDC(NULL);
    fOwnHDC = TRUE;
  }
  // Read bitmap
  *lphBitmap = SubReadDDB(hdc);
  // Release DC
  if (fOwnHDC) ReleaseDC(NULL, hdc);
  if (!(*lphBitmap)) return FALSE;
  return TRUE;
}

bool C4Group::ReadDDBSection(HBITMAP *lphBitmap, HDC hdc, int iSecX, int iSecY,
                             int iSecWdt, int iSecHgt, int iImgWdt, int iImgHgt,
                             BOOL fTransCol) {
  BOOL fOwnHDC = FALSE;
  // Init & argument check
  if (!lphBitmap) return FALSE;
  // Check DC
  if (!hdc) {
    hdc = (HDC)GetDC(NULL);
    fOwnHDC = TRUE;
  }
  // Read bitmap
  *lphBitmap = SubReadDDB(hdc, iSecX, iSecY, iSecWdt, iSecHgt, iImgWdt, iImgHgt,
                          fTransCol);
  // Release DC
  if (fOwnHDC) ReleaseDC(NULL, hdc);
  if (!(*lphBitmap)) return FALSE;
  return TRUE;
}

void BltAlpha2(DWORD &dwDst, DWORD dwSrc) {
  BYTE byAlphaDst = (BYTE)(dwSrc >> 24);
  BYTE byAlphaSrc = 255 - byAlphaDst;
  BYTE *pPixChanD = (BYTE *)&dwDst;
  BYTE *pPixChanS = (BYTE *)&dwSrc;
  *pPixChanD = (((int)*pPixChanD * byAlphaDst) >> 8) +
               (((int)*pPixChanS * byAlphaSrc) >> 8);
  ++pPixChanS;
  ++pPixChanD;
  *pPixChanD = (((int)*pPixChanD * byAlphaDst) >> 8) +
               (((int)*pPixChanS * byAlphaSrc) >> 8);
  ++pPixChanS;
  ++pPixChanD;
  *pPixChanD = (((int)*pPixChanD * byAlphaDst) >> 8) +
               (((int)*pPixChanS * byAlphaSrc) >> 8);
}

HBITMAP C4Group::SubReadPNG(HDC hdc, int sx, int sy, int swdt, int shgt,
                            int twdt, int thgt) {
  // read data
  int iSize = AccessedEntrySize();
  if (!iSize) return 0;
  BYTE *pData = new BYTE[iSize];
  if (!Read(pData, iSize)) {
    delete[] pData;
    return 0;
  }

  // load png
  CPNGFile png;
  if (!png.Load(pData, iSize)) {
    delete[] pData;
    return 0;
  }

  // free compressed data
  delete[] pData;

  // create DIB bitmap
  BITMAPINFO bmi;
  ZeroMemory(&bmi, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biWidth = png.iWdt;
  bmi.bmiHeader.biHeight = (int)png.iHgt;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  // read into mem; reverse orientation; render against white bg; make
  // transparent pixels have purple bg
  DWORD *tbuf = new DWORD[png.iWdt * png.iHgt];
  DWORD *pdwBuf = tbuf;
  for (int y = (int)png.iHgt - 1; y >= 0; --y)
    for (int x = 0; x < (int)png.iWdt; ++x) {
      DWORD dwPix = png.GetPix(x, y);
      DWORD dwWhite;
      if (dwPix & 0xa0000000) {
        dwWhite = 0xff00ff;
      } else {
        dwWhite = 0xffffff;
        BltAlpha2(dwWhite, dwPix);
      }
      *pdwBuf++ = dwWhite;
    }

  // No section specified, but target size specified: use full image as section
  if ((sx == -1) && (sy == -1) && (swdt == -1) && (shgt == -1))
    if ((twdt > -1) && (thgt > -1)) {
      sx = 0;
      sy = 0;
      swdt = png.iWdt;
      shgt = png.iHgt;
    }

  // cut bitmap to section if desired
  if ((sx > -1) && (sy > -1) && (swdt > -1) && (shgt > -1)) {
    if (twdt == -1) twdt = swdt;
    if (thgt == -1) thgt = shgt;

    if (sx + swdt > (int)png.iWdt || sy + shgt > (int)png.iHgt) {
      delete[] tbuf;
      return 0;
    }

    int tbufwdt = twdt;
    DWordAlign(tbufwdt);
    int tbufhgt = thgt;
    int sbufwdt = png.iWdt;
    int sbufhgt = png.iHgt;

    DWORD *tbuf2 = new DWORD[tbufhgt * tbufwdt];

    // Fill target buffer with purple background color
    int iFill = tbufhgt * tbufwdt;
    while (iFill) tbuf2[--iFill] = 0x00ff00ff;
    // FillMemory(tbuf2, tbufhgt * tbufwdt, 0xff); ...this was double - and it
    // was incorrect buffer size??

    BufferBlitAspectDw(reinterpret_cast<uint32_t *>(tbuf), sbufwdt, sbufhgt, sx,
                       sy, swdt, shgt, reinterpret_cast<uint32_t *>(tbuf2),
                       tbufwdt, tbufhgt, 0, 0, twdt, thgt);

    delete[] tbuf;
    tbuf = tbuf2;

    bmi.bmiHeader.biSizeImage = tbufwdt * tbufhgt * png.GetBitsPerPixel() / 8;
    bmi.bmiHeader.biWidth = twdt;
    bmi.bmiHeader.biHeight = thgt;
  } else {
  }

  // Create the bitmap
  HBITMAP hbmp = (HBITMAP)CreateDIBitmap(hdc, &(bmi.bmiHeader), CBM_INIT,
                                         (BYTE *)tbuf, &bmi, DIB_RGB_COLORS);

  // free image buffer
  delete[] tbuf;

  return hbmp;
}

bool C4Group::ReadPNGSection(HBITMAP *lphBitmap, HDC hdc, int iSecX, int iSecY,
                             int iSecWdt, int iSecHgt, int iImgWdt,
                             int iImgHgt) {
  BOOL fOwnHDC = FALSE;
  // Init & argument check
  if (!lphBitmap) return FALSE;
  // Check DC
  if (!hdc) {
    hdc = (HDC)GetDC(NULL);
    fOwnHDC = TRUE;
  }
  // Read bitmap
  *lphBitmap =
      SubReadPNG(hdc, iSecX, iSecY, iSecWdt, iSecHgt, iImgWdt, iImgHgt);
  // Release DC
  if (fOwnHDC) ReleaseDC(NULL, hdc);
  if (!(*lphBitmap)) return FALSE;
  return TRUE;
}
#endif  // C4ENGINE

const char *C4Group::GetName() { return FileName; }

int C4Group::EntryCount(const char *szWildCard) {
  int fcount;
  C4GroupEntry *tentry;
  // All files if no wildcard
  if (!szWildCard) szWildCard = "*";
  // Match wildcard
  ResetSearch();
  fcount = 0;
  while (tentry = SearchNextEntry(szWildCard)) fcount++;
  return fcount;
}

int C4Group::EntrySize(const char *szWildCard) {
  int fsize;
  C4GroupEntry *tentry;
  // All files if no wildcard
  if (!szWildCard) szWildCard = "*";
  // Match wildcard
  ResetSearch();
  fsize = 0;
  while (tentry = SearchNextEntry(szWildCard)) fsize += tentry->Size;
  return fsize;
}

unsigned int C4Group::EntryCRC32(const char *szWildCard) {
  if (!szWildCard) szWildCard = "*";
  // iterate thorugh child
  C4GroupEntry *pEntry;
  unsigned int iCRC = 0;
  ResetSearch();
  while (pEntry = SearchNextEntry(szWildCard)) {
    if (!CalcCRC32(pEntry)) return false;
    iCRC ^= pEntry->CRC;
  }
  // return
  return iCRC;
}

int C4Group::EntryTime(const char *szFilename) {
  int iTime = 0;
  switch (Status) {
    case GRPF_File:
      C4GroupEntry *pEntry;
      pEntry = GetEntry(szFilename);
      if (pEntry) iTime = pEntry->Time;
      break;
    case GRPF_Folder:
      char szPath[_MAX_FNAME + 1];
      sprintf(szPath, "%s%c%s", FileName, DirectorySeparator, szFilename);
      iTime = FileTime(szPath);
      break;
  }
  return iTime;
}

bool C4Group::LoadEntry(const char *szEntryName, char **lpbpBuf, size_t *ipSize,
                        int iAppendZeros) {
  size_t size;

  // Access entry, allocate buffer, read data
  (*lpbpBuf) = NULL;
  if (ipSize) *ipSize = 0;
  if (!AccessEntry(szEntryName, &size)) return Error("LoadEntry: Not found");
  if (!((*lpbpBuf) = new char[size + iAppendZeros]))
    return Error("LoadEntry: Insufficient memory");
  if (!Read(*lpbpBuf, size)) {
    delete[](*lpbpBuf);
    *lpbpBuf = NULL;
    return Error("LoadEntry: Reading error");
  }

  if (ipSize) *ipSize = size;

  if (iAppendZeros) ZeroMem((*lpbpBuf) + size, iAppendZeros);

  return TRUE;
}

bool C4Group::LoadEntry(const char *szEntryName, StdBuf &Buf) {
  size_t size;
  // Access entry, allocate buffer, read data
  if (!AccessEntry(szEntryName, &size)) return Error("LoadEntry: Not found");
  // Allocate memory
  Buf.New(size);
  // Load data
  if (!Read(Buf.getMData(), size)) {
    Buf.Clear();
    return Error("LoadEntry: Reading error");
  }
  // ok
  return true;
}

bool C4Group::LoadEntryString(const char *szEntryName, StdStrBuf &Buf) {
  size_t size;
  // Access entry, allocate buffer, read data
  if (!AccessEntry(szEntryName, &size)) return Error("LoadEntry: Not found");
  // Allocate memory
  Buf.SetLength(size);
  // other parts crash when they get a zero length buffer, so fail here
  if (!size) return false;
  // Load data
  if (!Read(Buf.getMData(), size)) {
    Buf.Clear();
    return Error("LoadEntry: Reading error");
  }
  // ok
  return true;
}

void C4Group::SetMaker(const char *szMaker) {
  if (!SEqual(szMaker, Head.Maker)) Modified = TRUE;
  SCopy(szMaker, Head.Maker, C4GroupMaxMaker);
}

void C4Group::SetPassword(const char *szPassword) {
  if (!SEqual(szPassword, Head.Password)) Modified = TRUE;
  SCopy(szPassword, Head.Password, C4GroupMaxPassword);
}

const char *C4Group::GetMaker() { return Head.Maker; }

const char *C4Group::GetPassword() { return Head.Password; }

int C4Group::GetVersion() { return Head.Ver1 * 10 + Head.Ver2; }

void C4Group::SetProcessCallback(BOOL (*fnCallback)(const char *, int)) {
  fnProcessCallback = fnCallback;
}

int SortRank(const char *szElement, const char *szSortList) {
  int cnt;
  char csegment[_MAX_FNAME + 1];

  for (cnt = 0; SCopySegment(szSortList, cnt, csegment, '|', _MAX_FNAME); cnt++)
    if (WildcardMatch(csegment, szElement))
      return (SCharCount('|', szSortList) + 1) - cnt;

  return 0;
}

bool C4Group::Sort(const char *szSortList) {
  BOOL fBubble;
  C4GroupEntry *centry, *prev, *next, *nextnext;

  if (!szSortList || !szSortList[0]) return FALSE;

  if (StdOutput) printf("Sorting...\n");

  do {
    fBubble = FALSE;

    for (prev = NULL, centry = FirstEntry; centry; prev = centry, centry = next)
      if (next = centry->Next) {
        // primary sort by file list
        int iS1 = SortRank(centry->FileName, szSortList);
        int iS2 = SortRank(next->FileName, szSortList);
        if (iS1 > iS2) continue;
        // secondary sort by filename
        if (iS1 == iS2)
          if (stricmp(centry->FileName, next->FileName) <= 0) continue;
        // wrong order: Swap!
        nextnext = next->Next;
        if (prev)
          prev->Next = next;
        else
          FirstEntry = next;
        next->Next = centry;
        centry->Next = nextnext;
        next = nextnext;

        fBubble = TRUE;
        Modified = TRUE;
      }

  } while (fBubble);

  return TRUE;
}

C4Group *C4Group::GetMother() { return Mother; }

#ifdef C4FRONTEND
bool C4Group::LoadIcon(const char *szEntryname, HICON *lphIcon) {
  if (!szEntryname || !lphIcon) return FALSE;

  *lphIcon = NULL;

  BYTE *bpBuf;
  unsigned int iSize;
  if (!LoadEntry(szEntryname, reinterpret_cast<char **>(&bpBuf), &iSize))
    return FALSE;

  *lphIcon = CreateIconFromResource(bpBuf, iSize, TRUE, 0x00030000);

  DWORD err = GetLastError();

  delete[] bpBuf;

  if (*lphIcon) return TRUE;
  return FALSE;
}
#endif  // C4FRONTEND

int C4Group::GetStatus() { return Status; }

bool C4Group::CloseExclusiveMother() {
  if (Mother && ExclusiveChild) {
    Mother->Close();
    delete Mother;
    Mother = NULL;
    return TRUE;
  }
  return FALSE;
}

int C4Group::GetCreation() { return Head.Creation; }

bool C4Group::SortByList(const char **ppSortList, const char *szFilename) {
  // No sort list specified
  if (!ppSortList) return FALSE;
  // No group name specified, use own
  if (!szFilename) szFilename = FileName;
  szFilename = GetFilename(szFilename);
  // Find matching filename entry in sort list
  const char **ppListEntry;
  for (ppListEntry = ppSortList; *ppListEntry; ppListEntry += 2)
    if (WildcardMatch(*ppListEntry, szFilename)) break;
  // Sort by sort list entry
  if (*ppListEntry && *(ppListEntry + 1)) Sort(*(ppListEntry + 1));
  // Success
  return TRUE;
}

void C4Group::ProcessOut(const char *szMessage, int iProcess) {
  if (fnProcessCallback) fnProcessCallback(szMessage, iProcess);
  if (C4Group_ProcessCallback) C4Group_ProcessCallback(szMessage, iProcess);
}

bool C4Group::EnsureChildFilePtr(C4Group *pChild) {
  // group file
  if (Status == GRPF_File) {
    // check if FilePtr has to be moved
    if (FilePtr != pChild->MotherOffset + pChild->EntryOffset + pChild->FilePtr)
      // move it to the position the child thinks it is
      if (!SetFilePtr(pChild->MotherOffset + pChild->EntryOffset +
                      pChild->FilePtr))
        return FALSE;
    // ok
    return TRUE;
  }

  // Open standard file is not the child file			...or StdFile ptr does not
  // match pChild->FilePtr
  char szChildPath[_MAX_PATH + 1];
  sprintf(szChildPath, "%s%c%s", FileName, DirectorySeparator,
          GetFilename(pChild->FileName));
  if (!ItemIdentical(StdFile.Name, szChildPath)) {
    // Reopen correct child stdfile
    if (!SetFilePtr2Entry(GetFilename(pChild->FileName))) return FALSE;
    // Advance to child's old file ptr
    if (!AdvanceFilePtr(pChild->EntryOffset + pChild->FilePtr)) return FALSE;
  }

  // Looks okay
  return TRUE;
}

StdStrBuf C4Group::GetFullName() const {
  char str[_MAX_PATH + 1];
  *str = '\0';
  char sep[] = "/";
  sep[0] = DirectorySeparator;
  for (const C4Group *pGroup = this; pGroup; pGroup = pGroup->Mother) {
    if (*str) SInsert(str, sep, 0, _MAX_PATH);
    SInsert(str, pGroup->FileName, 0, _MAX_PATH);
    if (pGroup->Status == GRPF_Folder)
      break;  // Folder is assumed to have full path
  }
  StdStrBuf sResult;
  sResult.Copy(str);
  return sResult;
}

void C4Group::MakeOriginal(BOOL fOriginal) {
  Modified = TRUE;
  if (fOriginal) {
    Head.Original = 1234567;
    MadeOriginal = TRUE;
  } else {
    Head.Original = 0;
    MadeOriginal = FALSE;
  }
}

bool C4Group::GetOriginal() { return (Head.Original == 1234567); }

bool C4Group::Add(const char *szEntryname, C4Group &hSource) {
  char *bpBuf;
  size_t iSize;
  // Load entry from source group to buffer
  if (!hSource.LoadEntry(szEntryname, &bpBuf, &iSize)) return FALSE;
  // Determine executable
  bool fExecutable =
      GetEntry(szEntryname) ? !!GetEntry(szEntryname)->Executable : false;
  // Add entry (hold buffer)
  if (!Add(szEntryname, bpBuf, iSize, false, true, 0, fExecutable)) {
    delete[] bpBuf;
    return FALSE;
  }
  // Success
  return TRUE;
}

bool C4Group::CalcCRC32(C4GroupEntry *pEntry) {
  // checksum already calculated?
  if (pEntry->HasCRC == C4GECS_New) return true;
  // child group?
  if (pEntry->ChildGroup || (pEntry->Status == C4GRES_OnDisk &&
                             (DirectoryExists(pEntry->DiskPath) ||
                              C4Group_IsGroup(pEntry->DiskPath)))) {
    // open
    C4Group Child;
    switch (pEntry->Status) {
      case C4GRES_InGroup:
        if (!Child.OpenAsChild(this, pEntry->FileName)) return 0;
        break;
      case C4GRES_OnDisk:
        if (!Child.Open(pEntry->DiskPath)) return 0;
        break;
      default:
        return 0;
    }
    // get checksum
    pEntry->CRC = Child.EntryCRC32();
  } else if (!pEntry->Size)
    pEntry->CRC = 0;
  else {
    // file checksum already calculated?
    if (pEntry->HasCRC != C4GECS_Old) {
      BYTE *pData = NULL;
      bool fOwnData;
      CStdFile f;
      // get data
      switch (pEntry->Status) {
        case C4GRES_InGroup:
          // create buffer
          pData = new BYTE[pEntry->Size];
          fOwnData = true;
          // go to entry
          if (!SetFilePtr2Entry(pEntry->FileName)) {
            delete[] pData;
            return false;
          }
          // read
          if (!Read(pData, pEntry->Size)) {
            delete[] pData;
            return false;
          }
          break;
        case C4GRES_OnDisk:
          // create buffer
          pData = new BYTE[pEntry->Size];
          fOwnData = true;
          // open
          if (!f.Open(pEntry->DiskPath)) {
            delete[] pData;
            return false;
          }
          // read
          if (!f.Read(pData, pEntry->Size)) {
            delete[] pData;
            return false;
          }
          break;
        case C4GRES_InMemory:
          // set
          pData = pEntry->bpMemBuf;
          fOwnData = false;
          break;
        default:
          return false;
      }
      if (!pData) return false;
      // calc crc
      pEntry->CRC = crc32(0, pData, pEntry->Size);
      // discard buffer
      if (fOwnData) delete[] pData;
    }
    // add file name
    pEntry->CRC = crc32(pEntry->CRC, reinterpret_cast<BYTE *>(pEntry->FileName),
                        SLen(pEntry->FileName));
  }
  // set flag
  pEntry->HasCRC = C4GECS_New;
  // ok
  return true;
}

bool C4Group::OpenChild(const char *strEntry) {
  // hack: The seach-handle would be closed twice otherwise
  FolderSearch.Reset();
  // Create a memory copy of ourselves
  C4Group *pOurselves = new C4Group;
  *pOurselves = *this;

  // Open a child from the memory copy
  C4Group hChild;
  if (!hChild.OpenAsChild(pOurselves, strEntry, FALSE)) {
    // Silently delete our memory copy
    pOurselves->Default();
    delete pOurselves;
    return FALSE;
  }

  // hack: The seach-handle would be closed twice otherwise
  FolderSearch.Reset();
  hChild.FolderSearch.Reset();

  // We now become our own child
  *this = hChild;

  // Make ourselves exclusive (until we hit our memory copy parent)
  for (C4Group *pGroup = this; pGroup != pOurselves; pGroup = pGroup->Mother)
    pGroup->ExclusiveChild = TRUE;

  // Reset the temporary child variable so it doesn't delete anything
  hChild.Default();

  // Yeehaw
  return TRUE;
}

bool C4Group::OpenMother() {
  // This only works if we are an exclusive child
  if (!Mother || !ExclusiveChild) return FALSE;

  // Store a pointer to our mother
  C4Group *pMother = Mother;

  // Clear ourselves without deleting our mother
  ExclusiveChild = FALSE;
  Clear();

  // hack: The seach-handle would be closed twice otherwise
  pMother->FolderSearch.Reset();
  FolderSearch.Reset();
  // We now become our own mother (whoa!)
  *this = *pMother;

  // Now silently delete our former mother
  pMother->Default();
  delete pMother;

  // Yeehaw
  return TRUE;
}

#ifdef _DEBUG
void C4Group::PrintInternals(const char *szIndent) {
  if (!szIndent) szIndent = "";
  printf("%sHead.id: '%s'\n", szIndent, Head.id);
  printf("%sHead.Ver1: %d\n", szIndent, Head.Ver1);
  printf("%sHead.Ver2: %d\n", szIndent, Head.Ver2);
  printf("%sHead.Entries: %d\n", szIndent, Head.Entries);
  printf("%sHead.Maker: '%s'\n", szIndent, Head.Maker);
  // printf("Head.Password: '%s'\n", szIndent, Head.Password);
  printf("%sHead.Creation: %d\n", szIndent, Head.Creation);
  printf("%sHead.Original: %d\n", szIndent, Head.Original);
  for (C4GroupEntry *p = FirstEntry; p; p = p->Next) {
    printf("%sEntry '%s':\n", szIndent, p->FileName);
    printf("%s  Packed: %d\n", szIndent, p->Packed);
    printf("%s  ChildGroup: %d\n", szIndent, p->ChildGroup);
    printf("%s  Size: %d\n", szIndent, p->Size);
    printf("%s  __Unused: %d\n", szIndent, p->__Unused);
    printf("%s  Offset: %d\n", szIndent, p->Offset);
    printf("%s  Time: %d\n", szIndent, p->Time);
    printf("%s  HasCRC: %d\n", szIndent, p->HasCRC);
    printf("%s  CRC: %08X\n", szIndent, p->CRC);
    if (p->ChildGroup) {
      C4Group hChildGroup;
      if (hChildGroup.OpenAsChild(this, p->FileName))
        hChildGroup.PrintInternals(
            FormatString("%s%s", szIndent, "    ").getData());
    }
  }
}
#endif
