#pragma once

#ifdef _DEBUG
template<class T>
void LOG_ELEM(T&& elem)
{
	std::cout << elem;
}
inline void LOG_ELEM(const std::wstring& s)
{
	std::wcout << s;
}
inline void LOG_ERROR() {}
template<class First, class...Rest>
void LOG_ERROR(First&& first, Rest&& ...rest)
{
	LOG_ELEM(first);
	LOG_ERROR(rest...);
}

#else
#define LOG_ERROR
#endif