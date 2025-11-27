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
#include "ExcludedAppsDialog.h"
#include "AppDelegate.h"
#include <CommCtrl.h>

extern void saveEnglishOnlyAppsData();

ExcludedAppsDialog::ExcludedAppsDialog(const HINSTANCE& hInstance, const int& resourceId)
	: BaseDialog(hInstance, resourceId) {
}

ExcludedAppsDialog::~ExcludedAppsDialog() {
}

void ExcludedAppsDialog::initDialog() {
	HINSTANCE hIns = GetModuleHandle(NULL);
	SET_DIALOG_ICON(IDI_APP_ICON);
	
	// Get dialog controls
	hListView = GetDlgItem(hDlg, IDC_LIST_EXCLUDED_APPS);
	hEditAppName = GetDlgItem(hDlg, IDC_EDIT_APP_NAME);
	hBtnAddManual = GetDlgItem(hDlg, IDC_BUTTON_ADD_MANUAL);
	hBtnAdd = GetDlgItem(hDlg, IDC_BUTTON_ADD_CURRENT_APP);
	hBtnDelete = GetDlgItem(hDlg, IDC_BUTTON_DELETE_APP);
	hBtnClose = GetDlgItem(hDlg, IDOK);
	
	// Initialize ListView with Unicode support
	LVCOLUMNW lvc = {};
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 340;
	lvc.pszText = (LPWSTR)L"Ứng dụng";
	ListView_InsertColumn(hListView, 0, &lvc);
	
	// Enable full row select
	ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	
	fillData();
}

void ExcludedAppsDialog::fillData() {
	refreshList();
}

void ExcludedAppsDialog::refreshList() {
	// Clear all items
	ListView_DeleteAllItems(hListView);
	
	vector<string> apps;
	getAllEnglishOnlyApps(apps);
	
	// Insert items using Unicode
	for (size_t i = 0; i < apps.size(); i++) {
		LVITEMW lvi = {};
		lvi.mask = LVIF_TEXT;
		lvi.iItem = (int)i;
		lvi.iSubItem = 0;
		
		// Convert UTF-8 to wide string
		wstring wAppName = utf8ToWideString(apps[i]);
		lvi.pszText = (LPWSTR)wAppName.c_str();
		
		ListView_InsertItem(hListView, &lvi);
	}
}

void ExcludedAppsDialog::onAddManual() {
	WCHAR appName[256] = {};
	GetWindowTextW(hEditAppName, appName, 256);
	
	if (lstrlenW(appName) == 0) {
		MessageBoxW(hDlg, L"Vui lòng nhập tên ứng dụng!", L"OpenKey", MB_OK | MB_ICONWARNING);
		return;
	}
	
	// Convert to UTF-8
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, appName, -1, NULL, 0, NULL, NULL);
	string strAppName(size_needed - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, appName, -1, &strAppName[0], size_needed, NULL, NULL);
	
	if (isEnglishOnlyApp(strAppName)) {
		MessageBoxW(hDlg, L"Ứng dụng này đã có trong danh sách!", L"OpenKey", MB_OK | MB_ICONINFORMATION);
		return;
	}
	
	addEnglishOnlyApp(strAppName);
	saveEnglishOnlyAppsData();
	
	// Clear textbox and refresh
	SetWindowTextW(hEditAppName, L"");
	refreshList();
}

void ExcludedAppsDialog::onAddCurrentApp() {
	string& currentApp = OpenKeyHelper::getFrontMostAppExecuteName();
	
	if (currentApp.compare("OpenKey64.exe") == 0 || currentApp.compare("OpenKey32.exe") == 0) {
		MessageBoxW(hDlg, L"Không thể thêm OpenKey vào danh sách loại trừ!", L"OpenKey", MB_OK | MB_ICONWARNING);
		return;
	}
	
	if (isEnglishOnlyApp(currentApp)) {
		MessageBoxW(hDlg, L"Ứng dụng này đã có trong danh sách!", L"OpenKey", MB_OK | MB_ICONINFORMATION);
		return;
	}
	
	addEnglishOnlyApp(currentApp);
	saveEnglishOnlyAppsData();
	refreshList();
}

void ExcludedAppsDialog::onDeleteSelected() {
	int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
	
	if (selectedIndex < 0) {
		MessageBoxW(hDlg, L"Vui lòng chọn một ứng dụng để xóa!", L"OpenKey", MB_OK | MB_ICONINFORMATION);
		return;
	}
	
	WCHAR appName[256] = {};
	ListView_GetItemText(hListView, selectedIndex, 0, appName, 256);
	
	// Convert to UTF-8
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, appName, -1, NULL, 0, NULL, NULL);
	string strAppName(size_needed - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, appName, -1, &strAppName[0], size_needed, NULL, NULL);
	
	removeEnglishOnlyApp(strAppName);
	saveEnglishOnlyAppsData();
	refreshList();
}

INT_PTR ExcludedAppsDialog::eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		this->hDlg = hDlg;
		initDialog();
		return TRUE;
		
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_ADD_MANUAL:
			onAddManual();
			break;
		case IDC_BUTTON_ADD_CURRENT_APP:
			onAddCurrentApp();
			break;
		case IDC_BUTTON_DELETE_APP:
			onDeleteSelected();
			break;
		case IDOK:
		case IDCANCEL:
			AppDelegate::getInstance()->closeDialog(this);
			break;
		}
		break;
	}
	return FALSE;
}
