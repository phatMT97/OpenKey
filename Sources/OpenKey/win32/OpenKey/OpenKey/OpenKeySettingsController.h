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
#include <functional>
#include <mutex>

/**
 * OpenKeySettingsController - Business Logic Layer
 * 
 * PURPOSE: Decouple UI from business logic to enable future UI framework migration.
 * PATTERN: Singleton with thread-safe access.
 * 
 * PHASE 1: Extracted from MainControlDialog.cpp
 * PHASE 2: Will be shared between Win32 UI and Duilib UI
 * PHASE 3: Exclusive controller for Duilib UI after Win32 removal
 */
class OpenKeySettingsController {
public:
    // Singleton Access
    static OpenKeySettingsController& getInstance();

    // Prevent copying
    OpenKeySettingsController(const OpenKeySettingsController&) = delete;
    void operator=(const OpenKeySettingsController&) = delete;

    // ========== CORE INPUT SETTINGS ==========
    
    /**
     * Set language mode (Vietnamese=1, English=0)
     * Handles Smart Switch Key data persistence if enabled
     */
    void setLanguage(int langCode);
    int getLanguage() const;

    /**
     * Set input method type (Telex=0, VNI=1, Simple Telex 1=2, Simple Telex 2=3)
     */
    void setInputType(int inputType);
    int getInputType() const;

    /**
     * Set code table (Unicode=0, VNI Windows=1, VNI=2, Unicode Compound=3, TCVN3=4)
     * Handles Smart Switch Key data persistence if enabled
     */
    void setCodeTable(int tableCode);
    int getCodeTable() const;

    /**
     * Set switch key status (complex bit field: modifiers + key code)
     * Format: [KeyCode:8][Reserved:8][Beep:1][Shift:1][Win:1][Alt:1][Ctrl:1]
     */
    void setSwitchKeyStatus(int status);
    int getSwitchKeyStatus() const;

    // ========== SPELLING & ENGINE SETTINGS ==========
    
    /**
     * Enable/disable spell checking
     * CRITICAL: Triggers engine re-initialization via vSetCheckSpelling()
     */
    void setCheckSpelling(bool enable);
    bool getCheckSpelling() const;

    void setUseModernOrthography(bool enable);
    bool getUseModernOrthography() const;

    void setRestoreIfWrongSpelling(bool enable);
    bool getRestoreIfWrongSpelling() const;

    void setAllowConsonantZFWJ(bool enable);
    bool getAllowConsonantZFWJ() const;

    void setTempOffSpelling(bool enable);
    bool getTempOffSpelling() const;

    // ========== QUICK TYPING FEATURES ==========
    
    void setQuickTelex(bool enable);
    bool getQuickTelex() const;

    void setQuickStartConsonant(bool enable);
    bool getQuickStartConsonant() const;

    void setQuickEndConsonant(bool enable);
    bool getQuickEndConsonant() const;

    // ========== MACRO SETTINGS ==========
    
    void setUseMacro(bool enable);
    bool getUseMacro() const;

    void setUseMacroInEnglishMode(bool enable);
    bool getUseMacroInEnglishMode() const;

    void setAutoCapsMacro(bool enable);
    bool getAutoCapsMacro() const;

    // ========== SMART FEATURES ==========
    
    /**
     * Enable/disable Smart Switch Key (per-application language memory)
     */
    void setUseSmartSwitchKey(bool enable);
    bool getUseSmartSwitchKey() const;

    /**
     * Enable/disable Remember Code Table (per-application code table memory)
     */
    void setRememberCode(bool enable);
    bool getRememberCode() const;

    void setUpperCaseFirstChar(bool enable);
    bool getUpperCaseFirstChar() const;

    void setOtherLanguage(bool enable);
    bool getOtherLanguage() const;

    void setTempOffOpenKey(bool enable);
    bool getTempOffOpenKey() const;

    // ========== BROWSER FIX SETTINGS ==========
    
    void setFixRecommendBrowser(bool enable);
    bool getFixRecommendBrowser() const;

    void setFixChromiumBrowser(bool enable);
    bool getFixChromiumBrowser() const;

    // ========== SYSTEM INTEGRATION ==========
    
    /**
     * Enable/disable Run with Windows startup
     * CRITICAL: Calls OpenKeyHelper::registerRunOnStartup() which:
     * - Modifies Registry (HKCU\Software\Microsoft\Windows\CurrentVersion\Run)
     * - OR creates Task Scheduler task (if vRunAsAdmin is enabled)
     */
    void setRunWithWindows(bool enable);
    bool getRunWithWindows() const;

    /**
     * Enable/disable Run as Administrator
     * CRITICAL: May trigger application restart with UAC elevation
     * Returns: true if restart is required, false otherwise
     */
    bool setRunAsAdmin(bool enable);
    bool getRunAsAdmin() const;

    /**
     * Request application restart as administrator
     * Returns: true if restart was initiated, false if already admin or user declined
     */
    bool requestRestartAsAdmin();

    void setShowOnStartUp(bool enable);
    bool getShowOnStartUp() const;

    void setSupportMetroApp(bool enable);
    bool getSupportMetroApp() const;

    void setUseGrayIcon(bool enable);
    bool getUseGrayIcon() const;

    void setCheckNewVersion(bool enable);
    bool getCheckNewVersion() const;

    void setSendKeyStepByStep(bool enable);
    bool getSendKeyStepByStep() const;

    // ========== DESKTOP & UI ==========
    
    /**
     * Create/remove desktop shortcut
     * CRITICAL: Performs file I/O operations
     */
    void setCreateDesktopShortcut(bool enable);
    bool getCreateDesktopShortcut() const;

    // ========== ENGLISH-ONLY APPS ==========
    
    void setExcludeApps(bool enable);
    bool getExcludeApps() const;

    // ========== PERSISTENCE ==========
    
    /**
     * Save all current settings to Registry
     * Called automatically by individual setters, but can be called manually
     */
    void saveConfiguration();

    /**
     * Load all settings from Registry
     * Should be called once during application initialization
     */
    void loadConfiguration();

private:
    OpenKeySettingsController();
    ~OpenKeySettingsController();

    // ========== INTERNAL HELPERS ==========
    
    /**
     * Update Smart Switch Key data when language or code table changes
     * Persists per-application input method status to Registry
     */
    void updateSmartSwitchKeyData();

    /**
     * Thread safety for engine state modifications
     */
    std::mutex m_stateMutex;
};
