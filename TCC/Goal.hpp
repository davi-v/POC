#pragma once
#include "vec2.hpp"
#include "ElemSelected.hpp"

class Goal : public ElemSelected
{
public:
	Goal(const vec2d& coord, float r);
	vec2d& accessCoord() override;
	float getRadius() override;
	float r = 15.f;
	vec2d coord;
};

double distanceSquared(const Goal& c1, const Goal& c2);
double distanceSquared(const vec2d& c1, const vec2d& c2);
double distance(const vec2d& c1, const vec2d& c2);