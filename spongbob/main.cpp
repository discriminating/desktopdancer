﻿#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <shellapi.h>
#include <vector>
#include <Commctrl.h>

#include "resource1.h"

#pragma comment(lib, "gdiplus.lib")

#define IDM_CONFIG 1
#define IDM_EXIT 2
#define TBM_GETPOS (WM_USER)

NOTIFYICONDATAW nid;
using namespace Gdiplus;

bool isTopMost = false;

// Locking movement
bool isDragging = false;
bool canDrag = false;
POINT lastMousePos;

// Attaching to window
RECT oldrect;
RECT* oldrectptr = &oldrect;
wchar_t g_windowName[260];

HWND GLOBALHWND;
HWND DIALOGHWND;

// Picking file path
LPCWSTR NAME_CONSTANT;
WCHAR filePath[MAX_PATH]; // To store the selected file path

typedef struct {
    int FrameDelaySliderPos;
    int FrameMSWaitTime;
    bool LockMovement;
    bool AttachToWindow;
    bool TopMost;
    wchar_t windowName[260];
    int offsetX;
    int offsetY;
} settings;

settings* settingsPtr = new settings;
settings* clonedSettings = new settings;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    WCHAR title[1024];
    if (GetWindowText(hwnd, title, 1024) > 0) 
        SendMessage((HWND)lParam, CB_ADDSTRING, 0, (LPARAM)title);

    return TRUE;
}

void SetWindowPos(wchar_t windowName[], bool forceUpdate) {
    RECT targetRect;
    GetWindowRect(FindWindowW(NULL, windowName), &targetRect);

    if (targetRect.left != oldrectptr->left || targetRect.top != oldrectptr->top ||
        targetRect.right != oldrectptr->right || targetRect.bottom != oldrectptr->bottom || forceUpdate) {

        int newX = targetRect.left + (settingsPtr->offsetX * 30);
        int newY = targetRect.top + (settingsPtr->offsetY * 30);

        SetWindowPos(GLOBALHWND, (isTopMost) ? HWND_TOPMOST : HWND_NOTOPMOST, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        oldrectptr->left = targetRect.left;
        oldrectptr->right = targetRect.right;
        oldrectptr->top = targetRect.top;
        oldrectptr->bottom = targetRect.bottom;
    }
}

INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    DIALOGHWND = hwndDlg;
    wchar_t buffer[50];
    double percentage;

    switch (uMsg) {
    case WM_INITDIALOG:
        memcpy(clonedSettings, settingsPtr, sizeof(settings));

        SetDlgItemTextW(hwndDlg, IDD_DIALOG1, L"Config");
        SetDlgItemTextW(hwndDlg, IDC_STATIC0, L"Please configure the settings below");
        SetDlgItemTextW(hwndDlg, IDC_STATIC2, L"Delay in frames (ms)");

        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER1), TBM_SETPOS, 1, settingsPtr->FrameDelaySliderPos);

        percentage = (double)settingsPtr->FrameDelaySliderPos / 10.0;
        _snwprintf_s(buffer, _countof(buffer), L"%.2f ms", percentage * 100.0);
        settingsPtr->FrameMSWaitTime = (int)(percentage * 100);

        SetDlgItemTextW(hwndDlg, IDC_STATIC3, buffer);

        _snwprintf_s(buffer, _countof(buffer), L"%.2f fps", (1 / percentage * 10));
        SetDlgItemTextW(hwndDlg, IDC_STATIC6, buffer);

        SetDlgItemTextW(hwndDlg, IDC_STATIC4, L"<3");

        SetDlgItemTextW(hwndDlg, IDC_STATIC5, L"Selected Gif: None");
        SetDlgItemTextW(hwndDlg, IDC_STATIC7, L"Select gif...");
        SetDlgItemTextW(hwndDlg, IDC_STATIC10, L"Offset");
        SetDlgItemTextW(hwndDlg, IDC_STATIC11, L" X");
        SetDlgItemTextW(hwndDlg, IDC_STATIC12, L" Y");

        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER2), TBM_SETPOS, 1, settingsPtr->offsetX);
        std::cout << (settingsPtr->offsetX * 30) << std::endl;
        _snwprintf_s(buffer, _countof(buffer), L"%i pixels", (settingsPtr->offsetX * 30));

        SetDlgItemTextW(hwndDlg, IDC_STATIC8, buffer);
        SetWindowPos(settingsPtr->windowName, true);

        SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER3), TBM_SETPOS, 1, settingsPtr->offsetY);
        std::cout << (settingsPtr->offsetY * 30) << std::endl;
        _snwprintf_s(buffer, _countof(buffer), L"%i pixels", (settingsPtr->offsetY * 30));

        SetDlgItemTextW(hwndDlg, IDC_STATIC9, buffer);
        SetWindowPos(settingsPtr->windowName, true);

        SetDlgItemTextW(hwndDlg, IDC_CHECK1, L"Lock movement");
        SetDlgItemTextW(hwndDlg, IDC_CHECK2, L"Attach to window");

        SetDlgItemTextW(hwndDlg, IDC_CHECK3, L"Top Most");
        CheckDlgButton(hwndDlg, IDC_CHECK3, (isTopMost) ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg, IDC_CHECK1, !canDrag);
        
        EnumWindows(EnumWindowsProc, (LPARAM)GetDlgItem(hwndDlg, IDC_COMBO1));
        SendMessage(GetDlgItem(hwndDlg, IDC_COMBO1), CB_SETDROPPEDWIDTH, 200, 0);

        RECT rect;
        GetWindowRect(GetDlgItem(hwndDlg, IDC_COMBO1), &rect);

        SetWindowPos(GetDlgItem(hwndDlg, IDC_COMBO1), NULL, rect.left, rect.top, rect.right - rect.left, (10 * SendMessage(GetDlgItem(hwndDlg, IDC_COMBO1), CB_GETITEMHEIGHT, 0, 0)), SWP_NOMOVE | SWP_NOZORDER);
        EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO1), false);

        if (settingsPtr->AttachToWindow) {
            EnableWindow(GetDlgItem(hwndDlg, IDC_SLIDER2), true);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SLIDER3), true);
            EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO1), true);

            CheckDlgButton(hwndDlg, IDC_CHECK2, BST_CHECKED);

            HWND hComboBox = GetDlgItem(hwndDlg, IDC_COMBO1); // Replace IDC_COMBO1 with your ComboBox control's ID

            // Set the text of the selected item in the ComboBox
            SendMessage(hComboBox, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)settingsPtr->windowName);
            std::cout << "HEELLEOEOLLEOEOEOOo" << std::endl;

        } else {
            EnableWindow(GetDlgItem(hwndDlg, IDC_SLIDER2), false);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SLIDER3), false);
        }

        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            if (wcslen(filePath) == 0) 
                MessageBox(0, L"Please select an image/gif file", L"Error", MB_ICONERROR);
            else {
                HWND hComboBox = GetDlgItem(hwndDlg, IDC_COMBO1);

                int selectedIndex = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);

                if (selectedIndex != CB_ERR) {
                    wchar_t buffer[256];
                    SendMessage(hComboBox, CB_GETLBTEXT, selectedIndex, (LPARAM)buffer);
                    wcscpy_s(settingsPtr->windowName, buffer);
                }

                EndDialog(hwndDlg, IDOK);
            }

        }
        else if (LOWORD(wParam) == IDCANCEL) {
            memcpy(settingsPtr, clonedSettings, sizeof(settings));
            SetWindowPos(settingsPtr->windowName, true);
            EndDialog(hwndDlg, IDCANCEL);
        }
        else if (LOWORD(wParam) == IDEXIT) {
            exit(0);
        }

        if (LOWORD(wParam) == IDC_CHECK1 && HIWORD(wParam) == BN_CLICKED)
            canDrag = !IsDlgButtonChecked(hwndDlg, IDC_CHECK1);

        if (LOWORD(wParam) == IDC_CHECK2 && HIWORD(wParam) == BN_CLICKED) {
            bool value = IsDlgButtonChecked(hwndDlg, IDC_CHECK2);
            settingsPtr->AttachToWindow = value;
            std::cout << settingsPtr->AttachToWindow << std::endl;

            EnableWindow(GetDlgItem(hwndDlg, IDC_SLIDER2), value);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SLIDER3), value);
            EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO1), value);
        }

        if (LOWORD(wParam) == IDC_CHECK3 && HIWORD(wParam) == BN_CLICKED) {
            bool isTopMost = IsDlgButtonChecked(hwndDlg, IDC_CHECK2);
            SetWindowPos(settingsPtr->windowName, true);
        }

        break;

    case WM_HSCROLL: {
        HWND hwndScrollBar = (HWND)lParam;

        if (hwndScrollBar == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
            settingsPtr->FrameDelaySliderPos = SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER1), TBM_GETPOS, 0, 0);;

            percentage = (double)settingsPtr->FrameDelaySliderPos / 10.0;
            _snwprintf_s(buffer, _countof(buffer), L"%.2f ms", percentage * 100.0);
            settingsPtr->FrameMSWaitTime = (int)(percentage * 100);

            SetDlgItemTextW(hwndDlg, IDC_STATIC3, buffer);

            _snwprintf_s(buffer, _countof(buffer), L"%.2f fps", (1 / percentage * 10));
            SetDlgItemTextW(hwndDlg, IDC_STATIC6, buffer);
        }

        if (hwndScrollBar == GetDlgItem(hwndDlg, IDC_SLIDER2)) {
            settingsPtr->offsetX = SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER2), TBM_GETPOS, 0, 0);

            std::cout << (settingsPtr->offsetX * 30) << std::endl;
            _snwprintf_s(buffer, _countof(buffer), L"%i pixels", (settingsPtr->offsetX * 30));

            SetDlgItemTextW(hwndDlg, IDC_STATIC8, buffer);
            SetWindowPos(settingsPtr->windowName, true);
        }

        if (hwndScrollBar == GetDlgItem(hwndDlg, IDC_SLIDER3)) {
            settingsPtr->offsetY = SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER3), TBM_GETPOS, 0, 0);

            std::cout << (settingsPtr->offsetY * 30) << std::endl;
            _snwprintf_s(buffer, _countof(buffer), L"%i pixels", (settingsPtr->offsetY * 30));

            SetDlgItemTextW(hwndDlg, IDC_STATIC9, buffer);
            SetWindowPos(settingsPtr->windowName, true);
        }
    }
    case WM_LBUTTONDOWN: 
        POINT clickPoint;
        GetCursorPos(&clickPoint);

        RECT staticRect;
        HWND hStatic = GetDlgItem(hwndDlg, IDC_STATIC7);
        GetWindowRect(hStatic, &staticRect);

        if (PtInRect(&staticRect, clickPoint)) {

            OPENFILENAME ofn = { 0 };
            WCHAR _filePath[MAX_PATH] = L"";

            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwndDlg;
            ofn.lpstrFile = _filePath;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"GIF Files (*.gif)\0*.gif\0All Files (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn)) {
                wcscpy_s(filePath, MAX_PATH, ofn.lpstrFile);

                SetDlgItemTextW(hwndDlg, IDC_STATIC5, filePath);
                SetDlgItemTextW(hwndDlg, IDC_STATIC7, L"Gif set");
            }
        } break;
    }

    return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    GLOBALHWND = hwnd;
    static Image image(static_cast<LPCWSTR>(filePath), FALSE);
    static ULONG frameCount = image.GetFrameCount(&FrameDimensionTime);
    static ULONG currentFrame = 0;
    static DWORD frameStartTime = GetTickCount();

    switch (msg) {
    case WM_PAINT: {
        SetWindowPos(settingsPtr->windowName, 0);

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        Graphics graphics(hdc);

        if (image.GetLastStatus() == Ok && frameCount > 1) {
            DWORD currentTime = GetTickCount();
            DWORD elapsed = currentTime - frameStartTime;

            if (elapsed >= settingsPtr->FrameMSWaitTime) { // Avoids calling this every single frame
                graphics.Clear(Color(0, 0, 0, 0));
                frameStartTime = currentTime;
                currentFrame = (currentFrame + 1) % frameCount;
                image.SelectActiveFrame(&FrameDimensionTime, currentFrame);
                graphics.DrawImage(&image, 0, 0, image.GetWidth(), image.GetHeight());
            }

            InvalidateRect(hwnd, NULL, FALSE);
        }

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_CLOSE:
        PostQuitMessage(0);
        break;

    case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        if (canDrag) isDragging = true;
        GetCursorPos(&lastMousePos);
        break;

    case WM_MOUSEMOVE:
        if (isDragging) {
            POINT currentMousePos;
            GetCursorPos(&currentMousePos);
            int dx = currentMousePos.x - lastMousePos.x;
            int dy = currentMousePos.y - lastMousePos.y;

            RECT windowRect;
            GetWindowRect(hwnd, &windowRect);

            SetWindowPos(hwnd, (HWND)HWND_TOPMOST, windowRect.left + dx, windowRect.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

            lastMousePos = currentMousePos;
        }
        break;

    case WM_LBUTTONUP:
        ReleaseCapture();
        isDragging = false;
        break;

    case WM_APP + 1:
        if (lParam == 512) { // I forgot what macro 512 was know it works when you click on it in the tray though
            HWND hConfigDlg = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), NULL, DialogProc);
            if (hConfigDlg != NULL) {
                ShowWindow(hConfigDlg, SW_SHOW);
                SetForegroundWindow(hConfigDlg);
            } break;
        } break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

int main() {
    //FreeConsole();

    settingsPtr->FrameDelaySliderPos = 0;
    settingsPtr->FrameMSWaitTime = 0;
    settingsPtr->LockMovement = true;
    settingsPtr->AttachToWindow = false;
    settingsPtr->TopMost = false;
    settingsPtr->offsetX = 0;
    settingsPtr->offsetY = 0;


    int result = DialogBoxW(0, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DialogProc);

    if (result == IDCANCEL) exit(0);

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Get the image dimensions
    Image image(static_cast<LPCWSTR>(filePath), FALSE);
    int imageWidth = image.GetWidth();
    int imageHeight = image.GetHeight();

    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"DummyWindow";
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST, wc.lpszClassName, L"DummyWindow",
        WS_POPUP | WS_VISIBLE, 100, 100, imageWidth, imageHeight, 
        NULL, NULL, GetModuleHandle(NULL), NULL);

    // Set transparency key color
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1; // "Unique" ID for the tray icon
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_APP + 1;
    nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDI_ICON123));
    wcscpy_s(nid.szTip, L"Desktop Dancer");

    Shell_NotifyIconW(NIM_ADD, &nid);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    Shell_NotifyIconW(NIM_DELETE, &nid);
    GdiplusShutdown(gdiplusToken);
    delete settingsPtr;

    return 0;
}