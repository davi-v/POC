#pragma once

wchar_t* TryOpenFile(const wchar_t* title = nullptr, int nExts = 0, const wchar_t* const* exts = nullptr);
wchar_t* TryWriteFile(const wchar_t* title = nullptr, int nExts = 0, const wchar_t* const* exts = nullptr);