cmake_minimum_required (VERSION 3.1.0)
project (clonk_rage CXX C)
set(CMAKE_CXX_STANDARD 11)

# options
option(PROJECT_FOLDERS "Put source files into subfolders in project file" ON)
set(CMAKE_MODULE_PATH "$(CMAKE_CURRENT_SOURCE_DIR)" "${CMAKE_SOURCE_DIR}/cmake/")

if(CMAKE_COMPILER_IS_GNUCXX)
	list(APPEND CXXFLAGS "-Wall -Wextra -Wredundant-decls -Wendif-labels -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Winit-self -Wsign-promo -Wno-reorder -Wno-unused-parameter -Wnon-virtual-dtor -Woverloaded-virtual")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	list(APPEND CXXFLAGS "-Wall -Wextra -Wredundant-decls -Wendif-labels -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Winit-self -Wsign-promo -Wno-reorder -Wno-unused-parameter -Wnon-virtual-dtor -Woverloaded-virtual")
endif()

set(CLONK_SOURCES
	src/C4AList.cpp
	src/C4Console.cpp
	src/C4DevmodeDlg.cpp
	src/C4EditCursor.cpp
	src/C4FogOfWar.cpp
	src/C4Fonts.cpp
	src/c4group/C4ComponentHost.cpp
	src/c4group/C4Extra.cpp
	src/c4group/C4Group.cpp
	src/c4group/C4GroupSet.cpp
	src/c4group/C4LangStringTable.cpp
	src/c4group/C4Language.cpp
	src/c4group/C4Update.cpp
	src/c4group/CStdFile.cpp
	src/C4Include.cpp
	src/C4InputValidation.cpp
	src/C4Log.cpp
	src/C4LogBuf.cpp
	src/C4NameList.cpp
	src/C4Network2HTTPClient.cpp
	src/C4ObjectList.cpp
	src/C4ObjectListDlg.cpp
	src/C4PropertyDlg.cpp
	src/C4Random.cpp
	src/C4Region.cpp
	src/C4RTF.cpp
	src/C4Stat.cpp
	src/C4ToolsDlg.cpp
	src/C4ValueList.cpp
	src/C4ValueMap.cpp
	src/C4VideoPlayback.cpp
	src/C4WinMain.cpp
	src/C4Wrappers.cpp
	src/config/C4Config.cpp
	src/control/C4Control.cpp
	src/control/C4GameControl.cpp
	src/control/C4GameParameters.cpp
	src/control/C4GameSave.cpp
	src/control/C4PlayerInfo.cpp
	src/control/C4PlayerInfoConflicts.cpp
	src/control/C4Record.cpp
	src/control/C4RoundResults.cpp
	src/control/C4Teams.cpp
	src/game/C4Application.cpp
	src/game/C4FullScreen.cpp
	src/game/C4Game.cpp
	src/game/C4GraphicsSystem.cpp
	src/game/C4Viewport.cpp
	src/gamescript/C4Effect.cpp
	src/gamescript/C4FindObject.cpp
	src/gamescript/C4TransferZone.cpp
	src/graphics/Bitmap256.cpp
	src/graphics/C4Facet.cpp
	src/graphics/C4FacetEx.cpp
	src/graphics/C4GraphicsResource.cpp
	src/graphics/C4Surface.cpp
	src/graphics/C4SurfaceFile.cpp
	src/graphics/StdPNG.cpp
	src/gui/C4ChatDlg.cpp
	src/gui/C4DownloadDlg.cpp
	src/gui/C4FileSelDlg.cpp
	src/gui/C4Folder.cpp
	src/gui/C4GameDialogs.cpp
	src/gui/C4GameLobby.cpp
	src/gui/C4GameMessage.cpp
	src/gui/C4GameOptions.cpp
	src/gui/C4GameOverDlg.cpp
	src/gui/C4Gui.cpp
	src/gui/C4GuiButton.cpp
	src/gui/C4GuiCheckBox.cpp
	src/gui/C4GuiComboBox.cpp
	src/gui/C4GuiContainers.cpp
	src/gui/C4GuiDialogs.cpp
	src/gui/C4GuiEdit.cpp
	src/gui/C4GuiLabels.cpp
	src/gui/C4GuiListBox.cpp
	src/gui/C4GuiMenu.cpp
	src/gui/C4GuiTabular.cpp
	src/gui/C4KeyboardInput.cpp
	src/gui/C4LoaderScreen.cpp
	src/gui/C4MainMenu.cpp
	src/gui/C4Menu.cpp
	src/gui/C4MessageBoard.cpp
	src/gui/C4MessageInput.cpp
	src/gui/C4MouseControl.cpp
	src/gui/C4PlayerInfoListBox.cpp
	src/gui/C4Scoreboard.cpp
	src/gui/C4Startup.cpp
	src/gui/C4StartupAboutDlg.cpp
	src/gui/C4StartupMainDlg.cpp
	src/gui/C4StartupNetDlg.cpp
	src/gui/C4StartupOptionsDlg.cpp
	src/gui/C4StartupPlrSelDlg.cpp
	src/gui/C4StartupScenSelDlg.cpp
	src/gui/C4UpdateDlg.cpp
	src/gui/C4UpperBoard.cpp
	src/landscape/C4Landscape.cpp
	src/landscape/C4Map.cpp
	src/landscape/C4MapCreatorS2.cpp
	src/landscape/C4MassMover.cpp
	src/landscape/C4Material.cpp
	src/landscape/C4MaterialList.cpp
	src/landscape/C4Particles.cpp
	src/landscape/C4PathFinder.cpp
	src/landscape/C4PXS.cpp
	src/landscape/C4Scenario.cpp
	src/landscape/C4Sky.cpp
	src/landscape/C4SolidMask.cpp
	src/landscape/C4Texture.cpp
	src/landscape/C4Weather.cpp
	src/lib/d3dutil.cpp
	src/lib/DInputX.cpp
	src/lib/DSoundX.cpp
	src/lib/dxutil.cpp
	src/lib/Fixed.cpp
	src/lib/Midi.cpp
	src/lib/OpenURL.cpp
	src/lib/PathFinder.cpp
	src/lib/Standard.cpp
	src/lib/StdBase64.cpp
	src/lib/StdBitmap.cpp
	src/lib/StdBuf.cpp
	src/lib/StdCompiler.cpp
	src/lib/StdConfig.cpp
	src/lib/StdD3D.cpp
	src/lib/StdDDraw2.cpp
	src/lib/StdFont.cpp
	src/lib/StdGL.cpp
	src/lib/StdGLCtx.cpp
	src/lib/StdMarkup.cpp
	src/lib/StdNoGfx.cpp
	src/lib/StdResStr2.cpp
	src/lib/StdSurface2.cpp
	src/lib/StdSurface8.cpp
	src/lib/StdTApp.cpp
	src/lib/StdVideo.cpp
	src/lib/StdXApp.cpp
	src/network/C4Client.cpp
	src/network/C4GameControlNetwork.cpp
	src/network/C4InteractiveThread.cpp
	src/network/C4League.cpp
	src/network/C4NetIO.cpp
	src/network/C4Network2.cpp
	src/network/C4Network2Client.cpp
	src/network/C4Network2Dialogs.cpp
	src/network/C4Network2Discover.cpp
	src/network/C4Network2IO.cpp
	src/network/C4Network2IRC.cpp
	src/network/C4Network2Players.cpp
	src/network/C4Network2Reference.cpp
	src/network/C4Network2Res.cpp
	src/network/C4Network2ResDlg.cpp
	src/network/C4Network2Stats.cpp
	src/network/C4Packet2.cpp
	src/object/C4Action.cpp
	src/object/C4Command.cpp
	src/object/C4Def.cpp
	src/object/C4DefGraphics.cpp
	src/object/C4GameObjects.cpp
	src/object/C4Id.cpp
	src/object/C4IDList.cpp
	src/object/C4InfoCore.cpp
	src/object/C4Movement.cpp
	src/object/C4Object.cpp
	src/object/C4ObjectCom.cpp
	src/object/C4ObjectInfo.cpp
	src/object/C4ObjectInfoList.cpp
	src/object/C4ObjectMenu.cpp
	src/object/C4Sector.cpp
	src/object/C4Shape.cpp
	src/platform/C4FileMonitor.cpp
	src/platform/C4GamePadCon.cpp
	src/platform/C4MusicFile.cpp
	src/platform/C4MusicSystem.cpp
	src/platform/C4SoundSystem.cpp
	src/platform/C4Video.cpp
	src/platform/StdFile.cpp
	src/platform/StdRegistry.cpp
	src/platform/StdScheduler.cpp
	src/player/C4Player.cpp
	src/player/C4PlayerList.cpp
	src/player/C4RankSystem.cpp
	src/script/C4Aul.cpp
	src/script/C4AulExec.cpp
	src/script/C4AulLink.cpp
	src/script/C4AulParse.cpp
	src/script/C4Script.cpp
	src/script/C4ScriptHost.cpp
	src/script/C4StringTable.cpp
	src/script/C4Value.cpp
)

if(PROJECT_FOLDERS)
  source_group("Core" src/.*)
  source_group("Platform abstraction" src/platform/.*)
  source_group("C4Group" src/c4group/.*)
  source_group("Game" src/game/.*)
  source_group("Graphics" src/graphics/.*)
  source_group("GUI" src/gui/.*)
  source_group("Network" src/network/.*)
  source_group("Object" src/object/.*)
  source_group("Landscape" src/landscape/.*)
	source_group("Lib" src/lib/.*)
  source_group("Player" src/player/.*)
  source_group("Script" src/*script/.*)
  source_group("Config" src/config/.*)
  source_group("Control" src/control/.*)
endif()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/lib
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/src/c4group
	${CMAKE_CURRENT_SOURCE_DIR}/src/config
	${CMAKE_CURRENT_SOURCE_DIR}/src/control
	${CMAKE_CURRENT_SOURCE_DIR}/src/game
	${CMAKE_CURRENT_SOURCE_DIR}/src/gamescript
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui
	${CMAKE_CURRENT_SOURCE_DIR}/src/landscape
	${CMAKE_CURRENT_SOURCE_DIR}/src/lib
	${CMAKE_CURRENT_SOURCE_DIR}/src/network
	${CMAKE_CURRENT_SOURCE_DIR}/src/object
	${CMAKE_CURRENT_SOURCE_DIR}/src/platform
	${CMAKE_CURRENT_SOURCE_DIR}/src/player
	${CMAKE_CURRENT_SOURCE_DIR}/src/res
	${CMAKE_CURRENT_SOURCE_DIR}/src/script
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(OperatingSystem "MAC")
	add_definitions(-DUSE_SDL_MAINLOOP)
	add_definitions(-D__APPLE__)

	Find_Package(SDL REQUIRED)
	include_directories(${SDL_INCLUDE_DIR})

	list(APPEND CLONK_SOURCES
		src/lib/MacUtility.mm
		src/lib/StdSDLApp.cpp
		src/lib/StdSDLWindow.cpp
	)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET(OperatingSystem "LINUX")
	add_definitions(-DUSE_X11)
	FIND_PACKAGE ( X11 REQUIRED )
	set(ADDITIONAL_LIBRARIES
		-lGLEW
		-lGLU
		-pthread
		-lrt
		-lXinerama
		-lXrandr
		-lXxf86vm
		-lXi
		-lXpm
		)
	list(APPEND CLONK_SOURCES
		src/lib/StdXWindow.cpp
	)
elseif(WIN32)
  SET(OperatingSystem "WIN32")
else()
  SET(OperatingSystem "UKNOWN")
endif()

if(NOT WIN32)
  add_definitions(-DHAVE_STDINT_H)
  add_definitions(-DHAVE_PTHREAD)
  add_definitions(-DHAVE_UNISTD_H)
	add_definitions(-DHAVE_LOCALE_H)
	add_definitions(-DHAVE_SYS_SOCKET_H)
	add_definitions(-DHAVE_SYS_STAT_H)
	add_definitions(-DHAVE_SYS_SYSCALL_H)
  add_definitions(-DUSE_GL)
	add_definitions(-DHAVE_FREETYPE)
endif()

if(NOT USE_CONSOLE)
	SET(JPEG_NAMES ${JPEG_NAMES} libjpeg jpeg-static)
	find_package(JPEG REQUIRED)
endif()
find_package(PNG REQUIRED)
find_package(Freetype REQUIRED)
set(CRYPTOPP_ROOT_DIR ${CMAKE_SOURCE_DIR}/lib/cryptopp/target)
find_package(CryptoPP REQUIRED)
include_directories(${JPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${CRYPTOPP_INCLUDE_DIRS})

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_libraries(${GLEW_LIBRARIES})
endif()
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
include_directories( ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS} "lib/zlib" )

add_definitions(-DC4ENGINE)

add_executable(engine
	${CLONK_SOURCES}
)

add_subdirectory("lib/zlib" zlib)

target_link_libraries(engine
	${FREETYPE_LIBRARIES}
	${PNG_LIBRARIES}
	${JPEG_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${CRYPTOPP_LIBRARIES}
	${ADDITIONAL_LIBRARIES}
	zlib
)

if (SDL_FOUND)
	target_link_libraries(engine ${SDL_LIBRARY})
endif()

if (X11_FOUND)
	target_link_libraries(engine ${X11_LIBRARIES})
endif()

if (NOT WIN32)
	target_link_libraries(engine ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} )
endif()
