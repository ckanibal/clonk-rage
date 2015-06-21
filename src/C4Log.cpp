/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Log file handling */

#include "C4Include.h"
#include "C4Log.h"

#ifndef BIG_C4INCLUDE
#include "C4Console.h"
#include "gui/C4GameLobby.h"
#include "C4LogBuf.h"
#include "c4group/C4Language.h"
#endif

#if defined(HAVE_SHARE_H) || defined(_WIN32)
#include <share.h>
#endif

FILE *C4LogFile = NULL;
time_t C4LogStartTime;
StdStrBuf sLogFileName;

StdStrBuf sFatalError;

BOOL OpenLog() {
  // open
  sLogFileName = C4CFN_Log;
  int iLog = 2;
#ifdef _WIN32
  while (!(C4LogFile = _fsopen(sLogFileName.getData(), "wt", _SH_DENYWR)))
#else
  while (!(C4LogFile = fopen(sLogFileName.getData(), "wb")))
#endif
  {
    // try different name
    sLogFileName.Format(C4CFN_LogEx, iLog++);
  }
  // save start time
  time(&C4LogStartTime);
  return true;
}

BOOL CloseLog() {
  // close
  if (C4LogFile) fclose(C4LogFile);
  C4LogFile = NULL;
  // ok
  return true;
}

int GetLogFD() {
  if (C4LogFile)
    return fileno(C4LogFile);
  else
    return -1;
}

bool LogSilent(const char *szMessage, bool fConsole) {
  if (!Application.AssertMainThread()) return false;
  // security
  if (!szMessage) return false;

  // add timestamp
  time_t timenow;
  time(&timenow);
  StdStrBuf TimeMessage;
  TimeMessage.SetLength(11 + SLen(szMessage) + 1);
  strftime(TimeMessage.getMData(), 11 + 1, "[%H:%M:%S] ", localtime(&timenow));

  // output until all data is written
  const char *pSrc = szMessage;
  do {
    // timestamp will always be that length
    char *pDest = TimeMessage.getMData() + 11;

    // copy rest of message, skip tags
    CMarkup Markup(false);
    while (*pSrc) {
      Markup.SkipTags(&pSrc);
      // break on crlf
      while (*pSrc == '\r') pSrc++;
      if (*pSrc == '\n') {
        pSrc++;
        break;
      }
      // copy otherwise
      if (*pSrc) *pDest++ = *pSrc++;
    }
    *pDest++ = '\n';
    *pDest = '\0';

#ifdef HAVE_ICONV
    StdStrBuf Line = Languages.IconvSystem(TimeMessage.getData());
#else
    StdStrBuf &Line = TimeMessage;
#endif

    // Save into log file
    if (C4LogFile) {
      fputs(Line.getData(), C4LogFile);
      fflush(C4LogFile);
    }

    // Write to console
    if (fConsole || Game.Verbose) {
#if defined(_DEBUG) && defined(_WIN32)
      // debug: output to VC console
      OutputDebugString(Line.getData());
#endif
      fputs(Line.getData(), stdout);
      fflush(stdout);
    }

  } while (*pSrc);

  return true;
}

bool LogSilent(const char *szMessage) { return LogSilent(szMessage, false); }

int iDisableLog = 0;

bool Log(const char *szMessage) {
  if (!Application.AssertMainThread()) return false;
  if (iDisableLog) return true;
  // security
  if (!szMessage) return false;
  // Pass on to console
  Console.Out(szMessage);
  // pass on to lobby
  C4GameLobby::MainDlg *pLobby = Game.Network.GetLobby();
  if (pLobby && Game.pGUI) pLobby->OnLog(szMessage);

  // Add message to log buffer
  bool fNotifyMsgBoard = false;
  if (Game.GraphicsSystem.MessageBoard.Active) {
    Game.GraphicsSystem.MessageBoard.AddLog(szMessage);
    fNotifyMsgBoard = true;
  }

  // log
  LogSilent(szMessage, true);

  // Notify message board
  if (fNotifyMsgBoard) Game.GraphicsSystem.MessageBoard.LogNotify();

  return true;
}

bool LogFatal(const char *szMessage) {
  if (!szMessage) szMessage = "(null)";
  // add to fatal error message stack - if not already in there (avoid
  // duplication)
  if (!SSearch(sFatalError.getData(), szMessage)) {
    if (!sFatalError.isNull()) sFatalError.AppendChar('|');
    sFatalError.Append(szMessage);
  }
  // write to log - note that Log might overwrite a static buffer also used in
  // szMessage
  return !!Log(FormatString(LoadResStr("IDS_ERR_FATAL"), szMessage).getData());
}

void ResetFatalError() { sFatalError.Clear(); }

const char *GetFatalError() { return sFatalError.getData(); }

BOOL LogF(const char *strMessage, ...) {
  va_list args;
  va_start(args, strMessage);
  // Compose formatted message
  StdStrBuf Buf;
  Buf.FormatV(strMessage, args);
  // Log
  return Log(Buf.getData());
}

BOOL LogSilentF(const char *strMessage, ...) {
  va_list args;
  va_start(args, strMessage);
  // Compose formatted message
  StdStrBuf Buf;
  Buf.FormatV(strMessage, args);
  // Log
  return LogSilent(Buf.getData());
}

BOOL DebugLog(const char *strMessage) {
  if (Game.DebugMode)
    return Log(strMessage);
  else
    return LogSilent(strMessage);
}

BOOL DebugLogF(const char *strMessage...) {
  va_list args;
  va_start(args, strMessage);
  StdStrBuf Buf;
  Buf.FormatV(strMessage, args);
  return DebugLog(Buf.getData());
}

size_t GetLogPos() {
  // get current log position
  return FileSize(sLogFileName.getData());
}

bool GetLogSection(size_t iStart, size_t iLength, StdStrBuf &rsOut) {
  if (!iLength) {
    rsOut.Clear();
    return true;
  }
  // read section from log file
  CStdFile LogFileRead;
  char *szBuf, *szBufOrig;
  size_t iSize;  // size exclusing terminator
  if (!LogFileRead.Load(sLogFileName.getData(), (BYTE **)&szBuf, (int *)&iSize,
                        1))
    return false;
  szBufOrig = szBuf;
  // reduce to desired buffer section
  if (iStart > iSize) iStart = iSize;
  if (iStart + iLength > iSize) iLength = iSize - iStart;
  szBuf += iStart;
  szBuf[iLength] = '\0';
  // strip timestamps; convert linebreaks to Clonk-linebreaks '|'
  char *szPosWrite = szBuf;
  const char *szPosRead = szBuf;
  while (*szPosRead) {
    // skip timestamp
    if (*szPosRead == '[')
      while (*szPosRead && *szPosRead != ']') {
        --iSize;
        ++szPosRead;
      }
    // skip whitespace behind timestamp
    if (!*szPosRead) break;
    szPosRead++;
    // copy data until linebreak
    size_t iLen = 0;
    while (*szPosRead && *szPosRead != 0x0d && *szPosRead != 0x0a) {
      ++szPosRead;
      ++iLen;
    }
    if (iLen && szPosRead - iLen != szPosWrite)
      memmove(szPosWrite, szPosRead - iLen, iLen);
    szPosWrite += iLen;
    // skip additional linebreaks
    while (*szPosRead == 0x0d || *szPosRead == 0x0a) ++szPosRead;
    // write a Clonk-linebreak
    if (*szPosRead) *szPosWrite++ = '|';
  }
  // done; create string buffer from data
  rsOut.Copy(szBuf, szPosWrite - szBuf);
  // old buf no longer used
  delete[] szBufOrig;
  // done, success
  return true;
}
