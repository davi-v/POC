#include "Pch.hpp"
#include "PreviewVoronoi.hpp"

#include "Application.hpp"
#include "Utilities.hpp"
#include "DrawUtil.hpp"

//std::vector<vec2d> v;

const char* PreviewVoronoi::getTitle()
{
	return "Voronoi Coverage";
}

void PreviewVoronoi::onImgChangeImpl()
{
	recalculateVoronoi();
}

void PreviewVoronoi::draw()
{
	dt = c.restart().asSeconds();
	auto& w = viewerBase.app.window;
	const auto px = sf::Mouse::getPosition(w);
	const auto mouseCoord = w.mapPixelToCoords(px);

	const auto n = robotCoords.size();
	vec2d mouseCoordD{ mouseCoord.x, mouseCoord.y };
	auto M = std::numeric_limits<double>::max();

	circleHovered = -1;
	if (drawRobots && NotHoveringIMGui())
	{
		// update circleHovered
		for (size_t i = 0; i != n; i++)
		{
			auto d2 = distanceSquared(robotCoords[i], mouseCoordD);
			if (d2 < M && d2 < square(robotRadius[i]))
			{
				circleHovered = i;
				M = d2;
			}
		}
	}

	auto& circle = viewerBase.circle;
	auto drawCircles = [&](bool draw, const std::vector<vec2d>& coords, const sf::Color& color)
	{
		if (draw)
		{
			circle.setFillColor(color);
			for (size_t i = 0, lim = coords.size(); i != lim; i++)
			{
				PrepareCircleRadius(circle, robotRadius[i]);
				circle.setPosition(coords[i]);
				w.draw(circle);
			}
		}
	};
	if (drawVoronoi)
		DrawVoronoiEdges(voronoiInfo.voronoi, lineColor, w);
	drawCircles(drawRobots, robotCoords, sf::Color::Red);
	drawCircles(drawCenters, voronoiInfo.centroids, sf::Color::Blue);
	drawCircles(drawTargets, voronoiInfo.targets, sf::Color::Cyan);

	if (highlightHoveredRobot)
	{
		if (circleHovered != -1)
		{
			circle.setFillColor(sf::Color::Yellow);
			PrepareCircleRadius(circle, robotRadius[circleHovered]);
			circle.setOutlineThickness(-.1f * robotRadius[circleHovered]);
			circle.setPosition(robotCoords[circleHovered]);
			w.draw(circle);
			circle.setOutlineThickness(0);
		}
	}
	if (highlightHoveredCell)
	{
		const auto point = sf::Mouse::getPosition(w);
		const auto coord = w.mapPixelToCoords(point);
		std::cout << coord.x << ' ' << coord.y << '\n';
		const auto index = voronoiInfo.findCell(coord.x, coord.y);
		if (index != -1)
		{
			const auto& c = voronoiInfo.voronoiCells[index];
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
				w.draw(v, 2, sf::Lines);
			}
		}
	}

	if (circleHovered != -1)
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
			changeRadius(1);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
			changeRadius(-1);
	}
}

sf::Vector2f PreviewVoronoi::getMouseCoord()
{
	auto& w = viewerBase.app.window;
	auto point = sf::Mouse::getPosition(w);
	return w.mapPixelToCoords(point);
}

void PreviewVoronoi::clear()
{
	robotCoords.clear();
	clearPointersToRobots();
	recalculateVoronoi();
}

void PreviewVoronoi::clearPointersToRobots()
{
	circleHovered = circleMovingIndex = -1;
}

sf::Vector2f PreviewVoronoi::getHoveredCircleCenter()
{
	const auto& c = robotCoords[circleHovered];
	sf::Vector2f center{ (float)c.x, (float)c.y };
	return center;
}

void PreviewVoronoi::recalculateVoronoi()
{
	voronoiInfo.update(robotCoords);
}

void PreviewVoronoi::pollEvent(const sf::Event& e)
{
	auto& w = viewerBase.app.window;
	if (NotHoveringIMGui())
	{
		if (circleHovered != -1)
		{
			switch (e.type)
			{
			case sf::Event::KeyPressed:
			{
				switch (e.key.code)
				{
				case sf::Keyboard::Delete:
				{
					robotCoords.erase(robotCoords.begin() + circleHovered);
					robotRadius.erase(robotRadius.begin() + circleHovered);
					robotRadiusOffs.erase(robotRadiusOffs.begin() + circleHovered);
					recalculateVoronoi();
					clearPointersToRobots();
				}
				break;
				}
			}
			break;
			}
		}
		switch (e.type)
		{
		case sf::Event::MouseMoved:
		{
			movedSinceLeftClick = true;
			if (circleMovingIndex != -1)
			{
				const auto& [x, y] = e.mouseMove;
				auto point = sf::Mouse::getPosition(w);
				auto newCoord = w.mapPixelToCoords(point);
				auto newPos = newCoord + circleOff;
				vec2d newP{ (double)newPos.x, (double)newPos.y };
				robotCoords[circleMovingIndex] = newP;
				recalculateVoronoi();
			}
		}
		break;
		case sf::Event::MouseButtonPressed:
		{
			switch (e.mouseButton.button)
			{
			case sf::Mouse::Left:
			{
				movedSinceLeftClick = false;
			}
			break;
			case sf::Mouse::Right:
			{
				if (circleHovered != -1)
				{
					circleMovingIndex = circleHovered;
					auto point = sf::Mouse::getPosition(w);
					auto mouseCoord = w.mapPixelToCoords(point);
					circleOff = getHoveredCircleCenter() - mouseCoord;
				}
			}
			break;
			}
		}
		break;
		case sf::Event::MouseButtonReleased:
		{
			switch (e.mouseButton.button)
			{
			case sf::Mouse::Left:
			{
				auto curLeftClick = getMouseCoord();
				if (!movedSinceLeftClick)
					addPoint(curLeftClick);
			}
			break;
			case sf::Mouse::Right:
			{
				circleMovingIndex = -1;
			}
			break;
			}
		}
		break;
		}
	}
}

void PreviewVoronoi::drawUIImpl()
{
	ImGui::Checkbox("Voronoi", &drawVoronoi);
	ColorPicker3U32("Line Color", lineColor, 0);
	ImGui::Checkbox("Robots", &drawRobots);
	ImGui::Checkbox("Centers", &drawCenters);
	ImGui::Checkbox("Targets", &drawTargets);
	ImGui::Checkbox("Highlight Hovered Robot", &highlightHoveredRobot);
	ImGui::Checkbox("Highlight Hovered Cell", &highlightHoveredCell);
	static constexpr float
		MIN_R = .5,
		MAX_R = 32;
	if (ImGui::SliderFloat("Radius", &viewerBase.radius, MIN_R, MAX_R))
	{
		for (size_t i = 0, lim = robotCoords.size(); i != lim; i++)
			robotRadius[i] = robotRadiusOffs[i] * viewerBase.radius;
	}
	if (ImGui::Button("Clear"))
		clear();
	if (!navigator)
		if (ImGui::Button("Navigator"))
			MakeUniquePtr(navigator, *this);
	if (navigator)
	{
		bool opened = true;
		ImGui::Begin("Navigator", &opened);
		if (opened)
			navigator->drawUI();
		else
			navigator.reset();
		ImGui::End();
	}
}

void PreviewVoronoi::changeRadius(float m)
{
	static constexpr float DELTA = 1.f;
	auto& cur = robotRadiusOffs[circleHovered];
	cur += DELTA * dt * m;
	cur = std::clamp(cur, MIN_FRAC, MAX_FRAC);
	robotRadius[circleHovered] = cur * viewerBase.radius;
}

void PreviewVoronoi::addPoint(const sf::Vector2f& coord)
{
	robotCoords.emplace_back(coord.x, coord.y);
	robotRadiusOffs.emplace_back(1.f);
	robotRadius.emplace_back(viewerBase.radius);
	recalculateVoronoi();
}

PreviewVoronoi::PreviewVoronoi(ViewerBase& viewerBase) :
	Previewer(viewerBase),
	drawRobots(true),
	lineColor(sf::Color::White),
	circleMovingIndex(-1),
	drawVoronoi(true),
	drawCenters(true),
	drawTargets(true),
	highlightHoveredRobot(true),
	highlightHoveredCell(false),
	circleHovered(-1),
	movedSinceLeftClick(false),
	voronoiInfo(viewerBase.accessImgViewer())
{
	auto& circle = viewerBase.circle;
	circle.setOutlineColor(sf::Color::Green);
}

PreviewVoronoi::NavigatorInfo::NavigatorInfo(PreviewVoronoi& previewer) :
	previewer(previewer),
	running(false),
	recalculate(false),
	speed(5.f),
	tickOff(1.f / DEFAULT_TICKS_PER_SECOND),
	dt(0.f)
{
}

void PreviewVoronoi::NavigatorInfo::drawUI()
{
	ImGui::SliderFloat("Speed", &speed, .01f, 20.f);
	if (running)
	{
		if (ImGui::Button("Pause"))
			running = false;
	}
	else
	{
		if (ImGui::Button("Resume"))
		{
			running = true;
			clk.restart();
		}
	}
	ImGui::Checkbox("Recalculate", &recalculate);
	if (running)
	{
		dt += clk.restart().asSeconds();
		if (dt >= tickOff)
		{
			dt -= tickOff;
			tick();
		}
	}
}

void PreviewVoronoi::NavigatorInfo::tick()
{
	auto& coords = previewer.robotCoords;
	const auto n = coords.size();
	for (size_t i = 0; i != n; i++)
	{
		const auto& target = previewer.voronoiInfo.targets[i];
		auto& coord = coords[i];
		if (auto off = target - coord)
		{
			const auto lenLeft = length(off);
			const auto walk = std::min(lenLeft, static_cast<double>(tickOff * speed));
			coord += walk / lenLeft * off;
		}
	}
	if (recalculate)
		previewer.recalculateVoronoi();
}