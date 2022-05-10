#pragma once
#include "vec2.hpp"
#include "ElemSelected.hpp"

class Goal : public ElemSelected
{
public:
	Goal();
	Goal(const vec2d& coord);
	float getRadius() override;
	vec2d& accessCoord() override;

	vec2d coord;
};

static constexpr auto DEFAULT_GOAL_RADIUS = 15.;

double distanceSquared(const Goal& c1, const Goal& c2);
double distanceSquared(const vec2d& c1, const vec2d& c2);
double distance(const Goal& c1, const Goal& c2);