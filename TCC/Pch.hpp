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

#define BOOST_DISABLE_CURRENT_LOCATION

#define BOOST_POLYGON_NO_DEPS
#include <boost/polygon/voronoi.hpp>
using boost::polygon::voronoi_builder;
using namespace boost::polygon;
using boost::polygon::voronoi_diagram;
using boost::polygon::x;
using boost::polygon::y;
using boost::polygon::low;
using boost::polygon::high;

#include <opencv2/opencv.hpp>
#ifdef _DEBUG
#define OPENCV_DEBUG_EXT "d"
#else
#define OPENCV_DEBUG_EXT
#endif
#pragma comment(lib, "opencv_world455" OPENCV_DEBUG_EXT)