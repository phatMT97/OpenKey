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
#include "MacroDialogSciter.h"
#include "stdafx.h"
#include "OpenKeyHelper.h"
#include <commdlg.h>
#include <dwmapi.h>
#include <CommCtrl.h>
#include <windowsx.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

// External macro functions from engine
extern void getAllMacro(std::vector<std::vector<Uint32>>& keys, std::vector<std::string>& macroText, std::vector<std::string>& macroContent);
extern bool addMacro(const std::string& macroName, const std::string& macroContent);
extern bool deleteMacro(const std::string& macroName);
extern bool hasMacro(const std::string& macroName);
extern void getMacroSaveData(std::vector<Byte>& data);
extern void readFromFile(const std::string& path, const bool& append);
extern void saveToFile(const std::string& path);
extern void initMacroMap(const Byte* pData, const int& size);

// Helper function to convert UTF-8 to wide string
extern std::wstring utf8ToWideString(const std::string& utf8str);
// Helper function to convert wide string to UTF-8
extern std::string wideStringToUtf8(const std::wstring& wstr);

// Acrylic blur structures
struct ACCENT_POLICY {
	int AccentState;
	int AccentFlags;
	int GradientColor;
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
	ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
	ACCENT_ENABLE_HOSTBACKDROP = 5
};

MacroDialogSciter::MacroDialogSciter() 
	: sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{ 0, 0, 400, 600 }) {
	
	// Load macro data from registry BEFORE loading HTML (subprocess needs to init)
	// NOTE: getRegBinary returns a static pointer - DO NOT delete[] it
	DWORD macroDataSize = 0;
	BYTE* macroData = OpenKeyHelper::getRegBinary(_T("macroData"), macroDataSize);
	if (macroData && macroDataSize > 0) {
		initMacroMap(macroData, (int)macroDataSize);
		// Do NOT delete[] macroData - it's managed by OpenKeyHelper
	}
	
	// Load HTML file
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	WCHAR* lastSlash = wcsrchr(exePath, L'\\');
	if (lastSlash) *lastSlash = L'\0';
	
	WCHAR htmlPath[MAX_PATH];
	swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\macro\\macro.html", exePath);
	
	if (!load(htmlPath)) {
		MessageBoxW(NULL, htmlPath, L"Failed to load macro.html", MB_OK | MB_ICONERROR);
		return;
	}
	
	// Show the window
	expand();
	
	// Set window title for anti-spam detection
	// "Bảng gõ tắt" = "B\u1EA3ng g\u00F5 t\u1EAFt"
	SetWindowTextW(get_hwnd(), L"B\u1EA3ng g\u00F5 t\u1EAFt");  // Must match MACRO_WINDOW_TITLE in AppDelegate.cpp
	
	// Use fixed window size (matches CSS container: 380x450)
	SetWindowPos(get_hwnd(), NULL, 0, 0, 380, 450, SWP_NOMOVE | SWP_NOZORDER);
	OutputDebugStringW(L"MacroDialog: window set to fixed size 380x450\n");
	
	// Center window on screen
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	RECT rc;
	GetWindowRect(get_hwnd(), &rc);
	int winWidth = rc.right - rc.left;
	int winHeight = rc.bottom - rc.top;
	int x = (screenWidth - winWidth) / 2;
	int y = (screenHeight - winHeight) / 2;
	SetWindowPos(get_hwnd(), HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE);
	
	// Enable blur effect
	enableAcrylicEffect();
	
	// Subclass for WM_NCHITTEST (drag) and WM_CLOSE
	SetWindowSubclass(get_hwnd(), MacroDialogSciter::SubclassProc, 1, (DWORD_PTR)this);
}

MacroDialogSciter::~MacroDialogSciter() {
}

void MacroDialogSciter::show() {
	ShowWindow(get_hwnd(), SW_SHOW);
	SetForegroundWindow(get_hwnd());
}

void MacroDialogSciter::enableAcrylicEffect() {
	HWND hwnd = get_hwnd();

	// 1. CRITICAL: Set WS_EX_LAYERED style first
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
			policy.GradientColor = 0x80FFFFFF;  // ABGR: 50% white tint for visible blur
			policy.AnimationId = 0;

			WINDOWCOMPOSITIONATTRIBDATA data = { 0 };
			data.Attrib = 19;  // WCA_ACCENT_POLICY
			data.pvData = &policy;
			data.cbData = sizeof(policy);

			SetWindowCompositionAttribute(hwnd, &data);
		}
		else {
			// Fallback to DWM Blur
			DWM_BLURBEHIND bb = { 0 };
			bb.dwFlags = DWM_BB_ENABLE;
			bb.fEnable = TRUE;
			bb.hRgnBlur = NULL;
			DwmEnableBlurBehindWindow(hwnd, &bb);
		}
	}

	// 3. Fix corners on Windows 11
	typedef enum {
		DWMWCP_DEFAULT = 0,
		DWMWCP_DONOTROUND = 1,
		DWMWCP_ROUND = 2,
		DWMWCP_ROUNDSMALL = 3
	} DWM_WINDOW_CORNER_PREFERENCE;

	DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
	DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));
}

LRESULT CALLBACK MacroDialogSciter::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	if (msg == WM_CLOSE) {
		ExitProcess(0);  // Force exit subprocess
		return 0;
	}
	
	if (msg == WM_NCHITTEST) {
		LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
		if (result == HTCLIENT) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ScreenToClient(hwnd, &pt);
			
			// Drag zone: title bar height (40px) for easy dragging
			// Exclude close button area (last 40px on right side)
			RECT winRect;
			GetClientRect(hwnd, &winRect);
			int closeButtonZone = winRect.right - 40;
			
			if (pt.y < 40 && pt.x < closeButtonZone) {
				return HTCAPTION;
			}
		}
		return result;
	}
	
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

bool MacroDialogSciter::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
	// Handle DOCUMENT_READY to populate macro list
	if (params.cmd == DOCUMENT_READY) {
		OutputDebugStringW(L"MacroDialog: DOCUMENT_READY - calling fillMacroList()\n");
		fillMacroList();
		// Fixed window size - no need to recalc
		return true;
	}
	
	// Handle VALUE_CHANGED for hidden inputs (JS sets these, C++ reads them)
	if (params.cmd == VALUE_CHANGED) {
		sciter::dom::element el(params.heTarget);
		std::wstring id = el.get_attribute("id");
		
		if (id == L"val-action") {
			sciter::dom::element root = this->root();
			sciter::value actionVal = el.get_value();
			std::wstring action = actionVal.is_string() ? actionVal.get<std::wstring>() : L"";
			
			OutputDebugStringW((L"MacroDialog: val-action changed to '" + action + L"'\n").c_str());
			
			if (action == L"add") {
				sciter::dom::element nameEl = root.find_first("#val-macro-name");
				sciter::dom::element contentEl = root.find_first("#val-macro-content");
				
				if (nameEl.is_valid() && contentEl.is_valid()) {
					sciter::value nameVal = nameEl.get_value();
					sciter::value contentVal = contentEl.get_value();
					std::wstring name = nameVal.is_string() ? nameVal.get<std::wstring>() : L"";
					std::wstring content = contentVal.is_string() ? contentVal.get<std::wstring>() : L"";
					
					if (!name.empty() && !content.empty()) {
						OutputDebugStringW(L"MacroDialog: calling onAddMacro()\n");
						onAddMacro(name, content);
					}
				}
				// Clear action
				el.set_value(sciter::value(L""));
				return true;
			}
			
			if (action == L"delete") {
				sciter::dom::element nameEl = root.find_first("#val-macro-name");
				if (nameEl.is_valid()) {
					sciter::value nameVal = nameEl.get_value();
					std::wstring name = nameVal.is_string() ? nameVal.get<std::wstring>() : L"";
					
					if (!name.empty()) {
						OutputDebugStringW(L"MacroDialog: calling onDeleteMacro()\n");
						onDeleteMacro(name);
					}
				}
				el.set_value(sciter::value(L""));
				return true;
			}
			
			if (action == L"import") {
				OutputDebugStringW(L"MacroDialog: import action\n");
				onImportMacro();
				el.set_value(sciter::value(L""));
				return true;
			}
			
			if (action == L"export") {
				OutputDebugStringW(L"MacroDialog: export action\n");
				onExportMacro();
				el.set_value(sciter::value(L""));
				return true;
			}
			
			if (action == L"close") {
				OutputDebugStringW(L"MacroDialog: close action\n");
				PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
				return true;
			}
		}
	}
	
	// Handle button clicks - only for btn-close (direct click without JS)
	if (params.cmd == BUTTON_CLICK) {
		sciter::dom::element el(params.heTarget);
		std::wstring id = el.get_attribute("id");
		
		if (id == L"btn-close") {
			OutputDebugStringW(L"MacroDialog: btn-close clicked directly\n");
			PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
			return true;
		}
	}
	
	// Handle macro item click
	if (params.cmd == HYPERLINK_CLICK || params.cmd == BUTTON_CLICK) {
		sciter::dom::element el(params.heTarget);
		std::wstring className = el.get_attribute("class");
		
		if (className.find(L"macro-item") != std::wstring::npos) {
			// Get macro data from attributes
			std::wstring name = el.get_attribute("data-name");
			std::wstring content = el.get_attribute("data-content");
			
			// Fill input fields
			sciter::dom::element root = this->root();
			sciter::dom::element nameEl = root.find_first("#macro-name");
			sciter::dom::element contentEl = root.find_first("#macro-content");
			sciter::dom::element btnAdd = root.find_first("#btn-add");
			
			if (nameEl.is_valid()) nameEl.set_value(sciter::value(name));
			if (contentEl.is_valid()) contentEl.set_value(sciter::value(content));
			if (btnAdd.is_valid()) btnAdd.set_text(L"+ Sửa");
			
			// Update selection
			sciter::dom::element list = root.find_first("#macro-list");
			if (list.is_valid()) {
				for (int i = 0; i < (int)list.children_count(); i++) {
					sciter::dom::element item = list.child(i);
					item.set_attribute("class", L"macro-item");
				}
				el.set_attribute("class", L"macro-item selected");
			}
			return true;
		}
	}
	
	return false;
}

void MacroDialogSciter::fillMacroList() {
	OutputDebugStringW(L"MacroDialog: fillMacroList() called\n");
	
	// Get all macros from engine
	keys.clear();
	macroText.clear();
	macroContent.clear();
	getAllMacro(keys, macroText, macroContent);
	
	wchar_t debugMsg[256];
	swprintf_s(debugMsg, L"MacroDialog: Found %zu macros\n", macroText.size());
	OutputDebugStringW(debugMsg);
	
	// Call JS function to clear list (use call_function inherited from sciter::window)
	call_function("clearMacroList");
	
	// Add items via JS (in reverse order - newest first)
	for (size_t i = 0; i < macroText.size(); i++) {
		size_t idx = macroText.size() - 1 - i;
		// Convert UTF-8 to wide string for proper Vietnamese display in Sciter
		std::wstring wName = utf8ToWideString(macroText[idx]);
		std::wstring wContent = utf8ToWideString(macroContent[idx]);
		call_function("addMacroToList", wName.c_str(), wContent.c_str());
	}
	
	OutputDebugStringW(L"MacroDialog: fillMacroList() completed\n");
}

void MacroDialogSciter::saveAndReload() {
	OutputDebugStringW(L"MacroDialog: saveAndReload() called\n");
	
	// Save macros to registry
	std::vector<Byte> macroData;
	getMacroSaveData(macroData);
	OpenKeyHelper::setRegBinary(_T("macroData"), macroData.data(), (int)macroData.size());
	
	// Notify main process to reload macros from registry
	HWND mainWnd = FindWindow(_T("OpenKeyVietnameseInputMethod"), NULL);
	if (mainWnd) {
		PostMessage(mainWnd, WM_USER + 101, 0, 0);
		OutputDebugStringW(L"MacroDialog: Notified main process to reload macros\n");
	}
	
	// Reload list
	fillMacroList();
	
	// Reset button text via JS (use call_function inherited from sciter::window)
	call_function("updateAddButtonText");
	
	OutputDebugStringW(L"MacroDialog: saveAndReload() completed\n");
}

void MacroDialogSciter::onAddMacro(const std::wstring& name, const std::wstring& content) {
	addMacro(wideStringToUtf8(name), wideStringToUtf8(content));
	saveAndReload();
}

void MacroDialogSciter::onDeleteMacro(const std::wstring& name) {
	wchar_t debugMsg[512];
	swprintf_s(debugMsg, L"MacroDialog: onDeleteMacro name='%s'\n", name.c_str());
	OutputDebugStringW(debugMsg);
	
	std::string utf8Name = wideStringToUtf8(name);
	OutputDebugStringA("MacroDialog: UTF-8 name = '");
	OutputDebugStringA(utf8Name.c_str());
	OutputDebugStringA("'\n");
	
	if (deleteMacro(utf8Name)) {
		OutputDebugStringW(L"MacroDialog: deleteMacro succeeded\n");
		saveAndReload();
	} else {
		OutputDebugStringW(L"MacroDialog: deleteMacro FAILED\n");
	}
}

void MacroDialogSciter::onImportMacro() {
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH] = { 0 };
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = get_hwnd();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("Text file (*.txt)\0*.txt\0All (*.*)\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE) {
		// Use Unicode escape sequences for proper display
		// "Bạn có muốn giữ lại dữ liệu hiện tại không?"
		// "Dữ liệu gõ tắt"
		int msgboxID = MessageBoxW(
			get_hwnd(),
			L"B\u1EA1n c\u00F3 mu\u1ED1n gi\u1EEF l\u1EA1i d\u1EEF li\u1EC7u hi\u1EC7n t\u1EA1i kh\u00F4ng?",
			L"D\u1EEF li\u1EC7u g\u00F5 t\u1EAFt",
			MB_ICONEXCLAMATION | MB_YESNO
		);
		std::wstring path = ofn.lpstrFile;
		readFromFile(wideStringToUtf8(path), msgboxID == IDYES);
		saveAndReload();
	}
}

void MacroDialogSciter::onExportMacro() {
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH] = { 'O', 'p', 'e', 'n', 'K', 'e', 'y', 'M', 'a', 'c', 'r', 'o' };
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = get_hwnd();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("Text file (*.txt)\0*.txt\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrDefExt = (LPCWSTR)L"txt";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	
	if (GetSaveFileName(&ofn) == TRUE) {
		std::wstring path = ofn.lpstrFile;
		saveToFile(wideStringToUtf8(path));
	}
}
