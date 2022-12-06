#pragma once
#include "Previewer.hpp"
#include "CellAreaCalculator.hpp"

class CommonEditor : public Previewer
{
	sf::Clock c;
	void draw() override;
public:
	sf::Color robotColor;
	void clearPointersToRobots();
	void pollEvent(const sf::Event& e) override;
	size_t circleHovered, circleMovingIndex;

	std::vector<vec2d> robotCoords; // por enquanto precisa ser sequencial por causa do backend do voronoi
	std::deque<float> robotRadiusOffs, robotRadius;

	sf::Vector2f lastClickedCoord;
	static constexpr float
		MAX_FRAC = 100,
		MIN_FRAC = 0.1f;

	bool movedSinceLeftClick;

	sf::Vector2f getMouseCoord();

	void addPoint(const sf::Vector2f& coord);
	void changeRadius(float m);
	float dt;
	CommonEditor(ViewerBase& viewerBase);
	sf::Vector2f circleOff;
	sf::Vector2f getHoveredCircleCenter();
	virtual void onAdd(float r);
	virtual void onDelete(float r);
	virtual void onMove();
	virtual void onChangeRadius(float r1, float r2);
	virtual void drawExtra();
};