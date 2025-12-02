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
#include "MainControlDialog.h"
#include "AppDelegate.h"
#include "OpenKeySettingsController.h"
#include <Shlobj.h>
#include <Uxtheme.h>

#pragma comment(lib, "UxTheme.lib")

static Uint16 _lastKeyCode;

MainControlDialog::MainControlDialog(const HINSTANCE& hInstance, const int& resourceId)
    : BaseDialog(hInstance, resourceId) {
}

MainControlDialog::~MainControlDialog() {
}

void MainControlDialog::initDialog() {
    HINSTANCE hIns = GetModuleHandleW(NULL);
    //dialog icon
    SET_DIALOG_ICON(IDI_APP_ICON);

    //set title version
    TCHAR title[256];
    TCHAR titleBuffer[256];
    LoadString(hIns, IDS_MAIN_DIALOG_TITLE, title, 256);
    wsprintfW(titleBuffer, title, OpenKeyHelper::getVersionString().c_str());
    SetWindowText(hDlg, titleBuffer);

    //input type
    comboBoxInputType = GetDlgItem(hDlg, IDC_COMBO_INPUT_TYPE);
    vector<LPCTSTR>& inputType = OpenKeyManager::getInputType();
    for (int i = 0; i < inputType.size(); i++) {
        SendMessage(comboBoxInputType, CB_ADDSTRING, i, reinterpret_cast<LPARAM>(inputType[i]));
    }
    createToolTip(comboBoxInputType, IDS_STRING_INPUT);

    //code table
    comboBoxTableCode = GetDlgItem(hDlg, IDC_COMBO_TABLE_CODE);
    vector<LPCTSTR>& tableCode = OpenKeyManager::getTableCode();
    for (int i = 0; i < tableCode.size(); i++) {
        SendMessage(comboBoxTableCode, CB_ADDSTRING, i, reinterpret_cast<LPARAM>(tableCode[i]));
    }
    createToolTip(comboBoxTableCode, IDS_STRING_CODE);

    //init tabview
    hTab = GetDlgItem(hDlg, IDC_TAB_CONTROL);
    TCITEM tci = { 0 };
    tci.mask = TCIF_TEXT;
    tci.pszText = (LPWSTR)_T("Bộ gõ");
    TabCtrl_InsertItem(hTab, 0, &tci);
    tci.pszText = (LPWSTR)_T("Gõ tắt");
    TabCtrl_InsertItem(hTab, 1, &tci);
    tci.pszText = (LPWSTR)_T("Hệ thống");
    TabCtrl_InsertItem(hTab, 2, &tci);
    tci.pszText = (LPWSTR)_T("Thông tin");
    TabCtrl_InsertItem(hTab, 3, &tci);
    RECT r;
    TabCtrl_GetItemRect(hTab, 0, &r);
    TabCtrl_SetItemSize(hTab, r.right - r.left, (r.bottom - r.top) * 1.428f);

    //create tab page
    hTabPage1 = CreateDialogParam(hIns, MAKEINTRESOURCE(IDD_DIALOG_TAB_GENERAL), hDlg, tabPageEventProc, (LPARAM)this);
    hTabPage2 = CreateDialogParam(hIns, MAKEINTRESOURCE(IDD_DIALOG_TAB_MACRO), hDlg, tabPageEventProc, (LPARAM)this);
    hTabPage3 = CreateDialogParam(hIns, MAKEINTRESOURCE(IDD_DIALOG_TAB_SYSTEM), hDlg, tabPageEventProc, (LPARAM)this);
    hTabPage4 = CreateDialogParam(hIns, MAKEINTRESOURCE(IDD_DIALOG_TAB_INFO), hDlg, tabPageEventProc, (LPARAM)this);
    RECT rc;//find tab control's rectangle
    GetWindowRect(hTab, &rc);
    POINT offset = { 0 };
    ScreenToClient(hDlg, &offset);
    OffsetRect(&rc, offset.x, offset.y); //convert to client coordinates
    rc.top += (LONG)((r.bottom - r.top) * 1.428f);
    SetWindowPos(hTabPage1, 0, rc.left + 1, rc.top + 3, rc.right - rc.left - 5, rc.bottom - rc.top - 5, SWP_HIDEWINDOW);
    SetWindowPos(hTabPage2, 0, rc.left + 1, rc.top + 3, rc.right - rc.left - 5, rc.bottom - rc.top - 6, SWP_HIDEWINDOW);
    SetWindowPos(hTabPage3, 0, rc.left + 1, rc.top + 3, rc.right - rc.left - 5, rc.bottom - rc.top - 6, SWP_HIDEWINDOW);
    SetWindowPos(hTabPage4, 0, rc.left + 1, rc.top + 3, rc.right - rc.left - 5, rc.bottom - rc.top - 6, SWP_HIDEWINDOW);
    onTabIndexChanged();

    checkCtrl = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_CTRL);
    createToolTip(checkCtrl, IDS_STRING_CTRL);

    checkAlt = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_ALT);
    createToolTip(checkAlt, IDS_STRING_ALT);

    checkWin = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_WIN);
    createToolTip(checkWin, IDS_STRING_WIN);

    checkShift = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_SHIFT);
    createToolTip(checkShift, IDS_STRING_SHIFT);

    textSwitchKey = GetDlgItem(hDlg, IDC_SWITCH_KEY_KEY);
    createToolTip(textSwitchKey, IDS_STRING_SWITCH_KEY);

    checkBeep = GetDlgItem(hDlg, IDC_CHECK_SWITCH_KEY_BEEP);
    createToolTip(checkBeep, IDS_STRING_BEEP);

    checkVietnamese = GetDlgItem(hDlg, IDC_RADIO_METHOD_VIETNAMESE);
    createToolTip(checkVietnamese, IDS_STRING_VIET);

    checkEnglish = GetDlgItem(hDlg, IDC_RADIO_METHOD_ENGLISH);
    createToolTip(checkEnglish, IDS_STRING_ENG);

    /*--------end common---------*/

    checkModernOrthorgraphy = GetDlgItem(hTabPage1, IDC_CHECK_MODERN_ORTHORGRAPHY);
    createToolTip(checkModernOrthorgraphy, IDS_STRING_MORDEN_ORTHORGRAPHY);

    checkFixRecommendBrowser = GetDlgItem(hTabPage1, IDC_CHECK_FIX_RECOMMEND_BROWSER);
    createToolTip(checkFixRecommendBrowser, IDS_STRING_FIX_BROWSER);

    checkSpelling = GetDlgItem(hTabPage1, IDC_CHECK_SPELLING);
    createToolTip(checkSpelling, IDS_STRING_SPELLING_CHECK);

    checkRestoreIfWrongSpelling = GetDlgItem(hTabPage1, IDC_CHECK_RESTORE_IF_WRONG_SPELLING);
    createToolTip(checkRestoreIfWrongSpelling, IDS_STRING_RESTORE_IF_WRONG_SPELLING);

    checkAllowZWJF = GetDlgItem(hTabPage1, IDC_CHECK_ALLOW_ZJWF);
    createToolTip(checkAllowZWJF, IDS_STRING_ALLOW_ZWFJ);

    checkTempOffSpelling = GetDlgItem(hTabPage1, IDC_CHECK_TEMP_OFF_SPELLING);
    createToolTip(checkTempOffSpelling, IDS_STRING_TEMP_OFF_SPELLING);

    checkSmartSwitchKey = GetDlgItem(hTabPage1, IDC_CHECK_SMART_SWITCH_KEY);
    createToolTip(checkSmartSwitchKey, IDS_STRING_SMART_SWITCH_KEY);

    checkCapsFirstChar = GetDlgItem(hTabPage1, IDC_CHECK_CAPS_FIRST_CHAR);
    createToolTip(checkCapsFirstChar, IDS_STRING_CAPS_FIRST_CHAR);

    checkRememberTableCode = GetDlgItem(hTabPage1, IDC_CHECK_SMART_SWITCH_CODE);
    createToolTip(checkRememberTableCode, IDS_STRING_REMEMBER_TABLE_CODE);

    checkAllowOtherLanguages = GetDlgItem(hTabPage1, IDC_CHECK_OTHER_LANGUAGES);
    createToolTip(checkAllowOtherLanguages, IDS_STRING_OTHER_LANGUAGES);

    checkTempOffOpenKey = GetDlgItem(hTabPage1, IDC_CHECK_TEMP_OFF_OPEN_KEY);
    createToolTip(checkTempOffOpenKey, IDS_STRING_TEMP_OFF_OPENKEY);
    
    checkExcludeApps = GetDlgItem(hTabPage1, IDC_CHECK_EXCLUDE_APPS);
    createToolTip(checkExcludeApps, IDS_STRING_EXCLUDE_APPS);
    
    hBtnManageExcludedApps = GetDlgItem(hTabPage1, IDC_BUTTON_MANAGE_EXCLUDED_APPS);
    createToolTip(hBtnManageExcludedApps, IDS_STRING_EXCLUDE_APPS);

    /*------------end tab 1----------------*/

    checkQuickStartConsonant = GetDlgItem(hTabPage2, IDC_CHECK_QUICK_START_CONSONANT);
    createToolTip(checkQuickStartConsonant, IDS_STRING_START_CONSONANT);

    checkQuickEndConsonant = GetDlgItem(hTabPage2, IDC_CHECK_QUICK_END_CONSONANT);
    createToolTip(checkQuickEndConsonant, IDS_STRING_END_CONSONANT);

    checkQuickTelex = GetDlgItem(hTabPage2, IDC_CHECK_QUICK_TELEX);
    createToolTip(checkQuickTelex, IDS_STRING_QUICK_TELEX);

    checkUseMacro = GetDlgItem(hTabPage2, IDC_CHECK_USE_MACRO);
    createToolTip(checkUseMacro, IDS_STRING_MACRO);

    checkUseMacroInEnglish = GetDlgItem(hTabPage2, IDC_CHECK_USE_MACRO_IN_ENGLISH);
    createToolTip(checkUseMacroInEnglish, IDS_STRING_MACRO_IN_ENG);

    checkMacroAutoCaps = GetDlgItem(hTabPage2, IDC_CHECK_AUTO_CAPS);
    createToolTip(checkMacroAutoCaps, IDS_STRING_MACRO_AUTO_CAP);

    hUpdateButton = GetDlgItem(hDlg, IDC_BUTTON_CHECK_UPDATE);

    /*------------end tab 2----------------*/

    checkModernIcon = GetDlgItem(hTabPage3, IDC_CHECK_MODERN_ICON);
    createToolTip(checkModernIcon, IDS_STRING_MODERN_ICON);

    checkShowOnStartup = GetDlgItem(hTabPage3, IDC_CHECK_SHOW_ON_STARTUP);
    createToolTip(checkShowOnStartup, IDS_STRING_SHOW_ON_STARTUP);

    checkRunWithWindows = GetDlgItem(hTabPage3, IDC_CHECK_RUN_WITH_WINDOWS);
    createToolTip(checkRunWithWindows, IDS_STRING_RUN_ON_STARTUP);

    checkCreateDesktopShortcut = GetDlgItem(hTabPage3, IDC_CHECK_CHECK_CREATE_SHORTCUT);
    createToolTip(checkCreateDesktopShortcut, IDS_STRING_CREATE_DESKTOP);

    checkRunAsAdmin = GetDlgItem(hTabPage3, IDC_CHECK_RUN_AS_ADMIN);
    createToolTip(checkRunAsAdmin, IDS_STRING_RUN_AS_ADMIN);

    checkCheckNewVersion = GetDlgItem(hTabPage3, IDC_CHECK_CHECK_UPDATE);
    createToolTip(checkCheckNewVersion, IDS_STRING_CHECK_UPDATE);

    checkSupportMetroApp = GetDlgItem(hTabPage3, IDC_CHECK_SUPPORT_METRO_APP);
    createToolTip(checkSupportMetroApp, IDS_STRING_SUPPORT_METRO);

    checkUseClipboard = GetDlgItem(hTabPage3, IDC_CHECK_USE_CLIPBOARD);
    createToolTip(checkUseClipboard, IDS_STRING_USE_CLIPBOARD);

    checkFixChromium = GetDlgItem(hTabPage3, IDC_CHECK_FIX_CHROMIUM);
    createToolTip(checkFixChromium, IDS_STRING_FIX_CHROMIUM);

    /*------------end tab 3----------------*/

    SendDlgItemMessage(hDlg, IDBUTTON_OK, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(hIns, MAKEINTRESOURCEW(IDI_ICON_OK_BUTTON)));
    SendDlgItemMessage(hDlg, ID_BTN_DEFAULT, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(hIns, MAKEINTRESOURCEW(IDI_ICON_DEFAULT_BUTTON)));
    SendDlgItemMessage(hDlg, IDBUTTON_EXIT, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(hIns, MAKEINTRESOURCEW(IDI_ICON_EXIT_BUTTON)));
    fillData();
}

INT_PTR MainControlDialog::eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        this->hDlg = hDlg;
        initDialog();
        return TRUE;
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDBUTTON_OK:
            AppDelegate::getInstance()->closeDialog(this);
            break;
        case IDBUTTON_EXIT:
            AppDelegate::getInstance()->onOpenKeyExit();
            break;
        case ID_BTN_DEFAULT: {
            int msgboxID = MessageBox(
                hDlg,
                _T("Bạn có chắc chắn muốn thiết lập lại cài đặt gốc?"),
                _T("OpenKey"),
                MB_ICONEXCLAMATION | MB_YESNO
            );
            if (msgboxID == IDYES) {
                AppDelegate::getInstance()->onDefaultConfig();
            }
            break;
        }
        case IDC_BUTTON_MACRO_TABLE:
            AppDelegate::getInstance()->onMacroTable();
            break;
        case IDC_BUTTON_CHECK_UPDATE:
            onUpdateButton();
            break;
        case IDC_BUTTON_GO_SOURCE_CODE:
            ShellExecute(NULL, _T("open"), _T("https://github.com/tuyenvm/OpenKey"), NULL, NULL, SW_SHOWNORMAL);
            break;
        case IDC_BUTTON_MANAGE_EXCLUDED_APPS:
            AppDelegate::getInstance()->onManageExcludedApps();
            break;
        default:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                this->onComboBoxSelected((HWND)lParam, LOWORD(wParam));
            }
            else if (HIWORD(wParam) == BN_CLICKED) {
                this->onCheckboxClicked((HWND)lParam);
            }
            else if (HIWORD(wParam) == EN_CHANGE) {
                _lastKeyCode = OpenKeyManager::_lastKeyCode;
                if (_lastKeyCode > 0) {
                    OpenKeyManager::_lastKeyCode = 0;
                    this->onCharacter((HWND)lParam, _lastKeyCode);
                }
            }
            break;
        }
        break;
    }
    case WM_NOTIFY: {
        switch (((LPNMHDR)lParam)->code) {
        case TCN_SELCHANGE:
            onTabIndexChanged();
            break;
        case NM_CLICK:
        case NM_RETURN: {
            PNMLINK link = (PNMLINK)lParam;
            if (link->hdr.idFrom == IDC_SYSLINK_HOME_PAGE)
                ShellExecute(NULL, _T("open"), _T("http://open-key.org"), NULL, NULL, SW_SHOWNORMAL);
            else if (link->hdr.idFrom == IDC_SYSLINK_FANPAGE)
                ShellExecute(NULL, _T("open"), _T("https://www.facebook.com/OpenKeyVN"), NULL, NULL, SW_SHOWNORMAL);
            else if (link->hdr.idFrom == IDC_SYSLINK_AUTHOR_EMAIL)
                ShellExecute(NULL, _T("open"), _T("mailto:maivutuyen.91@gmail.com"), NULL, NULL, SW_SHOWNORMAL);
            break;
        }
        }
        break;
    }
    }

    return FALSE;
}

INT_PTR MainControlDialog::tabPageEventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_INITDIALOG) {
#ifdef _WIN64
        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
#else
        SetWindowLong(hDlg, GWL_USERDATA, lParam);
#endif
        return TRUE;
    }
    else if (uMsg == WM_ERASEBKGND) {
        return TRUE;
    }
    else if ((uMsg == WM_CTLCOLORSTATIC || uMsg == WM_CTLCOLORBTN) && IsThemeActive()) {
        SetBkMode((HDC)wParam, TRANSPARENT);
        return (LRESULT)GetStockObject(COLOR_WINDOW + 1);
    }
    else if (uMsg == WM_PAINT && IsThemeActive()) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hDlg, &ps);

        // All painting occurs here, between BeginPaint and EndPaint.
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hDlg, &ps);

        return 0;
    }

#ifdef _WIN64
    LONG_PTR attr = GetWindowLongPtr(hDlg, GWLP_USERDATA);
#else
    long attr = GetWindowLong(hDlg, GWL_USERDATA);
#endif
    if (attr != 0) {
        return ((MainControlDialog*)attr)->eventProc(hDlg, uMsg, wParam, lParam);
    }
    return FALSE;
}

void MainControlDialog::fillData() {
    auto& controller = OpenKeySettingsController::getInstance();
    
    // Comboboxes
    SendMessage(comboBoxInputType, CB_SETCURSEL, controller.getInputType(), 0);
    SendMessage(comboBoxTableCode, CB_SETCURSEL, controller.getCodeTable(), 0);

    // Switch Key Status (bit-packed value)
    int switchKeyStatus = controller.getSwitchKeyStatus();
    SendMessage(checkCtrl, BM_SETCHECK, HAS_CONTROL(switchKeyStatus) ? 1 : 0, 0);
    SendMessage(checkAlt, BM_SETCHECK, HAS_OPTION(switchKeyStatus) ? 1 : 0, 0);
    SendMessage(checkWin, BM_SETCHECK, HAS_COMMAND(switchKeyStatus) ? 1 : 0, 0);
    SendMessage(checkShift, BM_SETCHECK, HAS_SHIFT(switchKeyStatus) ? 1 : 0, 0);
    setSwitchKeyText(textSwitchKey, (switchKeyStatus >> 24) & 0xFF);
    SendMessage(checkBeep, BM_SETCHECK, HAS_BEEP(switchKeyStatus) ? 1 : 0, 0);

    // Language Selection
    int lang = controller.getLanguage();
    SendMessage(checkVietnamese, BM_SETCHECK, lang, 0);
    SendMessage(checkEnglish, BM_SETCHECK, !lang, 0);

    // Core Settings
    SendMessage(checkModernOrthorgraphy, BM_SETCHECK, controller.getUseModernOrthography() ? 1 : 0, 0);
    SendMessage(checkFixRecommendBrowser, BM_SETCHECK, controller.getFixRecommendBrowser() ? 1 : 0, 0);
    SendMessage(checkShowOnStartup, BM_SETCHECK, controller.getShowOnStartUp() ? 1 : 0, 0);
    SendMessage(checkRunWithWindows, BM_SETCHECK, controller.getRunWithWindows() ? 1 : 0, 0);
    SendMessage(checkSpelling, BM_SETCHECK, controller.getCheckSpelling() ? 1 : 0, 0);
    SendMessage(checkRestoreIfWrongSpelling, BM_SETCHECK, controller.getRestoreIfWrongSpelling() ? 1 : 0, 0);
    SendMessage(checkModernIcon, BM_SETCHECK, controller.getUseGrayIcon() ? 1 : 0, 0);
    SendMessage(checkAllowZWJF, BM_SETCHECK, controller.getAllowConsonantZFWJ() ? 1 : 0, 0);
    SendMessage(checkTempOffSpelling, BM_SETCHECK, controller.getTempOffSpelling() ? 1 : 0, 0);
    SendMessage(checkQuickStartConsonant, BM_SETCHECK, controller.getQuickStartConsonant() ? 1 : 0, 0);
    SendMessage(checkQuickEndConsonant, BM_SETCHECK, controller.getQuickEndConsonant() ? 1 : 0, 0);
    SendMessage(checkRememberTableCode, BM_SETCHECK, controller.getRememberCode() ? 1 : 0, 0);
    SendMessage(checkAllowOtherLanguages, BM_SETCHECK, controller.getOtherLanguage() ? 1 : 0, 0);
    SendMessage(checkTempOffOpenKey, BM_SETCHECK, controller.getTempOffOpenKey() ? 1 : 0, 0);

    // Smart Features
    SendMessage(checkSmartSwitchKey, BM_SETCHECK, controller.getUseSmartSwitchKey() ? 1 : 0, 0);
    SendMessage(checkCapsFirstChar, BM_SETCHECK, controller.getUpperCaseFirstChar() ? 1 : 0, 0);
    SendMessage(checkQuickTelex, BM_SETCHECK, controller.getQuickTelex() ? 1 : 0, 0);
    SendMessage(checkUseMacro, BM_SETCHECK, controller.getUseMacro() ? 1 : 0, 0);
    SendMessage(checkUseMacroInEnglish, BM_SETCHECK, controller.getUseMacroInEnglishMode() ? 1 : 0, 0);

    // System & Advanced
    SendMessage(checkMacroAutoCaps, BM_SETCHECK, controller.getAutoCapsMacro() ? 1 : 0, 0);
    SendMessage(checkSupportMetroApp, BM_SETCHECK, controller.getSupportMetroApp() ? 1 : 0, 0);
    SendMessage(checkCreateDesktopShortcut, BM_SETCHECK, controller.getCreateDesktopShortcut() ? 1 : 0, 0);
    SendMessage(checkRunAsAdmin, BM_SETCHECK, controller.getRunAsAdmin() ? 1 : 0, 0);
    SendMessage(checkCheckNewVersion, BM_SETCHECK, controller.getCheckNewVersion() ? 1 : 0, 0);
    SendMessage(checkUseClipboard, BM_SETCHECK, controller.getSendKeyStepByStep() ? 0 : 1, 0); // Inverted logic
    SendMessage(checkFixChromium, BM_SETCHECK, controller.getFixChromiumBrowser() ? 1 : 0, 0);
    SendMessage(checkExcludeApps, BM_SETCHECK, controller.getExcludeApps() ? 1 : 0, 0);

    // UI Dependencies (Enable/Disable controls based on settings)
    bool spellingEnabled = controller.getCheckSpelling();
    EnableWindow(checkRestoreIfWrongSpelling, spellingEnabled);
    EnableWindow(checkAllowZWJF, spellingEnabled);
    EnableWindow(checkTempOffSpelling, spellingEnabled);
    
    bool browserFixEnabled = controller.getFixRecommendBrowser();
    EnableWindow(checkFixChromium, browserFixEnabled);

    // Tab Info (UI-only, no controller needed)
    wchar_t buffer[256];
    wsprintfW(buffer, _T("Phiên bản %s cho Windows - Ngày cập nhật: %s"), OpenKeyHelper::getVersionString().c_str(), _T(__DATE__));
    SendDlgItemMessage(hTabPage4, IDC_STATIC_APP_VERSION_INFO, WM_SETTEXT, 0, LPARAM(buffer));
}

void MainControlDialog::setSwitchKey(const unsigned short& code) {
    int status = OpenKeySettingsController::getInstance().getSwitchKeyStatus();
    status &= 0xFFFFFF00;
    status |= code;
    status &= 0x00FFFFFF;
    status |= ((unsigned int)code << 24);
    OpenKeySettingsController::getInstance().setSwitchKeyStatus(status);
}

void MainControlDialog::onComboBoxSelected(const HWND& hCombobox, const int& comboboxId) {
    if (hCombobox == comboBoxInputType) {
        OpenKeySettingsController::getInstance().setInputType((int)SendMessage(hCombobox, CB_GETCURSEL, 0, 0));
    }
    else if (hCombobox == comboBoxTableCode) {
        OpenKeySettingsController::getInstance().setCodeTable((int)SendMessage(hCombobox, CB_GETCURSEL, 0, 0));
        // Smart Switch Key logic is now handled inside setCodeTable()
    }
    // SystemTrayHelper::updateData() is called inside controller setters
}

void MainControlDialog::onCheckboxClicked(const HWND& hWnd) {
    int val = 0;
    auto& controller = OpenKeySettingsController::getInstance();
    
    // Switch Key Modifiers (bit manipulation handled by controller)
    if (hWnd == checkCtrl) {
        val = (int)SendMessage(checkCtrl, BM_GETCHECK, 0, 0);
        int status = controller.getSwitchKeyStatus();
        status &= (~0x100);
        status |= val << 8;
        controller.setSwitchKeyStatus(status);
    }
    else if (hWnd == checkAlt) {
        val = (int)SendMessage(checkAlt, BM_GETCHECK, 0, 0);
        int status = controller.getSwitchKeyStatus();
        status &= (~0x200);
        status |= val << 9;
        controller.setSwitchKeyStatus(status);
    }
    else if (hWnd == checkWin) {
        val = (int)SendMessage(checkWin, BM_GETCHECK, 0, 0);
        int status = controller.getSwitchKeyStatus();
        status &= (~0x400);
        status |= val << 10;
        controller.setSwitchKeyStatus(status);
    }
    else if (hWnd == checkShift) {
        val = (int)SendMessage(checkShift, BM_GETCHECK, 0, 0);
        int status = controller.getSwitchKeyStatus();
        status &= (~0x800);
        status |= val << 11;
        controller.setSwitchKeyStatus(status);
    }
    else if (hWnd == checkBeep) {
        val = (int)SendMessage(checkBeep, BM_GETCHECK, 0, 0);
        int status = controller.getSwitchKeyStatus();
        status &= (~0x8000);
        status |= val << 15;
        controller.setSwitchKeyStatus(status);
    }
    // Language Selection (Smart Switch Key logic is in controller)
    else if (hWnd == checkVietnamese) {
        val = (int)SendMessage(checkVietnamese, BM_GETCHECK, 0, 0);
        controller.setLanguage(val ? 1 : 0);
        // Smart Switch Key logic is now inside setLanguage()
    }
    else if (hWnd == checkEnglish) {
        val = (int)SendMessage(checkVietnamese, BM_GETCHECK, 0, 0);
        controller.setLanguage(val ? 1 : 0);
        // Smart Switch Key logic is now inside setLanguage()
    }
    // Core Settings
    else if (hWnd == checkModernOrthorgraphy) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setUseModernOrthography(val ? true : false);
    }
    else if (hWnd == checkFixRecommendBrowser) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        bool enabled = val ? true : false;
        controller.setFixRecommendBrowser(enabled);
        EnableWindow(checkFixChromium, enabled); // UI dependency remains here
    }
    else if (hWnd == checkShowOnStartup) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setShowOnStartUp(val ? true : false);
    }
    // System Operations
    else if (hWnd == checkRunWithWindows) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setRunWithWindows(val ? true : false);
        // registerRunOnStartup() is now inside setRunWithWindows()
    }
    else if (hWnd == checkSpelling) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        bool enabled = val ? true : false;
        controller.setCheckSpelling(enabled);
        // vSetCheckSpelling() is now inside setCheckSpelling()
        // UI dependencies remain here
        EnableWindow(checkRestoreIfWrongSpelling, enabled);
        EnableWindow(checkAllowZWJF, enabled);
        EnableWindow(checkTempOffSpelling, enabled);
    }
    else if (hWnd == checkRestoreIfWrongSpelling) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setRestoreIfWrongSpelling(val ? true : false);
    }
    else if (hWnd == checkUseClipboard) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setSendKeyStepByStep(val ? false : true); // Inverted logic
    }
    // Smart Features
    else if (hWnd == checkSmartSwitchKey) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setUseSmartSwitchKey(val ? true : false);
    }
    else if (hWnd == checkCapsFirstChar) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setUpperCaseFirstChar(val ? true : false);
    }
    else if (hWnd == checkQuickTelex) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setQuickTelex(val ? true : false);
    }
    // Macro Settings
    else if (hWnd == checkUseMacro) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setUseMacro(val ? true : false);
    }
    else if (hWnd == checkUseMacroInEnglish) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setUseMacroInEnglishMode(val ? true : false);
    }
    else if (hWnd == checkModernIcon) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setUseGrayIcon(val ? true : false);
    }
    // Spelling Features
    else if (hWnd == checkAllowZWJF) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setAllowConsonantZFWJ(val ? true : false);
    }
    else if (hWnd == checkTempOffSpelling) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setTempOffSpelling(val ? true : false);
    }
    // Quick Features
    else if (hWnd == checkQuickStartConsonant) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setQuickStartConsonant(val ? true : false);
    }
    else if (hWnd == checkQuickEndConsonant) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setQuickEndConsonant(val ? true : false);
    }
    else if (hWnd == checkSupportMetroApp) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setSupportMetroApp(val ? true : false);
    }
    else if (hWnd == checkMacroAutoCaps) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setAutoCapsMacro(val ? true : false);
    }
    else if (hWnd == checkCreateDesktopShortcut) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setCreateDesktopShortcut(val ? true : false);
        // createDesktopShortcut() is now inside setter
    }
    // Admin & System
    else if (hWnd == checkRunAsAdmin) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        bool needsRestart = controller.setRunAsAdmin(val ? true : false);
        // UI-specific prompt remains here
        if (needsRestart) {
            int msgboxID = MessageBox(hDlg,
                _T("Bạn cần phải khởi động lại OpenKey để kích hoạt chế độ Admin!\\nBạn có muốn khởi động lại OpenKey không?"),
                _T("OpenKey"),
                MB_ICONEXCLAMATION | MB_YESNO);
            if (msgboxID == IDYES) {
                PostQuitMessage(0);
                ShellExecute(0, L"runas", OpenKeyHelper::getFullPath().c_str(), 0, 0, SW_SHOWNORMAL);
            }
        }
    }
    else if (hWnd == checkCheckNewVersion) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setCheckNewVersion(val ? true : false);
    }
    else if (hWnd == checkRememberTableCode) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setRememberCode(val ? true : false);
    }
    else if (hWnd == checkAllowOtherLanguages) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setOtherLanguage(val ? true : false);
    }
    else if (hWnd == checkTempOffOpenKey) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setTempOffOpenKey(val ? true : false);
    }
    else if (hWnd == checkFixChromium) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setFixChromiumBrowser(val ? true : false);
    }
    else if (hWnd == checkExcludeApps) {
        val = (int)SendMessage(hWnd, BM_GETCHECK, 0, 0);
        controller.setExcludeApps(val ? true : false);
    }
    // SystemTrayHelper::updateData() is called inside controller setters
}

void MainControlDialog::onCharacter(const HWND& hWnd, const UINT16& keyCode) {
    if (keyCode == 0) return;
    if (hWnd == textSwitchKey) {
        UINT16 code = GET_SWITCH_KEY(vSwitchKeyStatus);
        if (keyCode == VK_DELETE || keyCode == VK_BACK) {
            code = 0xFE;
        }
        else if (keyCodeToCharacter(keyCode) != 0) {
            code = keyCode;
        }
        setSwitchKey(code);
        setSwitchKeyText(hWnd, code);
    }
}

void MainControlDialog::setSwitchKeyText(const HWND& hWnd, const UINT16& keyCode) {
    if (keyCode == KEY_SPACE) {
        SetWindowText(hWnd, _T("Space"));
    }
    else if (keyCode == 0xFE) {
        SetWindowText(hWnd, _T(""));
    }
    else {
        Uint16 key[] = { keyCode, 0 };
        SetWindowText(hWnd, (LPCWSTR)&key);
    }
}

void MainControlDialog::onTabIndexChanged() {
    int index = TabCtrl_GetCurSel(hTab);
    ShowWindow(hTabPage1, (index == 0) ? SW_SHOW : SW_HIDE);
    ShowWindow(hTabPage2, (index == 1) ? SW_SHOW : SW_HIDE);
    ShowWindow(hTabPage3, (index == 2) ? SW_SHOW : SW_HIDE);
    ShowWindow(hTabPage4, (index == 3) ? SW_SHOW : SW_HIDE);
}

void MainControlDialog::onUpdateButton() {
    EnableWindow(hUpdateButton, false);
    string newVersion;
    if (OpenKeyManager::checkUpdate(newVersion)) {
        WCHAR msg[256];
        wsprintf(msg,
            TEXT("OpenKey Có phiên bản mới (%s), bạn có muốn cập nhật không?"),
            utf8ToWideString(newVersion).c_str());

        int msgboxID = MessageBox(
            hDlg,
            msg,
            _T("OpenKey Update"),
            MB_ICONEXCLAMATION | MB_YESNO
        );
        if (msgboxID == IDYES) {
            //Call OpenKeyUpdate
            WCHAR path[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, path);
            wsprintf(path, TEXT("%s\\OpenKeyUpdate.exe"), path);
            ShellExecute(0, L"", path, 0, 0, SW_SHOWNORMAL);

            AppDelegate::getInstance()->onOpenKeyExit();
        }

    }
    else {
        MessageBox(hDlg, _T("Bạn đang dùng phiên bản mới nhất!"), _T("OpenKey Update"), MB_OK);
    }
    EnableWindow(hUpdateButton, true);
}


