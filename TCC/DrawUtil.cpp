#include "Pch.hpp"
#include "DrawUtil.hpp"

void PrepareCircleRadius(sf::CircleShape& circle, float r)
{
	circle.setRadius(r);
	circle.setOrigin(r, r);
}

sf::Vector2f ToSFML(const RVO::Vector2 v)
{
	return { v.x(), v.y() };
}
