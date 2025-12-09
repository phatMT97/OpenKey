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
#pragma once

// Undefine Windows macros that conflict with Sciter enums
#ifdef KEY_DOWN
#undef KEY_DOWN
#endif
#ifdef KEY_UP
#undef KEY_UP
#endif

#include "sciter-x-window.hpp"
#include <string>

class AboutDialog : public sciter::window {
public:
	AboutDialog();
	~AboutDialog();

	// Show the about dialog
	void show();

	// Override to avoid dependency on sciter::application
	HINSTANCE get_resource_instance() const { return NULL; }

	// Handle Sciter DOM events (button clicks, etc.)
	bool handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) override;

	// Native functions (called from handle_event, not JavaScript)
	void openUrl(std::string url);
	void checkUpdate();
	void closeWindow();
	void showUpdateDialog(std::string message, std::string newVersion);
	void showInfoMessage(std::string message);

private:
	// Set version information in the UI
	void setVersionInfo();
	
	// Enable Windows Acrylic blur effect
	void enableAcrylicEffect();
	
	// Subclass procedure for WM_NCHITTEST (window dragging)
	static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
};