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
#include "AppDelegate.h"
#include "AboutDialog.h"
#include "SettingsDialog.h"
#include <Shlobj.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
						_In_opt_ HINSTANCE hPrevInstance,
						_In_ LPWSTR    lpCmdLine,
						_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	
	// ===== ROUTER: Single Exe - Multi Personality =====
	// Check if we're running as About UI subprocess
	if (lpCmdLine && wcsstr(lpCmdLine, L"--about")) {
		// UI PROCESS MODE: Show About dialog only, then exit
		// This process is isolated - 30MB RAM, freed on close
		AboutDialog dialog;
		
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return 0;  // Clean exit, subprocess terminated
	}
	
	// Check if we're running as Settings UI subprocess
	if (lpCmdLine && wcsstr(lpCmdLine, L"--settings")) {
		SettingsDialog dialog;
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return 0;
	}
	
	// ===== ENGINE PROCESS MODE: Normal app =====
	UNREFERENCED_PARAMETER(lpCmdLine);
	
#if NDEBUG
	//check the program is run as administrator mode
	APP_GET_DATA(vRunAsAdmin, 0);
	if (vRunAsAdmin && !IsUserAnAdmin()) {
		//create admin process
		ShellExecute(0, L"runas", OpenKeyHelper::getFullPath().c_str(), 0, 0, SW_SHOWNORMAL);
		return 1;
	}
#endif
	AppDelegate app;
	return app.run(hInstance);
}