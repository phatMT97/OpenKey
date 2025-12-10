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
#include "sciter-x.h"
#include "sciter-x-window.hpp"
#include <string>
#include <vector>

class MacroDialogSciter : public sciter::window {
public:
	MacroDialogSciter();
	virtual ~MacroDialogSciter();

	// Event handler
	virtual bool handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) override;
	
	// Show the dialog
	void show();
	
	// Window subclass procedure for drag and close
	static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

private:
	// Data
	std::vector<std::vector<unsigned int>> keys;
	std::vector<std::string> macroText;
	std::vector<std::string> macroContent;
	
	// UI helpers
	void fillMacroList();
	void saveAndReload();
	
	// Actions
	void onAddMacro(const std::wstring& name, const std::wstring& content);
	void onDeleteMacro(const std::wstring& name);
	void onImportMacro();
	void onExportMacro();
	
	// DWM blur effect
	void enableAcrylicEffect();
};
