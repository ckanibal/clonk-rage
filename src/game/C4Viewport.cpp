/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* A viewport to each player */

#include "C4Include.h"
#include "game/C4Viewport.h"

#ifndef BIG_C4INCLUDE
#include "C4Console.h"
#include "C4UserMessages.h"
#include "object/C4Object.h"
#include "object/C4ObjectInfo.h"
#include "C4Wrappers.h"
#include "C4FullScreen.h"
#include "game/C4Application.h"
#include "object/C4ObjectCom.h"
#include "C4FogOfWar.h"
#include "C4Stat.h"
#include "gui/C4Gui.h"
#include "network/C4Network2Dialogs.h"
#include "gui/C4GameDialogs.h"
#include "player/C4Player.h"
#include "gui/C4ChatDlg.h"
#include "object/C4ObjectMenu.h"
#endif

#include <StdGL.h>
#include <StdRegistry.h>

#ifdef USE_X11
#include <X11/Xlib.h>
#ifdef WITH_DEVELOPER_MODE
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtktable.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkselection.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkhscrollbar.h>
#include <gtk/gtkvscrollbar.h>
#endif
#endif

namespace {
const int32_t ViewportScrollSpeed = 10;
}

#ifdef _WIN32
#include <shellapi.h>

LRESULT APIENTRY
    ViewportWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  // Determine viewport
  C4Viewport *cvp;
  if (!(cvp = Game.GraphicsSystem.GetViewport(hwnd)))
    return DefWindowProc(hwnd, uMsg, wParam, lParam);

  // Process message
  switch (uMsg) {
    //---------------------------------------------------------------------------------------------------------------------------
    case WM_KEYDOWN:
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      switch (wParam) {
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case VK_SCROLL:
          // key bound to this specific viewport. Don't want to pass this
          // through C4Game...
          cvp->TogglePlayerLock();
          break;
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        default:
          if (Game.DoKeyboardInput(wParam, KEYEV_Down, !!(lParam & 0x20000000),
                                   Application.IsControlDown(),
                                   Application.IsShiftDown(),
                                   !!(lParam & 0x40000000), NULL))
            return 0;
          break;
          // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
          // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      }
      break;
    //---------------------------------------------------------------------------------------------------------------------------
    case WM_KEYUP:
      if (Game.DoKeyboardInput(wParam, KEYEV_Up, !!(lParam & 0x20000000),
                               Application.IsControlDown(),
                               Application.IsShiftDown(), false, NULL))
        return 0;
      break;
    //------------------------------------------------------------------------------------------------------------
    case WM_SYSKEYDOWN:
      if (wParam == 18) break;
      if (Game.DoKeyboardInput(wParam, KEYEV_Down, !!(lParam & 0x20000000),
                               Application.IsControlDown(),
                               Application.IsShiftDown(),
                               !!(lParam & 0x40000000), NULL))
        return 0;
      break;
    //----------------------------------------------------------------------------------------------------------------------------------
    case WM_DESTROY:
      sprintf(OSTR, "Viewport%i", cvp->Player + 1);
      StoreWindowPosition(hwnd, OSTR, Config.GetSubkeyPath("Console"));
      break;
    //----------------------------------------------------------------------------------------------------------------------------------
    case WM_CLOSE:
      cvp->pWindow->Close();
      break;

    //----------------------------------------------------------------------------------------------------------------------------------
    case WM_DROPFILES:
      cvp->DropFiles((HANDLE)wParam);
      break;
    //----------------------------------------------------------------------------------------------------------------------------------
    case WM_USER_DROPDEF:
      Game.DropDef(lParam, cvp->ViewX + LOWORD(wParam),
                   cvp->ViewY + HIWORD(wParam));
      break;
    //----------------------------------------------------------------------------------------------------------------------------------
    case WM_SIZE:
      cvp->UpdateOutputSize();
      break;
    //----------------------------------------------------------------------------------------------------------------------------------
    case WM_PAINT:
      Game.GraphicsSystem.Execute();
      break;
    //----------------------------------------------------------------------------------------------------------------------------------
    case WM_HSCROLL:
      switch (LOWORD(wParam)) {
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
          cvp->ViewX = HIWORD(wParam);
          break;
        case SB_LINELEFT:
          cvp->ViewX -= ViewportScrollSpeed;
          break;
        case SB_LINERIGHT:
          cvp->ViewX += ViewportScrollSpeed;
          break;
        case SB_PAGELEFT:
          cvp->ViewX -= cvp->ViewWdt;
          break;
        case SB_PAGERIGHT:
          cvp->ViewX += cvp->ViewWdt;
          break;
      }
      cvp->Execute();
      cvp->ScrollBarsByViewPosition();
      return 0;
    //----------------------------------------------------------------------------------------------------------------------------------
    case WM_VSCROLL:
      switch (LOWORD(wParam)) {
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
          cvp->ViewY = HIWORD(wParam);
          break;
        case SB_LINEUP:
          cvp->ViewY -= ViewportScrollSpeed;
          break;
        case SB_LINEDOWN:
          cvp->ViewY += ViewportScrollSpeed;
          break;
        case SB_PAGEUP:
          cvp->ViewY -= cvp->ViewWdt;
          break;
        case SB_PAGEDOWN:
          cvp->ViewY += cvp->ViewWdt;
          break;
      }
      cvp->Execute();
      cvp->ScrollBarsByViewPosition();
      return 0;
      //----------------------------------------------------------------------------------------------------------------------------------
  }

  // Viewport mouse control
  if (Game.MouseControl.IsViewport(cvp) &&
      (Console.EditCursor.GetMode() == C4CNS_ModePlay)) {
    switch (uMsg) {
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_LBUTTONDOWN:
        Game.GraphicsSystem.MouseMove(C4MC_Button_LeftDown, LOWORD(lParam),
                                      HIWORD(lParam), wParam, cvp);
        break;
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_LBUTTONUP:
        Game.GraphicsSystem.MouseMove(C4MC_Button_LeftUp, LOWORD(lParam),
                                      HIWORD(lParam), wParam, cvp);
        break;
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_RBUTTONDOWN:
        Game.GraphicsSystem.MouseMove(C4MC_Button_RightDown, LOWORD(lParam),
                                      HIWORD(lParam), wParam, cvp);
        break;
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_RBUTTONUP:
        Game.GraphicsSystem.MouseMove(C4MC_Button_RightUp, LOWORD(lParam),
                                      HIWORD(lParam), wParam, cvp);
        break;
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_LBUTTONDBLCLK:
        Game.GraphicsSystem.MouseMove(C4MC_Button_LeftDouble, LOWORD(lParam),
                                      HIWORD(lParam), wParam, cvp);
        break;
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_RBUTTONDBLCLK:
        Game.GraphicsSystem.MouseMove(C4MC_Button_RightDouble, LOWORD(lParam),
                                      HIWORD(lParam), wParam, cvp);
        break;
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_MOUSEMOVE:
        if (Inside<int32_t>(LOWORD(lParam) - cvp->DrawX, 0, cvp->ViewWdt - 1) &&
            Inside<int32_t>(HIWORD(lParam) - cvp->DrawY, 0, cvp->ViewHgt - 1))
          SetCursor(NULL);
        Game.GraphicsSystem.MouseMove(C4MC_Button_None, LOWORD(lParam),
                                      HIWORD(lParam), wParam, cvp);
        break;
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_MOUSEWHEEL:
        Game.GraphicsSystem.MouseMove(C4MC_Button_Wheel, LOWORD(lParam),
                                      HIWORD(lParam), wParam, cvp);
        break;
        //----------------------------------------------------------------------------------------------------------------------------------
    }
  }
  // Console edit cursor control
  else {
    switch (uMsg) {
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_LBUTTONDOWN:
        // movement update needed before, so target is always up-to-date
        Console.EditCursor.Move(cvp->ViewX + LOWORD(lParam),
                                cvp->ViewY + HIWORD(lParam), wParam);
        Console.EditCursor.LeftButtonDown(wParam & MK_CONTROL);
        break;
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_LBUTTONUP:
        Console.EditCursor.LeftButtonUp();
        break;
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_RBUTTONDOWN:
        Console.EditCursor.RightButtonDown(wParam & MK_CONTROL);
        break;
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_RBUTTONUP:
        Console.EditCursor.RightButtonUp();
        break;
      //----------------------------------------------------------------------------------------------------------------------------------
      case WM_MOUSEMOVE:
        Console.EditCursor.Move(cvp->ViewX + LOWORD(lParam),
                                cvp->ViewY + HIWORD(lParam), wParam);
        break;
        //----------------------------------------------------------------------------------------------------------------------------------
    }
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

CStdWindow *C4ViewportWindow::Init(CStdApp *pApp, const char *Title,
                                   CStdWindow *pParent, bool) {
  Active = true;
  // Create window
  hWindow =
      CreateWindowEx(WS_EX_ACCEPTFILES, C4ViewportClassName, Title,
                     C4ViewportWindowStyle, CW_USEDEFAULT, CW_USEDEFAULT, 400,
                     250, pParent->hWindow, NULL, pApp->hInstance, NULL);
  return hWindow ? this : 0;
}

BOOL C4Viewport::DropFiles(HANDLE hDrop) {
  if (!Console.Editing) {
    Console.Message(LoadResStr("IDS_CNS_NONETEDIT"));
    return FALSE;
  }

  int32_t iFileNum = DragQueryFile((HDROP)hDrop, 0xFFFFFFFF, NULL, 0);
  POINT pntPoint;
  char szFilename[500 + 1];
  for (int32_t cnt = 0; cnt < iFileNum; cnt++) {
    DragQueryFile((HDROP)hDrop, cnt, szFilename, 500);
    DragQueryPoint((HDROP)hDrop, &pntPoint);
    Game.DropFile(szFilename, ViewX + pntPoint.x, ViewY + pntPoint.y);
  }
  DragFinish((HDROP)hDrop);
  return TRUE;
}

void UpdateWindowLayout(HWND hwnd) {
  BOOL fMinimized = IsIconic(hwnd);
  BOOL fMaximized = IsZoomed(hwnd);
  RECT rect;
  GetWindowRect(hwnd, &rect);
  MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left - 1,
             rect.bottom - rect.top, TRUE);
  MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left,
             rect.bottom - rect.top, TRUE);
}

BOOL C4Viewport::TogglePlayerLock() {
  // Disable player lock
  if (PlayerLock) {
    PlayerLock = FALSE;
    SetWindowLong(pWindow->hWindow, GWL_STYLE,
                  C4ViewportWindowStyle | WS_HSCROLL | WS_VSCROLL);
    UpdateWindowLayout(pWindow->hWindow);
    ScrollBarsByViewPosition();
  }
  // Enable player lock
  else if (ValidPlr(Player)) {
    PlayerLock = TRUE;
    SetWindowLong(pWindow->hWindow, GWL_STYLE, C4ViewportWindowStyle);
    UpdateWindowLayout(pWindow->hWindow);
  }
  return TRUE;
}

BOOL C4Viewport::ViewPositionByScrollBars() {
  if (PlayerLock) return FALSE;
  SCROLLINFO scroll;
  scroll.cbSize = sizeof(SCROLLINFO);
  // Vertical
  scroll.fMask = SIF_POS;
  GetScrollInfo(pWindow->hWindow, SB_VERT, &scroll);
  ViewY = scroll.nPos;
  // Horizontal
  scroll.fMask = SIF_POS;
  GetScrollInfo(pWindow->hWindow, SB_HORZ, &scroll);
  ViewX = scroll.nPos;
  return TRUE;
}

BOOL C4Viewport::ScrollBarsByViewPosition() {
  if (PlayerLock) return FALSE;
  SCROLLINFO scroll;
  scroll.cbSize = sizeof(SCROLLINFO);
  // Vertical
  scroll.fMask = SIF_ALL;
  scroll.nMin = 0;
  scroll.nMax = GBackHgt;
  scroll.nPage = ViewHgt;
  scroll.nPos = ViewY;
  SetScrollInfo(pWindow->hWindow, SB_VERT, &scroll, TRUE);
  // Horizontal
  scroll.fMask = SIF_ALL;
  scroll.nMin = 0;
  scroll.nMax = GBackWdt;
  scroll.nPage = ViewWdt;
  scroll.nPos = ViewX;
  SetScrollInfo(pWindow->hWindow, SB_HORZ, &scroll, TRUE);
  return TRUE;
}

#elif defined(WITH_DEVELOPER_MODE)
static GtkTargetEntry drag_drop_entries[] = {{"text/uri-list", 0, 0}};

// GTK+ Viewport window implementation
GtkWidget* C4ViewportWindow::InitGUI() {
  gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);

  // Cannot just use ScrolledWindow because this would just move
  // the GdkWindow of the DrawingArea.
  GtkWidget* table;

  drawing_area = gtk_drawing_area_new();
  h_scrollbar = gtk_hscrollbar_new(NULL);
  v_scrollbar = gtk_vscrollbar_new(NULL);
  table = gtk_table_new(2, 2, FALSE);

  GtkAdjustment* adjustment = gtk_range_get_adjustment(GTK_RANGE(h_scrollbar));
  adjustment->lower = 0;
  adjustment->upper = GBackWdt;
  adjustment->step_increment = ViewportScrollSpeed;

  g_signal_connect(G_OBJECT(adjustment), "value-changed",
                   G_CALLBACK(OnHScrollStatic), this);

  adjustment = gtk_range_get_adjustment(GTK_RANGE(v_scrollbar));
  adjustment->lower = 0;
  adjustment->upper = GBackHgt;
  adjustment->step_increment = ViewportScrollSpeed;

  g_signal_connect(G_OBJECT(adjustment), "value-changed",
                   G_CALLBACK(OnVScrollStatic), this);

  gtk_table_attach(GTK_TABLE(table), drawing_area, 0, 1, 0, 1,
                   static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL),
                   static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_table_attach(GTK_TABLE(table), v_scrollbar, 1, 2, 0, 1, GTK_SHRINK,
                   static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND), 0, 0);
  gtk_table_attach(GTK_TABLE(table), h_scrollbar, 0, 1, 1, 2,
                   static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL),
                   GTK_SHRINK, 0, 0);

  gtk_container_add(GTK_CONTAINER(window), table);

  gtk_widget_add_events(window,
                        GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
                            GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                            GDK_STRUCTURE_MASK | GDK_POINTER_MOTION_MASK);

  gtk_drag_dest_set(drawing_area, GTK_DEST_DEFAULT_ALL, drag_drop_entries, 1,
                    GDK_ACTION_COPY);
  g_signal_connect(G_OBJECT(drawing_area), "drag-data-received",
                   G_CALLBACK(OnDragDataReceivedStatic), this);
  g_signal_connect(G_OBJECT(drawing_area), "expose-event",
                   G_CALLBACK(OnExposeStatic), this);

  g_signal_connect(G_OBJECT(window), "key-press-event",
                   G_CALLBACK(OnKeyPressStatic), this);
  g_signal_connect(G_OBJECT(window), "key-release-event",
                   G_CALLBACK(OnKeyReleaseStatic), this);
  g_signal_connect(G_OBJECT(window), "scroll-event", G_CALLBACK(OnScrollStatic),
                   this);
  g_signal_connect(G_OBJECT(window), "button-press-event",
                   G_CALLBACK(OnButtonPressStatic), this);
  g_signal_connect(G_OBJECT(window), "button-release-event",
                   G_CALLBACK(OnButtonReleaseStatic), this);
  g_signal_connect(G_OBJECT(window), "motion-notify-event",
                   G_CALLBACK(OnMotionNotifyStatic), this);
  g_signal_connect(G_OBJECT(window), "configure-event",
                   G_CALLBACK(OnConfigureStatic), this);
  g_signal_connect(G_OBJECT(window), "realize", G_CALLBACK(OnRealizeStatic),
                   this);

  g_signal_connect_after(G_OBJECT(drawing_area), "configure-event",
                         G_CALLBACK(OnConfigureDareaStatic), this);

  // do not draw the default background
  gtk_widget_set_double_buffered(drawing_area, FALSE);

  return drawing_area;
}

BOOL C4Viewport::TogglePlayerLock() {
  if (PlayerLock) {
    PlayerLock = false;
    gtk_widget_show(pWindow->h_scrollbar);
    gtk_widget_show(pWindow->v_scrollbar);
    ScrollBarsByViewPosition();
  } else {
    PlayerLock = true;
    gtk_widget_hide(pWindow->h_scrollbar);
    gtk_widget_hide(pWindow->v_scrollbar);
  }

  return TRUE;
}

BOOL C4Viewport::ScrollBarsByViewPosition() {
  if (PlayerLock) return FALSE;

  GtkAdjustment* adjustment =
      gtk_range_get_adjustment(GTK_RANGE(pWindow->h_scrollbar));
  adjustment->page_increment = pWindow->drawing_area->allocation.width;
  adjustment->page_size = pWindow->drawing_area->allocation.width;
  adjustment->value = ViewX;
  gtk_adjustment_changed(adjustment);

  adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->v_scrollbar));
  adjustment->page_increment = pWindow->drawing_area->allocation.height;
  adjustment->page_size = pWindow->drawing_area->allocation.height;
  adjustment->value = ViewY;
  gtk_adjustment_changed(adjustment);

  return TRUE;
}

BOOL C4Viewport::ViewPositionByScrollBars() {
  if (PlayerLock) return FALSE;

  GtkAdjustment* adjustment =
      gtk_range_get_adjustment(GTK_RANGE(pWindow->h_scrollbar));
  ViewX = static_cast<int32_t>(adjustment->value);

  adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->v_scrollbar));
  ViewY = static_cast<int32_t>(adjustment->value);

  return TRUE;
}

void C4ViewportWindow::OnDragDataReceivedStatic(GtkWidget* widget,
                                                GdkDragContext* context, gint x,
                                                gint y, GtkSelectionData* data,
                                                guint info, guint time,
                                                gpointer user_data) {
  C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);

  gchar** uris = gtk_selection_data_get_uris(data);
  if (!uris) return;

  for (gchar** uri = uris; *uri != NULL; ++uri) {
    gchar* file = g_filename_from_uri(*uri, NULL, NULL);
    if (!file) continue;

    Game.DropFile(file, window->cvp->ViewX + x, window->cvp->ViewY + y);
    g_free(file);
  }

  g_strfreev(uris);
}

gboolean C4ViewportWindow::OnExposeStatic(GtkWidget* widget,
                                          GdkEventExpose* event,
                                          gpointer user_data) {
  C4Viewport* cvp = static_cast<C4ViewportWindow*>(user_data)->cvp;

  // TODO: Redraw only event->area
  cvp->Execute();
  return TRUE;
}

void C4ViewportWindow::OnRealizeStatic(GtkWidget* widget, gpointer user_data) {
  // Initial PlayerLock
  if (static_cast<C4ViewportWindow*>(user_data)->cvp->PlayerLock == TRUE) {
    gtk_widget_hide(static_cast<C4ViewportWindow*>(user_data)->h_scrollbar);
    gtk_widget_hide(static_cast<C4ViewportWindow*>(user_data)->v_scrollbar);
  }
}

gboolean C4ViewportWindow::OnKeyPressStatic(GtkWidget* widget,
                                            GdkEventKey* event,
                                            gpointer user_data) {
  if (event->keyval == GDK_Scroll_Lock)
    static_cast<C4ViewportWindow*>(user_data)->cvp->TogglePlayerLock();
#ifdef _DEBUG
  switch (event->keyval) {
    case GDK_1:
      DDrawCfg.fBlitOff -= 0.05;
      printf("%f\n", DDrawCfg.fBlitOff);
      break;
    case GDK_2:
      DDrawCfg.fBlitOff += 0.05;
      printf("%f\n", DDrawCfg.fBlitOff);
      break;
    case GDK_3:
      DDrawCfg.fTexIndent -= 0.05;
      printf("%f\n", DDrawCfg.fTexIndent);
      break;
    case GDK_4:
      DDrawCfg.fTexIndent += 0.05;
      printf("%f\n", DDrawCfg.fTexIndent);
      break;
  }
#endif
  DWORD key = XKeycodeToKeysym(GDK_WINDOW_XDISPLAY(event->window),
                               event->hardware_keycode, 0);
  Game.DoKeyboardInput(key, KEYEV_Down, !!(event->state & GDK_MOD1_MASK),
                       !!(event->state & GDK_CONTROL_MASK),
                       !!(event->state & GDK_SHIFT_MASK), false, NULL);
  return TRUE;
}

gboolean C4ViewportWindow::OnKeyReleaseStatic(GtkWidget* widget,
                                              GdkEventKey* event,
                                              gpointer user_data) {
  DWORD key = XKeycodeToKeysym(GDK_WINDOW_XDISPLAY(event->window),
                               event->hardware_keycode, 0);
  Game.DoKeyboardInput(key, KEYEV_Up, !!(event->state & GDK_MOD1_MASK),
                       !!(event->state & GDK_CONTROL_MASK),
                       !!(event->state & GDK_SHIFT_MASK), false, NULL);
  return TRUE;
}

gboolean C4ViewportWindow::OnScrollStatic(GtkWidget* widget,
                                          GdkEventScroll* event,
                                          gpointer user_data) {
  C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);

  if (Game.MouseControl.IsViewport(window->cvp) &&
      (Console.EditCursor.GetMode() == C4CNS_ModePlay)) {
    switch (event->direction) {
      case GDK_SCROLL_UP:
        Game.GraphicsSystem.MouseMove(
            C4MC_Button_Wheel, (int32_t)event->x, (int32_t)event->y,
            event->state + (short(1) << 16), window->cvp);
        break;
      case GDK_SCROLL_DOWN:
        Game.GraphicsSystem.MouseMove(
            C4MC_Button_Wheel, (int32_t)event->x, (int32_t)event->y,
            event->state + (short(-1) << 16), window->cvp);
        break;
    }
  }

  return TRUE;
}

gboolean C4ViewportWindow::OnButtonPressStatic(GtkWidget* widget,
                                               GdkEventButton* event,
                                               gpointer user_data) {
  C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);

  if (Game.MouseControl.IsViewport(window->cvp) &&
      (Console.EditCursor.GetMode() == C4CNS_ModePlay)) {
    switch (event->button) {
      case 1:
        if (event->type == GDK_BUTTON_PRESS)
          Game.GraphicsSystem.MouseMove(C4MC_Button_LeftDown, (int32_t)event->x,
                                        (int32_t)event->y, event->state,
                                        window->cvp);
        else if (event->type == GDK_2BUTTON_PRESS)
          Game.GraphicsSystem.MouseMove(C4MC_Button_LeftDouble,
                                        (int32_t)event->x, (int32_t)event->y,
                                        event->state, window->cvp);
        break;
      case 2:
        Game.GraphicsSystem.MouseMove(C4MC_Button_MiddleDown, (int32_t)event->x,
                                      (int32_t)event->y, event->state,
                                      window->cvp);
        break;
      case 3:
        if (event->type == GDK_BUTTON_PRESS)
          Game.GraphicsSystem.MouseMove(C4MC_Button_RightDown,
                                        (int32_t)event->x, (int32_t)event->y,
                                        event->state, window->cvp);
        else if (event->type == GDK_2BUTTON_PRESS)
          Game.GraphicsSystem.MouseMove(C4MC_Button_RightDouble,
                                        (int32_t)event->x, (int32_t)event->y,
                                        event->state, window->cvp);
        break;
    }
  } else {
    switch (event->button) {
      case 1:
        Console.EditCursor.LeftButtonDown(event->state & MK_CONTROL);
        break;
      case 3:
        Console.EditCursor.RightButtonDown(event->state & MK_CONTROL);
        break;
    }
  }

  return TRUE;
}

gboolean C4ViewportWindow::OnButtonReleaseStatic(GtkWidget* widget,
                                                 GdkEventButton* event,
                                                 gpointer user_data) {
  C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);

  if (Game.MouseControl.IsViewport(window->cvp) &&
      (Console.EditCursor.GetMode() == C4CNS_ModePlay)) {
    switch (event->button) {
      case 1:
        Game.GraphicsSystem.MouseMove(C4MC_Button_LeftUp, (int32_t)event->x,
                                      (int32_t)event->y, event->state,
                                      window->cvp);
        break;
      case 2:
        Game.GraphicsSystem.MouseMove(C4MC_Button_MiddleUp, (int32_t)event->x,
                                      (int32_t)event->y, event->state,
                                      window->cvp);
        break;
      case 3:
        Game.GraphicsSystem.MouseMove(C4MC_Button_RightUp, (int32_t)event->x,
                                      (int32_t)event->y, event->state,
                                      window->cvp);
        break;
    }
  } else {
    switch (event->button) {
      case 1:
        Console.EditCursor.LeftButtonUp();
        break;
      case 3:
        Console.EditCursor.RightButtonUp();
        break;
    }
  }

  return TRUE;
}

gboolean C4ViewportWindow::OnMotionNotifyStatic(GtkWidget* widget,
                                                GdkEventMotion* event,
                                                gpointer user_data) {
  C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);

  if (Game.MouseControl.IsViewport(window->cvp) &&
      (Console.EditCursor.GetMode() == C4CNS_ModePlay)) {
    Game.GraphicsSystem.MouseMove(C4MC_Button_None, (int32_t)event->x,
                                  (int32_t)event->y, event->state, window->cvp);
  } else {
    Console.EditCursor.Move(window->cvp->ViewX + (int32_t)event->x,
                            window->cvp->ViewY + (int32_t)event->y,
                            event->state);
  }

  return TRUE;
}

gboolean C4ViewportWindow::OnConfigureStatic(GtkWidget* widget,
                                             GdkEventConfigure* event,
                                             gpointer user_data) {
  C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);
  C4Viewport* cvp = window->cvp;

  // cvp->UpdateOutputSize();
  cvp->ScrollBarsByViewPosition();

  return FALSE;
}

gboolean C4ViewportWindow::OnConfigureDareaStatic(GtkWidget* widget,
                                                  GdkEventConfigure* event,
                                                  gpointer user_data) {
  C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);
  C4Viewport* cvp = window->cvp;

  cvp->UpdateOutputSize();

  return FALSE;
}

void C4ViewportWindow::OnVScrollStatic(GtkAdjustment* adjustment,
                                       gpointer user_data) {
  static_cast<C4ViewportWindow*>(user_data)->cvp->ViewPositionByScrollBars();
}

void C4ViewportWindow::OnHScrollStatic(GtkAdjustment* adjustment,
                                       gpointer user_data) {
  static_cast<C4ViewportWindow*>(user_data)->cvp->ViewPositionByScrollBars();
}

#else  // WITH_DEVELOPER_MODE
BOOL C4Viewport::TogglePlayerLock() { return false; }
BOOL C4Viewport::ScrollBarsByViewPosition() { return false; }
#if defined(USE_X11)
void C4ViewportWindow::HandleMessage(XEvent &e) {
  switch (e.type) {
    case KeyPress: {
      // Do not take into account the state of the various modifiers and locks
      // we don't need that for keyboard control
      DWORD key = XKeycodeToKeysym(e.xany.display, e.xkey.keycode, 0);
      // case VK_SCROLL:	cvp->TogglePlayerLock();
      Game.DoKeyboardInput(key, KEYEV_Down, Application.IsAltDown(),
                           Application.IsControlDown(),
                           Application.IsShiftDown(), false, NULL);
      break;
    }
    case KeyRelease: {
      DWORD key = XKeycodeToKeysym(e.xany.display, e.xkey.keycode, 0);
      Game.DoKeyboardInput(key, KEYEV_Up, e.xkey.state & Mod1Mask,
                           e.xkey.state & ControlMask, e.xkey.state & ShiftMask,
                           false, NULL);
      break;
    }
    case ButtonPress: {
      static int last_left_click, last_right_click;
      if (Game.MouseControl.IsViewport(cvp) &&
          (Console.EditCursor.GetMode() == C4CNS_ModePlay)) {
        switch (e.xbutton.button) {
          case Button1:
            if (timeGetTime() - last_left_click < 400) {
              Game.GraphicsSystem.MouseMove(C4MC_Button_LeftDouble, e.xbutton.x,
                                            e.xbutton.y, e.xbutton.state, cvp);
              last_left_click = 0;
            } else {
              Game.GraphicsSystem.MouseMove(C4MC_Button_LeftDown, e.xbutton.x,
                                            e.xbutton.y, e.xbutton.state, cvp);
              last_left_click = timeGetTime();
            }
            break;
          case Button2:
            Game.GraphicsSystem.MouseMove(C4MC_Button_MiddleDown, e.xbutton.x,
                                          e.xbutton.y, e.xbutton.state, cvp);
            break;
          case Button3:
            if (timeGetTime() - last_right_click < 400) {
              Game.GraphicsSystem.MouseMove(C4MC_Button_RightDouble,
                                            e.xbutton.x, e.xbutton.y,
                                            e.xbutton.state, cvp);
              last_right_click = 0;
            } else {
              Game.GraphicsSystem.MouseMove(C4MC_Button_RightDown, e.xbutton.x,
                                            e.xbutton.y, e.xbutton.state, cvp);
              last_right_click = timeGetTime();
            }
            break;
          case Button4:
            Game.GraphicsSystem.MouseMove(
                C4MC_Button_Wheel, e.xbutton.x, e.xbutton.y,
                e.xbutton.state + (short(1) << 16), cvp);
            break;
          case Button5:
            Game.GraphicsSystem.MouseMove(
                C4MC_Button_Wheel, e.xbutton.x, e.xbutton.y,
                e.xbutton.state + (short(-1) << 16), cvp);
            break;
          default:
            break;
        }
      } else {
        switch (e.xbutton.button) {
          case Button1:
            Console.EditCursor.LeftButtonDown(e.xbutton.state & MK_CONTROL);
            break;
          case Button3:
            Console.EditCursor.RightButtonDown(e.xbutton.state & MK_CONTROL);
            break;
        }
      }
    } break;
    case ButtonRelease:
      if (Game.MouseControl.IsViewport(cvp) &&
          (Console.EditCursor.GetMode() == C4CNS_ModePlay)) {
        switch (e.xbutton.button) {
          case Button1:
            Game.GraphicsSystem.MouseMove(C4MC_Button_LeftUp, e.xbutton.x,
                                          e.xbutton.y, e.xbutton.state, cvp);
            break;
          case Button2:
            Game.GraphicsSystem.MouseMove(C4MC_Button_MiddleUp, e.xbutton.x,
                                          e.xbutton.y, e.xbutton.state, cvp);
            break;
          case Button3:
            Game.GraphicsSystem.MouseMove(C4MC_Button_RightUp, e.xbutton.x,
                                          e.xbutton.y, e.xbutton.state, cvp);
            break;
          default:
            break;
        }
      } else {
        switch (e.xbutton.button) {
          case Button1:
            Console.EditCursor.LeftButtonUp();
            break;
          case Button3:
            Console.EditCursor.RightButtonUp();
            break;
        }
      }
      break;
    case MotionNotify:
      if (Game.MouseControl.IsViewport(cvp) &&
          (Console.EditCursor.GetMode() == C4CNS_ModePlay)) {
        Game.GraphicsSystem.MouseMove(C4MC_Button_None, e.xbutton.x,
                                      e.xbutton.y, e.xbutton.state, cvp);
      } else {
        Console.EditCursor.Move(cvp->ViewX + e.xbutton.x,
                                cvp->ViewY + e.xbutton.y, e.xbutton.state);
      }
      break;
    case ConfigureNotify:
      cvp->UpdateOutputSize();
      break;
  }
}
#endif  // USE_X11
#endif  // WITH_DEVELOPER_MODE/_WIN32

void C4ViewportWindow::Close() { Game.GraphicsSystem.CloseViewport(cvp); }

BOOL C4Viewport::UpdateOutputSize() {
  if (!pWindow) return FALSE;
  // Output size
  RECT rect;
#ifdef WITH_DEVELOPER_MODE
  // Use only size of drawing area without scrollbars
  rect.left = pWindow->drawing_area->allocation.x;
  rect.top = pWindow->drawing_area->allocation.y;
  rect.right = rect.left + pWindow->drawing_area->allocation.width;
  rect.bottom = rect.top + pWindow->drawing_area->allocation.height;
#else
  if (!pWindow->GetSize(&rect)) return FALSE;
#endif
  OutX = rect.left;
  OutY = rect.top;
  ViewWdt = rect.right - rect.left;
  ViewHgt = rect.bottom - rect.top;
  // Scroll bars
  ScrollBarsByViewPosition();
  // Reset menus
  ResetMenuPositions = TRUE;
#ifdef USE_GL
  // update internal GL size
  if (pCtx) pCtx->UpdateSize();
#endif
  // Done
  return TRUE;
}

C4Viewport::C4Viewport() { Default(); }

C4Viewport::~C4Viewport() { Clear(); }

void C4Viewport::Clear() {
#ifdef USE_GL
  if (pCtx) {
    delete pCtx;
    pCtx = NULL;
  }
#endif
  if (pWindow) {
    pWindow->Clear();
    delete pWindow;
    pWindow = NULL;
  }
  Player = NO_OWNER;
  ViewX = ViewY = ViewWdt = ViewHgt = 0;
  OutX = OutY = ViewWdt = ViewHgt = 0;
  DrawX = DrawY = 0;
  Regions.Clear();
  dViewX = dViewY = -31337;
  ViewOffsX = ViewOffsY = 0;
}

void C4Viewport::DrawOverlay(C4FacetEx &cgo) {
  if (!Game.C4S.Head.Film || !Game.C4S.Head.Replay) {
    C4ST_STARTNEW(FoWStat, "C4Viewport::DrawOverlay: FoW")
    // Player fog of war
    DrawPlayerFogOfWar(cgo);
    C4ST_STOP(FoWStat)
    // Player info
    C4ST_STARTNEW(CInfoStat, "C4Viewport::DrawOverlay: Cursor Info")
    DrawCursorInfo(cgo);
    C4ST_STOP(CInfoStat)
    C4ST_STARTNEW(PInfoStat, "C4Viewport::DrawOverlay: Player Info")
    DrawPlayerInfo(cgo);
    C4ST_STOP(PInfoStat)
    C4ST_STARTNEW(MenuStat, "C4Viewport::DrawOverlay: Menu")
    DrawMenu(cgo);
    C4ST_STOP(MenuStat)
  }
  // Game messages
  C4ST_STARTNEW(MsgStat, "C4Viewport::DrawOverlay: Messages")
  Game.Messages.Draw(cgo, Player);
  C4ST_STOP(MsgStat)

  // Control overlays (if not film/replay)
  if (!Game.C4S.Head.Film || !Game.C4S.Head.Replay)
    // Mouse control
    if (Game.MouseControl.IsViewport(this)) {
      C4ST_STARTNEW(MouseStat, "C4Viewport::DrawOverlay: Mouse")
      if (Config.Graphics.ShowCommands)  // Now, ShowCommands is respected even
                                         // for mouse control...
        DrawMouseButtons(cgo);
      Game.MouseControl.Draw(cgo);
      // Draw GUI-mouse in EM if active
      if (pWindow && Game.pGUI) Game.pGUI->RenderMouse(cgo);
      C4ST_STOP(MouseStat)
    }
    // Keyboard/Gamepad
    else {
      // Player menu button
      if (Config.Graphics.ShowCommands) {
        int32_t iSymbolSize = C4SymbolSize * 2 / 3;
        C4Facet ccgo;
        ccgo.Set(cgo.Surface, cgo.X + cgo.Wdt - iSymbolSize,
                 cgo.Y + C4SymbolSize + 2 * C4SymbolBorder, iSymbolSize,
                 iSymbolSize);
        ccgo.Y += iSymbolSize;
        DrawCommandKey(ccgo, COM_PlayerMenu, FALSE,
                       PlrControlKeyName(Player, Com2Control(COM_PlayerMenu),
                                         true).getData());
      }
    }
}

void C4Viewport::DrawCursorInfo(C4FacetEx &cgo) {
  C4Facet ccgo, ccgo2;

  // Get cursor
  C4Object *cursor, *realcursor;
  C4Player *pPlr = Game.Players.Get(Player);
  if (!pPlr) return;
  realcursor = pPlr->Cursor;
  if (!(cursor = pPlr->ViewCursor))
    if (!(cursor = realcursor)) return;

  // Draw info
  if (Config.Graphics.ShowPlayerInfoAlways)
    if (cursor->Info) {
      C4ST_STARTNEW(ObjInfStat, "C4Viewport::DrawCursorInfo: Object info")
      ccgo.Set(cgo.Surface, cgo.X + C4SymbolBorder, cgo.Y + C4SymbolBorder,
               3 * C4SymbolSize, C4SymbolSize);
      cursor->Info->Draw(
          ccgo,
          Config.Graphics
              .ShowPortraits,  // && Game.Players.Get(Player)->CursorFlash,
          (cursor == Game.Players.Get(Player)->Captain),
          cursor);
      C4ST_STOP(ObjInfStat)
    }

  C4ST_STARTNEW(ContStat, "C4Viewport::DrawCursorInfo: Contents")

  // Draw contents
  if (cursor->Contents.ObjectCount() == 1) {
    // Single object
    ccgo.Set(cgo.Surface, cgo.X + C4SymbolBorder,
             cgo.Y + cgo.Hgt - C4SymbolBorder - C4SymbolSize, C4SymbolSize,
             C4SymbolSize);
    cursor->Contents.GetObject()->DrawPicture(ccgo, FALSE, SetRegions);
  } else {
    // Object list
    ccgo.Set(cgo.Surface, cgo.X + C4SymbolBorder,
             cgo.Y + cgo.Hgt - C4SymbolBorder - C4SymbolSize, 7 * C4SymbolSize,
             C4SymbolSize);
    cursor->Contents.DrawIDList(ccgo, -1, Game.Defs, C4D_All, SetRegions,
                                COM_Contents, FALSE);
  }

  C4ST_STOP(ContStat)

  // Draw energy levels
  if (cursor->ViewEnergy || Config.Graphics.ShowPlayerInfoAlways)
    if (cgo.Hgt > 2 * C4SymbolSize + 2 * C4SymbolBorder) {
      int32_t cx = C4SymbolBorder;
      C4ST_STARTNEW(EnStat, "C4Viewport::DrawCursorInfo: Energy")
      int32_t bar_wdt = Game.GraphicsResource.fctEnergyBars.Wdt;
      int32_t iYOff = Config.Graphics.ShowPortraits ? 10 : 0;
      // Energy
      ccgo.Set(cgo.Surface, cgo.X + cx,
               cgo.Y + C4SymbolSize + 2 * C4SymbolBorder + iYOff, bar_wdt,
               cgo.Hgt - 3 * C4SymbolBorder - 2 * C4SymbolSize - iYOff);
      cursor->DrawEnergy(ccgo);
      ccgo.X += bar_wdt + 1;
      // Magic energy
      if (cursor->MagicEnergy) {
        cursor->DrawMagicEnergy(ccgo);
        ccgo.X += bar_wdt + 1;
      }
      // Breath
      if (cursor->Breath && (cursor->Breath < cursor->GetPhysical()->Breath)) {
        cursor->DrawBreath(ccgo);
        ccgo.X += bar_wdt + 1;
      }
      C4ST_STOP(EnStat)
    }

  // Draw commands
  if (Config.Graphics
          .ShowCommands /*|| Game.MouseControl.IsViewport(this)*/)  // Now,
                                                                    // ShowCommands
                                                                    // is
                                                                    // respected
                                                                    // even for
                                                                    // mouse
                                                                    // control
    if (realcursor)
      if (cgo.Hgt > C4SymbolSize) {
        C4ST_STARTNEW(CmdStat, "C4Viewport::DrawCursorInfo: Commands")
        int32_t iSize = 2 * C4SymbolSize / 3;
        int32_t iSize2 = 2 * iSize;
        // Primary area (bottom)
        ccgo.Set(cgo.Surface, cgo.X, cgo.Y + cgo.Hgt - iSize, cgo.Wdt, iSize);
        // Secondary area (side)
        ccgo2.Set(cgo.Surface, cgo.X + cgo.Wdt - iSize2, cgo.Y, iSize2,
                  cgo.Hgt - iSize - 5);
        // Draw commands
        realcursor->DrawCommands(ccgo, ccgo2, SetRegions);
        C4ST_STOP(CmdStat)
      }
}

void C4Viewport::DrawMenu(C4FacetEx &cgo) {
  // Get player
  C4Player *pPlr = Game.Players.Get(Player);

  // Player eliminated
  if (pPlr && pPlr->Eliminated) {
    Application.DDraw->TextOut(
        FormatString(LoadResStr(pPlr->Surrendered ? "IDS_PLR_SURRENDERED"
                                                  : "IDS_PLR_ELIMINATED"),
                     pPlr->GetName()).getData(),
        Game.GraphicsResource.FontRegular, 1.0, cgo.Surface,
        cgo.X + cgo.Wdt / 2, cgo.Y + 2 * cgo.Hgt / 3, 0xfaFF0000, ACenter);
    return;
  }

  // for menus, cgo is using GUI-syntax: TargetX/Y marks the drawing offset;
  // x/y/Wdt/Hgt marks the offset rect
  int32_t iOldTx = cgo.TargetX;
  int32_t iOldTy = cgo.TargetY;
  cgo.TargetX = cgo.X;
  cgo.TargetY = cgo.Y;
  cgo.X = 0;
  cgo.Y = 0;

  // Player cursor object menu
  if (pPlr && pPlr->Cursor && pPlr->Cursor->Menu) {
    if (ResetMenuPositions) pPlr->Cursor->Menu->ResetLocation();
    // if mouse is dragging, make it transparent to easy construction site
    // drag+drop
    bool fDragging = false;
    if (Game.MouseControl.IsDragging() && Game.MouseControl.IsViewport(this)) {
      fDragging = true;
      lpDDraw->ActivateBlitModulation(0xafffffff);
    }
    // draw menu
    pPlr->Cursor->Menu->Draw(cgo);
    // reset modulation for dragging
    if (fDragging) lpDDraw->DeactivateBlitModulation();
  }
  // Player menu
  if (pPlr && pPlr->Menu.IsActive()) {
    if (ResetMenuPositions) pPlr->Menu.ResetLocation();
    pPlr->Menu.Draw(cgo);
  }
  // Fullscreen menu
  if (FullScreen.pMenu && FullScreen.pMenu->IsActive()) {
    if (ResetMenuPositions) FullScreen.pMenu->ResetLocation();
    FullScreen.pMenu->Draw(cgo);
  }

  // Flag done
  ResetMenuPositions = FALSE;

  // restore cgo
  cgo.X = cgo.TargetX;
  cgo.Y = cgo.TargetY;
  cgo.TargetX = iOldTx;
  cgo.TargetY = iOldTy;
}

extern int32_t iLastControlSize, iPacketDelay, ScreenRate;
extern int32_t ControlQueueSize, ControlQueueDataSize;

void C4Viewport::Draw(C4FacetEx &cgo, bool fDrawOverlay) {
#ifdef USE_CONSOLE
  // No drawing in console mode
  return;
#endif

  // landscape mod by FoW
  C4Player *pPlr = Game.Players.Get(Player);
  if (pPlr && pPlr->fFogOfWar) {
    ClrModMap.Reset(Game.C4S.Landscape.FoWRes, Game.C4S.Landscape.FoWRes,
                    ViewWdt, ViewHgt, cgo.TargetX, cgo.TargetY, 0, 0, cgo.X,
                    cgo.Y, Game.FoWColor, cgo.Surface);
    pPlr->FoW2Map(ClrModMap, cgo.X - cgo.TargetX, cgo.Y - cgo.TargetY);
    lpDDraw->SetClrModMap(&ClrModMap);
    lpDDraw->SetClrModMapEnabled(true);
  } else
    lpDDraw->SetClrModMapEnabled(false);

  if (fDrawOverlay) {
    // Draw landscape borders. Only if overlay, so complete map screenshots
    // don't get messed up
    if (BorderLeft)
      Application.DDraw->BlitSurfaceTile(
          Game.GraphicsResource.fctBackground.Surface, cgo.Surface, DrawX,
          DrawY, BorderLeft, ViewHgt, -DrawX, -DrawY);
    if (BorderTop)
      Application.DDraw->BlitSurfaceTile(
          Game.GraphicsResource.fctBackground.Surface, cgo.Surface,
          DrawX + BorderLeft, DrawY, ViewWdt - BorderLeft - BorderRight,
          BorderTop, -DrawX - BorderLeft, -DrawY);
    if (BorderRight)
      Application.DDraw->BlitSurfaceTile(
          Game.GraphicsResource.fctBackground.Surface, cgo.Surface,
          DrawX + ViewWdt - BorderRight, DrawY, BorderRight, ViewHgt,
          -DrawX - ViewWdt + BorderRight, -DrawY);
    if (BorderBottom)
      Application.DDraw->BlitSurfaceTile(
          Game.GraphicsResource.fctBackground.Surface, cgo.Surface,
          DrawX + BorderLeft, DrawY + ViewHgt - BorderBottom,
          ViewWdt - BorderLeft - BorderRight, BorderBottom, -DrawX - BorderLeft,
          -DrawY - ViewHgt + BorderBottom);

    // Set clippers
    Application.DDraw->SetPrimaryClipper(DrawX + BorderLeft, DrawY + BorderTop,
                                         DrawX + ViewWdt - 1 - BorderRight,
                                         DrawY + ViewHgt - 1 - BorderBottom);
    cgo.X += BorderLeft;
    cgo.Y += BorderTop;
    cgo.Wdt -= BorderLeft + BorderRight;
    cgo.Hgt -= BorderTop + BorderBottom;
    cgo.TargetX += BorderLeft;
    cgo.TargetY += BorderTop;
  }

  C4ST_STARTNEW(SkyStat, "C4Viewport::Draw: Sky")
  Game.Landscape.Sky.Draw(cgo);
  C4ST_STOP(SkyStat)
  Game.BackObjects.DrawAll(cgo, Player);

  // Draw Landscape
  C4ST_STARTNEW(LandStat, "C4Viewport::Draw: Landscape")
  Game.Landscape.Draw(cgo, Player);
  C4ST_STOP(LandStat)

  // draw PXS (unclipped!)
  C4ST_STARTNEW(PXSStat, "C4Viewport::Draw: PXS")
  Game.PXS.Draw(cgo);
  C4ST_STOP(PXSStat)

  // draw objects
  C4ST_STARTNEW(ObjStat, "C4Viewport::Draw: Objects")
  Game.Objects.Draw(cgo, Player);
  C4ST_STOP(ObjStat)

  // draw global particles
  C4ST_STARTNEW(PartStat, "C4Viewport::Draw: Particles")
  Game.Particles.GlobalParticles.Draw(cgo, NULL);
  C4ST_STOP(PartStat)

  // draw foreground objects
  Game.ForeObjects.DrawIfCategory(cgo, Player, C4D_Parallax, true);

  // Draw PathFinder
  if (Game.GraphicsSystem.ShowPathfinder) Game.PathFinder.Draw(cgo);

  // Draw overlay
  if (!Game.C4S.Head.Film || !Game.C4S.Head.Replay)
    Game.DrawCursors(cgo, Player);

  // FogOfWar-mod off
  lpDDraw->SetClrModMapEnabled(false);

  // now restore complete cgo range for overlay drawing
  if (fDrawOverlay) {
    cgo.X -= BorderLeft;
    cgo.Y -= BorderTop;
    cgo.Wdt += BorderLeft + BorderRight;
    cgo.Hgt += BorderTop + BorderBottom;
    cgo.TargetX -= BorderLeft;
    cgo.TargetY -= BorderTop;
    Application.DDraw->SetPrimaryClipper(DrawX, DrawY, DrawX + ViewWdt - 1,
                                         DrawY + ViewHgt - 1);
  }

  // draw custom GUI objects
  Game.ForeObjects.DrawIfCategory(cgo, Player, C4D_Parallax, false);

  // Draw overlay
  C4ST_STARTNEW(OvrStat, "C4Viewport::Draw: Overlay")

  if (!Application.isFullScreen) Console.EditCursor.Draw(cgo);

  if (fDrawOverlay) DrawOverlay(cgo);

  // Netstats
  if (Game.GraphicsSystem.ShowNetstatus) Game.Network.DrawStatus(cgo);

  C4ST_STOP(OvrStat)

  // Remove clippers
  if (fDrawOverlay) Application.DDraw->NoPrimaryClipper();
}

void C4Viewport::BlitOutput() {
  if (pWindow) {
    RECT rtSrc, rtDst;
    rtSrc.left = DrawX;
    rtSrc.top = DrawY;
    rtSrc.right = DrawX + ViewWdt;
    rtSrc.bottom = DrawY + ViewHgt;
    rtDst.left = OutX;
    rtDst.top = OutY;
    rtDst.right = OutX + ViewWdt;
    rtDst.bottom = OutY + ViewHgt;
    Application.DDraw->PageFlip(&rtSrc, &rtDst, pWindow);
  }
}

void C4Viewport::Execute() {
  // Update regions
  static int32_t RegionUpdate = 0;
  SetRegions = NULL;
  RegionUpdate++;
  if (RegionUpdate >= 5) {
    RegionUpdate = 0;
    Regions.Clear();
    Regions.SetAdjust(-OutX, -OutY);
    SetRegions = &Regions;
  }
  // Adjust position
  AdjustPosition();
#ifdef USE_GL
  // select rendering context
  if (pCtx)
    if (!pCtx->Select()) return;
#endif
  // Current graphics output
  C4FacetEx cgo;
  cgo.Set(Application.DDraw->lpBack, DrawX, DrawY, ViewWdt, ViewHgt, ViewX,
          ViewY);
  // Draw
  Draw(cgo, true);
  // Video record & status (developer mode, first player viewport)
  if (!Application.isFullScreen)
    if (Player == 0 && (this == Game.GraphicsSystem.GetViewport((int32_t)0)))
      Game.GraphicsSystem.Video.Execute();
  // Blit output
  BlitOutput();
#ifdef USE_GL
  // switch back to original context
  if (pCtx) pGL->GetMainCtx().Select();
#endif
}

void C4Viewport::AdjustPosition() {
  int32_t ViewportScrollBorder =
      fIsNoOwnerViewport ? 0 : C4ViewportScrollBorder;
  // View position
  if (PlayerLock && ValidPlr(Player)) {
    C4Player *pPlr = Game.Players.Get(Player);
    int32_t iScrollRange = Min(ViewWdt / 10, ViewHgt / 10);
    int32_t iExtraBoundsX = 0, iExtraBoundsY = 0;
    if (pPlr->ViewMode == C4PVM_Scrolling) {
      iScrollRange = 0;
      iExtraBoundsX = iExtraBoundsY = ViewportScrollBorder;
    } else {
      // if view is close to border, allow scrolling
      if (pPlr->ViewX < ViewportScrollBorder)
        iExtraBoundsX = Min<int32_t>(ViewportScrollBorder - pPlr->ViewX,
                                     ViewportScrollBorder);
      else if (pPlr->ViewX >= GBackWdt - ViewportScrollBorder)
        iExtraBoundsX =
            Min<int32_t>(pPlr->ViewX - GBackWdt, 0) + ViewportScrollBorder;
      if (pPlr->ViewY < ViewportScrollBorder)
        iExtraBoundsY = Min<int32_t>(ViewportScrollBorder - pPlr->ViewY,
                                     ViewportScrollBorder);
      else if (pPlr->ViewY >= GBackHgt - ViewportScrollBorder)
        iExtraBoundsY =
            Min<int32_t>(pPlr->ViewY - GBackHgt, 0) + ViewportScrollBorder;
    }
    iExtraBoundsX = Max<int32_t>(iExtraBoundsX, (ViewWdt - GBackWdt) / 2 + 1);
    iExtraBoundsY = Max<int32_t>(iExtraBoundsY, (ViewHgt - GBackHgt) / 2 + 1);
    // calc target view position
    int32_t iTargetViewX = pPlr->ViewX - ViewWdt / 2;
    int32_t iTargetViewY = pPlr->ViewY - ViewHgt / 2;
    // add mouse auto scroll
    int32_t iPrefViewX = ViewX - ViewOffsX, iPrefViewY = ViewY - ViewOffsY;
    if (pPlr->MouseControl && Game.MouseControl.InitCentered &&
        Config.General.MouseAScroll) {
      int32_t iAutoScrollBorder =
          Min(Min(ViewWdt / 10, ViewHgt / 10), C4SymbolSize);
      if (iAutoScrollBorder) {
        iPrefViewX +=
            BoundBy<int32_t>(
                0, Game.MouseControl.VpX - ViewWdt + iAutoScrollBorder,
                Game.MouseControl.VpX - iAutoScrollBorder) *
            iScrollRange *
            BoundBy<int32_t>(Config.General.MouseAScroll, 0, 100) / 100 /
            iAutoScrollBorder;
        iPrefViewY +=
            BoundBy<int32_t>(
                0, Game.MouseControl.VpY - ViewHgt + iAutoScrollBorder,
                Game.MouseControl.VpY - iAutoScrollBorder) *
            iScrollRange *
            BoundBy<int32_t>(Config.General.MouseAScroll, 0, 100) / 100 /
            iAutoScrollBorder;
      }
    }
    // scroll range
    iTargetViewX = BoundBy(iPrefViewX, iTargetViewX - iScrollRange,
                           iTargetViewX + iScrollRange);
    iTargetViewY = BoundBy(iPrefViewY, iTargetViewY - iScrollRange,
                           iTargetViewY + iScrollRange);
    // bounds
    iTargetViewX = BoundBy<int32_t>(iTargetViewX, -iExtraBoundsX,
                                    GBackWdt - ViewWdt + iExtraBoundsX);
    iTargetViewY = BoundBy<int32_t>(iTargetViewY, -iExtraBoundsY,
                                    GBackHgt - ViewHgt + iExtraBoundsY);
    // smooth
    if (dViewX >= 0 && dViewY >= 0) {
      dViewX += (itofix(iTargetViewX) - dViewX) /
                BoundBy<int32_t>(Config.General.ScrollSmooth, 1, 50);
      ViewX = fixtoi(dViewX);
      dViewY += (itofix(iTargetViewY) - dViewY) /
                BoundBy<int32_t>(Config.General.ScrollSmooth, 1, 50);
      ViewY = fixtoi(dViewY);
    } else {
      dViewX = itofix(ViewX = iTargetViewX);
      dViewY = itofix(ViewY = iTargetViewY);
    }
    // apply offset
    ViewX += ViewOffsX;
    ViewY += ViewOffsY;
  }
  // NO_OWNER can't scroll
  if (fIsNoOwnerViewport) {
    ViewOffsX = 0;
    ViewOffsY = 0;
  }
  // clip at borders, update vars
  UpdateViewPosition();
#ifdef WITH_DEVELOPER_MODE
// ScrollBarsByViewPosition();
#endif
}

void C4Viewport::CenterPosition() {
  int32_t ViewportScrollBorder =
      Application.isFullScreen ? C4ViewportScrollBorder : 0;
  // center viewport position on map
  // set center position
  ViewX = (GBackWdt - ViewWdt) / 2;
  ViewY = (GBackHgt - ViewHgt) / 2;
  // clips and updates
  UpdateViewPosition();
}

void C4Viewport::UpdateViewPosition() {
  int32_t ViewportScrollBorder =
      Application.isFullScreen ? C4ViewportScrollBorder : 0;
  // no-owner viewports should not scroll outside viewing area
  if (fIsNoOwnerViewport) {
    if (Application.isFullScreen && GBackWdt < ViewWdt) {
      ViewX = (GBackWdt - ViewWdt) / 2;
    } else {
      ViewX = Min<int32_t>(ViewX, GBackWdt - ViewWdt);
      ViewX = Max<int32_t>(ViewX, 0);
    }
    if (Application.isFullScreen && GBackHgt < ViewHgt) {
      ViewY = (GBackHgt - ViewHgt) / 2;
    } else {
      ViewY = Min<int32_t>(ViewY, GBackHgt - ViewHgt);
      ViewY = Max<int32_t>(ViewY, 0);
    }
  }
  // update borders
  BorderLeft = Max<int32_t>(-ViewX, 0);
  BorderTop = Max<int32_t>(-ViewY, 0);
  BorderRight = Max<int32_t>(ViewWdt - GBackWdt + ViewX, 0);
  BorderBottom = Max<int32_t>(ViewHgt - GBackHgt + ViewY, 0);
}

void C4Viewport::Default() {
  pCtx = NULL;
  // hWnd=NULL;
  pWindow = NULL;
  Player = 0;
  ViewX = ViewY = ViewWdt = ViewHgt = 0;
  BorderLeft = BorderTop = BorderRight = BorderBottom = 0;
  OutX = OutY = ViewWdt = ViewHgt = 0;
  DrawX = DrawY = 0;
  Next = NULL;
  PlayerLock = TRUE;
  ResetMenuPositions = FALSE;
  SetRegions = NULL;
  Regions.Default();
  dViewX = dViewY = -31337;
  ViewOffsX = ViewOffsY = 0;
  fIsNoOwnerViewport = false;
}

void C4Viewport::DrawPlayerInfo(C4FacetEx &cgo) {
  C4Facet ccgo;
  if (!ValidPlr(Player)) return;

  // Wealth
  if (Game.Players.Get(Player)->ViewWealth ||
      Config.Graphics.ShowPlayerInfoAlways) {
    int32_t wdt = C4SymbolSize;
    int32_t hgt = C4SymbolSize / 2;
    ccgo.Set(cgo.Surface, cgo.X + cgo.Wdt - wdt - C4SymbolBorder,
             cgo.Y + C4SymbolBorder, wdt, hgt);
    Game.GraphicsResource.fctWealth.DrawValue(
        ccgo, Game.Players.Get(Player)->Wealth, 0, Config.Graphics.Currency);
  }

  // Value gain
  if ((Game.C4S.Game.ValueGain && Game.Players.Get(Player)->ViewValue) ||
      Config.Graphics.ShowPlayerInfoAlways) {
    int32_t wdt = C4SymbolSize;
    int32_t hgt = C4SymbolSize / 2;
    ccgo.Set(cgo.Surface, cgo.X + cgo.Wdt - 2 * wdt - 2 * C4SymbolBorder,
             cgo.Y + C4SymbolBorder, wdt, hgt);
    Game.GraphicsResource.fctScore.DrawValue(
        ccgo, Game.Players.Get(Player)->ValueGain);
  }

  // Crew
  if (Config.Graphics.ShowPlayerInfoAlways) {
    int32_t wdt = C4SymbolSize;
    int32_t hgt = C4SymbolSize / 2;
    ccgo.Set(cgo.Surface, cgo.X + cgo.Wdt - 3 * wdt - 3 * C4SymbolBorder,
             cgo.Y + C4SymbolBorder, wdt, hgt);
    Game.GraphicsResource.fctCrewClr.DrawValue2Clr(
        ccgo, Game.Players.Get(Player)->SelectCount,
        Game.Players.Get(Player)->ActiveCrewCount(),
        Game.Players.Get(Player)->ColorDw);
  }

  // Controls
  DrawPlayerControls(cgo);
  DrawPlayerStartup(cgo);
}

BOOL C4Viewport::Init(int32_t iPlayer, bool fSetTempOnly) {
  // Fullscreen viewport initialization
  // Set Player
  if (!ValidPlr(iPlayer)) iPlayer = NO_OWNER;
  Player = iPlayer;
  if (!fSetTempOnly) fIsNoOwnerViewport = (iPlayer == NO_OWNER);
  // Owned viewport: clear any flash message explaining observer menu
  if (ValidPlr(iPlayer)) Game.GraphicsSystem.FlashMessage("");
  return TRUE;
}

BOOL C4Viewport::Init(CStdWindow *pParent, CStdApp *pApp, int32_t iPlayer) {
  // Console viewport initialization
  // Set Player
  if (!ValidPlr(iPlayer)) iPlayer = NO_OWNER;
  Player = iPlayer;
  fIsNoOwnerViewport = (Player == NO_OWNER);
  // Create window
  pWindow = new C4ViewportWindow(this);
  if (!pWindow->Init(pApp,
                     (Player == NO_OWNER) ? LoadResStr("IDS_CNS_VIEWPORT")
                                          : Game.Players.Get(Player)->GetName(),
                     pParent, false))
    return FALSE;
  // Position and size
  sprintf(OSTR, "Viewport%i", Player + 1);
  pWindow->RestorePosition(OSTR, Config.GetSubkeyPath("Console"));
  // UpdateWindow(hWnd);
  // Updates
  UpdateOutputSize();
  // Disable player lock on unowned viewports
  if (!ValidPlr(Player)) TogglePlayerLock();
  // create rendering context
  if (lpDDraw) pCtx = lpDDraw->CreateContext(pWindow, pApp);
  // Draw
  Execute();
  // Success
  return TRUE;
}

StdStrBuf PlrControlKeyName(int32_t iPlayer, int32_t iControl, bool fShort) {
  // determine player
  C4Player *pPlr = Game.Players.Get(iPlayer);
  // player control
  if (pPlr) {
    if (Inside<int32_t>(pPlr->Control, C4P_Control_Keyboard1,
                        C4P_Control_Keyboard4))
      return C4KeyCodeEx::KeyCode2String(
          Config.Controls.Keyboard[pPlr->Control][iControl], true, fShort);
    if (Inside<int32_t>(pPlr->Control, C4P_Control_GamePad1,
                        C4P_Control_GamePadMax))
      return C4KeyCodeEx::KeyCode2String(
          Config.Gamepads[pPlr->Control - C4P_Control_GamePad1]
              .Button[iControl],
          true, fShort);
  }
  // global control
  else {
    // look up iControl for a matching mapping in global key map
    // and then display the key name - should at least work for
    // stuff in KEYSCOPE_FullSMenu...
    const char *szKeyID;
    switch (iControl) {
      case CON_Throw:
        szKeyID = "FullscreenMenuOK";
        break;
      case CON_Dig:
        szKeyID = "FullscreenMenuCancel";
        break;
      default:
        szKeyID = NULL;
        break;
    }
    if (szKeyID)
      return Game.KeyboardInput.GetKeyCodeNameByKeyName(szKeyID, fShort);
  }
  // undefined control
  return StdStrBuf();
}

void C4Viewport::DrawPlayerControls(C4FacetEx &cgo) {
  if (!ValidPlr(Player)) return;
  if (!Game.Players.Get(Player)->ShowControl) return;
  int32_t size = Min(cgo.Wdt / 3, 7 * cgo.Hgt / 24);
  int32_t tx;
  int32_t ty;
  switch (Game.Players.Get(Player)->ShowControlPos) {
    case 1:  // Position 1: bottom right corner
      tx = cgo.X + cgo.Wdt * 3 / 4 - size / 2;
      ty = cgo.Y + cgo.Hgt / 2 - size / 2;
      break;
    case 2:  // Position 2: bottom left corner
      tx = cgo.X + cgo.Wdt / 4 - size / 2;
      ty = cgo.Y + cgo.Hgt / 2 - size / 2;
      break;
    case 3:  // Position 3: top left corner
      tx = cgo.X + cgo.Wdt / 4 - size / 2;
      ty = cgo.Y + 15;
      break;
    case 4:  // Position 4: top right corner
      tx = cgo.X + cgo.Wdt * 3 / 4 - size / 2;
      ty = cgo.Y + 15;
      break;
    default:  // default: Top center
      tx = cgo.X + cgo.Wdt / 2 - size / 2;
      ty = cgo.Y + 15;
      break;
  }
  int32_t iShowCtrl = Game.Players.Get(Player)->ShowControl;
  int32_t iLastCtrl = Com2Control(Game.Players.Get(Player)->LastCom);
  int32_t scwdt = size / 3, schgt = size / 4;
  BOOL showtext;

  const int32_t C4MaxShowControl = 10;

  for (int32_t iCtrl = 0; iCtrl < C4MaxShowControl; iCtrl++)
    if (iShowCtrl & (1 << iCtrl)) {
      showtext = iShowCtrl & (1 << (iCtrl + C4MaxShowControl));
      if (iShowCtrl & (1 << (iCtrl + 2 * C4MaxShowControl)))
        if (Tick35 > 18) showtext = FALSE;
      C4Facet ccgo;
      ccgo.Set(cgo.Surface, tx + scwdt * (iCtrl % 3), ty + schgt * (iCtrl / 3),
               scwdt, schgt);
      DrawControlKey(
          ccgo, iCtrl, (iLastCtrl == iCtrl) ? 1 : 0,
          showtext ? PlrControlKeyName(Player, iCtrl, true).getData() : NULL);
    }
}

extern int32_t DrawMessageOffset;

void C4Viewport::DrawPlayerStartup(C4FacetEx &cgo) {
  C4Player *pPlr;
  if (!(pPlr = Game.Players.Get(Player))) return;
  if (!pPlr->LocalControl || !pPlr->ShowStartup) return;
  int32_t iNameHgtOff = 0;

  // Control
  if (pPlr->MouseControl)
    GfxR->fctMouse.Draw(cgo.Surface,
                        cgo.X + (cgo.Wdt - GfxR->fctKeyboard.Wdt) / 2 + 55,
                        cgo.Y + cgo.Hgt * 2 / 3 - 10 + DrawMessageOffset, 0, 0);
  if (Inside<int32_t>(pPlr->Control, C4P_Control_Keyboard1,
                      C4P_Control_Keyboard4)) {
    GfxR->fctKeyboard.Draw(cgo.Surface,
                           cgo.X + (cgo.Wdt - GfxR->fctKeyboard.Wdt) / 2,
                           cgo.Y + cgo.Hgt * 2 / 3 + DrawMessageOffset,
                           pPlr->Control - C4P_Control_Keyboard1, 0);
    iNameHgtOff = GfxR->fctKeyboard.Hgt;
  } else if (Inside<int32_t>(pPlr->Control, C4P_Control_GamePad1,
                             C4P_Control_GamePad4)) {
    GfxR->fctGamepad.Draw(cgo.Surface,
                          cgo.X + (cgo.Wdt - GfxR->fctKeyboard.Wdt) / 2,
                          cgo.Y + cgo.Hgt * 2 / 3 + DrawMessageOffset,
                          pPlr->Control - C4P_Control_GamePad1, 0);
    iNameHgtOff = GfxR->fctGamepad.Hgt;
  }

  // Name
  Application.DDraw->TextOut(
      pPlr->GetName(), Game.GraphicsResource.FontRegular, 1.0, cgo.Surface,
      cgo.X + cgo.Wdt / 2,
      cgo.Y + cgo.Hgt * 2 / 3 + iNameHgtOff + DrawMessageOffset,
      pPlr->ColorDw | 0xff000000, ACenter);
}

void C4Viewport::SetOutputSize(int32_t iDrawX, int32_t iDrawY, int32_t iOutX,
                               int32_t iOutY, int32_t iOutWdt,
                               int32_t iOutHgt) {
  // update view position: Remain centered at previous position
  int32_t ViewportScrollBorder =
      Application.isFullScreen ? C4ViewportScrollBorder : 0;
  ViewX += (ViewWdt - iOutWdt) / 2;
  ViewY += (ViewHgt - iOutHgt) / 2;
  // update output parameters
  DrawX = iDrawX;
  DrawY = iDrawY;
  OutX = iOutX;
  OutY = iOutY;
  ViewWdt = iOutWdt;
  ViewHgt = iOutHgt;
  UpdateViewPosition();
  // Reset menus
  ResetMenuPositions = TRUE;
  // player uses mouse control? then clip the cursor
  C4Player *pPlr;
  if (pPlr = Game.Players.Get(Player))
    if (pPlr->MouseControl) {
      Game.MouseControl.UpdateClip();
      // and inform GUI
      if (Game.pGUI)
        // Game.pGUI->SetBounds(C4Rect(iOutX, iOutY, iOutWdt, iOutHgt));
        Game.pGUI->SetPreferredDlgRect(C4Rect(iOutX, iOutY, iOutWdt, iOutHgt));
    }
}

void C4Viewport::ClearPointers(C4Object *pObj) { Regions.ClearPointers(pObj); }

void C4Viewport::DrawMouseButtons(C4FacetEx &cgo) {
  C4Facet ccgo;
  C4Region rgn;
  int32_t iSymbolSize = C4SymbolSize * 2 / 3;
  // Help
  ccgo.Set(cgo.Surface, cgo.X + cgo.Wdt - iSymbolSize,
           cgo.Y + C4SymbolSize + 2 * C4SymbolBorder, iSymbolSize, iSymbolSize);
  GfxR->fctKey.Draw(ccgo);
  GfxR->fctOKCancel.Draw(ccgo, TRUE, 0, 1);
  if (SetRegions) {
    rgn.Default();
    rgn.Set(ccgo, LoadResStr("IDS_CON_HELP"));
    rgn.Com = COM_Help;
    SetRegions->Add(rgn);
  }
  // Player menu
  ccgo.Y += iSymbolSize;
  DrawCommandKey(
      ccgo, COM_PlayerMenu, FALSE,
      PlrControlKeyName(Player, Com2Control(COM_PlayerMenu), true).getData());
  if (SetRegions) {
    rgn.Default();
    rgn.Set(ccgo, LoadResStr("IDS_CON_PLAYERMENU"));
    rgn.Com = COM_PlayerMenu;
    SetRegions->Add(rgn);
  }
  // Chat
  if (C4ChatDlg::IsChatActive()) {
    ccgo.Y += iSymbolSize;
    GfxR->fctKey.Draw(ccgo);
    C4GUI::Icon::GetIconFacet(C4GUI::Ico_Ex_Chat).Draw(ccgo, TRUE);
    if (SetRegions) {
      rgn.Default();
      rgn.Set(ccgo, LoadResStr("IDS_DLG_CHAT"));
      rgn.Com = COM_Chat;
      SetRegions->Add(rgn);
    }
  }
}

void C4Viewport::DrawPlayerFogOfWar(C4FacetEx &cgo) {
  /*
	C4Player *pPlr=Game.Players.Get(Player);
	if (!pPlr || !pPlr->FogOfWar) return;
	pPlr->FogOfWar->Draw(cgo);*/  // now done by modulation
}

void C4Viewport::NextPlayer() {
  C4Player *pPlr;
  int32_t iPlr;
  if (!(pPlr = Game.Players.Get(Player))) {
    if (!(pPlr = Game.Players.First)) return;
  } else if (!(pPlr = pPlr->Next))
    if (Game.C4S.Head.Film && Game.C4S.Head.Replay)
      pPlr = Game.Players.First;  // cycle to first in film mode only; in
                                  // network obs mode allow NO_OWNER-view
  if (pPlr)
    iPlr = pPlr->Number;
  else
    iPlr = NO_OWNER;
  if (iPlr != Player) Init(iPlr, true);
}

bool C4Viewport::IsViewportMenu(class C4Menu *pMenu) {
  // check all associated menus
  // Get player
  C4Player *pPlr = Game.Players.Get(Player);
  // Player eliminated: No menu
  if (pPlr && pPlr->Eliminated) return false;
  // Player cursor object menu
  if (pPlr && pPlr->Cursor && pPlr->Cursor->Menu == pMenu) return true;
  // Player menu
  if (pPlr && pPlr->Menu.IsActive() && &(pPlr->Menu) == pMenu) return true;
  // Fullscreen menu (if active, only one viewport can exist)
  if (FullScreen.pMenu && FullScreen.pMenu->IsActive() &&
      FullScreen.pMenu == pMenu)
    return true;
  // no match
  return false;
}
