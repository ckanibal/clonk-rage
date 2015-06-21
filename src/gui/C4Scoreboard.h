/* by Sven2, 2005 */
// script-controlled InGame dialog to show player infos

#ifndef INC_C4Scoreboard
#define INC_C4Scoreboard

#include "gui/C4Gui.h"
#include "script/C4StringTable.h"

class C4Scoreboard {
 public:
  enum {
    TitleKey = -1,
  };  // value used to index the title bars

 private:
  struct Entry {
    StdStrBuf Text;
    int32_t iVal;

    Entry() : Text(), iVal(0) {}
    void GrabFrom(Entry *pFrom)  // grab data w/o copy
    {
      Text.Take(pFrom->Text);
      iVal = pFrom->iVal;
    }
    void SwapWith(Entry *pSwap);
  };

 private:
  // array - row/col zero are row/coloumn headers
  int32_t iRows, iCols;
  Entry *pEntries;

  // realloc arrays, copy stuff
  void AddRow(int32_t iInsertBefore);
  void AddCol(int32_t iInsertBefore);
  void DelRow(int32_t iDelIndex);
  void DelCol(int32_t iDelIndex);

  // search row/coloumn by key value
  int32_t GetColByKey(int32_t iKey) const;
  int32_t GetRowByKey(int32_t iKey) const;

  // exchange two rows completely
  void SwapRows(int32_t iRow1, int32_t iRow2);

  // dialog control
  void InvalidateRows();  // recalculate row sizes

 protected:
  // displaying dialog
  class C4ScoreboardDlg *pDlg;  // NO-SAVE
  int32_t iDlgShow;             // ref counter for dialog show

  // not bounds-checked!
  Entry *GetCell(int32_t iCol, int32_t iRow) const {
    return pEntries + iRow * iCols + iCol;
  }

  friend class C4ScoreboardDlg;

 public:
  C4Scoreboard()
      : iRows(0), iCols(0), pEntries(NULL), pDlg(NULL), iDlgShow(0) {}
  ~C4Scoreboard() { Clear(); }

  void Clear();

  void SetCell(int32_t iColKey, int32_t iRowKey, const char *szValue,
               int32_t iValue);  // change cell value
  const char *GetCellString(int32_t iColKey, int32_t iRowKey);
  int32_t GetCellData(int32_t iColKey, int32_t iRowKey);
  void RemoveCol(int32_t iColKey);
  void RemoveRow(int32_t iRowKey);
  bool SortBy(int32_t iColKey, bool fReverse);

  void DoDlgShow(int32_t iChange, bool fUserToggle);
  void HideDlg();
  bool ShouldBeShown() { return iDlgShow > 0 && iRows && iCols; }
  bool CanBeShown() { return iDlgShow >= 0 && iRows && iCols; }

  bool KeyUserShow() {
    DoDlgShow(0, true);
    return true;
  }

  void CompileFunc(StdCompiler *pComp);
};

class C4ScoreboardDlg : public C4GUI::Dialog {
 private:
  int32_t *piColWidths;
  C4Scoreboard *pBrd;

  enum {
    XIndent = 4,
    YIndent = 4,
    XMargin = 3,
    YMargin = 3,
  };

 public:
  C4ScoreboardDlg(C4Scoreboard *pForScoreboard);
  ~C4ScoreboardDlg();

 protected:
  void InvalidateRows() {
    delete[] piColWidths;
    piColWidths = NULL;
  }
  void Update();  // update row widths and own size and caption

  virtual bool DoPlacement(C4GUI::Screen *pOnScreen,
                           const C4Rect &rPreferredDlgRect);
  virtual void Draw(C4FacetEx &cgo);
  virtual void DrawElement(C4FacetEx &cgo);

  virtual const char *GetID() { return "Scoreboard"; }

  virtual bool IsMouseControlled() { return false; }

  friend class C4Scoreboard;
};

#endif  // INC_C4Scoreboard
