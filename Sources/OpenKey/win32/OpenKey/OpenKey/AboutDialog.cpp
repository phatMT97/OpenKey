/*----------------------------------------------------------
OpenKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
Contact: maivutuyen.91@gmail.com
Github: https://github.com/tuyenvm/OpenKey
Fanpage: https://www.facebook.com/OpenKeyVN

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#include "stdafx.h"
#include "AboutDialog.h"
#include "AppDelegate.h"
#include "OpenKeyHelper.h"
#include "OpenKeyManager.h"
#include "../../../engine/Engine.h"
#include <shellapi.h>
#include <dwmapi.h>
#include <windowsx.h>  // For GET_X_LPARAM, GET_Y_LPARAM
#include <commctrl.h>  // For SetWindowSubclass
#include "sciter-x-dom.hpp"  // For SciterFindElement
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

// Implement missing Sciter application function
namespace sciter {
	namespace application {
		HINSTANCE hinstance() {
			return GetModuleHandle(NULL);
		}
	}
}

AboutDialog::AboutDialog()
	: sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{0, 0, 500, 600}) {
	
	// Load HTML file - get path relative to executable
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	
	// Remove executable name to get directory
	WCHAR* lastSlash = wcsrchr(exePath, L'\\');
	if (lastSlash) *lastSlash = L'\0';
	
	// Resources folder is at same level as executable
	WCHAR htmlPath[MAX_PATH];
	swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\about\\about.html", exePath);
	
	if (!load(htmlPath)) {
		MessageBoxW(NULL, htmlPath, L"Failed to load about.html", MB_OK | MB_ICONERROR);
		return;
	}

	// Show the window
	expand();
	
	// Enable Acrylic blur effect
	enableAcrylicEffect();
	
	// Subclass window for dragging
	SetWindowSubclass(get_hwnd(), AboutDialog::SubclassProc, 1, 0);
	
	// Set window title for anti-spam detection
	SetWindowTextW(get_hwnd(), L"About OpenKey");
	
	// Center window on screen
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	RECT rc;
	GetWindowRect(get_hwnd(), &rc);
	int winWidth = rc.right - rc.left;
	int winHeight = rc.bottom - rc.top;
	int x = (screenWidth - winWidth) / 2;
	int y = (screenHeight - winHeight) / 2;
	SetWindowPos(get_hwnd(), NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

AboutDialog::~AboutDialog() {
	if (get_hwnd()) {
		RemoveWindowSubclass(get_hwnd(), AboutDialog::SubclassProc, 1);
	}
}

// Subclass procedure for window dragging and close handling
LRESULT CALLBACK AboutDialog::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	// WM_CLOSE: Force exit subprocess (bypasses Sciter cleanup issues)
	if (msg == WM_CLOSE) {
		ExitProcess(0);  // Force exit - subprocess terminates immediately, RAM freed
		return 0;
	}
	
	if (msg == WM_NCHITTEST) {
		LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
		
		if (result == HTCLIENT) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ScreenToClient(hwnd, &pt);
			
			RECT rc;
			GetClientRect(hwnd, &rc);
			
			// Only allow dragging on very top (header area only)
			// Keep it small to ensure most content is clickable
			const int DRAG_ZONE_HEIGHT = 150;  // Only top 150px is draggable
			
			if (pt.y < DRAG_ZONE_HEIGHT) {
				return HTCAPTION;  // Enable dragging on header
			}
			// Otherwise return HTCLIENT (let Sciter handle clicks)
		}
		return result;
	}
	
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void AboutDialog::show() {
	// Show the window using Win32 API directly
	ShowWindow(get_hwnd(), SW_SHOW);
	SetForegroundWindow(get_hwnd());
}

// Handle Sciter DOM events - catches BUTTON_CLICK
bool AboutDialog::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
	// First let base class handle it
	if (sciter::window::handle_event(he, params))
		return true;
	
	if (params.cmd == BUTTON_CLICK) {
		sciter::dom::element el(params.heTarget);
		auto id = el.get_attribute("id");
		
		// Check for button clicks
		if (id == L"check-update") {
			checkUpdate();
			return true;
		}
		else if (id == L"close-btn") {
			closeWindow();
			return true;
		}
		
		// Check for link buttons with data-url attribute
		auto dataUrl = el.get_attribute("data-url");
		if (dataUrl.length() > 0) {
			std::wstring wurl(dataUrl.c_str());
			std::string url(wurl.begin(), wurl.end());
			openUrl(url);
			return true;
		}
	}
	
	return false;
}

void AboutDialog::setVersionInfo() {
	// Get version and build date
	std::wstring version = OpenKeyHelper::getVersionString();
	std::wstring buildDate = _T(__DATE__);
	
	// Convert to UTF-8 for JavaScript
	std::string versionUtf8 = wideStringToUtf8(version);
	std::string buildDateUtf8 = wideStringToUtf8(buildDate);
	
	// Call JavaScript function to set version info
	call_function("setVersionInfo", versionUtf8.c_str(), buildDateUtf8.c_str());
}

void AboutDialog::openUrl(std::string url) {
	// Open URL in system default browser
	ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void AboutDialog::checkUpdate() {
	std::string newVersion;
	bool hasUpdate = OpenKeyManager::checkUpdate(newVersion);
	
	// Call JavaScript function to show result
	// TODO: Implement proper JavaScript calling
	// call_function("showUpdateResult", hasUpdate, newVersion.c_str());
	
	// Temporary: show message box instead
	if (hasUpdate) {
		MessageBoxA(NULL, ("New version available: " + newVersion).c_str(), "Update", MB_OK);
	} else {
		MessageBoxA(NULL, "You are using the latest version!", "Update", MB_OK);
	}
}

void AboutDialog::closeWindow() {
	// Force exit subprocess (bypasses Sciter cleanup issues)
	ExitProcess(0);
}

void AboutDialog::showUpdateDialog(std::string message, std::string newVersion) {
	// Convert message to wide string
	std::wstring wideMessage = utf8ToWideString(message);
	
	int msgboxID = MessageBoxW(
		get_hwnd(),
		wideMessage.c_str(),
		L"OpenKey Update",
		MB_ICONEXCLAMATION | MB_YESNO
	);
	
	if (msgboxID == IDYES) {
		// Call OpenKeyUpdate
		WCHAR path[MAX_PATH];
		GetCurrentDirectoryW(MAX_PATH, path);
		wcscat_s(path, L"\\OpenKeyUpdate.exe");
		ShellExecuteW(0, L"", path, 0, 0, SW_SHOWNORMAL);
		
		AppDelegate::getInstance()->onOpenKeyExit();
	}
}

void AboutDialog::showInfoMessage(std::string message) {
	std::wstring wmessage = utf8ToWideString(message);
	MessageBoxW(get_hwnd(), wmessage.c_str(), L"OpenKey", MB_OK | MB_ICONINFORMATION);
}

// --- DWM Acrylic Effect Implementation ---

// Undocumented Windows API structures
struct ACCENT_POLICY {
	int AccentState;
	int AccentFlags;
	int GradientColor;  // ABGR format
	int AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA {
	int Attrib;
	void* pvData;
	size_t cbData;
};

enum ACCENT_STATE {
	ACCENT_DISABLED = 0,
	ACCENT_ENABLE_GRADIENT = 1,
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
	ACCENT_ENABLE_BLURBEHIND = 3,
	ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,  // Windows 10 1803+
	ACCENT_ENABLE_HOSTBACKDROP = 5         // Windows 11 (Mica)
};

void AboutDialog::enableAcrylicEffect() {
	HWND hwnd = get_hwnd();

	// 1. CRITICAL: Set WS_EX_LAYERED style first
	//    Without this, the window will have black background instead of blur
	SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

	// 2. Try Acrylic (Windows 10 1803+)
	HMODULE hUser = GetModuleHandle(L"user32.dll");
	if (hUser) {
		typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);
		auto SetWindowCompositionAttribute = 
			(pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

		if (SetWindowCompositionAttribute) {
			ACCENT_POLICY policy = { 0 };
			policy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
			policy.AccentFlags = 0;
			policy.GradientColor = 0x01FFFFFF;  // ABGR: 1% white tint (nearly transparent)
			policy.AnimationId = 0;

			WINDOWCOMPOSITIONATTRIBDATA data = { 0 };
			data.Attrib = 19;  // WCA_ACCENT_POLICY
			data.pvData = &policy;
			data.cbData = sizeof(policy);

			SetWindowCompositionAttribute(hwnd, &data);
		}
		else {
			// Fallback to DWM Blur (Windows 7/8)
			DWM_BLURBEHIND bb = { 0 };
			bb.dwFlags = DWM_BB_ENABLE;
			bb.fEnable = TRUE;
			bb.hRgnBlur = NULL;
			DwmEnableBlurBehindWindow(hwnd, &bb);
		}
	}

	// 3. Fix black corners on Windows 11
	//    Windows 11 requires explicit corner preference
	typedef enum {
		DWMWCP_DEFAULT = 0,
		DWMWCP_DONOTROUND = 1,
		DWMWCP_ROUND = 2,
		DWMWCP_ROUNDSMALL = 3
	} DWM_WINDOW_CORNER_PREFERENCE;

	DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
	DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));  // DWMWA_WINDOW_CORNER_PREFERENCE = 33
}