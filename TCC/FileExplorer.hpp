#pragma once

wchar_t* TryOpenFile(const wchar_t* title = nullptr, int nExts = 0, const wchar_t* const* exts = nullptr);
wchar_t* TryWriteFile(const wchar_t* title = nullptr, int nExts = 0, const wchar_t* const* exts = nullptr);

class Directory
{
	wchar_t* ptr;
public:
	nfdresult_t ret;
	Directory();
	~Directory();
	bool isOk() const;
	operator wchar_t*();
};

Directory TrySelectDirectory(const wchar_t* title);