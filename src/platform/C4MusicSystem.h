/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Handles Music.c4g and randomly plays songs */

#ifndef INC_C4MusicSystem
#define INC_C4MusicSystem

#include <c4group/C4Group.h>

class C4MusicFileInfoNode;
class C4MusicFile;

class C4MusicSystem {
  friend class C4SoundEffect;
  friend class C4SoundInstance;
  friend class C4SoundSystem;

 public:
  C4MusicSystem();
  ~C4MusicSystem();
  void Clear();
  int SetVolume(int);
  void Execute();
  void NotifySuccess();
  bool Init(const char *PlayList = NULL);
  bool InitForScenario(C4Group &hGroup);
  bool Play(const char *szSongname = NULL, bool fLoop = FALSE);
  bool Stop();
  void FadeOut(int fadeout_ms);

  int SetPlayList(const char *szPlayList);

  bool ToggleOnOff();  // keyboard callback

 protected:
  // song list
  C4MusicFile *Songs;
  int SongCount, ASongCount, SCounter;

  // play
  C4MusicFile *PlayMusicFile;
  int Volume;
  bool Loop;

  void LoadDir(
      const char *szPath);  // load some music files (by wildcard / directory)
  void Load(const char *szFile);  // load a music file
  void LoadMoreMusic();           // load music file names from MoreMusic.txt
  void ClearSongs();

  bool GrpContainsMusic(
      C4Group &rGrp);  // return whether this group contains music files

  // FMod / SDL_mixer
  bool MODInitialized;
  bool InitializeMOD();
  void DeinitializeMOD();
};

// --- helper stuff --- //

enum MusicType {
  MUSICTYPE_MID,
  MUSICTYPE_MOD,
  MUSICTYPE_MP3,
  MUSICTYPE_OGG,
  MUSICTYPE_UNKNOWN
};

class C4MusicFileInfoNode  // We need this for the MoreMusic.txt stuff
    {
 public:
  C4MusicFileInfoNode() {
    next = NULL;
    str = NULL;
  };
  ~C4MusicFileInfoNode() {
    if (str) delete[] str;
  }
  char *str;
  MusicType type;
  C4MusicFileInfoNode *next;
};

MusicType GetMusicFileTypeByExtension(const char *ext);

#endif
