/* by Sven2, 2008 */
// Engine internal C4Menus: Main menu, Options, Player join, Hostility, etc.

#ifndef INC_C4MainMenu
#define INC_C4MainMenu

#include "C4Menu.h"

// Menu identification constants for main menus
enum {
  C4MN_Hostility = 1,  // identification for hostility menu
  C4MN_Main = 2,       // identification for all other menus
  C4MN_TeamSelection = 3,
  C4MN_TeamSwitch = 4,
  C4MN_Observer = 5,
};

class C4MainMenu : public C4Menu {
 public:
  C4MainMenu();

  virtual void Default();

 protected:
  int32_t Player;

 public:
  bool Init(C4FacetExSurface &fctSymbol, const char *szEmpty, int32_t iPlayer,
            int32_t iExtra = C4MN_Extra_None, int32_t iExtraData = 0,
            int32_t iId = 0, int32_t iStyle = C4MN_Style_Normal);
  bool InitRefSym(const C4FacetEx &fctSymbol, const char *szEmpty,
                  int32_t iPlayer, int32_t iExtra = C4MN_Extra_None,
                  int32_t iExtraData = 0, int32_t iId = 0,
                  int32_t iStyle = C4MN_Style_Normal);

  bool ActivateMain(int32_t iPlayer);
  bool ActivateNewPlayer(int32_t iPlayer);
  bool ActivateHostility(int32_t iPlayer);
  bool ActivateGoals(int32_t iPlayer, bool fDoActivate);
  bool ActivateRules(int32_t iPlayer);
  bool ActivateSavegame(int32_t iPlayer);
  bool ActivateHost(int32_t iPlayer);
  bool ActivateClient(int32_t iPlayer);
  bool ActivateOptions(int32_t iPlayer, int32_t selection = 0);
  bool ActivateDisplay(int32_t iPlayer, int32_t selection = 0);
  bool ActivateSurrender(int32_t iPlayer);
  bool ActivateObserver();

 protected:
  virtual bool MenuCommand(const char *szCommand, bool fIsCloseCommand);

  virtual bool DoRefillInternal(bool &rfRefilled);

  virtual void OnSelectionChanged(int32_t iNewSelection);
  virtual void OnUserSelectItem(int32_t Player, int32_t iIndex);
  virtual void OnUserEnter(int32_t Player, int32_t iIndex, bool fRight);
  virtual void OnUserClose();
  virtual int32_t GetControllingPlayer() { return Player; }

 public:
  bool ActivateCommand(int32_t iPlayer, const char *szCommand);
};

#endif
