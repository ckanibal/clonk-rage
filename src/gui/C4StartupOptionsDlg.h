/* by Sven2, 2005 */
// Startup screen for non-parameterized engine start: Options dialog

#ifndef INC_C4StartupOptionsDlg
#define INC_C4StartupOptionsDlg

#include "C4Startup.h"

// startup dialog: Options
class C4StartupOptionsDlg : public C4StartupDlg {
  // main dlg stuff -----------------------------------------------------
 public:
  C4StartupOptionsDlg();   // ctor
  ~C4StartupOptionsDlg();  // dtor

 private:
  class C4KeyBinding *pKeyBack;
  bool fConfigSaved;  // set when config has been saved because dlg is closed;
                      // prevents double save
  bool fCanGoBack;    // set if dlg has not been recreated yet, in which case
                      // going back to a previous dialog is not possible
  C4GUI::Tabular *pOptionsTabular;

 protected:
  virtual bool OnEnter() { return false; }  // Enter ignored
  virtual bool OnEscape() {
    DoBack();
    return true;
  }
  bool KeyBack() {
    DoBack();
    return true;
  }
  virtual void OnClosed(
      bool fOK);  // callback when dlg got closed - save config
  virtual void UserClose(bool fOK)  // callback when the user tried to close the
                                    // dialog (e.g., by pressing Enter in an
                                    // edit) - just ignore this and save config
                                    // instead
  {
    if (fOK) SaveConfig(false, true);
  }

  void OnBackBtn(C4GUI::Control *btn) { DoBack(); }

  bool SaveConfig(bool fForce, bool fKeepOpen);  // save any config fields that
                                                 // are not stored directly;
                                                 // return whether all values
                                                 // are OK

 public:
  void DoBack();  // back to main menu

 public:
  void RecreateDialog(bool fFade);

  // program tab --------------------------------------------------------
 private:
  // button without fancy background
  class SmallButton : public C4GUI::Button {
   protected:
    virtual void DrawElement(C4FacetEx &cgo);  // draw the button

   public:
    SmallButton(const char *szText, const C4Rect &rtBounds)  // ctor
        : C4GUI::Button(szText, rtBounds) {}
    static int32_t GetDefaultButtonHeight();
  };

  class C4GUI::ComboBox *pLangCombo;
  class C4GUI::Label *pLangInfoLabel;
  class C4GUI::ComboBox *pFontFaceCombo, *pFontSizeCombo;

  void OnLangComboFill(C4GUI::ComboBox_FillCB *pFiller);
  bool OnLangComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);
  void UpdateLanguage();
  void OnFontFaceComboFill(C4GUI::ComboBox_FillCB *pFiller);
  void OnFontSizeComboFill(C4GUI::ComboBox_FillCB *pFiller);
  bool OnFontComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);
  void UpdateFontControls();
  bool SetGameFont(const char *szFontFace, int32_t iFontSize);
  int32_t FairCrewSlider2Strength(int32_t iSliderVal);
  int32_t FairCrewStrength2Slider(int32_t iStrengthVal);
  void OnFairCrewStrengthSliderChange(int32_t iNewVal);
  void OnResetConfigBtn(C4GUI::Control *btn);

  // graphics tab -------------------------------------------------------
 private:
  // checkbox that changes a config boolean directly
  class BoolConfig : public C4GUI::CheckBox {
   private:
    bool *pbVal, fInvert;
    int32_t *piVal, *piRestartChangeCfgVal;

   protected:
    void OnCheckChange(C4GUI::Element *pCheckBox);

   public:
    BoolConfig(const C4Rect &rcBounds, const char *szName, bool *pbVal,
               int32_t *piVal, bool fInvert = false,
               int32_t *piRestartChangeCfgVal = NULL);
  };
  // editbox below descriptive label sharing one window for common tooltip
  class EditConfig : public C4GUI::LabeledEdit {
   public:
    EditConfig(const C4Rect &rcBounds, const char *szName,
               ValidatedStdCopyStrBufBase *psConfigVal, int32_t *piConfigVal,
               bool fMultiline);

   private:
    ValidatedStdCopyStrBufBase *psConfigVal;
    int32_t *piConfigVal;

   public:
    void Save2Config();  // control to config
    static bool GetControlSize(int *piWdt, int *piHgt, const char *szForText,
                               bool fMultiline);
    int32_t GetIntVal() { return atoi(GetEdit()->GetText()); }
    void SetIntVal(int32_t iToVal) {
      GetEdit()->SetText(FormatString("%d", (int)iToVal).getData(), false);
    }
  } *pNetworkNameEdit, *pNetworkNickEdit;
  // message dialog with a timer; used to restore the resolution if the user
  // didn't press anything for a while
  class ResChangeConfirmDlg : public C4GUI::Dialog {
   private:
    C4Sec1TimerCallback<ResChangeConfirmDlg> *pSec1Timer;
    C4GUI::Label *pOperationCancelLabel;
    int32_t iResChangeSwitchTime;

   public:
    ResChangeConfirmDlg();
    ~ResChangeConfirmDlg();
    void OnSec1Timer();  // update timer label
   protected:
    virtual bool OnEnter() {
      return true;
    }  // Pressing Enter does not confirm this dialog, so "blind" users are less
       // likely to accept their change
    virtual const char *GetID() { return "ResChangeConfirmDialog"; }
  };

  void OnFullscreenChange(C4GUI::Element *pCheckBox);
  void OnGfxAllResolutionsChange(C4GUI::Element *pCheckBox);
  void OnGfxEngineCheck(C4GUI::Element *pCheckBox);
  void OnGfxTroubleCheck(C4GUI::Element *pCheckBox) {
    SaveGfxTroubleshoot();
  }  // immediate save and test
  void OnGfxResComboFill(C4GUI::ComboBox_FillCB *pFiller);
  bool OnGfxResComboSelChange(C4GUI::ComboBox *pForCombo,
                              int32_t idNewSelection);
  bool TryNewResolution(int32_t iResX, int32_t iResY);
  void OnGfxClrDepthCheck(C4GUI::Element *pCheckBox);
  StdStrBuf GetGfxResString(int32_t iResX,
                            int32_t iResY);  // convert resolution to string to
                                             // be displayed in resolution
                                             // choice combobox
  void OnEffectsSliderChange(int32_t iNewVal);

  C4GUI::CheckBox *pCheckGfxEngines[3], *pCheckGfxClrDepth[2];
  C4GUI::GroupBox *pGroupTrouble;
  C4GUI::CheckBox *pCheckGfxNoAlphaAdd, *pCheckGfxPointFilter,
      *pCheckGfxNoAddBlit, *pCheckGfxNoBoxFades, *pCheckGfxClipManually;
  int32_t iGfxTexIndent, iGfxBlitOff;
  EditConfig *pEdtGfxTexIndent, *pEdtGfxBlitOff;
  C4GUI::ScrollBar *pEffectLevelSlider;

  void LoadGfxTroubleshoot();
  void SaveGfxTroubleshoot();

  // sound tab ----------------------------------------------------------
 private:
  C4KeyBinding *pKeyToggleMusic;  // extra key binding for music: Must also
                                  // update checkbox in real-time
  BoolConfig *pFESoundCheck;
  C4GUI::CheckBox *pFEMusicCheck;

  void OnFEMusicCheck(C4GUI::Element *pCheckBox);  // toggling frontend music is
                                                   // reflected immediately
  void OnRXSoundCheck(C4GUI::Element *pCheckBox);  // toggling sounds on off
                                                   // must init/deinit sound
                                                   // system
  void OnMusicVolumeSliderChange(int32_t iNewVal);
  void OnSoundVolumeSliderChange(int32_t iNewVal);

 protected:
  bool KeyMusicToggle();

  // keyboard and gamepad control tabs ----------------------------------
 private:
  // dialog shown to user so he can select a key
  class KeySelDialog : public C4GUI::MessageDialog {
   private:
    class C4KeyBinding *pKeyListener;
    C4KeyCode key;
    bool fGamepad;
    int32_t iCtrlSet;

   protected:
    bool KeyDown(C4KeyCodeEx key);

   public:
    KeySelDialog(int32_t iKeyID, int32_t iCtrlSet, bool fGamepad);
    virtual ~KeySelDialog();

    C4KeyCode GetKeyCode() { return key; }
  };

  // Clonk-key-button with a label showing its key name beside it
  class KeySelButton : public C4GUI::IconButton {
   private:
    int32_t iKeyID;
    C4KeyCode key;

   protected:
    virtual void DrawElement(C4FacetEx &cgo);
    virtual bool IsComponentOutsideClientArea() { return true; }

   public:
    KeySelButton(int32_t iKeyID, const C4Rect &rcBounds, char cHotkey);
    void SetKey(C4KeyCode keyTo) { key = keyTo; }
  };

  // config area to define a keyboard set
  class ControlConfigArea : public C4GUI::Window {
   private:
    bool fGamepad;  // if set, gamepad control sets are being configured
    int32_t iMaxControlSets;   // number of control sets configured in this area
                               // - must be smaller or equal to C4MaxControlSet
    int32_t iSelectedCtrlSet;  // keyboard or gamepad set that is currently
                               // being configured
    class C4GUI::IconButton **ppKeyControlSetBtns;  // buttons to select
                                                    // configured control set -
                                                    // array in length of
                                                    // iMaxControlSets
    class KeySelButton *KeyControlBtns[C4MaxKey];   // buttons to configure
                                                   // individual kbd set buttons
    C4GamePadOpener *pGamepadOpener;  // opened gamepad for configuration
    C4StartupOptionsDlg *pOptionsDlg;
    class C4GUI::CheckBox *pGUICtrl;

   public:
    ControlConfigArea(const C4Rect &rcArea, int32_t iHMargin, int32_t iVMargin,
                      bool fGamepad, C4StartupOptionsDlg *pOptionsDlg);
    virtual ~ControlConfigArea();

   protected:
    void UpdateCtrlSet();

    void OnCtrlSetBtn(C4GUI::Control *btn);
    void OnCtrlKeyBtn(C4GUI::Control *btn);
    void OnResetKeysBtn(C4GUI::Control *btn);
    void OnGUIGamepadCheckChange(C4GUI::Element *pCheckBox);
  };

  class C4GamePadControl *GamePadCon;

  // network tab --------------------------------------------------------
 private:
  // checkbox to enable protocol and editbox to input port number
  class NetworkPortConfig : public C4GUI::Window {
   public:
    NetworkPortConfig(const C4Rect &rcBounds, const char *szName,
                      int32_t *pConfigValue, int32_t iDefault);  // ctor
   private:
    int32_t *pConfigValue;          // pointer into config set
    C4GUI::CheckBox *pEnableCheck;  // check box for whether port is enabled
    C4GUI::Edit *pPortEdit;         // edit field for port number

   public:
    void OnEnabledCheckChange(
        C4GUI::Element *pCheckBox);  // callback when checkbox is ticked
    void SavePort();                 // controls to config
    int32_t GetPort();  // get port as currently configured by control (or -1
                        // for deactivated)

    static bool GetControlSize(int *piWdt, int *piHgt);
  } *pPortCfgTCP, *pPortCfgUDP, *pPortCfgRef, *pPortCfgDsc;

  class NetworkServerAddressConfig : public C4GUI::Window {
   public:
    NetworkServerAddressConfig(const C4Rect &rcBounds, const char *szName,
                               int32_t *piConfigEnableValue,
                               char *szConfigAddressValue,
                               int iTabWidth);  // ctor
   private:
    int32_t *piConfigEnableValue;
    char *szConfigAddressValue;
    C4GUI::CheckBox *pEnableCheck;
    C4GUI::Edit *pAddressEdit;

   public:
    void OnEnabledCheckChange(
        C4GUI::Element *pCheckBox);  // callback when checkbox is ticked
    void Save2Config();              // controls to config

    static bool GetControlSize(int *piWdt, int *piHgt, int *piTabPos,
                               const char *szForText);
  } *pLeagueServerCfg;
};

#endif  // INC_C4StartupOptionsDlg
