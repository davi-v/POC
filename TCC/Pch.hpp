#pragma once
#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <random>
#include <filesystem>
#include <deque>
#include <queue>
#include <variant>

#include "tinyfiledialogs.h"

#include <SFML/Graphics.hpp>
#ifdef _DEBUG
#define DEBUG_EXT "-d"
#else
#define DEBUG_EXT
#endif
#pragma comment(lib, "sfml-graphics" DEBUG_EXT)
#pragma comment(lib, "sfml-window" DEBUG_EXT)
#pragma comment(lib, "sfml-system" DEBUG_EXT)

#include <../rvo2/RVO.h>
#pragma comment(lib, "rvo2")

typedef std::vector<unsigned char> bytearray;

template<class T, class...Rest>auto FileAppend(T&& f, Rest&&... rest)
{
	(f.write((char*)&rest, sizeof(rest)), ...);
}
template<class T, class...Rest>auto FileRead(T&& f, Rest&&... rest)
{
	(f.read((char*)&rest, sizeof(rest)), ...);
}

template<class...Rest>auto Clear(Rest&&... rest)
{
	(rest.clear(), ...);
}

template<class T, class...Rest>auto MakeUniquePtr(std::unique_ptr<T>& ptr, Rest&&... rest)
{
	ptr = std::make_unique<T>(rest...);
}

#ifdef _DEBUG
inline void LOG_ERROR() {}

template<typename First, typename ...Rest>
void LOG_ERROR(First&& first, Rest && ...rest)
{
	std::cout << std::forward<First>(first);
	LOG_ERROR(std::forward<Rest>(rest)...);
}
#else
#define LOG_ERROR
#endif