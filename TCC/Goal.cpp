#include "Pch.hpp"
#include "Goal.hpp"

double distanceSquared(const Goal& c1, const Goal& c2)
{
	return distanceSquared(c1.coord, c2.coord);
}

double distanceSquared(const vec2d& c1, const vec2d& c2)
{
	return square(c1 - c2);
}

double distance(const Goal& c1, const Goal& c2)
{
	return sqrt(distanceSquared(c1, c2));
}

Goal::Goal()
{
}

Goal::Goal(const vec2d& coord) :
	coord(coord)
{
}

float Goal::getRadius()
{
	return DEFAULT_GOAL_RADIUS;
}

vec2d& Goal::accessCoord()
{
	return coord;
}
