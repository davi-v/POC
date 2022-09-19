#include "Pch.hpp"
#include "NavigatorVoronoi.hpp"

#include "Application.hpp"
#include "Simulator2D.hpp"

/*
const auto& off = sim.app.viewerBase->accessImgViewer().imageOffset;
	vec2d doff{ off.x, off.y };*/

std::vector<vec2d> NavigatorVoronoi::getCoords()
{
	std::vector<vec2d> ret;
	for (const auto& coord : robotCoords)
		ret.emplace_back(coord);
	return ret;
}

void NavigatorVoronoi::addAgent(const Agent2D& agent)
{
	robotCoords.emplace_back(agent.coord);
}

void NavigatorVoronoi::tick()
{
	//todo move in direction of center of mass of cell
	robotCoords = getCoords();
}

void NavigatorVoronoi::drawUI()
{ 
	ImGui::Checkbox("Voronoi", &drawVoronoi);
	ImGui::Checkbox("Targets", &drawTargets);
	ImGui::Checkbox("Centers", &drawCenters);
	ImGui::Checkbox("Highlight Hovered", &highlightHovered);
}

void NavigatorVoronoi::draw()
{
	/*
	for (const auto& agent : robotCoords)
		sim.drawAgent(agent);
	*/

	/*
	if (drawCenters)
	{
		voronoiInfo.drawCenters(sim.r);
		sf::CircleShape circle{ sim.r };
		circle.setOrigin(sim.r, sim.r);
		circle.setFillColor(sf::Color::Blue);
		for (const auto& c : voronoiCenters)
		{
			circle.setPosition(c);
			sim.app.window.draw(circle);
		}
	}

	if (drawDestinationLines)
	{
		//	sf::CircleShape c(.5f);
		//	c.setFillColor(sf::Color::Blue);
		//	c.setOrigin(.5f, .5f);
		//{
		//		auto& t = v.front();
		//	c.setPosition(t);
		//	sim.app.window.draw(c);
		//}
		//{
		//	auto& t = v.back();
		//	c.setPosition(t);
		//	sim.app.window.draw(c);
		//}
		const auto n = robotCoords.size();
		for (size_t i = 0; i != n; i++)
		{
			const auto& [x1, y1] = robotCoords[i];
			const auto& [x2, y2] = robotTargets[i];
			sf::Vertex v[2]
			{
				{ {(float)x1, (float)y1}, lineToCenterColor},
				{ {(float)x2, (float)y2}, lineToCenterColor}
			};
			sim.app.window.draw(v, sizeof(v) / sizeof(*v), sf::Lines);
		}
	}

	if (drawTargets)
	{
		sf::CircleShape c{ sim.r };
		c.setOrigin(sim.r, sim.r);
		c.setFillColor(sf::Color::Cyan);
		for (const auto& coord : robotCoords)
		{
			c.setPosition(coord);
			sim.app.window.draw(c);
		}
	}

	if (drawVoronoi)
		sim.drawVoronoiEdges(*voronoi);

	if (highlightHovered)
	{
		auto& w = sim.app.window;
		const auto point = sf::Mouse::getPosition(w);
		const auto coord = w.mapPixelToCoords(point);
		std::cout << coord.x << ' ' << coord.y << '\n';
		const auto index = findCell(coord.x, coord.y);
		if (index != -1)
		{
			const auto& c = voronoiCells[index];
			const auto n = c.size();
			//std::vector<sf::Vertex> v(n);
			//for (size_t i = 0; i != n; i++)
			//{
			//	auto& p = c[i];
			//	v[i] = { {(float)p.x, (float)p.y}, sf::Color::Blue };
			//}
			//w.draw(v.data(), n, sf::TrianglesFan);
			for (size_t i = 0; i != n; i++)
			{
				const auto& p0 = c[i];
				const auto& p1 = c[(i + 1) % c.size()];
				sf::Vertex v[2]
				{
					{ {(float)p0.x, (float)p0.y}, sf::Color::Yellow },
					{ {(float)p1.x, (float)p1.y}, sf::Color::Yellow }
				};
				sim.app.window.draw(v, 2, sf::Lines);
			}
		}
	}
	*/
}

void NavigatorVoronoi::updateTimeStep(float timeStep)
{
	this->timeStep = timeStep;
}

std::vector<vec2d> NavigatorVoronoi::getAgentPositions()
{
	return robotCoords;
}

NavigatorVoronoi::NavigatorVoronoi(Simulator2D& sim) :
	voronoiInfo(sim.accessImgViewer()),
	sim(sim),
	drawVoronoi(true),
	drawTargets(true),
	drawCenters(true),
	highlightHovered(true),
	lineToCenterColor(sf::Color::Yellow)
{
	for (const auto& agent : sim.agents)
		addAgent(*agent);
	voronoiInfo.update(robotCoords);
}

