#pragma once

#ifdef _DEBUG
template<class T>
void LOG_ELEM(T&& elem)
{
	std::cout << elem;
}
inline void LOG_ELEM(const wchar_t* s)
{
	std::wcout << s;
}
inline void LOG_ELEM(const std::wstring& s)
{
	std::wcout << s;
}
inline void LOG() {}
template<class First, class...Rest>
void LOG(First&& first, Rest&& ...rest)
{
	LOG_ELEM(first);
	LOG(rest...);
}

#else
#define LOG
#endif