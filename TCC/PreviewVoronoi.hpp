#pragma once
#include "Previewer.hpp"
#include "vec2.hpp"
#include "VoronoiInfo.hpp"
#include "NavigatorVoronoi.hpp"

class PreviewVoronoi : public Previewer
{
	const char* getTitle() override;
	void onImgChangeImpl() override;
	void draw() override;
	void drawUIImpl() override;

public:
	struct NavigatorInfo
	{
		PreviewVoronoi& previewer;
		bool running;
		bool recalculate;
		float speed;
		float tickOff;
		float dt;
		sf::Clock clk;
		NavigatorInfo(PreviewVoronoi& previewer);
		void drawUI();
		void tick();
	};
	std::unique_ptr<NavigatorInfo> navigator;

	sf::Color lineColor;
	bool movedSinceLeftClick;
	sf::Vector2f getMouseCoord();
	void clear();
	void clearPointersToRobots();
	sf::Vector2f getHoveredCircleCenter();
	size_t circleHovered, circleMovingIndex;
	
	sf::Vector2f circleOff;

	void updatedCircleRadius();
	VoronoiInfo voronoiInfo;
	void recalculateVoronoi();
	
	bool drawVoronoi, drawRobots, drawCenters, drawTargets,
		highlightHoveredRobot,
		highlightHoveredCell;
	
	void pollEvent(const sf::Event& e) override;
	
	
	sf::Vector2f lastClickedCoord;
	std::vector<vec2d> robotCoords;
	void addPoint(const sf::Vector2f& coord);
public:
	PreviewVoronoi(ViewerBase& viewerBase);
};