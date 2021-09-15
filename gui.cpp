#include <math.h>
#include <shobjidl_core.h>
#include <gdiplus.h>
#include <VersionHelpers.h>
#include "gui.hpp"
#include "resource.h"
#include "analyze.h"

using namespace Gdiplus;

#define SZ_WINDOW_CLASS           L"{D4B393D8-7ACF-441F-9B4A-62E3123006FA}"
#define SZ_TITLEBAR_CLASS         L"{064CCE15-5A61-41B2-8806-B82254BCBAC2}"
#define SZ_EXITBUTTON_CLASS       L"{49060931-0E39-4337-9C2E-F5487DCE656F}"
#define SZ_BUTTON_CLASS           L"{C732B620-6362-4DAD-A7B1-B8D439FBA39F}"
#define SZ_CIRCULARBUTTON_CLASS   L"{58E8CEF9-DDF2-46DD-8AF4-B826B41E963C}"
#define SZ_TOASNOTIFICATION_CLASS L"{E17A57C2-C6EB-4E9F-845A-75758DCEEE20}"

#define TOAST_NOTIFICIATION_WIDTH  300
#define TOAST_NOTIFICIATION_HEIGHT 60
#define IDT_TOAST_NOTIFICAITON     101

#define NIM_COMMAND (WM_APP + 1)

static PWSTR        ToastNotificationMessage = NULL;
static ULONG        ToastNotificationMessageLenght = 0;
static HWND         ToastNotificationWindow = NULL;
static Font       * ToastNotificationFont;
static SolidBrush * ToastNotificationBackgroundColour;
static SolidBrush * ToastNotificationTextColour;

static LRESULT WINAPI ToastNotificationProcedure(
  IN HWND   hWindow,
  IN UINT   nMessage,
  IN WPARAM wParam,
  IN LPARAM lParam
  )
{
  if (nMessage == WM_PAINT) {
    PAINTSTRUCT PaintStruct;
    HDC         hDc = BeginPaint(hWindow, &PaintStruct);
    Graphics    Gfx(hDc);
    RectF       TextBoundtryBox;

    Gfx.FillRectangle(ToastNotificationBackgroundColour, 0, 0, PaintStruct.rcPaint.right, PaintStruct.rcPaint.bottom );
    Gfx.MeasureString(L" ", 1, ToastNotificationFont, { 0, 0 }, &TextBoundtryBox);

    Gfx.SetTextRenderingHint(TextRenderingHint::TextRenderingHintAntiAlias);
    Gfx.DrawString(L"EVE", 3, ToastNotificationFont, { 0, 0 }, ToastNotificationTextColour);
    Gfx.DrawString(ToastNotificationMessage, (INT)ToastNotificationMessageLenght, ToastNotificationFont, { 0, TextBoundtryBox.Height }, ToastNotificationTextColour);

    EndPaint(hWindow, &PaintStruct);
    
    return LRESULT_SUCCESS;
  } else if (nMessage == WM_TIMER) {
    ShowWindow(hWindow, SW_HIDE);
    
    return LRESULT_SUCCESS;
  } else if (nMessage == WM_SHOWWINDOW) {
    if (wParam) {
      ULONG X = GetSystemMetrics(SM_CXFULLSCREEN) - TOAST_NOTIFICIATION_WIDTH - 8;
      ULONG Y = GetSystemMetrics(SM_CYFULLSCREEN) - TOAST_NOTIFICIATION_HEIGHT - 8;

      MoveWindow(hWindow, X, Y, TOAST_NOTIFICIATION_WIDTH, TOAST_NOTIFICIATION_HEIGHT, TRUE);

      KillTimer(hWindow, IDT_TOAST_NOTIFICAITON);
      SetTimer(hWindow, IDT_TOAST_NOTIFICAITON, 3000, NULL);

      return LRESULT_SUCCESS;
    }
 
    KillTimer(hWindow, IDT_TOAST_NOTIFICAITON);

    return LRESULT_SUCCESS;
  } else if (nMessage == WM_CREATE) {
    ToastNotificationFont = new Font(L"Segoe UI", 11);
    ToastNotificationBackgroundColour = new SolidBrush(Color::Black);
    ToastNotificationTextColour = new SolidBrush(Color::White);

    return LRESULT_SUCCESS;
  }

  return DefWindowProcW(hWindow, nMessage, wParam, lParam);
}

extern "C" {
  VOID EVEAPI TastNotificationShow(
    IN PWSTR szMessage
    )
  {
    ToastNotificationMessageLenght = (ULONG)wcslen(szMessage);
    ToastNotificationMessage = szMessage;

    ShowWindow(ToastNotificationWindow, SW_SHOW);
  }
}

namespace Gui {
  #define WINDOW_WIDTH         300
  #define WINDOW_HEIGHT        450
  #define TITLEBAR_HEIGHT      32
  #define BUTTON_HEIGHT        32
  #define EXITBUTTON_DIMENSION TITLEBAR_HEIGHT

  #define IDB_QUICK_SCAN  100
  #define IDB_FULL_SCAN   101
  #define IDB_CUSTOM_SCAN 102
  #define IDB_SCAN        103
 
  #define CBN_TOGGLE    (WM_APP + 1)
  #define CCBC_PROGRESS (WM_APP + 2)
  #define CCBC_SETTEXT  (WM_APP + 3)

  typedef struct {
    ULONG  dwID;
    PWSTR  szText;
    SIZE_T dwTextLenght;
    HWND   hParentWindow;
    BOOL   bHover;
    BOOL   bDown;
    BOOL   bToggle;
  } BUTTON_OBJECT, * PBUTTON_OBJECT;

  typedef struct {
    ULONG  dwID;
    PWSTR  szText;
    SIZE_T dwTextLenght;
    HWND   hParentWindow;
    BOOL   bHover;
    BOOL   bDown;
    BOOL   bProgress;
    FLOAT  fProgressDot[3];
  } CIRCULAR_BUTTON_OBJECT, * PCIRCULAR_BUTTON_OBJECT;

  static HWND         MainWindow;
  static Font       * TitleBarFont;
  static SolidBrush * TitleBarTextColour;
  static SolidBrush * ExitButtonWhite;
  static SolidBrush * ExitButtonGray;
  static SolidBrush * ExitButtonLightGray;
  static Pen        * ExitButtonPen;
  static BOOL         ExitButtonHover = FALSE;
  static BOOL         ExitButtonDown = FALSE;
  static BOOL         TitleBarMouseLock = FALSE;
  static POINT        TitleBarMouseLockPoint;
  static Font       * ButtonFont;
  static SolidBrush * ButtonTextColour;
  static SolidBrush * ButtonColour;
  static SolidBrush * ButtonHoverColour;
  static SolidBrush * ButtonDownColour;
  static SolidBrush * ButtonToggleColour;
  static SolidBrush * ButtonToggleHoverColour;
  static SolidBrush * ButtonToggleDownColour;
  static HWND         ButtonQuickScan;
  static HWND         ButtonFullScan;
  static HWND         ButtonCustomScan;
  static HWND         CircularButtonScan;
  static HWND         ButtonSelectedHandle;
  static Font       * ContactInfoFont;
  static SolidBrush * ContactInfoColour;
  static BOOL         ScanButtonActivated = FALSE;
  
  static LRESULT WINAPI CircularButtonProcedure(
    IN HWND   hWindow,
    IN UINT   nMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
  {
    #define DEGREE_TO_RADIANTS 0.01745329f
    #define PROGRESS_DOT_SPACE (5 * DEGREE_TO_RADIANTS)

    if (nMessage == WM_TIMER) {
      RECT                    Rect;
      PCIRCULAR_BUTTON_OBJECT ButtonObject = (PCIRCULAR_BUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);
      
      ButtonObject->fProgressDot[0] += PROGRESS_DOT_SPACE;
      ButtonObject->fProgressDot[1] += PROGRESS_DOT_SPACE;
      ButtonObject->fProgressDot[2] += PROGRESS_DOT_SPACE;

      GetClientRect(hWindow, &Rect);
      InvalidateRect(hWindow, &Rect, FALSE);
      UpdateWindow(hWindow);

      return LRESULT_SUCCESS;
    } else if (nMessage == WM_COMMAND) {
      PCIRCULAR_BUTTON_OBJECT ButtonObject = (PCIRCULAR_BUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);

      if (wParam == CCBC_PROGRESS) {
        #define IDT_TIMER1 100

        ButtonObject->bProgress = (BOOL)lParam;

        if (ButtonObject->bProgress) {
          ButtonObject->fProgressDot[0] = PROGRESS_DOT_SPACE * 0;
          ButtonObject->fProgressDot[1] = PROGRESS_DOT_SPACE * 1;
          ButtonObject->fProgressDot[2] = PROGRESS_DOT_SPACE * 2;

          SetTimer(hWindow, IDT_TIMER1, 1, NULL);
        } else {
          KillTimer(hWindow, IDT_TIMER1);
        }

        #undef IDT_TIMER1

        return LRESULT_SUCCESS;
      } else if (wParam == CCBC_SETTEXT) {
        ButtonObject->szText = (PWSTR)lParam;
        ButtonObject->dwTextLenght = wcslen((PWSTR)lParam);

        return LRESULT_SUCCESS;
      }
    } else if (nMessage == WM_LBUTTONUP) {
      RECT Rect;
      LONG X = LOWORD(lParam);
      LONG Y = HIWORD(lParam);
      LONG dwRadius;
      LONG dwMouseRadius;
      POINT Center;
      PCIRCULAR_BUTTON_OBJECT ButtonObject = (PCIRCULAR_BUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);

      GetClientRect(hWindow, &Rect);

      Center = { Rect.right / 2, Rect.bottom / 2 };
      dwRadius = Rect.right / 2;
      dwMouseRadius = (LONG)sqrtf((FLOAT)(pow(X - Center.x, 2) + pow(Y - Center.y, 2)));

      if (dwMouseRadius <= dwRadius) {
        SendMessageW(ButtonObject->hParentWindow, WM_COMMAND, BN_CLICKED, ButtonObject->dwID);
   
        return LRESULT_SUCCESS;
      }
    } else if (nMessage == WM_LBUTTONDOWN) {
      RECT Rect;
      LONG X = LOWORD(lParam);
      LONG Y = HIWORD(lParam);
      LONG dwRadius;
      LONG dwMouseRadius;
      POINT Center;
      PCIRCULAR_BUTTON_OBJECT ButtonObject = (PCIRCULAR_BUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);

      GetClientRect(hWindow, &Rect);

      Center = { Rect.right / 2, Rect.bottom / 2 };
      dwRadius = Rect.right / 2;
      dwMouseRadius = (LONG)sqrtf((FLOAT)(pow(X - Center.x, 2) + pow(Y - Center.y, 2)));

      if (dwMouseRadius <= dwRadius) {
        if (!ButtonObject->bDown) {
          ButtonObject->bDown = TRUE;

          InvalidateRect(hWindow, &Rect, TRUE);
          UpdateWindow(hWindow);

          return LRESULT_SUCCESS;
        }
      } else if (ButtonObject->bHover || ButtonObject->bDown) {
        ButtonObject->bHover = FALSE;
        ButtonObject->bDown = FALSE;

        InvalidateRect(hWindow, &Rect, TRUE);
        UpdateWindow(hWindow);

        return LRESULT_SUCCESS;
      }
    } else if (nMessage == WM_MOUSELEAVE) {
      PCIRCULAR_BUTTON_OBJECT ButtonObject = (PCIRCULAR_BUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);

      if (ButtonObject->bHover || ButtonObject->bDown) {
        RECT Rect;

        ButtonObject->bHover = FALSE;
        ButtonObject->bDown = FALSE;

        GetClientRect(hWindow, &Rect);
        InvalidateRect(hWindow, &Rect, TRUE);
        UpdateWindow(hWindow);

        return LRESULT_SUCCESS;
      }
    } else if (nMessage == WM_MOUSEMOVE) {
      RECT  Rect;
      LONG  X = LOWORD(lParam);
      LONG  Y = HIWORD(lParam);
      LONG  dwRadius;
      LONG  dwMouseRadius;
      POINT Center;
      PCIRCULAR_BUTTON_OBJECT ButtonObject = (PCIRCULAR_BUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);

      GetClientRect(hWindow, &Rect);

      Center = { Rect.right / 2, Rect.bottom / 2 };
      dwRadius = Rect.right / 2;
      dwMouseRadius = (LONG)sqrtf((FLOAT)(pow(X - Center.x, 2) + pow(Y - Center.y, 2)));

      if (dwMouseRadius <= dwRadius) {
        if (!ButtonObject->bHover) {
          TRACKMOUSEEVENT TrackMouseEventStruct;

          ZeroMemory(&TrackMouseEventStruct, sizeof(TrackMouseEventStruct));
          TrackMouseEventStruct.cbSize = sizeof(TrackMouseEventStruct);
          TrackMouseEventStruct.hwndTrack = hWindow;
          TrackMouseEventStruct.dwFlags = TME_LEAVE;

          ButtonObject->bHover = TRUE;

          InvalidateRect(hWindow, &Rect, TRUE);
          UpdateWindow(hWindow);

          TrackMouseEvent(&TrackMouseEventStruct);

          return LRESULT_SUCCESS;
        }
      } else if (ButtonObject->bHover || ButtonObject->bDown) {
        TRACKMOUSEEVENT TrackMouseEventStruct;

        ZeroMemory(&TrackMouseEventStruct, sizeof(TrackMouseEventStruct));
        TrackMouseEventStruct.cbSize = sizeof(TrackMouseEventStruct);
        TrackMouseEventStruct.hwndTrack = hWindow;
        TrackMouseEventStruct.dwFlags = TME_LEAVE;

        ButtonObject->bHover = FALSE;
        ButtonObject->bDown = FALSE;

        InvalidateRect(hWindow, &Rect, TRUE);
        UpdateWindow(hWindow);

        TrackMouseEvent(&TrackMouseEventStruct);
        
        return LRESULT_SUCCESS;
      }
    } else if (nMessage == WM_PAINT) {
      PAINTSTRUCT             PaintStruct;
      HDC                     hDc = BeginPaint(hWindow, &PaintStruct);
      Graphics                Gfx(hDc);
      RectF                   TextBoundryBox;
      PCIRCULAR_BUTTON_OBJECT ButtonObject = (PCIRCULAR_BUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);
      SolidBrush            * BackgroundColour = ButtonColour;

      if (ButtonObject->bDown) {
        BackgroundColour = Gui::ButtonDownColour;
      } else if (ButtonObject->bHover) {
        BackgroundColour = Gui::ButtonHoverColour;
      }

      Gfx.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
      Gfx.FillEllipse(BackgroundColour, 0, 0, PaintStruct.rcPaint.right, PaintStruct.rcPaint.bottom);
      Gfx.SetTextRenderingHint(TextRenderingHint::TextRenderingHintAntiAlias);
      Gfx.MeasureString(ButtonObject->szText, (INT)ButtonObject->dwTextLenght, Gui::ButtonFont, { 0.0f, 0.0f }, &TextBoundryBox);
      Gfx.DrawString(ButtonObject->szText, (INT)ButtonObject->dwTextLenght, Gui::ButtonFont, { PaintStruct.rcPaint.right / 2 - TextBoundryBox.Width / 2, PaintStruct.rcPaint.bottom / 2 - TextBoundryBox.Height / 2 }, Gui::ButtonTextColour);

      if (ButtonObject->bProgress) {
        #define DOT_RADIUS 5.0f

        FLOAT fRadius = (FLOAT)PaintStruct.rcPaint.right / 2;

        PointF Points[3] = {
          { cosf(ButtonObject->fProgressDot[0]) * fRadius, sinf(ButtonObject->fProgressDot[0]) * fRadius },
          { cosf(ButtonObject->fProgressDot[1]) * fRadius, sinf(ButtonObject->fProgressDot[1]) * fRadius },
          { cosf(ButtonObject->fProgressDot[2]) * fRadius, sinf(ButtonObject->fProgressDot[2]) * fRadius },
        };

        for (ULONG_PTR i = 0; i != ARRAYSIZE(Points); i++) {  
          if (Points[i].X < 0) {
            Points[i].X += fRadius;
          } else {
            Points[i].X += fRadius - DOT_RADIUS;
          }

          if (Points[i].Y < 0) {
            Points[i].Y += fRadius;
          } else {
            Points[i].Y += fRadius - DOT_RADIUS;
          }
        
          Gfx.FillEllipse(Gui::ButtonToggleDownColour, Points[i].X, Points[i].Y, DOT_RADIUS, DOT_RADIUS);
        }

        #undef DOT_RADIUS
      }

      EndPaint(hWindow, &PaintStruct);

      return LRESULT_SUCCESS;
    } else if (nMessage == WM_CREATE) {
      SetWindowLongPtrW(hWindow, GWLP_USERDATA, (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);

      return LRESULT_SUCCESS;
    }

    #undef DEGREE_TO_RADIANTS
    #undef PROGRESS_DOT_SPACE

    return DefWindowProcW(hWindow, nMessage, wParam, lParam);
  }

  static LRESULT WINAPI ButtonProcedure(
    IN HWND   hWindow,
    IN UINT   nMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
  {
    if (nMessage == WM_COMMAND) {
      if (wParam == CBN_TOGGLE) {
        RECT           Rect; 
        PBUTTON_OBJECT ButtonObject = (PBUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);
      
        ButtonObject->bToggle = (BOOL)lParam;

        GetClientRect(hWindow, &Rect);
        InvalidateRect(hWindow, &Rect, TRUE);
        UpdateWindow(hWindow);

        return LRESULT_SUCCESS;
      }

    } else if (nMessage == WM_MOUSELEAVE) {
      PBUTTON_OBJECT ButtonObject = (PBUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);

      if (ButtonObject->bDown || ButtonObject->bHover) {
        RECT Rect;

        ButtonObject->bDown = FALSE;
        ButtonObject->bHover = FALSE;

        GetClientRect(hWindow, &Rect);
        InvalidateRect(hWindow, &Rect, TRUE);
        UpdateWindow(hWindow);
      
        return LRESULT_SUCCESS;
      }

    } else if (nMessage == WM_LBUTTONUP) {
      RECT           Rect;
      PBUTTON_OBJECT ButtonObject = (PBUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);

      ButtonObject->bDown = TRUE;
      ButtonObject->bHover = TRUE;

      GetClientRect(hWindow, &Rect);
      InvalidateRect(hWindow, &Rect, TRUE);
      UpdateWindow(hWindow);

      SendMessageW(ButtonObject->hParentWindow, WM_COMMAND, BN_CLICKED, ButtonObject->dwID);

      return LRESULT_SUCCESS;
    } else if (nMessage == WM_LBUTTONDOWN) {
      PBUTTON_OBJECT ButtonObject = (PBUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);

      if (!ButtonObject->bDown) {
        RECT Rect;

        ButtonObject->bDown = TRUE;

        GetClientRect(hWindow, &Rect);
        InvalidateRect(hWindow, &Rect, TRUE);
        UpdateWindow(hWindow);
      
        return LRESULT_SUCCESS;
      }
    } else if (nMessage == WM_MOUSEMOVE) {
      PBUTTON_OBJECT ButtonObject = (PBUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);

      if (!ButtonObject->bHover) {
        RECT Rect;
        TRACKMOUSEEVENT TrackMouseEventStruct;

        ZeroMemory(&TrackMouseEventStruct, sizeof(TrackMouseEventStruct));
        TrackMouseEventStruct.cbSize = sizeof(TrackMouseEventStruct);
        TrackMouseEventStruct.hwndTrack = hWindow;
        TrackMouseEventStruct.dwFlags = TME_LEAVE;

        ButtonObject->bHover = TRUE;

        GetClientRect(hWindow, &Rect);
        InvalidateRect(hWindow, &Rect, TRUE);
        UpdateWindow(hWindow);

        TrackMouseEvent(&TrackMouseEventStruct);
      
        return LRESULT_SUCCESS;
      }
    } else if (nMessage == WM_PAINT) {
      PAINTSTRUCT    PaintStruct;
      HDC            hDc = BeginPaint(hWindow, &PaintStruct);
      Graphics       Gfx(hDc);
      RectF          TextBoundryBox;
      PBUTTON_OBJECT ButtonObject = (PBUTTON_OBJECT)GetWindowLongPtrW(hWindow, GWLP_USERDATA);
      SolidBrush *   BackgroundColour = NULL;

      if (ButtonObject->bToggle) {
        if (ButtonObject->bDown) {
          BackgroundColour = Gui::ButtonToggleDownColour;
        } else if (ButtonObject->bHover) {
          BackgroundColour = Gui::ButtonToggleHoverColour;
        } else {
          BackgroundColour = Gui::ButtonToggleColour;
        }
      } else {
        if (ButtonObject->bDown) {
          BackgroundColour = Gui::ButtonDownColour;
        } else if (ButtonObject->bHover) {
          BackgroundColour = Gui::ButtonHoverColour;
        }
      }

      if (BackgroundColour) {
        Gfx.FillRectangle(BackgroundColour, 0, 0, (INT)PaintStruct.rcPaint.right, (INT)PaintStruct.rcPaint.bottom);
      }

      Gfx.SetTextRenderingHint(TextRenderingHint::TextRenderingHintAntiAlias);
      Gfx.MeasureString(ButtonObject->szText, (INT)ButtonObject->dwTextLenght, Gui::ButtonFont, { 0.0f, 0.0f }, &TextBoundryBox);
      Gfx.DrawString(ButtonObject->szText, (INT)ButtonObject->dwTextLenght, Gui::ButtonFont, { PaintStruct.rcPaint.right / 2 - TextBoundryBox.Width / 2, PaintStruct.rcPaint.bottom / 2 - TextBoundryBox.Height / 2 }, Gui::ButtonTextColour);

      EndPaint(hWindow, &PaintStruct);

      return LRESULT_SUCCESS;
    } else if (nMessage == WM_CREATE) {
      SetWindowLongPtrW(hWindow, GWLP_USERDATA, (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);

      return LRESULT_SUCCESS;
    }

    return DefWindowProcW(hWindow, nMessage, wParam, lParam);
  }

  static VOID EVEAPI ButtonInit(VOID) {
    #define ARGB(A, R, G, B) ((A << Color::AlphaShift) | (R << Color::RedShift) | (G << Color::GreenShift) | (B << Color::BlueShift))

    ButtonFont = new Font(L"Segoe UI", 11);
    ButtonTextColour = new SolidBrush(Color::Black);
    ButtonColour = new SolidBrush((Color)ARGB(255, 255, 255, 255));
    ButtonHoverColour = new SolidBrush((Color)ARGB(255, 218, 218, 218));
    ButtonDownColour = new SolidBrush((Color)ARGB(255, 194, 194, 194));
    ButtonToggleColour = new SolidBrush((Color)ARGB(255, 145, 193, 231));
    ButtonToggleHoverColour = new SolidBrush((Color)ARGB(255, 97, 169, 226));
    ButtonToggleDownColour = new SolidBrush((Color)ARGB(255, 72, 156, 223));

    #undef ARGB
  }

  HWND EVEAPI CreateCircularButton(
    IN ULONG     dwID,
    IN PWSTR     szText,
    IN ULONG     X,
    IN ULONG     Y,
    IN ULONG     dwWidth,
    IN ULONG     dwHeight,
    IN HWND      hParentWindow,
    IN HINSTANCE hInstance
    )
  {
    HWND hWindow = NULL;
    PCIRCULAR_BUTTON_OBJECT ButtonObject = (PCIRCULAR_BUTTON_OBJECT)RtlpAlloc(sizeof(CIRCULAR_BUTTON_OBJECT));

    ZeroMemory(ButtonObject, sizeof(CIRCULAR_BUTTON_OBJECT));
    ButtonObject->dwID = dwID;
    ButtonObject->szText = szText;
    ButtonObject->dwTextLenght = wcslen(szText);
    ButtonObject->hParentWindow = hParentWindow;

    if (!(hWindow = CreateWindowExW(0, SZ_CIRCULARBUTTON_CLASS, L"EveAntivirusCircularButton", WS_CHILD, X, Y, dwWidth, dwHeight, hParentWindow, NULL, hInstance, ButtonObject))) {
      RtlpFree(ButtonObject);
    }

    return hWindow;
  }

  HWND EVEAPI CreateButton(
    IN ULONG     dwID,
    IN PWSTR     szText,
    IN ULONG     X,
    IN ULONG     Y,
    IN ULONG     dwWidth,
    IN ULONG     dwHeight,
    IN HWND      hParentWindow,
    IN HINSTANCE hInstance
    )
  {
    HWND hWindow = NULL;
    PBUTTON_OBJECT ButtonObject = (PBUTTON_OBJECT)RtlpAlloc(sizeof(BUTTON_OBJECT));

    ZeroMemory(ButtonObject, sizeof(BUTTON_OBJECT));
    ButtonObject->dwID = dwID;
    ButtonObject->szText = szText;
    ButtonObject->dwTextLenght = wcslen(szText);
    ButtonObject->hParentWindow = hParentWindow;

    if (!(hWindow = CreateWindowExW(0, SZ_BUTTON_CLASS, L"EveAntivirusButton", WS_CHILD, X, Y, dwWidth, dwHeight, hParentWindow, NULL, hInstance, ButtonObject))) {
      RtlpFree(ButtonObject);
    }

    return hWindow;
  }

  static LRESULT WINAPI LowLevelMouseHook(
    IN INT    nCode,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
  {
    if (wParam == WM_LBUTTONUP) {
      Gui::TitleBarMouseLock = FALSE;
    }

    if (Gui::TitleBarMouseLock) {
      LPMOUSEHOOKSTRUCT MouseHookStruct = (LPMOUSEHOOKSTRUCT)lParam;

      MoveWindow(Gui::MainWindow, MouseHookStruct->pt.x - TitleBarMouseLockPoint.x, MouseHookStruct->pt.y - TitleBarMouseLockPoint.y, WINDOW_WIDTH, WINDOW_HEIGHT, TRUE);
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
  }

  static LRESULT WINAPI ExitButtonProcedure(
    IN HWND   hWindow,
    IN UINT   nMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
  {
    if (nMessage == WM_MOUSELEAVE) {
      if (Gui::ExitButtonHover || Gui::ExitButtonDown) {
        RECT Rect;
        
        Gui::ExitButtonHover = FALSE;
        Gui::ExitButtonDown = FALSE;

        GetClientRect(hWindow, &Rect);
        InvalidateRect(hWindow, &Rect, FALSE);
        UpdateWindow(hWindow);
      
        return LRESULT_SUCCESS;
      }
    } else if (nMessage == WM_LBUTTONUP) {
      SendMessageW(Gui::MainWindow, WM_CLOSE, 0, 0);

      return LRESULT_SUCCESS;
    } else if (nMessage == WM_LBUTTONDOWN) {
      Gui::ExitButtonHover = FALSE;
      
      if (!Gui::ExitButtonDown) {
        RECT Rect;

        Gui::ExitButtonDown = TRUE;

        GetClientRect(hWindow, &Rect);
        InvalidateRect(hWindow, &Rect, FALSE);
        UpdateWindow(hWindow);
      
        return LRESULT_SUCCESS;
      }
    } else if (nMessage == WM_MOUSEMOVE) {
      if (!Gui::ExitButtonHover) {
        RECT            Rect;
        TRACKMOUSEEVENT TrackMouseEventStruct;

        ZeroMemory(&TrackMouseEventStruct, sizeof(TrackMouseEventStruct));
        TrackMouseEventStruct.cbSize = sizeof(TrackMouseEventStruct);
        TrackMouseEventStruct.hwndTrack = hWindow;
        TrackMouseEventStruct.dwFlags = TME_LEAVE;

        Gui::ExitButtonHover = TRUE;

        GetClientRect(hWindow, &Rect);
        InvalidateRect(hWindow, &Rect, FALSE);
        UpdateWindow(hWindow);

        TrackMouseEvent(&TrackMouseEventStruct);
      
        return LRESULT_SUCCESS;
      }
    } else if (nMessage == WM_PAINT) {
      PAINTSTRUCT PaintStruct;
      HDC         hDc = BeginPaint(hWindow, &PaintStruct);
      Graphics    Gfx(hDc);

      if (Gui::ExitButtonDown) {
        Gfx.FillRectangle(Gui::ExitButtonGray, 0, 0, EXITBUTTON_DIMENSION, EXITBUTTON_DIMENSION);
      } else if (Gui::ExitButtonHover) {
        Gfx.FillRectangle(Gui::ExitButtonLightGray, 0, 0, EXITBUTTON_DIMENSION, EXITBUTTON_DIMENSION);
      } else {
        Gfx.FillRectangle(Gui::ExitButtonWhite, 0, 0, EXITBUTTON_DIMENSION, EXITBUTTON_DIMENSION);
      }

      Gfx.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
      Gfx.DrawLine(Gui::ExitButtonPen, (FLOAT)EXITBUTTON_DIMENSION * (1.0f / 3.0f), (FLOAT)EXITBUTTON_DIMENSION * (1.0f / 3.0f), (FLOAT)EXITBUTTON_DIMENSION - (FLOAT)EXITBUTTON_DIMENSION * (1.0f / 3.0f), (FLOAT)EXITBUTTON_DIMENSION - (FLOAT)EXITBUTTON_DIMENSION * (1.0f / 3.0f));
      Gfx.DrawLine(Gui::ExitButtonPen, (FLOAT)EXITBUTTON_DIMENSION * (1.0f / 3.0f), (FLOAT)EXITBUTTON_DIMENSION - (FLOAT)EXITBUTTON_DIMENSION * (1.0f / 3.0f), (FLOAT)EXITBUTTON_DIMENSION - (FLOAT)EXITBUTTON_DIMENSION * (1.0f / 3.0f), (FLOAT)EXITBUTTON_DIMENSION * (1.0f / 3.0f));

      EndPaint(hWindow, &PaintStruct);

      return LRESULT_SUCCESS;
    } else if (nMessage == WM_CREATE) {
      SolidBrush * PenColour = new SolidBrush(Color::Black);

      Gui::ExitButtonWhite = new SolidBrush(Color::White);
      Gui::ExitButtonGray = new SolidBrush(Color::Gray);
      Gui::ExitButtonLightGray = new SolidBrush(Color::LightGray);
      Gui::ExitButtonPen = new Pen(PenColour, 2);

      return LRESULT_SUCCESS;
    }

    return DefWindowProcW(hWindow, nMessage, wParam, lParam);
  }

  static LRESULT WINAPI TitleBarProcedure(
    IN HWND   hWindow,
    IN UINT   nMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
  {
    if (nMessage == WM_LBUTTONDOWN) {
      if (!Gui::TitleBarMouseLock) {
        Gui::TitleBarMouseLock = TRUE;
        Gui::TitleBarMouseLockPoint.x = LOWORD(lParam);
        Gui::TitleBarMouseLockPoint.y = HIWORD(lParam);
      
        return LRESULT_SUCCESS;
      }
    } else if (nMessage == WM_PAINT) {
      PAINTSTRUCT PaintStruct;
      HDC         hDc = BeginPaint(hWindow, &PaintStruct);
      Graphics    Gfx(hDc);
      RectF       TextBoundryBox;

      Gfx.SetTextRenderingHint(TextRenderingHint::TextRenderingHintAntiAlias);
      Gfx.MeasureString(L"EVE", 3, Gui::TitleBarFont, { 0, 0 }, &TextBoundryBox);
      Gfx.DrawString(L"EVE", 3, Gui::TitleBarFont, { 4, (TITLEBAR_HEIGHT - TextBoundryBox.Height) / 2 }, Gui::TitleBarTextColour);

      EndPaint(hWindow, &PaintStruct);

      return LRESULT_SUCCESS;
    } else if (nMessage == WM_CREATE) {
      Gui::TitleBarFont = new Font(L"Segoe UI", 11);
      Gui::TitleBarTextColour = new SolidBrush(Color::Black);

      return LRESULT_SUCCESS;
    }

    return DefWindowProcW(hWindow, nMessage, wParam, lParam);
  }

  static LRESULT WINAPI WindowProcedure(
    IN HWND   hWindow,
    IN UINT   nMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
  {
    if (nMessage == WM_CLOSE) {
      ShowWindow(hWindow, SW_HIDE);

      return LRESULT_SUCCESS;
    } else if (nMessage == NIM_COMMAND) {
      if (lParam == WM_LBUTTONUP) {
        ShowWindow(hWindow, SW_SHOW);

        return LRESULT_SUCCESS;
      }
    } else if (nMessage == WM_COMMAND) {
      if (wParam == BN_CLICKED) {
        if (lParam == IDB_QUICK_SCAN) {
          SendMessageW(ButtonQuickScan, WM_COMMAND, CBN_TOGGLE, TRUE);
          SendMessageW(ButtonFullScan, WM_COMMAND, CBN_TOGGLE, FALSE);
          SendMessageW(ButtonCustomScan, WM_COMMAND, CBN_TOGGLE, FALSE);
          Gui::ButtonSelectedHandle = ButtonQuickScan;

          return LRESULT_SUCCESS;
        } else if (lParam == IDB_FULL_SCAN) {
          SendMessageW(ButtonQuickScan, WM_COMMAND, CBN_TOGGLE, FALSE);
          SendMessageW(ButtonFullScan, WM_COMMAND, CBN_TOGGLE, TRUE);
          SendMessageW(ButtonCustomScan, WM_COMMAND, CBN_TOGGLE, FALSE);
          Gui::ButtonSelectedHandle = ButtonFullScan;

          return LRESULT_SUCCESS;
        } else if (lParam == IDB_CUSTOM_SCAN) {
          SendMessageW(ButtonQuickScan, WM_COMMAND, CBN_TOGGLE, FALSE);
          SendMessageW(ButtonFullScan, WM_COMMAND, CBN_TOGGLE, FALSE);
          SendMessageW(ButtonCustomScan, WM_COMMAND, CBN_TOGGLE, TRUE);
          Gui::ButtonSelectedHandle = ButtonCustomScan;

          return LRESULT_SUCCESS;
        } else if (lParam == IDB_SCAN) {
          if (Gui::ScanButtonActivated) {
            AnalyzeStoppage = TRUE;
            Gui::ScanButtonActivated = FALSE;

            SendMessageW(CircularButtonScan, WM_COMMAND, CCBC_PROGRESS, FALSE);
            SendMessageW(CircularButtonScan, WM_COMMAND, CCBC_SETTEXT, (LPARAM)L"Scan");

            return LRESULT_SUCCESS;
          }

          if (Gui::ButtonSelectedHandle == ButtonQuickScan) {
            AnalyzeStoppage = FALSE;
            AnalyzeStopped = FALSE;

            if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnlyzeQuickScan, NULL, 0, NULL)) {
              Gui::ScanButtonActivated = TRUE;

              SendMessageW(CircularButtonScan, WM_COMMAND, CCBC_PROGRESS, TRUE);
              SendMessageW(CircularButtonScan, WM_COMMAND, CCBC_SETTEXT, (LPARAM)L"Cancel");
            }
          } else if (Gui::ButtonSelectedHandle == ButtonFullScan) {
            AnalyzeStoppage = FALSE;
            AnalyzeStopped = FALSE;

            if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnlyzeFullScan, NULL, 0, NULL)) {
              Gui::ScanButtonActivated = TRUE;

              SendMessageW(CircularButtonScan, WM_COMMAND, CCBC_PROGRESS, TRUE);
              SendMessageW(CircularButtonScan, WM_COMMAND, CCBC_SETTEXT, (LPARAM)L"Cancel");
            }
          } else if (Gui::ButtonSelectedHandle == ButtonCustomScan) {
            HRESULT           hResult;
            IFileOpenDialog * IFOD;

            if (HRESULT_SUCCESS(hResult = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&IFOD)))) {
              FILEOPENDIALOGOPTIONS FileOpenDialogOptions;

              IFOD->GetOptions(&FileOpenDialogOptions);

              if (FileOpenDialogOptions & FOS_ALLOWMULTISELECT) {
                FileOpenDialogOptions ^= FOS_ALLOWMULTISELECT;
              }

              IFOD->SetOptions(FileOpenDialogOptions ^ FOS_PICKFOLDERS | FOS_FORCESHOWHIDDEN);

              if (HRESULT_SUCCESS(hResult = IFOD->Show(NULL))) {
                IShellItem * ShellItem;

                if (HRESULT_SUCCESS(hResult = IFOD->GetResult(&ShellItem))) {
                  PWSTR szFilePath;

                  if (HRESULT_SUCCESS(hResult = ShellItem->GetDisplayName(SIGDN_FILESYSPATH, &szFilePath))) {
                    SIZE_T cbFilePath = (wcslen(szFilePath) + 1) * sizeof(WCHAR);
                    PVOID  pFilePathNew;

                    if ((pFilePathNew = VirtualAlloc(NULL, cbFilePath + 3 * sizeof(WCHAR) + LONG_PATH_MAX, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
                      CopyMemory(pFilePathNew, szFilePath, cbFilePath);
                      CopyMemory((PVOID)((ULONG_PTR)pFilePathNew + cbFilePath - 2), L"\\*", 3 * sizeof(WCHAR));

                      AnalyzeStoppage = FALSE;
                      AnalyzeStopped = FALSE;

                      if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnalyzeFolder, pFilePathNew, 0, NULL)) {
                        Gui::ScanButtonActivated = TRUE;

                        SendMessageW(CircularButtonScan, WM_COMMAND, CCBC_PROGRESS, TRUE);
                        SendMessageW(CircularButtonScan, WM_COMMAND, CCBC_SETTEXT, (LPARAM)L"Cancel");
                      }
                    }
                  }

                  ShellItem->Release();
                }
              }

              IFOD->Release();
            } 

            SetLastError(hResult);

            if (HRESULT_FAILED(hResult)) {
              return LRESULT_ERROR;
            }
          }

          return LRESULT_SUCCESS;
        }
      }
    } else if (nMessage == WM_PAINT) {
      PAINTSTRUCT PaintStruct;
      HDC         hDc = BeginPaint(hWindow, &PaintStruct);
      Graphics    Gfx(hDc);
      RectF       TextBoundryBox;

      static WCHAR szContactInfo[] = L"eveantivirus@mail.com";
      static INT   dwContactInfoLenght = (INT)wcslen(szContactInfo);

      Gfx.SetTextRenderingHint(TextRenderingHint::TextRenderingHintAntiAlias);
      Gfx.MeasureString(szContactInfo, dwContactInfoLenght, Gui::ContactInfoFont, { 0, 0 }, &TextBoundryBox);
      Gfx.DrawString(szContactInfo, dwContactInfoLenght, Gui::ContactInfoFont, { WINDOW_WIDTH / 2 - TextBoundryBox.Width / 2, WINDOW_HEIGHT - TextBoundryBox.Height }, Gui::ContactInfoColour);

      EndPaint(hWindow, &PaintStruct);

      return LRESULT_SUCCESS;
    } else if (nMessage == WM_CREATE) {
      ContactInfoFont = new Font(L"Segoe UI", 11);
      ContactInfoColour = new SolidBrush(Color::Black);

      return LRESULT_SUCCESS;
    }

    return DefWindowProcW(hWindow, nMessage, wParam, lParam);
  }

  HRESULT EVEAPI Init(
    IN HINSTANCE hInstance
    )
  {
    HWND            hTitleBar;
    HWND            hExitButton;
    WNDCLASSW       WindowClassW;
    ULONG           X = GetSystemMetrics(SM_CXFULLSCREEN) / 2 - WINDOW_WIDTH / 2;
    ULONG           Y = GetSystemMetrics(SM_CYFULLSCREEN) / 2 - WINDOW_HEIGHT / 2;
    ULONG           dwToastNoficicationX = GetSystemMetrics(SM_CXFULLSCREEN) - TOAST_NOTIFICIATION_WIDTH - 8;
    ULONG           dwToastNoficicationY = GetSystemMetrics(SM_CYFULLSCREEN) - TOAST_NOTIFICIATION_HEIGHT - 8;
    RECT            WindowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    NOTIFYICONDATAW NotifyIconDataW;

    if (!SetWindowsHookExW(WH_MOUSE_LL, Gui::LowLevelMouseHook, NULL, 0)) {
      DebugPrint("failed to set low lever mouse hook");

      return GetLastError();
    }
  
    Gui::ButtonInit();

  #pragma region class init
    ZeroMemory(&WindowClassW, sizeof(WindowClassW));
    WindowClassW.lpfnWndProc = Gui::WindowProcedure;
    WindowClassW.hInstance = hInstance;
    WindowClassW.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(RESOURCE_ICON));
    WindowClassW.hCursor = LoadCursorW(NULL, (PWSTR)IDC_ARROW);
    WindowClassW.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
    WindowClassW.lpszClassName = SZ_WINDOW_CLASS;

    if (!RegisterClassW(&WindowClassW)) {
      DebugPrint("failed to register the main window class");

      return ERROR_INVALID_PARAMETER;
    }

    WindowClassW.hIcon = NULL;
    WindowClassW.lpfnWndProc = Gui::TitleBarProcedure;
    WindowClassW.lpszClassName = SZ_TITLEBAR_CLASS;

    if (!RegisterClassW(&WindowClassW)) {
      DebugPrint("failed to register the title bar class");

      return ERROR_INVALID_PARAMETER;
    }

    WindowClassW.lpfnWndProc = Gui::ExitButtonProcedure;
    WindowClassW.lpszClassName = SZ_EXITBUTTON_CLASS;

    if (!RegisterClassW(&WindowClassW)) {
      DebugPrint("failed to register the exit button class");

      return ERROR_INVALID_PARAMETER;
    }

    WindowClassW.lpfnWndProc = Gui::ButtonProcedure;
    WindowClassW.lpszClassName = SZ_BUTTON_CLASS;

    if (!RegisterClassW(&WindowClassW)) {
      DebugPrint("failed to register the button class");

      return ERROR_INVALID_PARAMETER;
    }

    WindowClassW.lpfnWndProc = Gui::CircularButtonProcedure;
    WindowClassW.lpszClassName = SZ_CIRCULARBUTTON_CLASS;

    if (!RegisterClassW(&WindowClassW)) {
      DebugPrint("failed to register the circular button class");

      return ERROR_INVALID_PARAMETER;
    }

    WindowClassW.hIcon = NULL;
    WindowClassW.lpfnWndProc = ToastNotificationProcedure;
    WindowClassW.lpszClassName = SZ_TOASNOTIFICATION_CLASS;

    if (!RegisterClassW(&WindowClassW)) {
      DebugPrint("failed to register the toast notification class");

      return ERROR_INVALID_PARAMETER;
    }
  #pragma endregion class init

    AdjustWindowRect(&WindowRect, WS_POPUPWINDOW, FALSE);

  #pragma region windows create
    if (!(Gui::MainWindow = CreateWindowExW(0, SZ_WINDOW_CLASS, L"Eve Antivirus", WS_POPUP, X, Y, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL))) {
      DebugPrint("failed to create the window");

      return ERROR_INVALID_PARAMETER;
    }

    if (!(hTitleBar = CreateWindowExW(0, SZ_TITLEBAR_CLASS, L"EveAntivirusTitleBar", WS_CHILD, 0, 0, WINDOW_WIDTH, TITLEBAR_HEIGHT, Gui::MainWindow, NULL, hInstance, NULL))) {
      DebugPrint("failed to create the title bar");
       
      return ERROR_INVALID_PARAMETER;
    }

    if (!(hExitButton = CreateWindowExW(0, SZ_EXITBUTTON_CLASS, L"EveAntivirusExitButton", WS_CHILD, WINDOW_WIDTH - EXITBUTTON_DIMENSION, 0, EXITBUTTON_DIMENSION, EXITBUTTON_DIMENSION, hTitleBar, NULL, hInstance, NULL))) {
      DebugPrint("failed to create the exit button");
       
      return ERROR_INVALID_PARAMETER;
    }

    if (!(ButtonQuickScan = Gui::CreateButton(IDB_QUICK_SCAN, (PWSTR)L"Quick Scan", 0, TITLEBAR_HEIGHT + (BUTTON_HEIGHT * 0), WINDOW_WIDTH, BUTTON_HEIGHT, Gui::MainWindow, hInstance))) {
      DebugPrint("failed to create the quick scan button");
       
      return ERROR_INVALID_PARAMETER;
    }

    if (!(ButtonFullScan = Gui::CreateButton(IDB_FULL_SCAN, (PWSTR)L"Full Scan", 0, TITLEBAR_HEIGHT + (BUTTON_HEIGHT * 1), WINDOW_WIDTH, BUTTON_HEIGHT, Gui::MainWindow, hInstance))) {
      DebugPrint("failed to create the full scan button");
       
      return ERROR_INVALID_PARAMETER;
    }

    if (!(ButtonCustomScan = Gui::CreateButton(IDB_CUSTOM_SCAN, (PWSTR)L"Custom Scan", 0, TITLEBAR_HEIGHT + (BUTTON_HEIGHT * 2), WINDOW_WIDTH, BUTTON_HEIGHT, Gui::MainWindow, hInstance))) {
      DebugPrint("failed to create the custom scan button");
       
      return ERROR_INVALID_PARAMETER;
    }

    #define DIAMETER ((ULONG)(WINDOW_WIDTH - (FLOAT)WINDOW_WIDTH * 0.5))

    if (!(CircularButtonScan = Gui::CreateCircularButton(IDB_SCAN, (PWSTR)L"Scan", (WINDOW_WIDTH - DIAMETER) / 2, WINDOW_HEIGHT / 2 - DIAMETER / 2, DIAMETER, DIAMETER, Gui::MainWindow, hInstance))) {
      DebugPrint("failed to create the custom scan button");
       
      return ERROR_INVALID_PARAMETER;
    }

    #undef DIAMETER

    if (!(ToastNotificationWindow = CreateWindowExW(0, SZ_TOASNOTIFICATION_CLASS, L"EveToastNofication", WS_POPUP, dwToastNoficicationX, dwToastNoficicationY, TOAST_NOTIFICIATION_WIDTH, TOAST_NOTIFICIATION_HEIGHT, NULL, NULL, hInstance, NULL))) {
      DebugPrint("failed to create the custom scan button");
       
      return ERROR_INVALID_PARAMETER;
    }
  #pragma endregion windows create
       
    UpdateWindow(hExitButton);
    UpdateWindow(hTitleBar);
    UpdateWindow(ButtonQuickScan);
    UpdateWindow(ButtonFullScan);
    UpdateWindow(ButtonCustomScan);
    UpdateWindow(CircularButtonScan);
    UpdateWindow(ToastNotificationWindow);
    ShowWindow(hExitButton, SW_SHOW);
    ShowWindow(hTitleBar, SW_SHOW);
    ShowWindow(ButtonQuickScan, SW_SHOW);
    ShowWindow(ButtonFullScan, SW_SHOW);
    ShowWindow(ButtonCustomScan, SW_SHOW);
    ShowWindow(CircularButtonScan, SW_SHOW);

    SendMessageW(ButtonQuickScan, WM_COMMAND, CBN_TOGGLE, TRUE);
    ButtonSelectedHandle = ButtonQuickScan;

    ZeroMemory(&NotifyIconDataW, sizeof(NotifyIconDataW));

    if (IsWindows7OrGreater()) {
      GUID NotifyIconGUID = { 0x20BF115B, 0xDBB4, 0x469D, {0x97, 0xD7, 0x11, 0x59, 0x9A, 0x6D, 0x47, 0x7A} };

      NotifyIconDataW.cbSize = sizeof(NotifyIconDataW);
      NotifyIconDataW.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_MESSAGE | NIF_GUID;
      NotifyIconDataW.uVersion = NOTIFYICON_VERSION_4;

      CopyMemory(&NotifyIconDataW.guidItem, &NotifyIconGUID, sizeof(NotifyIconGUID));
    } else {
      NotifyIconDataW.cbSize = NOTIFYICONDATA_V3_SIZE;
      NotifyIconDataW.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_MESSAGE;
      NotifyIconDataW.uID = 0x20BF115B;
      NotifyIconDataW.uVersion = NOTIFYICON_VERSION;
    }

    NotifyIconDataW.hWnd = Gui::MainWindow;
    NotifyIconDataW.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(RESOURCE_SMALL_ICON));
    NotifyIconDataW.uCallbackMessage = NIM_COMMAND;
    wcscpy_s(NotifyIconDataW.szTip, L"Eve Antivirus Helper");

    Shell_NotifyIconW(NIM_DELETE, &NotifyIconDataW);
    Shell_NotifyIconW(NIM_ADD, &NotifyIconDataW);
    Shell_NotifyIconW(NIM_SETVERSION, &NotifyIconDataW);

    return ERROR_SUCCESS;
  }

  HRESULT EVEAPI Run(VOID) {
    MSG  Message;
    LONG lResult;

    UpdateWindow(Gui::MainWindow);

    while (TRUE) {
      if ((lResult = PeekMessageW(&Message, NULL, 0, 0, PM_REMOVE))) {
        if (lResult < 0) {
          return GetLastError();
        }

        TranslateMessage(&Message);
        DispatchMessageW(&Message);
      }

      if (AnalyzeStopped) {
        AnalyzeStopped = FALSE;
        Gui::ScanButtonActivated = FALSE;

        SendMessageW(CircularButtonScan, WM_COMMAND, CCBC_PROGRESS, FALSE);
        SendMessageW(CircularButtonScan, WM_COMMAND, CCBC_SETTEXT, (LPARAM)L"Scan");
      }

      YieldProcessor();
    }

    return ERROR_SUCCESS;
  }
}
