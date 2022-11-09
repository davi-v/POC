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
	sf::Clock c;
	float dt;
	void changeRadius(float m);

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

	

	VoronoiInfo voronoiInfo;
	void recalculateVoronoi();
	
	bool drawVoronoi, drawRobots, drawCenters, drawTargets,
		highlightHoveredRobot,
		highlightHoveredCell;
	
	void pollEvent(const sf::Event& e) override;
	
	
	sf::Vector2f lastClickedCoord;
	static constexpr float
		MAX_FRAC = 10,
		MIN_FRAC = 0.1f;
	std::vector<vec2d> robotCoords; // por enquanto precisa ser sequencial por causa do backend do voronoi
	std::deque<float> robotRadiusOffs, robotRadius;
	void addPoint(const sf::Vector2f& coord);
public:
	PreviewVoronoi(ViewerBase& viewerBase);
};