#pragma once
#include "vec2.hpp"

static constexpr double EPSILON = .0001;

time_t GetCurTime();

bool TimeToFormat(time_t t, const char* format, std::string& out);

std::string TimeToDay(time_t t);

std::string TimeToHour(time_t t);

std::string GetUniqueNameWithCurrentTime(const std::string& prepend, const std::string& ext);

bool TryUpdateClosestCircle(const vec2d& coordDouble, double& curMinD2, const vec2d& coord, double r);

void HelpMarker(const char* desc);

bool NotHoveringIMGui();

sf::Color ToSFMLColor(float colors[3]);