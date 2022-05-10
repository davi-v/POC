#include "Pch.hpp"
#include "Agent.hpp"

const sf::Color Agent2D::DEFAULT_COLOR = sf::Color::Red;

void Agent2D::updateGoal(const Goal& goal)
{
	goalPtr = &goal;
}