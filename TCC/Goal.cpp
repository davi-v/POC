#include "Pch.hpp"
#include "Goal.hpp"

double distanceSquared(const Goal& c1, const Goal& c2)
{
	auto off = c1 - c2;
	return dot(off, off);
}

double distance(const Goal& c1, const Goal& c2)
{
	return sqrt(distanceSquared(c1, c2));
}
