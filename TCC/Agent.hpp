#pragma once
#include "Goal.hpp"

class Agent2D : public ElemSelected
{
	static const sf::Color DEFAULT_COLOR;

	float getRadius() override;
	vec2d& accessCoord() override;

public:
	Agent2D(const vec2d& coord, double radius = DEFAULT_RADIUS, const sf::Color& color = DEFAULT_COLOR);
	vec2d coord;
	const Goal* goalPtr; // se nullptr, o agente est� ocioso (n�o assigned a nenhum goal)
	double radius;
	sf::Color color;
	void updateGoal(const Goal& goal);
	static constexpr auto DEFAULT_RADIUS = 30.f; // em metros
};