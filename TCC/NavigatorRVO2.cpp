#include "Pch.hpp"
#include "NavigatorRVO2.hpp"

#include "Simulator2D.hpp"
#include "DrawUtil.hpp"
#include "Application.hpp"
#include "Utilities.hpp"

NavigatorRVO2::NavigatorRVO2(Simulator2D& simulator2D, float neighbourDist) :
	simulator2D(simulator2D),
	neighbourDist(neighbourDist),
	drawGoals(false),
	drawTrajectories(false)
{
	MakeUniquePtr(sim);
	sim->setTimeStep(timeStep);
}

void NavigatorRVO2::drawUI()
{
	bool any = false;
	any |= ImGui::InputFloat("neighbourDist", &neighbourDist);
	any |= ImGui::InputScalar("maxNeighbours", ImGuiDataType_U64, &maxNeighbours);
	any |= ImGui::InputFloat("timeHorizon", &timeHorizon);
	any |= ImGui::InputFloat("timeHorizonObst", &timeHorizonObst);
	auto changedMaxSpeed = ImGui::InputFloat("maxSpeed", &maxSpeed);

	any |= changedMaxSpeed;

	if (changedMaxSpeed)
	{
		accel = maxSpeed / .4f;
		decel = maxSpeed / .4f;
	}

	if (ImGui::Button("Restart"))
		restart();

	ImGui::Checkbox("Draw Goals", &drawGoals);
	ImGui::Checkbox("Draw Trajectories", &drawTrajectories);
	if (ImGui::MenuItem("Dump CSV with coordinates"))
		dumpCSV();

	if (any)
	{
		running = false;
		restart();
	}
}

void NavigatorRVO2::dumpCSV()
{
	auto nRobots = coordsThroughTime.size();
	if (!nRobots)
		return;

	auto path = GetUniqueNameWithCurrentTime("trajectories-", ".csv");
	std::ofstream f(path);
	auto nFrames = coordsThroughTime.front().size();
	f << "x\ty\tid\n";
	for (size_t i = 0; i != nFrames; i++)
		for (size_t j = 0; j != nRobots; j++)
		{
			const auto& c = coordsThroughTime[j][i];
			f << c.x << '\t' << c.y << '\t' << j << '\n';
		}
}

std::vector<vec2d> NavigatorRVO2::getAgentPositions()
{
	auto n = sim->getNumAgents();
	std::vector<vec2d> r(n);
	for (size_t i = 0; i != n; i++)
	{
		const auto& c = sim->getAgentPosition(i);
		auto
			x = static_cast<double>(c.x()),
			y = static_cast<double>(c.y());
		r[i] = { x , y };
	}
	return r;
}

void NavigatorRVO2::addAgentImpl(const Agent2D& agent)
{
	sim->addAgent(
		RVO::Vector2(static_cast<float>(agent.coord.x), static_cast<float>(agent.coord.y)),
		neighbourDist,
		maxNeighbours,
		timeHorizon,
		timeHorizonObst,
		static_cast<float>(agent.radius) * RADIUS_MULTIPLIER_FACTOR,
		maxSpeed
	);
	coordsThroughTime.emplace_back();
}

void NavigatorRVO2::restart()
{
	MakeUniquePtr(sim);
	sim->setTimeStep(timeStep);

	coordsThroughTime.clear();
	for (auto& agentPtr : orgAgents)
		addAgentImpl(*agentPtr);
}

void NavigatorRVO2::addAgent(const Agent2D& agent)
{
	orgAgents.emplace_back(&agent);
	addAgentImpl(agent);
}

void NavigatorRVO2::tick()
{
#ifdef _OPENMP
#pragma omp parallel for
#endif
	for (size_t i = 0, end = sim->getNumAgents(); i != end; i++)
	{
		const auto& goalPtr = orgAgents[i]->goalPtr;
		RVO::Vector2 prefVel;
		if (goalPtr)
		{
			auto vecToGoal = static_cast<RVO::Vector2>(goalPtr->coord) - sim->getAgentPosition(i);
			auto lenSquared = vecToGoal * vecToGoal; // "*" is RVO's dot product

			static constexpr float EPS_TOO_CLOSE = 1e-3f;
			if (lenSquared < EPS_TOO_CLOSE) // chegou
				prefVel = RVO::Vector2(0, 0);
			else
			{
				auto len = sqrt(lenSquared);

				const auto& curVel = sim->getAgentVelocity(i);
				auto speed = RVO::abs(curVel);

				auto nextMax = std::min(speed + accel * timeStep, maxSpeed);
				sim->setAgentMaxSpeed(i, nextMax);

				auto D = square(speed) / decel * .5f; // distância que esse agente ainda vai percorrer em linha reta se começar a frear agora
				
				if (len < D)
					speed -= decel * timeStep;
				else
					speed = nextMax;
					
				prefVel = speed / len * vecToGoal; // vecToGoal normalized times speed
			}
		}

		sim->setAgentPrefVelocity(i, prefVel);
	}

	sim->doStep();
}

void NavigatorRVO2::draw()
{
	auto& circle = simulator2D.circle;
	auto& window = simulator2D.app.window;

	auto nAgents = sim->getNumAgents();

	if (drawGoals)
	{
		circle.setFillColor(sf::Color::Green);
		PrepareCircleRadius(circle, simulator2D.r);
		for (size_t i = 0; i != nAgents; i++)
		{
			const auto& goalPtr = orgAgents[i]->goalPtr;
			if (goalPtr)
			{
				const auto& goal = *goalPtr;
				circle.setPosition(goal.coord);
				window.draw(circle);
			}
		}
	}

	// agents
	for (size_t i = 0; i != nAgents; i++)
	{
		PrepareCircleRadius(circle, sim->getAgentRadius(i) / RADIUS_MULTIPLIER_FACTOR);
		const auto& coord = sim->getAgentPosition(i);
		auto x = coord.x();
		auto y = coord.y();
		coordsThroughTime[i].emplace_back(x, y);
		circle.setFillColor(simulator2D.getColor(x, y));
		circle.setPosition({ x, y });
		window.draw(circle);
	}

	// destination lines
	for (size_t i = 0; i != nAgents; i++)
	{
		const auto& goalPtr = orgAgents[i]->goalPtr;
		if (goalPtr)
		{
			const auto& goal = *goalPtr;
			if (drawDestinationLines)
			{
				sf::Vertex vertices[2]
				{
					sf::Vertex{ ToSFML(sim->getAgentPosition(i)), sf::Color::Magenta},
					sf::Vertex{ goal.coord, sf::Color::Magenta },
				};
				window.draw(vertices, 2, sf::Lines);
			}
		}
	}

	if (drawTrajectories)
	{
		for (auto& coords : coordsThroughTime)
		{
			auto n = coords.size();
			std::vector<sf::Vertex> vertices(n);
			auto trajectoryColor = sf::Color::Yellow;
			for (size_t i = 0; i != n; i++)
			{
				auto& v = vertices[i];
				v.color = trajectoryColor;
				const auto& c = coords[i];
				v.position = { c.x, c.y };
			}
			
			window.draw(vertices.data(), n, sf::LineStrip);
		}
	}
}

void NavigatorRVO2::updateTimeStep(float t)
{
	timeStep = t;
	sim->setTimeStep(timeStep);
}
