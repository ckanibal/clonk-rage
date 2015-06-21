/* by Sven2, 2005 */
// Custom game options and configuration dialog

#include "C4Include.h"
#include "gui/C4GameOptions.h"
#include "game/C4Game.h"

// ----------- C4GameOptionsList::Option
// ----------------------------------------------------------------

C4GameOptionsList::Option::Option(C4GameOptionsList *pForDlg)
    : BaseClass(C4Rect(0, 0, 0, 0)), pPrimarySubcomponent(NULL) {}

void C4GameOptionsList::Option::InitOption(C4GameOptionsList *pForDlg) {
  // post-call after initialization: Adds to list box and does initial update
  // add to listbox (will eventually get moved)
  pForDlg->AddElement(this);
  // first-time update
  Update();
}

// ----------- C4GameOptionsList::OptionDropdown
// ----------------------------------------------------------------

C4GameOptionsList::OptionDropdown::OptionDropdown(
    class C4GameOptionsList *pForDlg, const char *szCaption, bool fReadOnly)
    : Option(pForDlg) {
  CStdFont &rUseFont = C4GUI::GetRes()->TextFont;
  // get size of caption label
  bool fTabular = pForDlg->IsTabular();
  int32_t iCaptWidth, iCaptHeight;
  if (fTabular) {
    // tabular layout: Caption label width by largest caption
    rUseFont.GetTextExtent(LoadResStr("IDS_NET_RUNTIMEJOIN"), iCaptWidth,
                           iCaptHeight, true);
    iCaptWidth = iCaptWidth * 5 / 4;
  } else {
    rUseFont.GetTextExtent(szCaption, iCaptWidth, iCaptHeight, true);
  }
  // calc total height for component
  int iHorizontalMargin = 1;
  int iVerticalMargin = 1;
  int iComboMargin = 5;
  int iSelComboHgt = C4GUI::ComboBox::GetDefaultHeight();
  SetBounds(C4Rect(0, 0, pForDlg->GetItemWidth(),
                   (!fTabular) * (iCaptHeight + iVerticalMargin * 2) +
                       iVerticalMargin * 2 + iSelComboHgt));
  C4GUI::ComponentAligner ca(GetContainedClientRect(), iHorizontalMargin,
                             iVerticalMargin);
  // create subcomponents
  AddElement(pCaption = new C4GUI::Label(
                 FormatString("%s:", szCaption).getData(),
                 fTabular ? ca.GetFromLeft(iCaptWidth, iCaptHeight)
                          : ca.GetFromTop(iCaptHeight),
                 ALeft));
  ca.ExpandLeft(-iComboMargin);
  AddElement(pPrimarySubcomponent = pDropdownList =
                 new C4GUI::ComboBox(ca.GetAll()));
  pDropdownList->SetReadOnly(fReadOnly);
  pDropdownList->SetComboCB(
      new C4GUI::ComboBox_FillCallback<C4GameOptionsList::OptionDropdown>(
          this, &C4GameOptionsList::OptionDropdown::OnDropdownFill,
          &C4GameOptionsList::OptionDropdown::OnDropdownSelChange));
  // final init
  InitOption(pForDlg);
}

// ----------- C4GameOptionsList::OptionControlMode
// ----------------------------------------------------------------

// Unfortunately, the control mode cannot be changed in the lobby
C4GameOptionsList::OptionControlMode::OptionControlMode(
    class C4GameOptionsList *pForDlg)
    : C4GameOptionsList::OptionDropdown(
          pForDlg, LoadResStr("IDS_TEXT_CONTROLMODE"),
          !Game.Control.isCtrlHost() || !Game.Control.isNetwork() ||
              !Game.Control.Network.IsEnabled() || !pForDlg->IsRuntime()) {
  SetToolTip(LoadResStr("IDS_DESC_CHANGESTHEWAYCONTROLDATAI"));
}

void C4GameOptionsList::OptionControlMode::DoDropdownFill(
    C4GUI::ComboBox_FillCB *pFiller) {
  // change possible?
  if (!Game.Control.isNetwork() || !Game.Control.Network.IsEnabled() ||
      !Game.Control.isCtrlHost())
    return;
  // add possible modes
  pFiller->AddEntry(LoadResStr("IDS_NET_CTRLMODE_CENTRAL"), CNM_Central);
  pFiller->AddEntry(LoadResStr("IDS_NET_CTRLMODE_DECENTRAL"), CNM_Decentral);
  if (!Game.Parameters.isLeague())
    pFiller->AddEntry("[!]Asynchroner Netzwerkmodus (experimentell!)",
                      CNM_Async);
}

void C4GameOptionsList::OptionControlMode::DoDropdownSelChange(
    int32_t idNewSelection) {
  // change possible?
  if (!Game.Control.isNetwork() || !Game.Control.Network.IsEnabled() ||
      !Game.Control.isCtrlHost())
    return;
  // perform it
  Game.Network.SetCtrlMode(idNewSelection);
  // update done in parent call
}

void C4GameOptionsList::OptionControlMode::Update() {
  const char *szControlMode;
  if (!Game.Control.isNetwork() || !Game.Control.Network.IsEnabled())
    szControlMode = LoadResStr("IDS_NET_NONET");
  else {
    switch (Game.Control.Network.GetCtrlMode()) {
      case CNM_Central:
        szControlMode = LoadResStr("IDS_NET_CTRLMODE_CENTRAL");
        break;
      case CNM_Decentral:
        szControlMode = LoadResStr("IDS_NET_CTRLMODE_DECENTRAL");
        break;
      case CNM_Async:
        szControlMode = "[!]Asynchroner Netzwerkmodus (experimentell!)";
        break;
      default:
        szControlMode = LoadResStr("IDS_NET_CTRLMODE_NONE");
        break;
    }
  }
  pDropdownList->SetText(szControlMode);
}

// ----------- C4GameOptionsList::OptionControlRate
// ----------------------------------------------------------------

C4GameOptionsList::OptionControlRate::OptionControlRate(
    class C4GameOptionsList *pForDlg)
    : C4GameOptionsList::OptionDropdown(pForDlg,
                                        LoadResStr("IDS_CTL_CONTROLRATE"),
                                        !Game.Control.isCtrlHost()) {
  SetToolTip(LoadResStr("IDS_CTL_CONTROLRATE_DESC"));
}

void C4GameOptionsList::OptionControlRate::DoDropdownFill(
    C4GUI::ComboBox_FillCB *pFiller) {
  for (int i = 1; i < Min(C4MaxControlRate, 10); ++i)
    pFiller->AddEntry(FormatString("%d", i).getData(), i);
}

void C4GameOptionsList::OptionControlRate::DoDropdownSelChange(
    int32_t idNewSelection) {
  // adjust rate
  int32_t iNewRate = idNewSelection;
  if (!iNewRate || iNewRate == Game.Control.ControlRate) return;
  Game.Control.AdjustControlRate(iNewRate - Game.Control.ControlRate);
}

void C4GameOptionsList::OptionControlRate::Update() {
  if (atoi(pDropdownList->GetText().getData()) == Game.Control.ControlRate)
    return;
  pDropdownList->SetText(
      FormatString("%d", Game.Control.ControlRate).getData());
}

// ----------- C4GameOptionsList::OptionRuntimeJoin
// ----------------------------------------------------------------

C4GameOptionsList::OptionRuntimeJoin::OptionRuntimeJoin(
    class C4GameOptionsList *pForDlg)
    : C4GameOptionsList::OptionDropdown(
          pForDlg, LoadResStr("IDS_NET_RUNTIMEJOIN"), !Game.Network.isHost()) {
  SetToolTip(LoadResStr("IDS_NET_RUNTIMEJOIN_DESC"));
}

void C4GameOptionsList::OptionRuntimeJoin::DoDropdownFill(
    C4GUI::ComboBox_FillCB *pFiller) {
  pFiller->AddEntry(LoadResStr("IDS_NET_RUNTIMEJOINBARRED"), 0);
  pFiller->AddEntry(LoadResStr("IDS_NET_RUNTIMEJOINFREE"), 1);
}

void C4GameOptionsList::OptionRuntimeJoin::DoDropdownSelChange(
    int32_t idNewSelection) {
  // adjust mode
  bool fAllowed = !!idNewSelection;
  Config.Network.NoRuntimeJoin = !fAllowed;
  if (Game.IsRunning) Game.Network.AllowJoin(fAllowed);
}

void C4GameOptionsList::OptionRuntimeJoin::Update() {
  const char *szText;
  if (Config.Network.NoRuntimeJoin)
    szText = LoadResStr("IDS_NET_RUNTIMEJOINBARRED");
  else
    szText = LoadResStr("IDS_NET_RUNTIMEJOINFREE");
  pDropdownList->SetText(szText);
}

// ----------- C4GameOptionsList::OptionTeamDist
// ----------------------------------------------------------------

C4GameOptionsList::OptionTeamDist::OptionTeamDist(
    class C4GameOptionsList *pForDlg)
    : C4GameOptionsList::OptionDropdown(pForDlg, LoadResStr("IDS_MSG_TEAMDIST"),
                                        !Game.Control.isCtrlHost()) {
  SetToolTip(LoadResStr("IDS_MSG_TEAMDIST_DESC"));
}

void C4GameOptionsList::OptionTeamDist::DoDropdownFill(
    C4GUI::ComboBox_FillCB *pFiller) {
  Game.Teams.FillTeamDistOptions(pFiller);
}

void C4GameOptionsList::OptionTeamDist::DoDropdownSelChange(
    int32_t idNewSelection) {
  // adjust team distribution
  Game.Teams.SendSetTeamDist(C4TeamList::TeamDist(idNewSelection));
}

void C4GameOptionsList::OptionTeamDist::Update() {
  StdStrBuf sOption;
  sOption.Take(Game.Teams.GetTeamDistString());
  pDropdownList->SetText(sOption.getData());
}

// ----------- C4GameOptionsList::OptionTeamColors
// ----------------------------------------------------------------

C4GameOptionsList::OptionTeamColors::OptionTeamColors(
    class C4GameOptionsList *pForDlg)
    : C4GameOptionsList::OptionDropdown(pForDlg,
                                        LoadResStr("IDS_MSG_TEAMCOLORS"),
                                        !Game.Control.isCtrlHost()) {
  SetToolTip(LoadResStr("IDS_MSG_TEAMCOLORS_DESC"));
}

void C4GameOptionsList::OptionTeamColors::DoDropdownFill(
    C4GUI::ComboBox_FillCB *pFiller) {
  pFiller->AddEntry(LoadResStr("IDS_MSG_ENABLED"), 1);
  pFiller->AddEntry(LoadResStr("IDS_MSG_DISABLED"), 0);
}

void C4GameOptionsList::OptionTeamColors::DoDropdownSelChange(
    int32_t idNewSelection) {
  bool fEnabled = !!idNewSelection;
  Game.Teams.SendSetTeamColors(fEnabled);
}

void C4GameOptionsList::OptionTeamColors::Update() {
  pDropdownList->SetText(Game.Teams.IsTeamColors()
                             ? LoadResStr("IDS_MSG_ENABLED")
                             : LoadResStr("IDS_MSG_DISABLED"));
}

// ----------- C4GameOptionsList
// -----------------------------------------------------------------------

C4GameOptionsList::C4GameOptionsList(const C4Rect &rcBounds, bool fActive,
                                     bool fRuntime)
    : C4GUI::ListBox(rcBounds), pSec1Timer(NULL), fRuntime(fRuntime) {
  // initial option fill
  InitOptions();
  if (fActive) Activate();
}

void C4GameOptionsList::InitOptions() {
  // creates option selection components
  new OptionControlMode(this);
  new OptionControlRate(this);
  if (Game.Network.isHost()) new OptionRuntimeJoin(this);
  if (!IsRuntime()) {
    if (Game.Teams.HasTeamDistOptions()) new OptionTeamDist(this);
    if (Game.Teams.IsMultiTeams()) new OptionTeamColors(this);
  }
}

void C4GameOptionsList::Update() {
  // update all option items
  for (Option *pItem = static_cast<Option *>(pClientWindow->GetFirst()); pItem;
       pItem = pItem->GetNext())
    pItem->Update();
}

void C4GameOptionsList::Activate() {
  // create timer if necessary
  if (!pSec1Timer)
    pSec1Timer = new C4Sec1TimerCallback<C4GameOptionsList>(this);
  // force an update
  Update();
}

void C4GameOptionsList::Deactivate() {
  // release timer if set
  if (pSec1Timer) {
    pSec1Timer->Release();
    pSec1Timer = NULL;
  }
}
