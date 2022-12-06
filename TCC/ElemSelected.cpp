#include "Pch.hpp"
#include "ElemSelected.hpp"

ElementInteractable::ElementInteractable(const vec2d& coord, float radius, ElementInteractable* const par) :
	coord(coord),
	radius(radius),
	par(par)
{
}