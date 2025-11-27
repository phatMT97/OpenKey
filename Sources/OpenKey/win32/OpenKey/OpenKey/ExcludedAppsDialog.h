//English-only Apps Dialog Header
#pragma once
#include "BaseDialog.h"
#include <vector>
#include <string>

using namespace std;

class ExcludedAppsDialog : public BaseDialog {
private:
	HWND hListView;
	HWND hBtnAdd;
	HWND hBtnDelete;
	HWND hBtnClose;
	
	void initDialog();
	void refreshList();
	void onAddCurrentApp();
	void onDeleteSelected();
	
protected:
	INT_PTR eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
public:
	ExcludedAppsDialog(const HINSTANCE& hInstance, const int& resourceId);
	virtual ~ExcludedAppsDialog();
	virtual void fillData() override;
};
