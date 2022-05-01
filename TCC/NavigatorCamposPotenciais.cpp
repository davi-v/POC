#include "Pch.hpp"
#include "NavigatorCamposPotenciais.hpp"

#include "Utilities.hpp"
#include "Simulator2D.hpp"
#include "DrawUtil.hpp"

static constexpr double ARBITRARY_CONSTANT = 3;

Coord NavigatorCamposPotenciais::getAttractionVec(const Coord& c1, const Coord& c2)
{
	auto off = c2 - c1;
	if (off.tryNormalize())
		off *= DEFAULT_AGENT_MAX_VELOCITY;
	return off;
}

Coord NavigatorCamposPotenciais::getRepulsionVec(const Object& c1, const Object& c2)
{
	auto off = c1.cur - c2.cur;
	auto d = distance(c1.cur, c2.cur) - (c1.radius + c2.radius);
	d = std::max(d, EPSILON); // caso os c�rculos estivessem colidindo, para n�o ter repuls�o negativa que atrairia-os
	return ARBITRARY_CONSTANT / square(d) * off;
}

void NavigatorCamposPotenciais::addAgent(const Agent2D& agent)
{
	Coord dst;
	if (const auto& goalPtr = agent.goalPtr)
		dst = *goalPtr;
	else
		dst = agent.coord;

	agents.emplace_back(
		agent.radius,
		agent.coord,
		dst
		);
}

void NavigatorCamposPotenciais::tick()
{
	// naive algorithm to check for distances
	auto
		beg = agents.begin(),
		end = agents.end();
	for (auto it1 = beg; it1 != end; it1++)
	{
		auto& e1 = *it1;
		auto vec = getAttractionVec(e1.cur, e1.dst);
		for (auto it2 = beg; it2 != end; it2++)
			if (it1 != it2)
			{
				const auto& e2 = *it2;
				vec += getRepulsionVec(e1, e2);
			}
		//LOG(vec.x, ' ', vec.y, '\n');
		e1.cur += vec * static_cast<double>(timeStep);
	}
}

void NavigatorCamposPotenciais::draw()
{
	sf::CircleShape circle;

	// destinations
	circle.setFillColor(sf::Color::Green);
	PrepareCircle(circle, DEFAULT_GOAL_RADIUS);
	for (const auto& agent : agents)
	{
		circle.setPosition(agent.dst);
		window.draw(circle);
	}

	// agents
	circle.setFillColor(sf::Color::Red);
	for (const auto& agent : agents)
	{
		PrepareCircle(circle, static_cast<float>(agent.radius));
		circle.setPosition(agent.cur);
		window.draw(circle);
	}

	// Lines
	if (drawDestinationLines)
		for (const auto& agent : agents)
		{
			sf::Vertex vertices[2]
			{
				sf::Vertex{ agent.cur, sf::Color::Magenta},
				sf::Vertex{ agent.dst, sf::Color::Magenta },
			};
			window.draw(vertices, 2, sf::Lines);
		}
}

void NavigatorCamposPotenciais::updateTimeStep(float timeStep)
{
	this->timeStep = timeStep;
}

NavigatorCamposPotenciais::NavigatorCamposPotenciais(sf::RenderWindow& window) :
	window(window),
	timeStep(DEFAULT_TIME_STEP)
{
}
