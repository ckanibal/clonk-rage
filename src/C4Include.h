/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Main header to include all others */

#pragma once

/*
#ifdef _MSC_VER
#pragma warning(disable: 4786)
#pragma warning(disable: 4706)
#pragma warning(disable: 4239)
#endif
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif //HAVE_CONFIG_H

#ifdef _WIN32
	#define C4_OS "win32"
#elif defined(__linux__)
	#define C4_OS "linux"
#elif defined(__APPLE__)
	#define C4_OS "mac"
#else
	#define C4_OS "unknown";
#endif

#ifdef C4ENGINE

#ifndef HAVE_CONFIG_H
// different debugrec options
//#define DEBUGREC

// define directive STAT here to activate statistics
#undef STAT

#endif // HAVE_CONFIG_H

#ifdef DEBUGREC
#define DEBUGREC_SCRIPT
#define DEBUGREC_START_FRAME 0
#define DEBUGREC_PXS
#define DEBUGREC_OBJCOM
#define DEBUGREC_MATSCAN
//#define DEBUGREC_RECRUITMENT
#define DEBUGREC_MENU
#define DEBUGREC_OCF
#endif

// solidmask debugging
//#define SOLIDMASK_DEBUG

// fmod
#if defined USE_FMOD && !defined HAVE_SDL_MIXER
#define C4SOUND_USE_FMOD
#endif

#ifdef _WIN32
// resources
#include "../res/resource.h"
#define POINTER_64 __ptr64
#endif // _WIN32

// Probably not working
#if defined(HAVE_MIDI_H) && !defined(USE_FMOD)
#define USE_WINDOWS_MIDI
#endif
#endif // C4ENGINE

#include <Standard.h>
#include <CStdFile.h>
#include <Fixed.h>
#include <StdAdaptors.h>
#include <StdBuf.h>
#include <StdConfig.h>
#include <StdCompiler.h>
#include <StdDDraw2.h>
#include <StdFacet.h>
#include <StdFile.h>
#include <StdFont.h>
#include <StdMarkup.h>
#include <StdPNG.h>
#include <StdResStr2.h>
#include <StdSurface2.h>

#include "object/C4Id.h"
#include "C4Prototypes.h"
#include "config/C4Constants.h"

#ifdef _WIN32
#include <mmsystem.h>
#endif
#include <time.h>
#include <map>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <stdarg.h>
#include <functional>

//#define BIG_C4INCLUDE

/*
#if defined(BIG_C4INCLUDE) && defined(C4ENGINE)
#include "C4AList.h"
#include "game/C4Application.h"
#include "script/C4Aul.h"
#include "gui/C4ChatDlg.h"
#include "network/C4Client.h"
#include "object/C4Command.h"
#include "c4group/C4ComponentHost.h"
#include "C4Components.h"
#include "config/C4Config.h"
#include "C4Console.h"
#include "control/C4Control.h"
#include "object/C4DefGraphics.h"
#include "object/C4Def.h"
#include "C4DevmodeDlg.h"
#include "C4EditCursor.h"
#include "gamescript/C4Effects.h"
#include "c4group/C4Extra.h"
#include "graphics/C4FacetEx.h"
#include "graphics/C4Facet.h"
#include "C4FileClasses.h"
#include "gui/C4FileSelDlg.h"
#include "gamescript/C4FindObject.h"
#include "C4FogOfWar.h"
#include "C4Fonts.h"
#include "game/C4FullScreen.h"
#include "control/C4GameControl.h"
#include "network/C4GameControlNetwork.h"
#include "gui/C4GameDialogs.h"
#include "game/C4Game.h"
#include "gui/C4GameLobby.h"
#include "gui/C4GameMessage.h"
#include "object/C4GameObjects.h"
#include "gui/C4GameOptions.h"
#include "gui/C4GameOverDlg.h"
#include "platform/C4GamePadCon.h"
#include "control/C4GameSave.h"
#include "graphics/C4GraphicsResource.h"
#include "game/C4GraphicsSystem.h"
#include "c4group/C4Group.h"
#include "c4group/C4GroupSet.h"
#include "gui/C4Gui.h"
#include "object/C4IDList.h"
#include "object/C4InfoCore.h"
#include "C4InputValidation.h"
#include "gui/C4KeyboardInput.h"
#include "landscape/C4Landscape.h"
#include "c4group/C4LangStringTable.h"
#include "c4group/C4Language.h"
#ifndef NONETWORK
# include "C4League.h"
#endif
#include "gui/C4LoaderScreen.h"
#include "C4LogBuf.h"
#include "C4Log.h"
#include "landscape/C4MapCreatorS2.h"
#include "landscape/C4Map.h"
#include "landscape/C4MassMover.h"
#include "landscape/C4Material.h"
#include "landscape/C4MaterialList.h"
#include "gui/C4Menu.h"
#include "gui/C4MessageBoard.h"
#include "gui/C4MessageInput.h"
#include "gui/C4MouseControl.h"
#include "platform/C4MusicFile.h"
#include "platform/C4MusicSystem.h"
#include "C4NameList.h"
#include "network/C4NetIO.h"
#include "network/C4Network2Client.h"
#include "network/C4Network2Dialogs.h"
#include "network/C4Network2Discover.h"
#include "network/C4Network2.h"
#include "network/C4Network2IO.h"
#include "network/C4Network2Players.h"
#include "network/C4Network2Res.h"
#include "network/C4Network2Stats.h"
#include "object/C4ObjectCom.h"
#include "object/C4Object.h"
#include "object/C4ObjectInfo.h"
#include "object/C4ObjectInfoList.h"
#include "C4ObjectList.h"
#include "object/C4ObjectMenu.h"
#include "network/C4PacketBase.h"
#include "landscape/C4Particles.h"
#include "landscape/C4PathFinder.h"
#include "game/C4Physics.h"
#include "player/C4Player.h"
#include "control/C4PlayerInfo.h"
#include "gui/C4PlayerInfoListBox.h"
#include "player/C4PlayerList.h"
#include "C4PropertyDlg.h"
#include "landscape/C4PXS.h"
#include "C4Random.h"
#include "player/C4RankSystem.h"
#include "control/C4Record.h"
#include "C4Region.h"
#include "control/C4RoundResults.h"
#include "C4RTF.H"
#include "landscape/C4Scenario.h"
#include "gui/C4Scoreboard.h"
#include "script/C4Script.h"
#include "script/C4ScriptHost.h"
#include "object/C4Sector.h"
#include "object/C4Shape.h"
#include "landscape/C4Sky.h"
#include "landscape/C4SolidMask.h"
#include "platform/C4SoundSystem.h"
#include "gui/C4Startup.h"
#include "gui/C4StartupMainDlg.h"
#include "gui/C4StartupNetDlg.h"
#include "gui/C4StartupOptionsDlg.h"
#include "gui/C4StartupAboutDlg.h"
#include "gui/C4StartupPlrSelDlg.h"
#include "gui/C4StartupScenSelDlg.h"
#include "C4Stat.h"
#include "script/C4StringTable.h"
#include "graphics/C4SurfaceFile.h"
#include "graphics/C4Surface.h"
#include "control/C4Teams.h"
#include "landscape/C4Texture.h"
#include "C4ToolsDlg.h"
#include "gamescript/C4TransferZone.h"
#include "c4group/C4Update.h"
#include "gui/C4UpperBoard.h"
#include "C4UserMessages.h"
#include "script/C4Value.h"
#include "C4ValueList.h"
#include "C4ValueMap.h"
#include "platform/C4Video.h"
#include "game/C4Viewport.h"
#include "landscape/C4Weather.h"
#include "C4Wrappers.h"
#endif
*/
