#pragma once
#include "NavigatorInterface.hpp"
#include "VoronoiInfo.hpp"

class Simulator2D;

class NavigatorVoronoi : public NavigatorInterface
{
	VoronoiInfo voronoiInfo;
	
	bool drawCenters;
	sf::Color lineToCenterColor;
	
	std::vector<vec2d> getCoords();
	std::vector<vec2d> robotCoords;
	bool drawVoronoi, drawTargets, highlightHovered;
	Simulator2D& sim;
	void addAgent(const Agent2D& agent) override;
	void tick() override;
	void drawUI() override;
	void draw() override;
	void updateTimeStep(float timeStep) override;
	std::vector<vec2d> getAgentPositions() override;
public:
	NavigatorVoronoi(Simulator2D& sim);
};