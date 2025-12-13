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
#include "stdafx.h"
#include "MainControlDialog.h"
#include "AboutDialog.h"
#include "ConvertToolDialog.h"
#include "MacroDialog.h"
#include <vector>

class BaseDialog;
class AboutDialog; // Forward declaration for Sciter window

class AppDelegate {
private:
	HINSTANCE hInstance;
	BaseDialog* mainDialog = NULL, *macroDialog = NULL, *convertDialog = NULL, *excludedAppsDialog = NULL;
	AboutDialog* aboutDialog = NULL; // Sciter window - managed separately
	std::vector<HANDLE> m_childProcesses;  // Track subprocess handles for cleanup
private:
	bool isDialogMsg(MSG & msg) const;
	void checkUpdate();
public:
	AppDelegate();
	static AppDelegate* getInstance();
	int run(HINSTANCE hInstance);
	void createMainDialog();
	void closeDialog(BaseDialog* dialog);
	void closeAboutDialog(); // Separate method for AboutDialog
	
	// Subprocess management
	void trackChildProcess(HANDLE hProcess);
	void terminateAllChildren();
	
public: //event
	void onInputMethodChangedFromHotKey();
	void onDefaultConfig();

	void onToggleVietnamese();
	void onToggleCheckSpelling();
	void onToggleUseSmartSwitchKey();
	void onToggleUseMacro();

	void onMacroTable();
	void onConvertTool();
	void onQuickConvert();
	void onManageExcludedApps();
	void onSpawnExcludedAppsSciter();  // Spawn excluded apps Sciter subprocess (called via IPC)

	void onInputType(const int& type);
	void onTableCode(const int& code);

	void onControlPanel();
	void onOpenKeyAbout();
	void onOpenKeyExit();
};

// Global configuration variables
extern int vLanguage;
extern int vInputType;
extern int vFreeMark;
extern int vCodeTable;
extern int vCheckSpelling;
extern int vUseModernOrthography;
extern int vQuickTelex;
extern int vSwitchKeyStatus;
extern int vRestoreIfWrongSpelling;
extern int vFixRecommendBrowser;
extern int vUseMacro;
extern int vUseMacroInEnglishMode;
extern int vAutoCapsMacro;
extern int vSendKeyStepByStep;
extern int vUseSmartSwitchKey;
extern int vUpperCaseFirstChar;
extern int vTempOffSpelling;
extern int vAllowConsonantZFWJ;
extern int vQuickStartConsonant;
extern int vQuickEndConsonant;
extern int vOtherLanguage;
extern int vRememberCode;
extern int vTempOffOpenKey;
extern int vUseGrayIcon;
extern int vShowOnStartUp;
extern int vRunWithWindows;
extern int vSupportMetroApp;
extern int vCreateDesktopShortcut;
extern int vRunAsAdmin;
extern int vCheckNewVersion;
extern int vFixChromiumBrowser;
extern int vExcludeApps;