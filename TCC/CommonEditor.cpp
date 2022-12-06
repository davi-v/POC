#include "Pch.hpp"
#include "CommonEditor.hpp"

#include "Application.hpp"
#include "Utilities.hpp"

void CommonEditor::pollEvent(const sf::Event& e)
{
	auto& w = viewerBase.app->window;
	if (NotHoveringIMGui())
	{
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
				onMove();
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

void CommonEditor::onAdd(float)
{
	
}

void CommonEditor::onDelete(float)
{
}

void CommonEditor::onMove()
{
}

void CommonEditor::onChangeRadius(float , float )
{
}

void CommonEditor::draw()
{
	dt = c.restart().asSeconds();

	circleHovered = -1;
	if (NotHoveringIMGui())
	{
		auto& w = viewerBase.app->window;
		const auto px = sf::Mouse::getPosition(w);
		const auto mouseCoord = w.mapPixelToCoords(px);
		const auto n = robotCoords.size();
		vec2d mouseCoordD{ mouseCoord.x, mouseCoord.y };
		auto M = std::numeric_limits<double>::max();
		for (size_t i = 0; i != n; i++)
		{
			auto d2 = distanceSquared(robotCoords[i], mouseCoordD);
			if (d2 < M && d2 < square(robotRadius[i]))
			{
				circleHovered = i;
				M = d2;
			}
		}
		if (circleHovered != -1)
		{
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
				changeRadius(1);
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
				changeRadius(-1);
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Delete))
			{
				robotCoords.erase(robotCoords.begin() + circleHovered);
				auto prevr = robotRadius[circleHovered];
				robotRadius.erase(robotRadius.begin() + circleHovered);
				robotRadiusOffs.erase(robotRadiusOffs.begin() + circleHovered);
				onDelete(prevr);
				clearPointersToRobots();
			}
		}
	}

	drawExtra();
}

void CommonEditor::drawExtra()
{
}

CommonEditor::CommonEditor(ViewerBase& viewerBase) :
	Previewer(viewerBase),
	robotColor(sf::Color::Red),
	circleMovingIndex(-1),
	circleHovered(-1),
	movedSinceLeftClick(false),
	dt(0)
{
}



sf::Vector2f CommonEditor::getMouseCoord()
{
	auto& w = viewerBase.app->window;
	auto point = sf::Mouse::getPosition(w);
	return w.mapPixelToCoords(point);
}

void CommonEditor::addPoint(const sf::Vector2f& coord)
{
	robotCoords.emplace_back(coord.x, coord.y);
	robotRadiusOffs.emplace_back(1.f);
	robotRadius.emplace_back(viewerBase.radius);
	onAdd(robotRadius.back());
}

void CommonEditor::changeRadius(float m)
{
	static constexpr float DELTA = 10.f;
	auto& cur = robotRadiusOffs[circleHovered];
	cur += DELTA * dt * m;
	cur = std::clamp(cur, MIN_FRAC, MAX_FRAC);
	auto prv = robotRadius[circleHovered];
	const auto& newR = robotRadius[circleHovered] = cur * viewerBase.radius;
	onChangeRadius(prv, newR);
}


void CommonEditor::clearPointersToRobots()
{
	circleHovered = circleMovingIndex = -1;
}

sf::Vector2f CommonEditor::getHoveredCircleCenter()
{
	const auto& c = robotCoords[circleHovered];
	sf::Vector2f center{ (float)c.x, (float)c.y };
	return center;
}
