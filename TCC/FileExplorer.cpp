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