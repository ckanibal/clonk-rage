/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Some functions to help with loading and saving AVI files using Video for
 * Windows */

#ifndef INC_STDVIDEO
#define INC_STDVIDEO

#ifdef _WIN32
#pragma once

#include <mmsystem.h>
#include <vfw.h>
#include <io.h>
#include "StdBuf.h"

BOOL AVIOpenGrab(const char *szFilename, PAVISTREAM *ppAviStream,
                 PGETFRAME *ppGetFrame, int &rAviLength, int &rFrameWdt,
                 int &rFrameHgt, int &rFrameBitsPerPixel, int &rFramePitch);

void AVICloseGrab(PAVISTREAM *ppAviStream, PGETFRAME *ppGetFrame);

BOOL AVIOpenOutput(const char *szFilename, PAVIFILE *ppAviFile,
                   PAVISTREAM *ppAviStream, int iWidth, int iHeight);

BOOL AVICloseOutput(PAVIFILE *ppAviFile, PAVISTREAM *ppAviStream);

BOOL AVIPutFrame(PAVISTREAM pAviStream, long lFrame, void *lpInfo,
                 long lInfoSize, void *lpData, long lDataSize);

// AVI file reading class
class CStdAVIFile {
 private:
  // file data
  StdStrBuf sFilename;
  PAVIFILE pAVIFile;

  // video data
  AVISTREAMINFO StreamInfo;
  PAVISTREAM pStream;
  PGETFRAME pGetFrame;

  // video processing helpers
  BITMAPINFO *pbmi;
  HDRAWDIB hOutDib;
  HBITMAP hBitmap;
  HDRAWDIB hDD;
  HDC hDC;
  BYTE *pFrameData;
  HWND hWnd;

  // audio data
  PAVISTREAM pAudioStream;
  BYTE *pAudioData;
  LONG iAudioDataLength, iAudioBufferLength;
  WAVEFORMAT *pAudioInfo;
  LONG iAudioInfoLength;

  // frame data
  int32_t iFinalFrame, iWdt, iHgt;
  time_t iTimePerFrame;  // [ms/frame]

 public:
  CStdAVIFile();
  ~CStdAVIFile();

  void Clear();
  bool OpenFile(const char *szFilename, HWND hWnd, int32_t iOutBitDepth);

  int32_t GetWdt() const { return iWdt; }
  int32_t GetHgt() const { return iHgt; }

  // get desired frame from time offset to video start - return false if past
  // video length
  bool GetFrameByTime(time_t iTime, int32_t *piFrame);

  // dump RGBa-data for specified frame
  bool GrabFrame(int32_t iFrame, class CSurface *sfc) const;

  // getting audio data
  bool OpenAudioStream();
  BYTE *GetAudioStreamData(size_t *piStreamLength);
  void CloseAudioStream();
};
#endif  //_WIN32

#endif  // INC_STDVIDEO
