/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Lots of file helpers */

#ifndef STDFILE_INCLUDED
#define STDFILE_INCLUDED

#include <Standard.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#else
#include <dirent.h>
#include <limits.h>
#define _O_BINARY 0
#define _MAX_PATH PATH_MAX
#define _MAX_FNAME NAME_MAX

bool CreateDirectory(const char *pathname, void * = 0);
bool CopyFile(const char *szSource, const char *szTarget, bool FailIfExists);
#endif

#ifdef _WIN32
#define DirectorySeparator '\\'
#define AltDirectorySeparator '/'
#else
#define DirectorySeparator '/'
#define AltDirectorySeparator '\\'
#endif
#define Wildcard '*'

const char *GetWorkingDirectory();
bool SetWorkingDirectory(const char *szPath);
char *GetFilename(char *path);
char *GetFilenameWeb(char *path);
const char *GetFilenameOnly(const char *strFilename);
const char *GetC4Filename(const char *szPath);  // returns path to file starting
                                                // at first .c4*-directory
int GetTrailingNumber(const char *strString);
char *GetExtension(char *fname);
const char *GetFilename(const char *path);
const char *GetFilenameWeb(const char *path);
const char *GetExtension(const char *fname);
void DefaultExtension(char *szFileName, const char *szExtension);
void DefaultExtension(class StdStrBuf *sFilename, const char *szExtension);
void EnforceExtension(char *szFileName, const char *szExtension);
void EnforceExtension(class StdStrBuf *sFilename, const char *szExtension);
void RemoveExtension(char *szFileName);
void RemoveExtension(StdStrBuf *psFileName);
void AppendBackslash(char *szFileName);
void TruncateBackslash(char *szFilename);
void MakeTempFilename(char *szFileName);
void MakeTempFilename(class StdStrBuf *sFileName);
bool WildcardListMatch(
    const char *szWildcardList,
    const char *szString);  // match string in list like *.png|*.bmp
bool WildcardMatch(const char *szFName1, const char *szFName2);
bool TruncatePath(char *szPath);
// szBuffer has to be of at least _MAX_PATH length.
bool GetParentPath(const char *szFilename, char *szBuffer);
bool GetParentPath(const char *szFilename, StdStrBuf *outBuf);
bool GetRelativePath(const char *strPath, const char *strRelativeTo,
                     char *strBuffer, int iBufferSize = _MAX_PATH);
const char *GetRelativePathS(const char *strPath, const char *strRelativeTo);
bool IsGlobalPath(const char *szPath);

bool DirectoryExists(const char *szFileName);
// bool FileExists(const char *szFileName, int *lpAttr=NULL);
bool FileExists(const char *szFileName);
size_t FileSize(const char *fname);
size_t FileSize(int fdes);
int FileTime(const char *fname);
bool EraseFile(const char *szFileName);
bool EraseFiles(const char *szFilePath);
bool RenameFile(const char *szFileName, const char *szNewFileName);
bool MakeOriginalFilename(char *szFilename);
void MakeFilenameFromTitle(char *szTitle);

bool CopyDirectory(const char *szSource, const char *szTarget,
                   bool fResetAttributes = false);
bool EraseDirectory(const char *szDirName);

int ItemAttributes(const char *szItemName);
bool ItemIdentical(const char *szFilename1, const char *szFilename2);
inline bool ItemExists(const char *szItemName) {
  return FileExists(szItemName);
}
bool RenameItem(const char *szItemName, const char *szNewItemName);
bool EraseItem(const char *szItemName);
bool EraseItems(const char *szItemPath);
bool CopyItem(const char *szSource, const char *szTarget,
              bool fResetAttributes = false);
bool CreateItem(const char *szItemname);
bool MoveItem(const char *szSource, const char *szTarget);

// int ForEachFile(const char *szPath, int lAttrib, bool (*fnCallback)(const
// char *));
int ForEachFile(const char *szDirName, bool (*fnCallback)(const char *));

class DirectoryIterator {
 public:
  DirectoryIterator(const char *dirname);
  DirectoryIterator();
  ~DirectoryIterator();
  // Does not actually copy anything, but does prevent misuses from crashing (I
  // hope)
  DirectoryIterator(const DirectoryIterator &);
  const char *operator*() const;
  DirectoryIterator &operator++();
  void operator++(int);
  void Reset(const char *dirname);
  void Reset();

 protected:
  char filename[_MAX_PATH + 1];
#ifdef _WIN32
  struct _finddata_t fdt;
  int fdthnd;
  friend class C4GroupEntry;
#else
  DIR *d;
  dirent *ent;
#endif
};

bool LocateInFile(FILE *file, char *index, BYTE wrap = 1, BYTE lbeg = 0);
bool ReadFileLine(FILE *fhnd, char *tobuf, int maxlen);
bool ReadFileInfoLine(FILE *fhnd, char *info, char *tbuf, int maxlen = 256,
                      int wrap = 1);
bool WriteFileLine(FILE *hfile, const char *szLine);
DWORD ReadFileUntil(FILE *fhnd, char *tbuf, char smark, int maxlen);
void AdvanceFileLine(FILE *fhnd);

#endif  // STDFILE_INCLUDED
