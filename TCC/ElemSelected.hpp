#pragma once
#include "vec2.hpp"

class ElementInteractable
{
public:
	ElementInteractable* par;
	float radius;
	vec2d coord;
	ElementInteractable(const vec2d& coord, float radius, ElementInteractable* const par = nullptr);
};
typedef ElementInteractable Agent, Goal;