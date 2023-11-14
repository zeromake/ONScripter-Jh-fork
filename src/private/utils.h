/* -*- C++ -*-
 *
 *  Utils.h
 *
 *  Copyright (C) 2014 jh10001 <jh10001@live.cn>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef ANDROID
#include <android/log.h>
#elif defined(WINRT)
#include "debugapi.h"
#include "windows.h"
static BOOL MByteToWChar(LPCSTR lpcszStr, LPWSTR lpwszStr, DWORD dwSize) {
    DWORD dwMinSize;
    dwMinSize = MultiByteToWideChar(CP_ACP, 0, lpcszStr, -1, NULL, 0);
    if (dwSize < dwMinSize) {
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, lpcszStr, -1, lpwszStr, dwMinSize);
    return TRUE;
}
#endif
#include <stdarg.h>
#include <stdio.h>

#include <string>
#include <utility>
#include <chrono>

namespace utils {

inline std::chrono::steady_clock::time_point now() {
    return std::chrono::steady_clock::now();
}

inline float duration(std::chrono::steady_clock::time_point prev) {
    return std::chrono::duration_cast<std::chrono::microseconds>(now() - prev).count() / 1000.0f;
}

inline void printDebug(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
#ifdef ANDROID
    __android_log_vprint(ANDROID_LOG_DEBUG, "DEBUG", format, ap);
#elif defined(WINRT)
    const int __size = 256;
    char *buf = new char[__size]{0};
    vsnprintf(buf, __size, format, ap);
    LPWSTR wstr = new WCHAR[128];
    MByteToWChar(buf, wstr, 256);
    OutputDebugString(wstr);
#else
    vprintf(format, ap);
#endif
    va_end(ap);
}

inline void printInfo(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
#ifdef ANDROID
    __android_log_vprint(ANDROID_LOG_VERBOSE, "INFO", format, ap);
#elif defined(WINRT)
    const int __size = 256;
    char *buf = new char[__size]{0};
    vsnprintf(buf, __size, format, ap);
    LPWSTR wstr = new WCHAR[128];
    MByteToWChar(buf, wstr, 256);
    OutputDebugString(wstr);
#else
    vprintf(format, ap);
#endif
    va_end(ap);
}

inline void printError(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
#ifdef ANDROID
    __android_log_vprint(ANDROID_LOG_ERROR, "ERR", format, ap);
#elif defined(WINRT)
    char *buf = new char[256]{0};
    vsnprintf(buf, 256, format, ap);
    LPWSTR wstr = new WCHAR[128];
    MByteToWChar(buf, wstr, 256);
    OutputDebugString(wstr);
#else
    vfprintf(stderr, format, ap);
#endif
    va_end(ap);
}

template <typename T>
T(min)
(T a, T b) {
    return a < b ? a : b;
}

template <typename T>
T(clamp)
(T x, T min, T max) {
    return x < min ? min : (x > max ? max : x);
}
}  // namespace utils

#endif
