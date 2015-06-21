/* by Sven2, 2005 */
// Custom game options and configuration dialog

#ifndef INC_C4GameOptions
#define INC_C4GameOptions

#include "gui/C4Gui.h"

class C4GameOptionsList;

// scenario-defined game options (2do)
class C4GameOptions {
 public:
  // base class for an option
  class Option {
   private:
    Option *pNext;  // singly linked list maintained by C4GameOptions class
    friend class C4GameOptions;

   public:
    Option() : pNext(NULL) {}
    virtual ~Option() {}

    static Option *CreateOptionByTypeName(const char *szTypeName);
    virtual void CompileFunc(StdCompiler *pComp) = 0;

   private:
    virtual C4GUI::Element *CreateOptionControl(
        C4GameOptionsList *pContainer) = 0;
  };

  // option of enumeration type
  class OptionEnum : public Option {
   public:
    // element of the enumeration
    struct Element {
      StdCopyStrBuf Name;
      int32_t id;

      Element *pNext;  // singly linked list maintained by OptionEnum class

      Element() : Name(), id(0), pNext(NULL) {}
      void CompileFunc(StdCompiler *pComp);
    };

   private:
    Element *pFirstElement;

   public:
    OptionEnum() : pFirstElement(NULL) {}
    virtual ~OptionEnum() { Clear(); }

    void Clear();
    virtual void CompileFunc(StdCompiler *pComp);

   private:
    virtual C4GUI::Element *CreateOptionControl(C4GameOptionsList *pContainer);
  };

 private:
  Option *pFirstOption;  // linked list of options

 public:
  C4GameOptions() : pFirstOption(NULL) {}
  ~C4GameOptions() { Clear(); }

  void Clear();

  void CompileFunc(StdCompiler *pComp);
};

// options dialog: created as listbox inside another dialog
// used to configure some standard runtime options, as well as custom game
// options
class C4GameOptionsList : public C4GUI::ListBox {
 public:
  enum { IconLabelSpacing = 2 };  // space between an icon and its text

 private:
  class Option : public C4GUI::Control {
   protected:
    typedef C4GUI::Control BaseClass;

    // primary subcomponent: forward focus to this element
    C4GUI::Control *pPrimarySubcomponent;

    virtual bool IsFocused(C4GUI::Control *pCtrl) {
      // also forward own focus to primary control
      return BaseClass::IsFocused(pCtrl) ||
             (HasFocus() && pPrimarySubcomponent == pCtrl);
    }

   public:
    Option(class C4GameOptionsList *pForDlg);  // ctor - adds to list
    void InitOption(
        C4GameOptionsList *pForDlg);  // add to list and do initial update

    virtual void Update(){};  // update data

    Option *GetNext() { return static_cast<Option *>(BaseClass::GetNext()); }
  };

  // dropdown list option
  class OptionDropdown : public Option {
   public:
    OptionDropdown(class C4GameOptionsList *pForDlg, const char *szCaption,
                   bool fReadOnly);

   protected:
    C4GUI::Label *pCaption;
    C4GUI::ComboBox *pDropdownList;

    virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller) = 0;
    void OnDropdownFill(C4GUI::ComboBox_FillCB *pFiller) {
      DoDropdownFill(pFiller);
    }
    virtual void DoDropdownSelChange(int32_t idNewSelection) = 0;
    bool OnDropdownSelChange(C4GUI::ComboBox *pForCombo,
                             int32_t idNewSelection) {
      DoDropdownSelChange(idNewSelection);
      Update();
      return true;
    }
  };

  // drop down list to specify central/decentral control mode
  class OptionControlMode : public OptionDropdown {
   public:
    OptionControlMode(class C4GameOptionsList *pForDlg);

   protected:
    virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
    virtual void DoDropdownSelChange(int32_t idNewSelection);

    virtual void Update();  // update data to current control rate
  };

  // drop down list option to adjust control rate
  class OptionControlRate : public OptionDropdown {
   public:
    OptionControlRate(class C4GameOptionsList *pForDlg);

   protected:
    virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
    virtual void DoDropdownSelChange(int32_t idNewSelection);

    virtual void Update();  // update data to current control rate
  };

  // drop down list option to adjust team usage
  class OptionTeamDist : public OptionDropdown {
   public:
    OptionTeamDist(class C4GameOptionsList *pForDlg);

   protected:
    virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
    virtual void DoDropdownSelChange(int32_t idNewSelection);

    virtual void Update();  // update data to current team mode
  };

  // drop down list option to adjust team color state
  class OptionTeamColors : public OptionDropdown {
   public:
    OptionTeamColors(class C4GameOptionsList *pForDlg);

   protected:
    virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
    virtual void DoDropdownSelChange(int32_t idNewSelection);

    virtual void Update();  // update data to current team color mode
  };

  // drop down list option to adjust control rate
  class OptionRuntimeJoin : public OptionDropdown {
   public:
    OptionRuntimeJoin(class C4GameOptionsList *pForDlg);

   protected:
    virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
    virtual void DoDropdownSelChange(int32_t idNewSelection);

    virtual void Update();  // update data to current runtime join state
  };

 public:
  C4GameOptionsList(const C4Rect &rcBounds, bool fActive, bool fRuntime);
  ~C4GameOptionsList() { Deactivate(); }

 private:
  C4Sec1TimerCallback<C4GameOptionsList> *
      pSec1Timer;  // engine timer hook for updates
  bool fRuntime;   // set for runtime options dialog - does not provide pre-game
                   // options such as team colors

  void InitOptions();  // creates option selection components

 public:
  // update all option flags by current game state
  void Update();
  void OnSec1Timer() { Update(); }

  // activate/deactivate periodic updates
  void Activate();
  void Deactivate();

  // config
  bool IsTabular() const {
    return fRuntime;
  }  // wide runtime dialog allows tabular layout
  bool IsRuntime() const { return fRuntime; }
};
#endif  // INC_C4GameOptions
