#pragma once
#include "vec2.hpp"

typedef std::vector<vec2d> VecCoords;
typedef VecCoords Cell;

static constexpr auto DEFAULT_TICKS_PER_SECOND = 144; // >= 1
static constexpr double EPSILON = .0001;
extern sf::Clock globalClock;
time_t GetCurTime();

sf::Vector2f ToFloat(const sf::Vector2u& p);

bool TimeToFormat(time_t t, const char* format, std::string& out);

std::string TimeToDay(time_t t);

std::string TimeToHour(time_t t);

// ext with '.' (.png)
std::string GetUniqueNameWithCurrentTime(const std::string& prepend, const std::string& ext);

bool TryUpdateClosestCircle(const vec2d& coordDouble, double& curMinD2, const vec2d& coord, double r);

void HelpMarker(const char* desc);

bool NotHoveringIMGui();

sf::Color ToSFMLColor(float colors[3]);

bool ShowRadiusOption(float& r, float maxR);

double cross(const vec2d& a, const vec2d& b);

// v ccw
bool IsInsideConvexShape(const vec2d& p, const std::vector<vec2d>& v);

double TwiceAreaPolygonSigned(const Cell& cell);

// CCW
double AreaTrig(const vec2d& p1, const vec2d& p2, const vec2d& p3);

vec2d CalculateCentroid(const Cell& cell);
std::vector<vec2d> CalculateCentroids(const std::vector<Cell>& cells);
bool ColorPicker3U32(const char* label, sf::Color& c, ImGuiColorEditFlags flags = ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);

static constexpr double EPS_DBS = .0001;
static constexpr auto PI = std::numbers::pi_v<float>;