#include "Pch.hpp"
#include "NavigatorRVO2.hpp"

#include "Simulator2D.hpp"
#include "Application.hpp"
#include "Utilities.hpp"

NavigatorRVO2::NavigatorRVO2(Simulator2D& sim) :
	NavigatorRVO2(sim.app.window, sim.app.viewerBase.get(), sim.agents)
{
}

NavigatorRVO2::NavigatorRVO2(ViewerBase& viewerBase, Agents& agents) :
	NavigatorRVO2(viewerBase.app->window, &viewerBase, agents)
{
}

void NavigatorRVO2::drawUIExtra()
{
	if (ImGui::CollapsingHeader("Colors"))
	{
		ColorPicker3U32("Destination Lines", dstLineColor);
		ColorPicker3U32("Agent", agentColor);
		ColorPicker3U32("Goal", goalColor);
		ColorPicker3U32("Trajectory Color", trajectoryColor);
	}

	bool any = false;
	any |= ImGui::InputFloat("neighbourDist", &neighbourDist);
	any |= ImGui::InputScalar("maxNeighbours", ImGuiDataType_U64, &maxNeighbours);
	any |= ImGui::InputFloat("timeHorizon", &timeHorizon);
	//any |= ImGui::InputFloat("timeHorizonObst", &timeHorizonObst);
	auto changedMaxSpeed = ImGui::InputFloat("maxSpeed", &maxSpeed);

	any |= changedMaxSpeed;

	if (changedMaxSpeed)
	{
		accel = maxSpeed / .4f;
		decel = maxSpeed / .4f;
	}

	if (ImGui::MenuItem("Restart"))
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

sf::Color NavigatorRVO2::getColor(float x, float y) const
{
	if (viewerBase)
		return viewerBase->getColor(x, y);
	return agentColor;
}

void NavigatorRVO2::readd()
{
	MakeUniquePtr(rvoSim);
	rvoSim->setTimeStep(timeStep);
	for (const auto& agent : agents)
		addAgentImpl(agent);
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
	auto n = rvoSim->getNumAgents();
	std::vector<vec2d> r(n);
	for (size_t i = 0; i != n; i++)
	{
		const auto& c = rvoSim->getAgentPosition(i);
		auto
			x = static_cast<double>(c.x()),
			y = static_cast<double>(c.y());
		r[i] = { x , y };
	}
	return r;
}

void NavigatorRVO2::addAgentImpl(const Agent& agent)
{
	rvoSim->addAgent(
		RVO::Vector2(static_cast<float>(agent.coord.x), static_cast<float>(agent.coord.y)),
		neighbourDist,
		maxNeighbours,
		timeHorizon,
		timeHorizonObst,
		agent.radius,
		maxSpeed
	);
	coordsThroughTime.emplace_back();
}

void NavigatorRVO2::init()
{
	neighbourDist = 6 * std::max_element(agents.begin(), agents.end(), [](const Agent& a, const Agent& b)
		{
			return a.radius < b.radius;
		})->radius;
	readd();
}

NavigatorRVO2::NavigatorRVO2(sf::RenderWindow& w, ViewerBase* viewerBase) :
	w(w),
	viewerBase(viewerBase),
	drawGoals(false),
	drawTrajectories(false),
	agentColor(sf::Color::Red),
	goalColor(sf::Color::Green),
	dstLineColor(sf::Color::Magenta)
{
}

void NavigatorRVO2::restart()
{
	coordsThroughTime.clear();
	readd();
}

void NavigatorRVO2::addAgent(const Agent& agent)
{
	agents.emplace_back(agent);
	addAgentImpl(agent);
}

void NavigatorRVO2::tick()
{
	for (size_t i = 0, end = rvoSim->getNumAgents(); i != end; i++)
	{
		RVO::Vector2 prefVel;
		if (const auto& goalPtr = agents[i].par)
		{
			auto vecToGoal = static_cast<RVO::Vector2>(goalPtr->coord) - rvoSim->getAgentPosition(i);
			auto lenSquared = vecToGoal * vecToGoal; // "*" is RVO's dot product

			static constexpr float EPS_TOO_CLOSE = 1e-3f;
			if (lenSquared < EPS_TOO_CLOSE) // chegou
				prefVel = RVO::Vector2(0, 0);
			else
			{
				auto len = sqrt(lenSquared);

				const auto& curVel = rvoSim->getAgentVelocity(i);
				auto speed = RVO::abs(curVel);

				auto nextMax = std::min(speed + accel * timeStep, maxSpeed);
				rvoSim->setAgentMaxSpeed(i, nextMax);

				auto D = square(speed) / decel * .5f; // distância que esse agente ainda vai percorrer em linha reta se começar a frear agora
				
				if (len < D)
					speed -= decel * timeStep;
				else
					speed = nextMax;
					
				prefVel = speed / len * vecToGoal; // vecToGoal normalized times speed
			}
		}
		rvoSim->setAgentPrefVelocity(i, prefVel);
	}
	rvoSim->doStep();
}

void NavigatorRVO2::draw()
{
	sf::CircleShape circle;
	const auto nAgents = rvoSim->getNumAgents();
	if (drawGoals)
	{
		circle.setFillColor(goalColor);
		for (size_t i = 0; i != nAgents; i++)
		{
			const auto& agent = agents[i];
			PrepareCircleRadius(circle, agent.radius);
			if (const auto& goalPtr = agent.par)
			{
				const auto& goal = *goalPtr;
				circle.setPosition(goal.coord);
				w.draw(circle);
			}
		}
	}

	// agents
	for (size_t i = 0; i != nAgents; i++)
	{
		const auto& agent = agents[i];
		PrepareCircleRadius(circle, agent.radius);
		//PrepareCircleRadius(circle, rvoSim->getAgentRadius(i));
		const auto& coord = rvoSim->getAgentPosition(i);
		auto x = coord.x();
		auto y = coord.y();
		coordsThroughTime[i].emplace_back(x, y);
		circle.setFillColor(getColor(x, y));
		circle.setPosition({ x, y });
		w.draw(circle);
	}

	// destination lines
	for (size_t i = 0; i != nAgents; i++)
	{
		const auto& goalPtr = agents[i].par;
		if (goalPtr)
		{
			const auto& goal = *goalPtr;
			if (drawDestinationLines)
			{
				sf::Vertex vertices[2]
				{
					sf::Vertex{ ToSFML(rvoSim->getAgentPosition(i)), dstLineColor},
					sf::Vertex{ goal.coord, dstLineColor },
				};
				w.draw(vertices, 2, sf::Lines);
			}
		}
	}

	if (drawTrajectories)
		for (const auto& coords : coordsThroughTime)
		{
			auto n = coords.size();
			std::vector<sf::Vertex> vertices(n);
			for (size_t i = 0; i != n; i++)
			{
				auto& v = vertices[i];
				v.color = trajectoryColor;
				const auto& c = coords[i];
				v.position = { c.x, c.y };
			}
			
			w.draw(vertices.data(), n, sf::LineStrip);
		}
}

void NavigatorRVO2::updateTimeStep(float t)
{
	timeStep = t;
	rvoSim->setTimeStep(timeStep);
}