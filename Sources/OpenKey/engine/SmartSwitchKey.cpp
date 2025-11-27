//
//  SmartSwitchKey.cpp
//  OpenKey
//
//  Created by Tuyen on 8/13/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#include "SmartSwitchKey.h"
#include <map>
#include <set>
#include <iostream>
#include <memory.h>

//main data, i use `map` because it has O(Log(n))
static map<string, Int8> _smartSwitchKeyData;
static string _cacheKey = ""; //use cache for faster
static Int8 _cacheData = 0; //use cache for faster

void initSmartSwitchKey(const Byte* pData, const int& size) {
    _smartSwitchKeyData.clear();
    if (pData == NULL) return;
    Uint16 count = 0;
    Uint32 cursor = 0;
    if (size >= 2) {
        memcpy(&count, pData + cursor, 2);
        cursor+=2;
    }
    Uint8 bundleIdSize;
    Uint8 value;
    for (int i = 0; i < count; i++) {
        bundleIdSize = pData[cursor++];
        string bundleId((char*)pData + cursor, bundleIdSize);
        cursor += bundleIdSize;
        value = pData[cursor++];
        _smartSwitchKeyData[bundleId] = value;
    }
}

void getSmartSwitchKeySaveData(vector<Byte>& outData) {
    outData.clear();
    Uint16 count = (Uint16)_smartSwitchKeyData.size();
    outData.push_back((Byte)count);
    outData.push_back((Byte)(count>>8));
    
    for (std::map<string, Int8>::iterator it = _smartSwitchKeyData.begin(); it != _smartSwitchKeyData.end(); ++it) {
        outData.push_back((Byte)it->first.length());
        for (int j = 0; j < it->first.length(); j++) {
            outData.push_back(it->first[j]);
        }
        outData.push_back(it->second);
    }
}

int getAppInputMethodStatus(const string& bundleId, const int& currentInputMethod) {
    if (_cacheKey.compare(bundleId) == 0) {
        return _cacheData;
    }
    if (_smartSwitchKeyData.find(bundleId) != _smartSwitchKeyData.end()) {
        _cacheKey = bundleId;
        _cacheData = _smartSwitchKeyData[bundleId];
        return _cacheData;
    }
    _cacheKey = bundleId;
    _cacheData = currentInputMethod;
    _smartSwitchKeyData[bundleId] = _cacheData;
    return -1;
}

void setAppInputMethodStatus(const string& bundleId, const int& language) {
    _smartSwitchKeyData[bundleId] = language;
    _cacheKey = bundleId;
    _cacheData = language;
}

//English-only apps data
static std::set<string> _englishOnlyApps;

void initEnglishOnlyApps(const Byte* pData, const int& size) {
    _englishOnlyApps.clear();
    if (pData == NULL) return;
    Uint16 count = 0;
    Uint32 cursor = 0;
    if (size >= 2) {
        memcpy(&count, pData + cursor, 2);
        cursor += 2;
    }
    Uint8 bundleIdSize;
    for (int i = 0; i < count; i++) {
        bundleIdSize = pData[cursor++];
        string bundleId((char*)pData + cursor, bundleIdSize);
        cursor += bundleIdSize;
        _englishOnlyApps.insert(bundleId);
    }
}

void getEnglishOnlyAppsSaveData(vector<Byte>& outData) {
    outData.clear();
    Uint16 count = (Uint16)_englishOnlyApps.size();
    outData.push_back((Byte)count);
    outData.push_back((Byte)(count >> 8));
    
    for (std::set<string>::iterator it = _englishOnlyApps.begin(); it != _englishOnlyApps.end(); ++it) {
        outData.push_back((Byte)it->length());
        for (int j = 0; j < it->length(); j++) {
            outData.push_back((*it)[j]);
        }
    }
}

bool isEnglishOnlyApp(const string& bundleId) {
    return _englishOnlyApps.find(bundleId) != _englishOnlyApps.end();
}

void addEnglishOnlyApp(const string& bundleId) {
    _englishOnlyApps.insert(bundleId);
}

void removeEnglishOnlyApp(const string& bundleId) {
    _englishOnlyApps.erase(bundleId);
}

void getAllEnglishOnlyApps(vector<string>& apps) {
    apps.clear();
    for (std::set<string>::iterator it = _englishOnlyApps.begin(); it != _englishOnlyApps.end(); ++it) {
        apps.push_back(*it);
    }
}
