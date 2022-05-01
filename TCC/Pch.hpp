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

#include <RVO.h>
#pragma comment(lib, "rvo2")

#include <Hungarian.h>
#pragma comment(lib, "Hungarian")

#include "Logger.hpp"

#include <imgui.h>
#include <imgui-SFML.h>
#pragma comment(lib, "imgui")
#pragma comment(lib, "opengl32")

#include "TemplateUtils.hpp"
#include "DefineUtils.hpp"