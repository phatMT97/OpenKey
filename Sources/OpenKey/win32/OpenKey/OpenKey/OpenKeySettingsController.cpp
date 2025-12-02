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
#include "OpenKeySettingsController.h"
#include "AppDelegate.h"
#include "OpenKeyHelper.h"
#include "OpenKeyManager.h"
#include "../../../engine/SmartSwitchKey.h"

// External references to global variables
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

// External engine functions
extern void vSetCheckSpelling();
extern void saveSmartSwitchKeyData();

// ========== SINGLETON IMPLEMENTATION ==========

OpenKeySettingsController& OpenKeySettingsController::getInstance() {
    static OpenKeySettingsController instance;
    return instance;
}

OpenKeySettingsController::OpenKeySettingsController() {
    // Private constructor - no initialization needed
    // All settings are loaded via loadConfiguration()
}

OpenKeySettingsController::~OpenKeySettingsController() {
    // Cleanup if needed
}

// ========== INTERNAL HELPERS ==========

void OpenKeySettingsController::updateSmartSwitchKeyData() {
    // This method handles the "Smart Switch Key" feature logic
    // Originally scattered across multiple event handlers in MainControlDialog.cpp
    
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    if (vUseSmartSwitchKey || vRememberCode) {
        string& currentApp = OpenKeyHelper::getFrontMostAppExecuteName();
        
        // Build status value: language (bit 0) + code table (bits 1+)
        int status = vLanguage | (vCodeTable << 1);
        
        setAppInputMethodStatus(currentApp, status);
        saveSmartSwitchKeyData();
    }
}

// ========== CORE INPUT SETTINGS ==========

void OpenKeySettingsController::setLanguage(int langCode) {
    vLanguage = langCode;
    OpenKeyHelper::setRegInt(_T("vLanguage"), vLanguage);
    
    // Smart Switch Key: remember language per-app
    updateSmartSwitchKeyData();
    
    // Update system tray icon/menu
    SystemTrayHelper::updateData();
}

int OpenKeySettingsController::getLanguage() const {
    return vLanguage;
}

void OpenKeySettingsController::setInputType(int inputType) {
    vInputType = inputType;
    OpenKeyHelper::setRegInt(_T("vInputType"), vInputType);
    SystemTrayHelper::updateData();
}

int OpenKeySettingsController::getInputType() const {
    return vInputType;
}

void OpenKeySettingsController::setCodeTable(int tableCode) {
    vCodeTable = tableCode;
    OpenKeyHelper::setRegInt(_T("vCodeTable"), vCodeTable);
    
    // Smart Switch Key: remember code table per-app
    updateSmartSwitchKeyData();
    
    SystemTrayHelper::updateData();
}

int OpenKeySettingsController::getCodeTable() const {
    return vCodeTable;
}

void OpenKeySettingsController::setSwitchKeyStatus(int status) {
    vSwitchKeyStatus = status;
    OpenKeyHelper::setRegInt(_T("vSwitchKeyStatus"), vSwitchKeyStatus);
}

int OpenKeySettingsController::getSwitchKeyStatus() const {
    return vSwitchKeyStatus;
}

// ========== SPELLING & ENGINE SETTINGS ==========

void OpenKeySettingsController::setCheckSpelling(bool enable) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    vCheckSpelling = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vCheckSpelling"), vCheckSpelling);
    
    // CRITICAL: Trigger engine re-initialization
    vSetCheckSpelling();
}

bool OpenKeySettingsController::getCheckSpelling() const {
    return vCheckSpelling != 0;
}

void OpenKeySettingsController::setUseModernOrthography(bool enable) {
    vUseModernOrthography = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vUseModernOrthography"), vUseModernOrthography);
}

bool OpenKeySettingsController::getUseModernOrthography() const {
    return vUseModernOrthography != 0;
}

void OpenKeySettingsController::setRestoreIfWrongSpelling(bool enable) {
    vRestoreIfWrongSpelling = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vRestoreIfWrongSpelling"), vRestoreIfWrongSpelling);
}

bool OpenKeySettingsController::getRestoreIfWrongSpelling() const {
    return vRestoreIfWrongSpelling != 0;
}

void OpenKeySettingsController::setAllowConsonantZFWJ(bool enable) {
    vAllowConsonantZFWJ = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vAllowConsonantZFWJ"), vAllowConsonantZFWJ);
}

bool OpenKeySettingsController::getAllowConsonantZFWJ() const {
    return vAllowConsonantZFWJ != 0;
}

void OpenKeySettingsController::setTempOffSpelling(bool enable) {
    vTempOffSpelling = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vTempOffSpelling"), vTempOffSpelling);
}

bool OpenKeySettingsController::getTempOffSpelling() const {
    return vTempOffSpelling != 0;
}

// ========== QUICK TYPING FEATURES ==========

void OpenKeySettingsController::setQuickTelex(bool enable) {
    vQuickTelex = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vQuickTelex"), vQuickTelex);
}

bool OpenKeySettingsController::getQuickTelex() const {
    return vQuickTelex != 0;
}

void OpenKeySettingsController::setQuickStartConsonant(bool enable) {
    vQuickStartConsonant = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vQuickStartConsonant"), vQuickStartConsonant);
}

bool OpenKeySettingsController::getQuickStartConsonant() const {
    return vQuickStartConsonant != 0;
}

void OpenKeySettingsController::setQuickEndConsonant(bool enable) {
    vQuickEndConsonant = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vQuickEndConsonant"), vQuickEndConsonant);
}

bool OpenKeySettingsController::getQuickEndConsonant() const {
    return vQuickEndConsonant != 0;
}

// ========== MACRO SETTINGS ==========

void OpenKeySettingsController::setUseMacro(bool enable) {
    vUseMacro = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vUseMacro"), vUseMacro);
}

bool OpenKeySettingsController::getUseMacro() const {
    return vUseMacro != 0;
}

void OpenKeySettingsController::setUseMacroInEnglishMode(bool enable) {
    vUseMacroInEnglishMode = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vUseMacroInEnglishMode"), vUseMacroInEnglishMode);
}

bool OpenKeySettingsController::getUseMacroInEnglishMode() const {
    return vUseMacroInEnglishMode != 0;
}

void OpenKeySettingsController::setAutoCapsMacro(bool enable) {
    vAutoCapsMacro = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vAutoCapsMacro"), vAutoCapsMacro);
}

bool OpenKeySettingsController::getAutoCapsMacro() const {
    return vAutoCapsMacro != 0;
}

// ========== SMART FEATURES ==========

void OpenKeySettingsController::setUseSmartSwitchKey(bool enable) {
    vUseSmartSwitchKey = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vUseSmartSwitchKey"), vUseSmartSwitchKey);
}

bool OpenKeySettingsController::getUseSmartSwitchKey() const {
    return vUseSmartSwitchKey != 0;
}

void OpenKeySettingsController::setRememberCode(bool enable) {
    vRememberCode = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vRememberCode"), vRememberCode);
}

bool OpenKeySettingsController::getRememberCode() const {
    return vRememberCode != 0;
}

void OpenKeySettingsController::setUpperCaseFirstChar(bool enable) {
    vUpperCaseFirstChar = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vUpperCaseFirstChar"), vUpperCaseFirstChar);
}

bool OpenKeySettingsController::getUpperCaseFirstChar() const {
    return vUpperCaseFirstChar != 0;
}

void OpenKeySettingsController::setOtherLanguage(bool enable) {
    vOtherLanguage = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vOtherLanguage"), vOtherLanguage);
}

bool OpenKeySettingsController::getOtherLanguage() const {
    return vOtherLanguage != 0;
}

void OpenKeySettingsController::setTempOffOpenKey(bool enable) {
    vTempOffOpenKey = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vTempOffOpenKey"), vTempOffOpenKey);
}

bool OpenKeySettingsController::getTempOffOpenKey() const {
    return vTempOffOpenKey != 0;
}

// ========== BROWSER FIX SETTINGS ==========

void OpenKeySettingsController::setFixRecommendBrowser(bool enable) {
    vFixRecommendBrowser = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vFixRecommendBrowser"), vFixRecommendBrowser);
}

bool OpenKeySettingsController::getFixRecommendBrowser() const {
    return vFixRecommendBrowser != 0;
}

void OpenKeySettingsController::setFixChromiumBrowser(bool enable) {
    vFixChromiumBrowser = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vFixChromiumBrowser"), vFixChromiumBrowser);
}

bool OpenKeySettingsController::getFixChromiumBrowser() const {
    return vFixChromiumBrowser != 0;
}

// ========== SYSTEM INTEGRATION ==========

void OpenKeySettingsController::setRunWithWindows(bool enable) {
    vRunWithWindows = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vRunWithWindows"), vRunWithWindows);
    
    // CRITICAL: System hook management
    OpenKeyHelper::registerRunOnStartup(vRunWithWindows);
}

bool OpenKeySettingsController::getRunWithWindows() const {
    return vRunWithWindows != 0;
}

bool OpenKeySettingsController::setRunAsAdmin(bool enable) {
    vRunAsAdmin = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vRunAsAdmin"), vRunAsAdmin);
    
    // If enabling admin mode but not currently admin, restart is needed
    if (enable && !IsUserAnAdmin()) {
        return requestRestartAsAdmin();
    } else {
        // Re-register startup hook with potentially changed admin status
        OpenKeyHelper::registerRunOnStartup(vRunWithWindows);
        return false;
    }
}

bool OpenKeySettingsController::getRunAsAdmin() const {
    return vRunAsAdmin != 0;
}

bool OpenKeySettingsController::requestRestartAsAdmin() {
    // This is the extracted logic from MainControlDialog::requestRestartAsAdmin()
    
    // Unregister current startup hook
    OpenKeyHelper::registerRunOnStartup(false);
    
    if (vRunAsAdmin && !IsUserAnAdmin()) {
        // NOTE: The MessageBox is UI-specific, so it should be called by the UI layer
        // This method just performs the restart logic
        // Return true to indicate UI should prompt user
        return true;
    } else {
        // Re-register with current settings
        OpenKeyHelper::registerRunOnStartup(vRunWithWindows);
        return false;
    }
}

void OpenKeySettingsController::setShowOnStartUp(bool enable) {
    vShowOnStartUp = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vShowOnStartUp"), vShowOnStartUp);
}

bool OpenKeySettingsController::getShowOnStartUp() const {
    return vShowOnStartUp != 0;
}

void OpenKeySettingsController::setSupportMetroApp(bool enable) {
    vSupportMetroApp = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vSupportMetroApp"), vSupportMetroApp);
}

bool OpenKeySettingsController::getSupportMetroApp() const {
    return vSupportMetroApp != 0;
}

void OpenKeySettingsController::setUseGrayIcon(bool enable) {
    vUseGrayIcon = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vUseGrayIcon"), vUseGrayIcon);
    SystemTrayHelper::updateData();
}

bool OpenKeySettingsController::getUseGrayIcon() const {
    return vUseGrayIcon != 0;
}

void OpenKeySettingsController::setCheckNewVersion(bool enable) {
    vCheckNewVersion = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vCheckNewVersion"), vCheckNewVersion);
}

bool OpenKeySettingsController::getCheckNewVersion() const {
    return vCheckNewVersion != 0;
}

void OpenKeySettingsController::setSendKeyStepByStep(bool enable) {
    vSendKeyStepByStep = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vSendKeyStepByStep"), vSendKeyStepByStep);
}

bool OpenKeySettingsController::getSendKeyStepByStep() const {
    return vSendKeyStepByStep != 0;
}

// ========== DESKTOP & UI ==========

void OpenKeySettingsController::setCreateDesktopShortcut(bool enable) {
    vCreateDesktopShortcut = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vCreateDesktopShortcut"), vCreateDesktopShortcut);
    
    // CRITICAL: File I/O operation
    if (enable) {
        OpenKeyManager::createDesktopShortcut();
    }
}

bool OpenKeySettingsController::getCreateDesktopShortcut() const {
    return vCreateDesktopShortcut != 0;
}

// ========== ENGLISH-ONLY APPS ==========

void OpenKeySettingsController::setExcludeApps(bool enable) {
    vExcludeApps = enable ? 1 : 0;
    OpenKeyHelper::setRegInt(_T("vExcludeApps"), vExcludeApps);
}

bool OpenKeySettingsController::getExcludeApps() const {
    return vExcludeApps != 0;
}

// ========== PERSISTENCE ==========

void OpenKeySettingsController::saveConfiguration() {
    // All settings are saved individually via setRegInt in each setter
    // This method is here for future use if batch saving is needed
}

void OpenKeySettingsController::loadConfiguration() {
    // Settings are loaded during OpenKeyInit() in OpenKey.cpp
    // This method is a placeholder for future explicit loading
}
