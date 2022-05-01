#include "Pch.hpp"
#include "Coord.hpp"

Coord::operator sf::Vector2f() const
{
	return { static_cast<float>(x), static_cast<float>(y) };
}

Coord::operator RVO::Vector2() const
{
	return RVO::Vector2(static_cast<float>(x), static_cast<float>(y));
}