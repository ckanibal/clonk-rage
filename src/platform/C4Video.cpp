/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Captures uncompressed AVI from game view */

#include "C4Include.h"
#include "C4Video.h"

#ifndef BIG_C4INCLUDE
#include "game/C4Viewport.h"
#include "config/C4Config.h"
#include "game/C4Application.h"
#include "game/C4Game.h"
#include "player/C4Player.h"
#endif

#ifdef _WIN32
#include <vfw.h>
#include <StdVideo.h>
#endif

C4Video::C4Video() { Default(); }

C4Video::~C4Video() { Clear(); }

void C4Video::Default() {
  Active = false;
  pAviFile = NULL;
  pAviStream = NULL;
  AviFrame = 0;
  AspectRatio = 1.333;
  X = 0;
  Y = 0;
  Width = 768;
  Height = 576;
  Buffer = NULL;
  BufferSize = 0;
  InfoSize = 0;
  Recording = false;
  Surface = NULL;
  ShowFlash = 0;
}

void C4Video::Clear() { Stop(); }

void C4Video::Init(SURFACE sfcSource, int iWidth, int iHeight) {
  // Activate
  Active = true;
  // Set source surface
  Surface = sfcSource;
  // Set video size
  Width = iWidth;
  Height = iHeight;
  AspectRatio = (double)iWidth / (double)iHeight;
}

bool C4Video::Enlarge() {
  if (!Config.Graphics.VideoModule) return false;
  Resize(+16);
  return true;
}

bool C4Video::Reduce() {
  if (!Config.Graphics.VideoModule) return false;
  Resize(-16);
  return true;
}

void C4Video::Execute()  // Record frame, draw status
{
#ifdef _WIN32
  // Not active
  if (!Active) return;
  // Flash time
  if (ShowFlash) ShowFlash--;
  // Record
  if (Recording) RecordFrame();
  // Draw
  Draw();
#endif
}

bool C4Video::Toggle() {
  if (!Config.Graphics.VideoModule) return false;
  if (!Recording) return (Start());
  return (Stop());
}

bool C4Video::Stop() {
#ifdef _WIN32
  // Recording: close file
  if (Recording) AVICloseOutput(&pAviFile, &pAviStream);
  // Recording flag
  Recording = false;
  // Clear buffer
  if (Buffer) delete[] Buffer;
  Buffer = NULL;
#endif
  // Done
  return true;
}

bool C4Video::Start() {
  // Determine new filename
  char szFilename[_MAX_PATH + 1];
  int iIndex = 0;
  do {
    sprintf(szFilename, "Video%03d.avi", iIndex++);
  } while (ItemExists(szFilename));
  // Start recording
  return (Start(szFilename));
}

bool C4Video::Start(const char *szFilename) {
#ifdef _WIN32
  // Stop any recording
  Stop();
  // Open output file
  if (!AVIOpenOutput(szFilename, &pAviFile, &pAviStream, Width, Height)) {
    Log("AVIOpenOutput failure");
    AVICloseOutput(&pAviFile, &pAviStream);
    return false;
  }
  // Create video buffer
  AviFrame = 0;
  InfoSize = sizeof(BITMAPINFOHEADER);
  if (Config.Graphics.BitDepth == 8) InfoSize += sizeof(RGBQUAD) * 256;
  BufferSize =
      InfoSize + DWordAligned(Width) * Height * Config.Graphics.BitDepth / 8;
  Buffer = new BYTE[BufferSize];
  // Set bitmap info
  BITMAPINFO *pInfo = (BITMAPINFO *)Buffer;
  ZeroMem((BYTE *)pInfo, sizeof(BITMAPINFOHEADER));
  pInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  pInfo->bmiHeader.biPlanes = 1;
  pInfo->bmiHeader.biWidth = Width;
  pInfo->bmiHeader.biHeight = Height;
  pInfo->bmiHeader.biBitCount = Config.Graphics.BitDepth;
  pInfo->bmiHeader.biCompression = 0;
  pInfo->bmiHeader.biSizeImage =
      DWordAligned(Width) * Height * Config.Graphics.BitDepth / 8;
  if (Config.Graphics.BitDepth == 8)
    for (int cnt = 0; cnt < 256; cnt++) {
      pInfo->bmiColors[cnt].rgbRed =
          Game.GraphicsResource.GamePalette[cnt * 3 + 0];
      pInfo->bmiColors[cnt].rgbGreen =
          Game.GraphicsResource.GamePalette[cnt * 3 + 1];
      pInfo->bmiColors[cnt].rgbBlue =
          Game.GraphicsResource.GamePalette[cnt * 3 + 2];
    }
  // Recording flag
  Recording = true;
#endif  //_WIN32
  // Success
  return true;
}

void C4Video::Resize(int iChange) {
  // Not while recording
  if (Recording) return;
  // Resize
  Width = BoundBy(Width + iChange, 56, 800);
  Height = BoundBy((int)((double)Width / AspectRatio), 40, 600);
  // Adjust position
  AdjustPosition();
  // Show flash
  ShowFlash = 50;
}

void C4Video::Draw(C4FacetEx &cgo) {
  // Not active
  if (!Active) return;
  // No show
  if (!ShowFlash) return;
  // Draw frame
  Application.DDraw->DrawFrame(cgo.Surface, X + cgo.X, Y + cgo.Y,
                               X + cgo.X + Width - 1, Y + cgo.Y + Height - 1,
                               Recording ? CRed : CWhite);
  // Draw status
  if (Recording)
    sprintf(OSTR, "%dx%d Frame %d", Width, Height, AviFrame);
  else
    sprintf(OSTR, "%dx%d", Width, Height);
  Application.DDraw->TextOut(OSTR, Game.GraphicsResource.FontRegular, 1.0,
                             cgo.Surface, cgo.X + X + 4, cgo.Y + Y + 3,
                             Recording ? 0xfaFF0000 : 0xfaFFFFFF);
}

bool C4Video::AdjustPosition() {
  // Get source player & viewport
  C4Viewport *pViewport = Game.GraphicsSystem.GetFirstViewport();
  if (!pViewport) return false;
  C4Player *pPlr = Game.Players.Get(pViewport->GetPlayer());
  if (!pPlr) return false;
  // Set camera position
  X = pPlr->ViewX - pViewport->ViewX + pViewport->DrawX - Width / 2;
  X = BoundBy(X, 0, pViewport->ViewWdt - Width);
  Y = pPlr->ViewY - pViewport->ViewY + pViewport->DrawY - Height / 2;
  Y = BoundBy(Y, 0, pViewport->ViewHgt - Height);
  // Success
  return true;
}

#ifdef _WIN32
bool C4Video::RecordFrame() {
  // No buffer
  if (!Buffer) return false;
  // Lock source
  int iPitch, iWidth, iHeight;
  if (!Surface->Lock()) {
    Log("Video: lock surface failure");
    Stop();
    return false;
  }
  iPitch = Surface->PrimarySurfaceLockPitch;
  BYTE *bypBits = Surface->PrimarySurfaceLockBits;
  iWidth = Surface->Wdt;
  iHeight = Surface->Hgt;
  // Adjust source position
  if (!AdjustPosition()) {
    Log("Video: player/viewport failure");
    Stop();
    return false;
  }
  // Blit screen to buffer
  StdBlit((uint8_t *)bypBits, iPitch, -iHeight, X, Y, Width, Height,
          (uint8_t *)Buffer + InfoSize,
          DWordAligned(Width) * (Config.Graphics.BitDepth / 8), Height, 0, 0,
          Width, Height, Config.Graphics.BitDepth / 8);
  // Unlock source
  Surface->Unlock();
  // Write frame to file
  if (!AVIPutFrame(pAviStream, AviFrame, Buffer, InfoSize, Buffer + InfoSize,
                   BufferSize - InfoSize)) {
    Log("AVIPutFrame failure");
    Stop();
    return false;
  }
  // Advance frame
  AviFrame++;
  // Show flash
  ShowFlash = 1;
  // Success
  return true;
}

void C4Video::Draw() {
  // Not active
  if (!Active) return;
  // Get viewport
  C4Viewport *pViewport;
  if (pViewport = Game.GraphicsSystem.GetFirstViewport()) {
    C4FacetEx cgo;
    cgo.Set(Surface, pViewport->DrawX, pViewport->DrawY, pViewport->ViewWdt,
            pViewport->ViewHgt, pViewport->ViewX, pViewport->ViewY);
    Draw(cgo);
  }
}
#endif  //_WIN32
