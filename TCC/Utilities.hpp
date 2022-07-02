#pragma once
#include "vec2.hpp"

static constexpr auto DEFAULT_TICKS_PER_SECOND = 144; // >= 1
static constexpr double EPSILON = .0001;
extern sf::Clock globalClock;
time_t GetCurTime();

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