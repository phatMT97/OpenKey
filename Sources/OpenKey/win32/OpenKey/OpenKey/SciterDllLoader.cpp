/*----------------------------------------------------------
OpenKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
-----------------------------------------------------------*/
#include "SciterDllLoader.h"
#include "resource.h"
#include <shlobj.h>
#include <stdio.h>

static WCHAR g_sciterDllPath[MAX_PATH] = {0};

bool EnsureSciterDll(LPCWSTR& outPath) {
#ifdef NDEBUG
    // 1. Determine path: %APPDATA%\OpenKey\sciter.dll
    WCHAR appData[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
        return false;
    }
    
    swprintf_s(g_sciterDllPath, L"%s\\OpenKey\\sciter.dll", appData);

    // Create directory if needed
    WCHAR dirPath[MAX_PATH];
    swprintf_s(dirPath, L"%s\\OpenKey", appData);
    CreateDirectoryW(dirPath, NULL);

    // 2. Use Named Mutex for multi-instance synchronization
    // Ensures only one process can write the DLL at a time
    HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Global\\OpenKeySciterDllLock");
    if (hMutex == NULL) {
        return false;
    }

    // Wait for mutex (5s timeout to prevent deadlock if another process crashes)
    DWORD waitResult = WaitForSingleObject(hMutex, 5000);
    
    if (waitResult == WAIT_ABANDONED || waitResult == WAIT_OBJECT_0) {
        // --- CRITICAL SECTION START ---
        
        // Re-check file (may have been extracted by previous mutex holder)
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        bool needExtract = true;
        
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_SCITER_DLL), RT_RCDATA);
        if (!hRes) {
            WCHAR errMsg[256];
            swprintf_s(errMsg, L"FindResource failed for IDR_SCITER_DLL (%d)\nError: %lu\nCheck if sciter.dll is embedded in OpenKey.rc", IDR_SCITER_DLL, GetLastError());
            MessageBoxW(NULL, errMsg, L"OpenKey Debug", MB_OK | MB_ICONERROR);
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
            return false;
        }
        
        DWORD resSize = SizeofResource(NULL, hRes);

        // If file exists with matching size, skip extraction
        if (GetFileAttributesExW(g_sciterDllPath, GetFileExInfoStandard, &fileInfo)) {
            if (fileInfo.nFileSizeLow == resSize) {
                needExtract = false;
            }
        }

        if (needExtract) {
            HGLOBAL hData = LoadResource(NULL, hRes);
            if (hData) {
                LPVOID data = LockResource(hData);
                
                // Write file (safe - we hold the mutex)
                HANDLE hFile = CreateFileW(g_sciterDllPath, GENERIC_WRITE, 0, NULL, 
                                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                
                if (hFile != INVALID_HANDLE_VALUE) {
                    DWORD written;
                    WriteFile(hFile, data, resSize, &written, NULL);
                    CloseHandle(hFile);
                }
            }
        }

        // --- CRITICAL SECTION END ---
        ReleaseMutex(hMutex);
    } else if (waitResult == WAIT_TIMEOUT) {
        // Log timeout but still try to use existing file
        OutputDebugStringW(L"OpenKey: Mutex timeout waiting for DLL extraction\n");
    }
    
    CloseHandle(hMutex);
    
    // Final check - verify file exists and is loadable
    if (GetFileAttributesW(g_sciterDllPath) == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    outPath = g_sciterDllPath;
#else
    // Debug: use DLL from output folder
    outPath = L"sciter.dll";
#endif
    return true;
}
