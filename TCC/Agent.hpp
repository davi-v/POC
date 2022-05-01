#pragma once
#include "Goal.hpp"

class Agent2D
{
	static const sf::Color DEFAULT_COLOR;

public:
	Agent2D(const Coord& coord, double radius = DEFAULT_RADIUS, const sf::Color& color = DEFAULT_COLOR);
	Coord coord;
	Goal* goalPtr; // se nullptr, o agente está ocioso (não assigned a nenhum goal)
	double radius;
	sf::Color color;
	void updateGoal(Goal& goal);
	static constexpr auto DEFAULT_RADIUS = 30.f; // em metros
};