#pragma once
#include "vec2.hpp"

class ElemSelected
{
public:
	virtual vec2d& accessCoord() = 0;
	virtual float getRadius() = 0;
};