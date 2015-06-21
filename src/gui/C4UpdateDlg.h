/* by matthes, 2007 */
// dialogs for update

#ifndef INC_C4UpdateDialogs
#define INC_C4UpdateDialogs

#include "gui/C4Gui.h"
#include "game/C4GameVersion.h"
#include "network/C4Network2Reference.h"

// dialog showing info about a connected client
class C4UpdateDlg : public C4GUI::InfoDialog {
 protected:
  virtual const char *GetID() { return "UpdateDialog"; }
  virtual void UpdateText();
  virtual void UserClose(bool fOK);

  bool UpdateRunning;

  // Misc process variables which are shared between the static DoUpdate and the
  // update dialog
  static int pid;
  static int c4group_output[2];
  static bool succeeded;

 public:
  C4UpdateDlg();  // ctor

 public:
  void Open(C4GUI::Screen *pScreen);
  void Write(const char *szText);

 public:
  static bool IsValidUpdate(const C4GameVersion &rNewVer);  // Returns whether
                                                            // we can update to
                                                            // the specified
                                                            // version
  static bool CheckForUpdates(C4GUI::Screen *pScreen,
                              bool fAutomatic = false);  // Checks for available
                                                         // updates and prompts
                                                         // the user whether to
                                                         // apply
  static bool DoUpdate(const C4GameVersion &rUpdateVersion,
                       C4GUI::Screen *pScreen);  // Static funtion for
                                                 // downloading and applying
                                                 // updates
  static bool ApplyUpdate(
      const char *strUpdateFile, bool fDeleteUpdate,
      C4GUI::Screen *pScreen);  // Static funtion for applying updates
};

// Loads current version string (mini-HTTP-client)
class C4Network2VersionInfoClient : public C4Network2HTTPClient {
  C4GameVersion Version;

 protected:
  virtual int32_t GetDefaultPort() { return C4NetStdPortHTTP; }

 public:
  C4Network2VersionInfoClient() : C4Network2HTTPClient() {}

  bool QueryVersion();
  bool GetVersion(C4GameVersion *pSaveToVer);
  bool GetRedirect(StdStrBuf &rRedirect);
};

#endif  // INC_C4UpdateDialogs
