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

class ExcludedAppsDialogSciter : public sciter::window {
public:
    ExcludedAppsDialogSciter();
    virtual ~ExcludedAppsDialogSciter();
    
    virtual bool handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) override;
    void show();
    
    static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, 
                                         LPARAM lParam, UINT_PTR uIdSubclass, 
                                         DWORD_PTR dwRefData);
private:
    void enableAcrylicEffect();
    void fillAppsList();
    void saveAndReload();
    void onAddManual(const std::wstring& appName);
    void onAddCurrentApp();
    void onDeleteApp(const std::wstring& appName);
    
    // Window Picker
    void startWindowPicking();
    void stopWindowPicking();
    std::string getExeNameFromWindow(HWND hwnd);
    void onAddPickedApp(const std::string& exeName);
    
    std::vector<std::string> m_appsList;
    
    // Window Picker state
    bool m_isPickingWindow = false;
    HCURSOR m_hOldCursor = NULL;
    HCURSOR m_hSavedArrowCursor = NULL;  // Saved arrow cursor before replacing
};
