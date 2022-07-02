#include "Pch.hpp"
#include "NavigatorCamposPotenciais.hpp"

#include "Utilities.hpp"
#include "Application.hpp"
#include "Simulator2D.hpp"
#include "DrawUtil.hpp"

static constexpr double ARBITRARY_CONSTANT = 3;

std::vector<vec2d> NavigatorCamposPotenciais::getAgentPositions()
{
	auto n = agents.size();
	std::vector<vec2d> r(n);
	for (size_t i = 0; i != n; i++)
		r.emplace_back(agents[i].cur);
	return r;
}

vec2d NavigatorCamposPotenciais::getAttractionVec(const vec2d& c1, const vec2d& c2)
{
	auto off = c2 - c1;
	if (off.tryNormalize())
		off *= maxVel;
	return off;
}

vec2d NavigatorCamposPotenciais::getRepulsionVec(const Object& c1, const Object& c2, double distance)
{
	auto off = c1.cur - c2.cur;
	auto d = distance - (c1.radius + c2.radius);
	d = std::max(d, EPSILON); // caso os círculos estivessem colidindo, para não ter repulsão negativa que atrairia-os
	return ARBITRARY_CONSTANT / square(d) * off;
}

void NavigatorCamposPotenciais::addAgent(const Agent2D& agent)
{
	vec2d dst;
	if (const auto& goalPtr = agent.goalPtr)
		dst = goalPtr->coord;
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
				auto d = distance(e1.cur, e2.cur);
				if (d < maxRadius)
					vec += getRepulsionVec(e1, e2, d);
			}
		//LOG(vec.x, ' ', vec.y, '\n');
		e1.cur += vec * static_cast<double>(timeStep);
	}
}

void NavigatorCamposPotenciais::drawUI()
{
	ImGui::DragFloat("Max Radius", &maxRadius, 1.0f, 0, std::numeric_limits<float>::max());
	ImGui::InputDouble("Max Vel", &maxVel);
	ImGui::Checkbox("Draw Radius", &drawRadius);
}

void NavigatorCamposPotenciais::draw()
{
	auto& window = simulator2D.app.window;

	// destinations
	auto& circle = simulator2D.circle;
	circle.setFillColor(sf::Color::Green);
	PrepareCircleRadius(circle, simulator2D.r);
	for (const auto& agent : agents)
	{
		circle.setPosition(agent.dst);
		window.draw(circle);
	}

	// agents
	circle.setFillColor(sf::Color::Red);
	for (const auto& agent : agents)
	{
		PrepareCircleRadius(circle, static_cast<float>(agent.radius));
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

	if (drawRadius)
	{
		circle.setFillColor(sf::Color(0, 0, 0, 0)); // a gente só liga pro alpha = 0
		circle.setOutlineColor(sf::Color::Yellow);
		circle.setOutlineThickness(-1); // -1 fica para dentro
		PrepareCircleRadius(circle, maxRadius);
		for (const auto& agent : agents)
		{
			circle.setPosition(agent.cur);
			window.draw(circle);
		}
		circle.setOutlineThickness(0);
	}
}

void NavigatorCamposPotenciais::updateTimeStep(float timeStep)
{
	this->timeStep = timeStep;
}

NavigatorCamposPotenciais::NavigatorCamposPotenciais(Simulator2D& simulator2D, float r) :
	maxVel(DEFAULT_AGENT_MAX_VELOCITY),
	simulator2D(simulator2D),
	maxRadius(r),
	drawRadius(true)
{
}
