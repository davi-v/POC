#include "Pch.hpp"
#include "NavigatorRVO2.hpp"

#include "Simulator2D.hpp"
#include "DrawUtil.hpp"

NavigatorRVO2::NavigatorRVO2(Simulator2D& simulator2D) :
	simulator2D(simulator2D)
{
	sim.setTimeStep(DEFAULT_TIME_STEP);
}

void NavigatorRVO2::addAgent(const Agent2D& agent)
{
	goals.emplace_back(agent.goalPtr);

	sim.addAgent(
		RVO::Vector2(static_cast<float>(agent.coord.x), static_cast<float>(agent.coord.y)),
		DEFAULT_NEIGHBOUR_DIST,
		DEFAULT_MAX_NEIGHBOURS,
		DEFAULT_TIME_HORIZON,
		DEFAULT_TIME_HORIZON_OBST,
		static_cast<float>(agent.radius) * EXTRA,
		DEFAULT_AGENT_MAX_VELOCITY
	);
}

void NavigatorRVO2::tick()
{
#ifdef _OPENMP
#pragma omp parallel for
#endif
	for (size_t i = 0, end = sim.getNumAgents(); i != end; i++)
	{
		const auto& goalPtr = goals[i];
		RVO::Vector2 goalVec;
		if (const auto& goalPtr = goals[i])
		{
			goalVec = static_cast<RVO::Vector2>(goalPtr->coord) - sim.getAgentPosition(i);
			if (goalVec != RVO::Vector2(0, 0))
				goalVec = RVO::normalize(goalVec) * DEFAULT_AGENT_MAX_VELOCITY;
		}

		sim.setAgentPrefVelocity(i, goalVec);
	}

	sim.doStep();
}

void NavigatorRVO2::draw()
{
	auto& circle = simulator2D.circle;
	auto& window = simulator2D.window;
	circle.setFillColor(sf::Color::Green);
	PrepareCircleRadius(circle, DEFAULT_GOAL_RADIUS);

	auto nAgents = sim.getNumAgents();

	for (size_t i = 0; i != nAgents; i++)
	{
		const auto& goalPtr = goals[i];
		if (goalPtr)
		{
			const auto& goal = *goalPtr;
			circle.setPosition(goal.coord);
			window.draw(circle);
		}
	}

	circle.setFillColor(sf::Color::Red);
	for (size_t i = 0; i != nAgents; i++)
	{
		PrepareCircleRadius(circle, sim.getAgentRadius(i) / EXTRA);
		const auto& coord = sim.getAgentPosition(i);
		circle.setPosition({ coord.x(), coord.y() });
		window.draw(circle);
	}

	for (size_t i = 0; i != nAgents; i++)
	{
		const auto& goalPtr = goals[i];
		if (goalPtr)
		{
			const auto& goal = *goalPtr;
			if (drawDestinationLines)
			{
				sf::Vertex vertices[2]
				{
					sf::Vertex{ ToSFML(sim.getAgentPosition(i)), sf::Color::Magenta},
					sf::Vertex{ goal.coord, sf::Color::Magenta },
				};
				window.draw(vertices, 2, sf::Lines);
			}
		}
	}
}

void NavigatorRVO2::updateTimeStep(float timeStep)
{
	sim.setTimeStep(timeStep);
}
