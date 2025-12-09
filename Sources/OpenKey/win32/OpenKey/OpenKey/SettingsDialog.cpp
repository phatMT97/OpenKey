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
#include "SettingsDialog.h"
#include "OpenKeyHelper.h"
#include "../../../engine/Engine.h"
#include <shellapi.h>
#include <dwmapi.h>
#include <windowsx.h>
#include <commctrl.h>
#include "sciter-x-dom.hpp"
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

#define TIMER_RESIZE_WINDOW 1001

SettingsDialog::SettingsDialog()
	: sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{0, 0, 350, 380}) {
	
	// Load settings from registry FIRST (subprocess starts fresh)
	APP_GET_DATA(vLanguage, 1);          // Default: Vietnamese
	APP_GET_DATA(vInputType, 0);         // Default: Telex
	APP_GET_DATA(vCodeTable, 0);         // Default: Unicode
	APP_GET_DATA(vSwitchKeyStatus, 0);   // Default: no modifier keys
	APP_GET_DATA(vUseSmartSwitchKey, 0); // Default: disabled
	
	// Load HTML file
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	WCHAR* lastSlash = wcsrchr(exePath, L'\\');
	if (lastSlash) *lastSlash = L'\0';
	
	WCHAR htmlPath[MAX_PATH];
	swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\settings\\settings.html", exePath);
	
	if (!load(htmlPath)) {
		MessageBoxW(NULL, htmlPath, L"Failed to load settings.html", MB_OK | MB_ICONERROR);
		return;
	}

	// Show the window
	expand();
	
	// Subclass window for dragging and close
	SetWindowSubclass(get_hwnd(), SettingsDialog::SubclassProc, 1, (DWORD_PTR)this);
	
	// Set window title for anti-spam detection
	SetWindowTextW(get_hwnd(), L"OpenKey Settings");
	
	// Auto-fit window to content size from HTML
	sciter::dom::element rootEl = this->root();
	sciter::dom::element container = rootEl.find_first(".container");
	if (container) {
		// get_location returns RECT, takes ELEMENT_AREAS flag
		RECT contentRect = container.get_location(CONTENT_BOX);
		int contentWidth = contentRect.right - contentRect.left;
		int contentHeight = contentRect.bottom - contentRect.top;
		
		// Add small padding for window chrome
		contentWidth = max(contentWidth, 350);
		contentHeight = max(contentHeight, 200);
		
		// Resize window to fit content
		SetWindowPos(get_hwnd(), NULL, 0, 0, contentWidth, contentHeight, SWP_NOMOVE | SWP_NOZORDER);
	}
	
	// Center window on screen
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	RECT rc;
	GetWindowRect(get_hwnd(), &rc);
	int winWidth = rc.right - rc.left;
	int winHeight = rc.bottom - rc.top;
	int x = (screenWidth - winWidth) / 2;
	int y = (screenHeight - winHeight) / 2;
	SetWindowPos(get_hwnd(), HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE);  // Remove topmost
	
	// Enable Acrylic blur effect (after resize)
	enableAcrylicEffect();
	
	// Load settings from registry
	loadSettings();
}

SettingsDialog::~SettingsDialog() {
	if (get_hwnd()) {
		RemoveWindowSubclass(get_hwnd(), SettingsDialog::SubclassProc, 1);
	}
}

// --- DWM Acrylic Effect Implementation ---

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

void SettingsDialog::enableAcrylicEffect() {
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

LRESULT CALLBACK SettingsDialog::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	if (msg == WM_CLOSE) {
		ExitProcess(0);  // Force exit subprocess
		return 0;
	}
	
	if (msg == WM_NCHITTEST) {
		LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
		if (result == HTCLIENT) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ScreenToClient(hwnd, &pt);
			
			// Drag zone: only very top edge (10px) to allow clicks on header elements
			// User can still drag by clicking the very top edge
			if (pt.y < 10) {
				return HTCAPTION;
			}
		}
		return result;
	}
	
	// Handle notification from main process to update UI (bidirectional sync)
	if (msg == WM_USER + 102) {
		OutputDebugStringW(L"OpenKey: Received WM_USER+102 - updating UI from registry\n");
		
		// Reload ALL settings from registry
		APP_GET_DATA(vLanguage, 1);
		APP_GET_DATA(vInputType, 0);
		APP_GET_DATA(vCodeTable, 0);
		APP_GET_DATA(vSwitchKeyStatus, 0);
		APP_GET_DATA(vUseSmartSwitchKey, 0);
		APP_GET_DATA(vCheckSpelling, 1);
		APP_GET_DATA(vUseMacro, 0);
		
		// Get the SettingsDialog instance from dwRefData
		SettingsDialog* dialog = reinterpret_cast<SettingsDialog*>(dwRefData);
		if (dialog) {
			// Update UI by refreshing all elements
			sciter::dom::element root = dialog->root();
			
			// Update language toggle (vLanguage=0 means English/checked)
			sciter::dom::element toggleLang = root.find_first("#toggle-language");
			if (toggleLang) {
				if (vLanguage == 0) { // English mode
					toggleLang.set_attribute("class", L"toggle-switch checked");
				} else { // Vietnamese mode
					toggleLang.set_attribute("class", L"toggle-switch");
				}
			}
			
			// Update dropdowns
			sciter::dom::element inputType = root.find_first("#input-type");
			sciter::dom::element bangMa = root.find_first("#bang-ma");
			if (inputType) inputType.set_value(sciter::value(vInputType));
			if (bangMa) bangMa.set_value(sciter::value(vCodeTable));
			
			// Update key toggles
			sciter::dom::element keyCtrl = root.find_first("#key-ctrl");
			sciter::dom::element keyAlt = root.find_first("#key-alt");
			sciter::dom::element keyWin = root.find_first("#key-win");
			sciter::dom::element keyShift = root.find_first("#key-shift");
			
			if (keyCtrl) {
				if (vSwitchKeyStatus & 0x100) keyCtrl.set_attribute("class", L"toggle-switch-small checked");
				else keyCtrl.set_attribute("class", L"toggle-switch-small");
			}
			if (keyAlt) {
				if (vSwitchKeyStatus & 0x200) keyAlt.set_attribute("class", L"toggle-switch-small checked");
				else keyAlt.set_attribute("class", L"toggle-switch-small");
			}
			if (keyWin) {
				if (vSwitchKeyStatus & 0x400) keyWin.set_attribute("class", L"toggle-switch-small checked");
				else keyWin.set_attribute("class", L"toggle-switch-small");
			}
			if (keyShift) {
				if (vSwitchKeyStatus & 0x800) keyShift.set_attribute("class", L"toggle-switch-small checked");
				else keyShift.set_attribute("class", L"toggle-switch-small");
			}
			
			// Update beep toggle
			sciter::dom::element beep = root.find_first("#beep-sound");
			if (beep) {
				if (vSwitchKeyStatus & 0x8000) beep.set_attribute("class", L"toggle-switch-small checked");
				else beep.set_attribute("class", L"toggle-switch-small");
			}
			
			// Update smart switch toggle
			sciter::dom::element smartSwitch = root.find_first("#smart-switch");
			if (smartSwitch) {
				if (vUseSmartSwitchKey) smartSwitch.set_attribute("class", L"toggle-switch-small checked");
				else smartSwitch.set_attribute("class", L"toggle-switch-small");
			}
		}
		
		return 0;
	}
	
	// Handle timer for window resize after animation
	if (msg == WM_TIMER && wParam == TIMER_RESIZE_WINDOW) {
		KillTimer(hwnd, TIMER_RESIZE_WINDOW);
		SettingsDialog* dialog = reinterpret_cast<SettingsDialog*>(dwRefData);
		if (dialog) {
			dialog->recalcWindowSize();
		}
		return 0;
	}
	
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void SettingsDialog::show() {
	ShowWindow(get_hwnd(), SW_SHOW);
	SetForegroundWindow(get_hwnd());
}

void SettingsDialog::loadSettings() {
	// Note: Settings are loaded directly in constructor via handle_event
	// JavaScript handles initial values via HTML
}

void SettingsDialog::saveSettings() {
	// Settings are saved immediately when changed via handle_event
}

// Notify main process to reload settings from registry
static void notifyMainProcess() {
	HWND mainWnd = FindWindow(_T("OpenKeyVietnameseInputMethod"), NULL);
	if (mainWnd) {
		PostMessage(mainWnd, WM_USER + 101, 0, 0);
	}
}

bool SettingsDialog::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
	// Debug: Log all events with target info for click events
	sciter::dom::element targetEl(params.heTarget);
	std::wstring targetId = targetEl.get_attribute("id");
	std::wstring targetClass = targetEl.get_attribute("class");
	std::string targetTag = targetEl.get_tag();
	
	wchar_t debugMsg[512];
	swprintf_s(debugMsg, L"OpenKey: event cmd=%d target=[%S#%s.%s]\n", 
		params.cmd, 
		targetTag.c_str(),
		targetId.c_str(),
		targetClass.c_str());
	OutputDebugStringW(debugMsg);
	
	if (sciter::window::handle_event(he, params))
		return true;
	
	// Handle DOCUMENT_READY to set initial values from registry
	if (params.cmd == DOCUMENT_READY) {
		OutputDebugStringW(L"OpenKey: DOCUMENT_READY event\n");
		sciter::dom::element root = this->root();
		
		// Set dropdown values
		sciter::dom::element inputType = root.find_first("#input-type");
		sciter::dom::element bangMa = root.find_first("#bang-ma");
		
		if (inputType) inputType.set_value(sciter::value(vInputType));
		if (bangMa) bangMa.set_value(sciter::value(vCodeTable));
		
		// Set toggle states by adding/removing 'checked' class (div-based toggles)
		// Language toggle: vLanguage=0 means English (checked), vLanguage=1 means Vietnamese (unchecked)
		sciter::dom::element toggleLang = root.find_first("#toggle-language");
		if (toggleLang) {
			if (vLanguage == 0) { // English mode
				toggleLang.set_attribute("class", L"toggle-switch checked");
			} else { // Vietnamese mode
				toggleLang.set_attribute("class", L"toggle-switch");
			}
		}
		
		// Switch key toggles
		sciter::dom::element keyCtrl = root.find_first("#key-ctrl");
		sciter::dom::element keyAlt = root.find_first("#key-alt");
		sciter::dom::element keyWin = root.find_first("#key-win");
		sciter::dom::element keyShift = root.find_first("#key-shift");
		
		if (keyCtrl) {
			if (vSwitchKeyStatus & 0x100) keyCtrl.set_attribute("class", L"toggle-switch-small checked");
			else keyCtrl.set_attribute("class", L"toggle-switch-small");
		}
		if (keyAlt) {
			if (vSwitchKeyStatus & 0x200) keyAlt.set_attribute("class", L"toggle-switch-small checked");
			else keyAlt.set_attribute("class", L"toggle-switch-small");
		}
		if (keyWin) {
			if (vSwitchKeyStatus & 0x400) keyWin.set_attribute("class", L"toggle-switch-small checked");
			else keyWin.set_attribute("class", L"toggle-switch-small");
		}
		if (keyShift) {
			if (vSwitchKeyStatus & 0x800) keyShift.set_attribute("class", L"toggle-switch-small checked");
			else keyShift.set_attribute("class", L"toggle-switch-small");
		}
		
		// Custom key input - get character from high byte
		sciter::dom::element keyChar = root.find_first("#switch-key-char");
		if (keyChar) {
			int charCode = (vSwitchKeyStatus >> 24) & 0xFF;
			if (charCode > 0) {
				std::wstring charStr(1, (wchar_t)charCode);
				keyChar.set_value(sciter::value(charStr));
			}
		}
		
		// Beep toggle
		sciter::dom::element beep = root.find_first("#beep-sound");
		if (beep) {
			if (vSwitchKeyStatus & 0x8000) beep.set_attribute("class", L"toggle-switch-small checked");
			else beep.set_attribute("class", L"toggle-switch-small");
		}
		
		// Smart switch toggle
		sciter::dom::element smartSwitch = root.find_first("#smart-switch");
		if (smartSwitch) {
			if (vUseSmartSwitchKey) smartSwitch.set_attribute("class", L"toggle-switch-small checked");
			else smartSwitch.set_attribute("class", L"toggle-switch-small");
		}
		
		return true;
	}
	
	// Handle button clicks
	if (params.cmd == BUTTON_CLICK) {
		sciter::dom::element el(params.heTarget);
		std::wstring id = el.get_attribute("id");
		std::wstring className = el.get_attribute("class");
		std::string tagName = el.get_tag();
		
		// DEBUG: Log ALL button clicks to diagnose issue
		wchar_t debugMsg[512];
		swprintf_s(debugMsg, L"OpenKey: BUTTON_CLICK - id='%s' class='%s' tag='%S'\n", 
			id.empty() ? L"(empty)" : id.c_str(), 
			className.empty() ? L"(empty)" : className.c_str(),
			tagName.empty() ? "(empty)" : tagName.c_str());
		OutputDebugStringW(debugMsg);
		
		if (id == L"btn-close") {
			// Close the window
			PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
			return true;
		}
		
		// Handle advanced settings button - toggle expansion
		if (id == L"btn-advanced") {
			OutputDebugStringW(L"OpenKey: btn-advanced MATCHED!\n");
			
			// Toggle expanded class on container via JavaScript
			sciter::dom::element root = this->root();
			sciter::dom::element container = root.find_first("#main-container");
			if (container) {
				std::wstring currentClass = container.get_attribute("class");
				if (currentClass.find(L"expanded") != std::wstring::npos) {
					// Remove expanded
					container.set_attribute("class", L"container");
					m_isExpanded = false;
					OutputDebugStringW(L"OpenKey: Collapsing panel\n");
				} else {
					// Add expanded
					container.set_attribute("class", L"container expanded");
					m_isExpanded = true;
					OutputDebugStringW(L"OpenKey: Expanding panel\n");
				}
				// Resize window after CSS applies
				SetTimer(get_hwnd(), TIMER_RESIZE_WINDOW, 50, NULL);
			}
			return true;
		}
	}
	// Handle VALUE_CHANGED for dropdowns AND checkboxes
	else if (params.cmd == VALUE_CHANGED) {
		sciter::dom::element el(params.heTarget);
		std::wstring id = el.get_attribute("id");
		
		OutputDebugStringW((L"OpenKey: VALUE_CHANGED id=" + id + L"\n").c_str());
		
		// Handle dropdowns by ID
		if (id == L"input-type") {
			sciter::value val = el.get_value();
			int value = 0;
			if (val.is_int()) value = val.get<int>();
			else if (val.is_string()) value = _wtoi(val.get<std::wstring>().c_str());
			onInputTypeChange(value);
			return true;
		}
		else if (id == L"bang-ma") {
			sciter::value val = el.get_value();
			int value = 0;
			if (val.is_int()) value = val.get<int>();
			else if (val.is_string()) value = _wtoi(val.get<std::wstring>().c_str());
			onCodeTableChange(value);
			return true;
		}
		else if (id == L"switch-key-char") {
			// Handle custom switch key character input
			sciter::value val = el.get_value();
			if (val.is_string()) {
				std::wstring str = val.get<std::wstring>();
				if (str.length() > 0) {
					int charCode = (int)str[0];
					// Store the character in the high byte (bits 24-31)
					vSwitchKeyStatus &= 0x00FFFFFF;  // Clear high byte
					vSwitchKeyStatus |= ((unsigned int)charCode << 24);
					APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
					notifyMainProcess();
				}
			}
			return true;
		}
		// Handle hidden inputs (toggle values with val- prefix)
		else if (id == L"val-toggle-language") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			OutputDebugStringW(checked ? L"OpenKey: val-toggle-language = CHECKED\n" : L"OpenKey: val-toggle-language = UNCHECKED\n");
			vLanguage = checked ? 0 : 1;  // checked = English (0)
			APP_SET_DATA(vLanguage, vLanguage);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-key-ctrl") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			if (checked) vSwitchKeyStatus |= 0x100;
			else vSwitchKeyStatus &= ~0x100;
			APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-key-alt") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			if (checked) vSwitchKeyStatus |= 0x200;
			else vSwitchKeyStatus &= ~0x200;
			APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-key-win") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			if (checked) vSwitchKeyStatus |= 0x400;
			else vSwitchKeyStatus &= ~0x400;
			APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-key-shift") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			if (checked) vSwitchKeyStatus |= 0x800;
			else vSwitchKeyStatus &= ~0x800;
			APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-beep-sound") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			if (checked) vSwitchKeyStatus |= 0x8000;
			else vSwitchKeyStatus &= ~0x8000;
			APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-smart-switch") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			vUseSmartSwitchKey = checked ? 1 : 0;
			APP_SET_DATA(vUseSmartSwitchKey, vUseSmartSwitchKey);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-expand-state") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool expanded = (strVal == L"1");
			OutputDebugStringW(expanded ? L"OpenKey: val-expand-state = EXPANDED\n" : L"OpenKey: val-expand-state = COLLAPSED\n");
			m_isExpanded = expanded;
			// Set timer to resize window after CSS transition (300ms + 50ms buffer)
			SetTimer(get_hwnd(), TIMER_RESIZE_WINDOW, 350, NULL);
			return true;
		}
	}
	
	// Handle HYPERLINK_CLICK events for toggle switches AND clickable divs
	// Note: JavaScript already toggles the 'checked' class, we just need to detect the change
	if (params.cmd == HYPERLINK_CLICK || params.cmd == BUTTON_CLICK) {
		sciter::dom::element el(params.heTarget);
		std::wstring elId = el.get_attribute("id");
		std::wstring className = el.get_attribute("class");
		
		// Check for btn-advanced clickable row (or its children)
		if (elId == L"btn-advanced" || className.find(L"setting-row-clickable") != std::wstring::npos) {
			OutputDebugStringW(L"OpenKey: btn-advanced clicked - toggling via hidden input\n");
			
			// Find hidden input and toggle its value - JS will handle UI changes via CSS
			sciter::dom::element root = this->root();
			sciter::dom::element checkbox = root.find_first("#expand-checkbox");
			
			if (checkbox) {
				// Toggle checked state
				bool isChecked = checkbox.get_state(STATE_CHECKED) != 0;
				checkbox.set_state(isChecked ? 0 : STATE_CHECKED, STATE_CHECKED);
				m_isExpanded = !isChecked;
				
				OutputDebugStringW(m_isExpanded ? L"OpenKey: Checkbox now CHECKED (expand)\n" : L"OpenKey: Checkbox now UNCHECKED (collapse)\n");
				
				// Resize window after CSS transition
				SetTimer(get_hwnd(), TIMER_RESIZE_WINDOW, 100, NULL);
			} else {
				OutputDebugStringW(L"OpenKey: ERROR - expand-checkbox not found!\n");
			}
			return true;
		}
		
		// Check if this is a toggle element (has toggle-switch or toggle-switch-small class)
		bool isToggle = (!className.empty() && (
			className.find(L"toggle-switch") != std::wstring::npos ||
			className.find(L"toggle-thumb") != std::wstring::npos
		));
		
		if (isToggle) {
			// Find the actual toggle container (might have clicked on thumb)
			sciter::dom::element toggleEl = el;
			if (className.find(L"thumb") != std::wstring::npos) {
				toggleEl = el.parent();  // Get parent toggle-switch element
			}
			
			std::wstring id = toggleEl.get_attribute("id");
			if (id.empty()) return false;
			
			// After JS click handler runs, the class will be toggled
			// We need to check the CURRENT state (after toggle)
			std::wstring toggleClass = toggleEl.get_attribute("class");
			bool isChecked = (toggleClass.find(L"checked") != std::wstring::npos);
			
			// Handle each toggle
			if (id == L"toggle-language") {
				vLanguage = isChecked ? 0 : 1;  // checked = English
				APP_SET_DATA(vLanguage, vLanguage);
				notifyMainProcess();
				return true;
			}
			else if (id == L"key-ctrl") {
				if (isChecked) vSwitchKeyStatus |= 0x100;
				else vSwitchKeyStatus &= ~0x100;
				APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
				notifyMainProcess();
				return true;
			}
			else if (id == L"key-alt") {
				if (isChecked) vSwitchKeyStatus |= 0x200;
				else vSwitchKeyStatus &= ~0x200;
				APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
				notifyMainProcess();
				return true;
			}
			else if (id == L"key-win") {
				if (isChecked) vSwitchKeyStatus |= 0x400;
				else vSwitchKeyStatus &= ~0x400;
				APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
				notifyMainProcess();
				return true;
			}
			else if (id == L"key-shift") {
				if (isChecked) vSwitchKeyStatus |= 0x800;
				else vSwitchKeyStatus &= ~0x800;
				APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
				notifyMainProcess();
				return true;
			}
			else if (id == L"beep-sound") {
				if (isChecked) vSwitchKeyStatus |= 0x8000;
				else vSwitchKeyStatus &= ~0x8000;
				APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
				notifyMainProcess();
				return true;
			}
			else if (id == L"smart-switch") {
				vUseSmartSwitchKey = isChecked ? 1 : 0;
				APP_SET_DATA(vUseSmartSwitchKey, vUseSmartSwitchKey);
				notifyMainProcess();
				return true;
			}
		}
	}
	
	return false;
}

void SettingsDialog::onLanguageToggle(bool isEnglish) {
	OutputDebugStringW(L"OpenKey: SOM onLanguageToggle called!\n");
	vLanguage = isEnglish ? 0 : 1;
	APP_SET_DATA(vLanguage, vLanguage);
	notifyMainProcess();
}

void SettingsDialog::onInputTypeChange(int value) {
	wchar_t msg[128];
	swprintf_s(msg, L"OpenKey: SOM onInputTypeChange called, value=%d\n", value);
	OutputDebugStringW(msg);
	vInputType = value;
	APP_SET_DATA(vInputType, vInputType);
	notifyMainProcess();
}

void SettingsDialog::onCodeTableChange(int value) {
	vCodeTable = value;
	APP_SET_DATA(vCodeTable, vCodeTable);
	notifyMainProcess();
}

void SettingsDialog::onSwitchKeyCtrlChange(bool enabled) {
	if (enabled) {
		vSwitchKeyStatus |= 0x100;
	} else {
		vSwitchKeyStatus &= ~0x100;
	}
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	notifyMainProcess();
}

void SettingsDialog::onSwitchKeyAltChange(bool enabled) {
	if (enabled) {
		vSwitchKeyStatus |= 0x200;
	} else {
		vSwitchKeyStatus &= ~0x200;
	}
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	notifyMainProcess();
}

void SettingsDialog::onSwitchKeyWinChange(bool enabled) {
	if (enabled) {
		vSwitchKeyStatus |= 0x400;
	} else {
		vSwitchKeyStatus &= ~0x400;
	}
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	notifyMainProcess();
}

void SettingsDialog::onSwitchKeyShiftChange(bool enabled) {
	if (enabled) {
		vSwitchKeyStatus |= 0x800;
	} else {
		vSwitchKeyStatus &= ~0x800;
	}
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	notifyMainProcess();
}

void SettingsDialog::onSwitchKeyCharChange(int charCode) {
	// Store the character in the high byte (bits 24-31)
	vSwitchKeyStatus &= 0x00FFFFFF;  // Clear high byte
	vSwitchKeyStatus |= ((unsigned int)charCode << 24);
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	notifyMainProcess();
}

void SettingsDialog::onBeepChange(bool enabled) {
	if (enabled) {
		vSwitchKeyStatus |= 0x8000;
	} else {
		vSwitchKeyStatus &= ~0x8000;
	}
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	notifyMainProcess();
}

void SettingsDialog::onSmartSwitchChange(bool enabled) {
	vUseSmartSwitchKey = enabled ? 1 : 0;
	APP_SET_DATA(vUseSmartSwitchKey, vUseSmartSwitchKey);
	notifyMainProcess();
}

void SettingsDialog::onOpenAdvancedSettings() {
	openAdvancedSettings();
}

void SettingsDialog::openAdvancedSettings() {
	// Toggle expansion state - handled by JavaScript
	// Just call the JS function if we need to programmatically toggle
	OutputDebugStringW(L"OpenKey: openAdvancedSettings called\n");
}

void SettingsDialog::onExpandChange(bool isExpanded) {
	wchar_t msg[128];
	swprintf_s(msg, L"OpenKey: onExpandChange called, isExpanded=%d\n", isExpanded);
	OutputDebugStringW(msg);
	
	m_isExpanded = isExpanded;
	
	// Set timer to resize window after CSS transition (300ms + 50ms buffer)
	SetTimer(get_hwnd(), TIMER_RESIZE_WINDOW, 350, NULL);
}

void SettingsDialog::recalcWindowSize() {
	OutputDebugStringW(L"OpenKey: recalcWindowSize called\n");
	
	// Get current window position
	RECT rc;
	GetWindowRect(get_hwnd(), &rc);
	int x = rc.left;
	int y = rc.top;
	
	// Auto-fit: measure container size from HTML/CSS (per sciter-integration-guide.md)
	sciter::dom::element rootEl = this->root();
	sciter::dom::element container = rootEl.find_first(".container");
	int newWidth = 350;
	int newHeight = 200;
	
	if (container) {
		RECT contentRect = container.get_location(CONTENT_BOX);
		newWidth = max(contentRect.right - contentRect.left, 350);
		newHeight = max(contentRect.bottom - contentRect.top, 200);
	}
	
	// Resize window
	SetWindowPos(get_hwnd(), NULL, x, y, newWidth, newHeight, SWP_NOZORDER);
	
	wchar_t sizeMsg[128];
	swprintf_s(sizeMsg, L"OpenKey: Window resized to %dx%d (auto-fit)\n", newWidth, newHeight);
	OutputDebugStringW(sizeMsg);
}

