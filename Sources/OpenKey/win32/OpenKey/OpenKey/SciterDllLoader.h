/*----------------------------------------------------------
OpenKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
-----------------------------------------------------------*/
#pragma once
#include <windows.h>

// Extract sciter.dll from embedded resources (Release) or use local file (Debug)
// Returns path to usable DLL
// Uses Named Mutex for multi-instance synchronization
bool EnsureSciterDll(LPCWSTR& outPath);
