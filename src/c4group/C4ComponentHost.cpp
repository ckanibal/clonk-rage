/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Holds a single text file component from a group */

#include "C4Include.h"
#include "c4group/C4ComponentHost.h"
#include "game/C4Application.h"
#include "c4group/C4Language.h"
#include <StdRegistry.h>

C4ComponentHost *pCmpHost = NULL;

extern C4Config *pConfig;  // Some cheap temporary hack to get to config in
                           // Engine + Frontend...
                           // This is so good - we can use it everywear!!!!

#if defined(C4ENGINE) && defined(_WIN32)

BOOL CALLBACK
    ComponentDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam) {
  if (!pCmpHost) return FALSE;

  switch (Msg) {
    //------------------------------------------------------------------
    case WM_CLOSE:
      pCmpHost->Close();
      break;
    //-------------------------------------------------------------------------------------------------
    case WM_DESTROY:
      StoreWindowPosition(hDlg, "Component", Config.GetSubkeyPath("Console"),
                          FALSE);
      break;
    //-------------------------------------------------------------------------------------------------
    case WM_INITDIALOG:
      pCmpHost->InitDialog(hDlg);
      RestoreWindowPosition(hDlg, "Component", Config.GetSubkeyPath("Console"));
      return TRUE;
    //-------------------------------------------------------------------------------------------------
    case WM_COMMAND:
      // Evaluate command
      switch (LOWORD(wParam)) {
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // - - - - -
        case IDCANCEL:
          pCmpHost->Close();
          return TRUE;
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // - - - - -
        case IDOK:
          // IDC_EDITDATA to Data
          char buffer[65000];
          GetDlgItemText(hDlg, IDC_EDITDATA, buffer, 65000);
          pCmpHost->Modified = TRUE;
          pCmpHost->Data.Copy(buffer);
          pCmpHost->Close();
          return TRUE;
          // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
          // - - - - - -
      }
      return FALSE;
      //-----------------------------------------------------------------------------------------------------------------------------------
  }
  return FALSE;
}

void C4ComponentHost::InitDialog(HWND hDlg) {
  hDialog = hDlg;
  // Set text
  SetWindowText(hDialog, Name);
  SetDlgItemText(hDialog, IDOK, LoadResStr("IDS_BTN_OK"));
  SetDlgItemText(hDialog, IDCANCEL, LoadResStr("IDS_BTN_CANCEL"));
  if (Data.getLength()) SetDlgItemText(hDialog, IDC_EDITDATA, Data.getData());
}

#endif

C4ComponentHost::C4ComponentHost() { Default(); }

C4ComponentHost::~C4ComponentHost() { Clear(); }

void C4ComponentHost::Default() {
  Data.Clear();
  Modified = FALSE;
  Name[0] = 0;
  Filename[0] = 0;
  FilePath[0] = 0;
#ifdef _WIN32
  hDialog = 0;
#endif
}

void C4ComponentHost::Clear() {
  Data.Clear();
#ifdef _WIN32
  if (hDialog) DestroyWindow(hDialog);
  hDialog = NULL;
#endif
}

BOOL C4ComponentHost::Load(const char *szName, C4Group &hGroup,
                           const char *szFilename, const char *szLanguage) {
  // Clear any old stuff
  Clear();
  // Store name & filename
  SCopy(szName, Name);
  SCopy(szFilename, Filename);
  // Load component - try all segmented filenames
  char strEntry[_MAX_FNAME + 1], strEntryWithLanguage[_MAX_FNAME + 1];
  for (int iFilename = 0;
       SCopySegment(Filename, iFilename, strEntry, '|', _MAX_FNAME);
       iFilename++) {
    // Try to insert all language codes provided into the filename
    char strCode[3] = "";
    for (int iLang = 0;
         SCopySegment(szLanguage ? szLanguage : "", iLang, strCode, ',', 2);
         iLang++) {
      // Insert language code
      sprintf(strEntryWithLanguage, strEntry, strCode);
      if (hGroup.LoadEntryString(strEntryWithLanguage, Data)) {
        if (pConfig->General.fUTF8) Data.EnsureUnicode();
        // Store actual filename
        hGroup.FindEntry(strEntryWithLanguage, Filename);
        CopyFilePathFromGroup(hGroup);
        // Got it
        return TRUE;
      }
      // Couldn't insert language code anyway - no point in trying other
      // languages
      if (!SSearch(strEntry, "%s")) break;
    }
  }
  // Truncate any additional segments from stored filename
  SReplaceChar(Filename, '|', 0);
  CopyFilePathFromGroup(hGroup);
  // Not loaded
  return FALSE;
}

BOOL C4ComponentHost::Load(const char *szName, C4GroupSet &hGroupSet,
                           const char *szFilename, const char *szLanguage) {
  // Clear any old stuff
  Clear();
  // Store name & filename
  SCopy(szName, Name);
  SCopy(szFilename, Filename);
  // Load component - try all segmented filenames
  char strEntry[_MAX_FNAME + 1], strEntryWithLanguage[_MAX_FNAME + 1];
  for (int iFilename = 0;
       SCopySegment(Filename, iFilename, strEntry, '|', _MAX_FNAME);
       iFilename++) {
    // Try to insert all language codes provided into the filename
    char strCode[3] = "";
    for (int iLang = 0;
         SCopySegment(szLanguage ? szLanguage : "", iLang, strCode, ',', 2);
         iLang++) {
      // Insert language code
      sprintf(strEntryWithLanguage, strEntry, strCode);
      if (hGroupSet.LoadEntryString(strEntryWithLanguage, Data)) {
        if (pConfig->General.fUTF8) Data.EnsureUnicode();
        // Store actual filename
        C4Group *pGroup = hGroupSet.FindEntry(strEntryWithLanguage);
        pGroup->FindEntry(strEntryWithLanguage, Filename);
        CopyFilePathFromGroup(*pGroup);
        // Got it
        return TRUE;
      }
      // Couldn't insert language code anyway - no point in trying other
      // languages
      if (!SSearch(strEntry, "%s")) break;
    }
  }
  // Truncate any additional segments from stored filename
  SReplaceChar(Filename, '|', 0);
  // skip full path (unknown)
  FilePath[0] = 0;
  // Not loaded
  return FALSE;
}

BOOL C4ComponentHost::LoadEx(const char *szName, C4Group &hGroup,
                             const char *szFilename, const char *szLanguage) {
  // Load from a group set containing the provided group and
  // alternative groups for cross-loading from a language pack
  C4GroupSet hGroups;
  hGroups.RegisterGroup(
      hGroup, false, 1000,
      C4GSCnt_Component);  // Provided group gets highest priority
  hGroups.RegisterGroups(Languages.GetPackGroups(pConfig->AtExeRelativePath(
                             hGroup.GetFullName().getData())),
                         C4GSCnt_Language);
  // Load from group set
  return Load(szName, hGroups, szFilename, szLanguage);
}

BOOL C4ComponentHost::LoadAppend(const char *szName, C4Group &hGroup,
                                 const char *szFilename,
                                 const char *szLanguage) {
  Clear();

  // Store name & filename
  SCopy(szName, Name);
  SCopy(szFilename, Filename);

  // Load component (segmented filename)
  char str1[_MAX_FNAME + 1], str2[_MAX_FNAME + 1];
  int iFileCnt = 0, iFileSizeSum = 0;
  int cseg, clseg;
  for (cseg = 0; SCopySegment(Filename, cseg, str1, '|', _MAX_FNAME); cseg++) {
    char szLang[3] = "";
    for (clseg = 0;
         SCopySegment(szLanguage ? szLanguage : "", clseg, szLang, ',', 2);
         clseg++) {
      sprintf(str2, str1, szLang);
      // Check existance
      size_t iFileSize;
      if (hGroup.FindEntry(str2, NULL, &iFileSize)) {
        iFileCnt++;
        iFileSizeSum += 1 + iFileSize;
        break;
      }
      if (!SSearch(str1, "%s")) break;
    }
  }

  // No matching files found?
  if (!iFileCnt) return FALSE;

  // Allocate memory
  Data.SetLength(iFileSizeSum);

  // Search files to read contents
  char *pPos = Data.getMData();
  *pPos = 0;
  for (cseg = 0; SCopySegment(Filename, cseg, str1, '|', _MAX_FNAME); cseg++) {
    char szLang[3] = "";
    for (clseg = 0;
         SCopySegment(szLanguage ? szLanguage : "", clseg, szLang, ',', 2);
         clseg++) {
      sprintf(str2, str1, szLang);
      // Load data
      char *pTemp;
      if (hGroup.LoadEntry(str2, &pTemp, NULL, 1)) {
        *pPos++ = '\n';
        SCopy(pTemp, pPos, Data.getPtr(Data.getLength()) - pPos);
        pPos += SLen(pPos);
        delete[] pTemp;
        break;
      }
      delete[] pTemp;
      if (!SSearch(str1, "%s")) break;
    }
  }

  SReplaceChar(Filename, '|', 0);
  CopyFilePathFromGroup(hGroup);
  return !!iFileCnt;
}

// Construct full path
void C4ComponentHost::CopyFilePathFromGroup(const C4Group &hGroup) {
  SCopy(pConfig->AtExeRelativePath(hGroup.GetFullName().getData()), FilePath,
        _MAX_PATH - 1);
  SAppendChar(DirectorySeparator, FilePath);
  SAppend(Filename, FilePath, _MAX_PATH);
}

BOOL C4ComponentHost::Set(const char *szData) {
  // clear existing data
  Clear();
  // copy new data
  Data.Copy(szData);
  return TRUE;
}

BOOL C4ComponentHost::Save(C4Group &hGroup) {
  if (!Modified) return TRUE;
  if (!Data) return hGroup.Delete(Filename);
  return hGroup.Add(Filename, Data);
}

void C4ComponentHost::Open() {
  pCmpHost = this;

#if defined(C4ENGINE) && defined(_WIN32)

  DialogBox(Application.hInstance, MAKEINTRESOURCE(IDD_COMPONENT),
            Application.pWindow->hWindow, (DLGPROC)ComponentDlgProc);

#endif

  pCmpHost = NULL;
}

bool C4ComponentHost::GetLanguageString(const char *szLanguage,
                                        StdStrBuf &rTarget) {
  const char *cptr;
  // No good parameters
  if (!szLanguage || !Data) return false;
  // Search for two-letter language identifier in text body, i.e. "DE:"
  char langindex[4] = "";
  for (int clseg = 0;
       SCopySegment(szLanguage ? szLanguage : "", clseg, langindex, ',', 2);
       clseg++) {
    SAppend(":", langindex);
    if (cptr = SSearch(Data.getData(), langindex)) {
      // Return the according string
      int iEndPos = SCharPos('\r', cptr);
      if (iEndPos < 0) iEndPos = SCharPos('\n', cptr);
      if (iEndPos < 0) iEndPos = strlen(cptr);
      rTarget.Copy(cptr, iEndPos);
      return true;
    }
  }
  // Language string not found
  return false;
}

void C4ComponentHost::Close() {
#ifdef _WIN32
  if (!hDialog) return;
  EndDialog(hDialog, 1);
  hDialog = FALSE;
#endif
}

/*BOOL C4ComponentHost::SetLanguageString(const char *szLanguage, const char
   *szString)
        {
        // Safety
        if (!szLanguage || !szString) return FALSE;
        // Allocate temp buffer
        char *cpBuffer = new char [Size+SLen(szLanguage)+1+SLen(szString)+2+1];
        cpBuffer[0]=0;
        // Copy all lines except language line
        const char *cpPos=Data;
        while (cpPos && *cpPos)
                {
                if (!SEqual2(cpPos,szLanguage))
                        {
                        SCopyUntil( cpPos, cpBuffer+SLen(cpBuffer), 0x0A );
                        if (cpBuffer[0]) if (cpBuffer[SLen(cpBuffer)-1]!=0x0D)
   SAppendChar(0x0D,cpBuffer);
                        SAppendChar(0x0A,cpBuffer);
                        }
                cpPos = SAdvancePast(cpPos,0x0A);
                }
        // Append new language line
        SAppend(szLanguage,cpBuffer);
        SAppendChar(':',cpBuffer);
        SAppend(szString,cpBuffer);
        // Set new data, delete temp buffer
        delete [] Data; Data=NULL;
        Data = new char [SLen(cpBuffer)+1];
        SCopy(cpBuffer,Data);
        Size=SLen(Data);
        delete [] cpBuffer;
        // Success
        Modified=TRUE;
        return TRUE;
        }*/

void C4ComponentHost::TrimSpaces() {
  Data.TrimSpaces();
  Modified = TRUE;
}
