#include "Pch.hpp"
#include "FileExplorer.hpp"

wchar_t* TryOpenFile(const wchar_t* title, int nExts, const wchar_t* const* exts)
{
	return tinyfd_openFileDialogW( // there is also a wchar_t version
		title, // title
		nullptr, // optional initial directory
		nExts, // number of filter patterns
		exts, // char const * lFilterPatterns[2] = { "*.txt", "*.jpg" };
		NULL, // optional filter description
		0 // forbid multiple selections
	);
}

wchar_t* TryWriteFile(const wchar_t* title, int nExts, const wchar_t* const* exts)
{
	return tinyfd_saveFileDialogW( // there is also a wchar_t version
		L"Select where to save the current agents and goals", // title
		nullptr, // optional initial directory
		nExts, // number of filter patterns
		exts, // char const * lFilterPatterns[2] = { "*.txt", "*.jpg" };
		nullptr
	);
}

Directory TrySelectDirectory(const wchar_t* title)
{
	//return tinyfd_selectFolderDialogW(title, nullptr);
	return Directory();
}

Directory::Directory()
{
	ret = NFD_PickFolderN(&ptr, nullptr);
}

Directory::~Directory()
{
	if (isOk())
		NFD_FreePathN(ptr);
}

bool Directory::isOk() const
{
	return ret == NFD_OKAY;
}

Directory::operator wchar_t* ()
{
	return ptr;
}