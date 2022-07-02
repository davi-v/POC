#pragma once
#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <numeric>
#include <numbers>
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

#include <rvo2/RVO.h>
#pragma comment(lib, "rvo2")

#include "Logger.hpp"

#include <imgui.h>
#include <imgui-SFML.h>
#pragma comment(lib, "imgui")
#pragma comment(lib, "opengl32")

#include <implot.h>
#pragma comment(lib, "implot")

#include "TemplateUtils.hpp"
#include "DefineUtils.hpp"

#include <opencv2/opencv.hpp>
#ifdef _DEBUG
#define OPENCV_DEBUG_EXT "d"
#else
#define OPENCV_DEBUG_EXT
#endif
#pragma comment(lib, "opencv_world455" OPENCV_DEBUG_EXT)