#pragma once
#include "vec2.hpp"

class SelectableEdge
{
public:
	SelectableEdge(const vec2d& A, const vec2d& B);
	vec2d A, B, dir;
};