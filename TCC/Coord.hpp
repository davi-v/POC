#pragma once

struct Coord
{
	double x, y;
	operator sf::Vector2f() const;
	operator RVO::Vector2() const;
};