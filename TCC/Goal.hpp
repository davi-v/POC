#pragma once
#include "vec2.hpp"

typedef Coord Goal;

static constexpr auto DEFAULT_GOAL_RADIUS = 15.f;

double distanceSquared(const Goal& c1, const Goal& c2);
double distance(const Goal& c1, const Goal& c2);