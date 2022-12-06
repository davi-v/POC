#pragma once
#include "ElemSelected.hpp"

class NavigatorInterface
{
public:
	sf::Color trajectoryColor;
	int tickRate;
	NavigatorInterface();
	virtual void addAgent(const Agent& agent) = 0;
	virtual void tick() = 0;
	virtual void drawUIExtra() = 0;
	bool drawUI();
	virtual void draw() = 0;
	virtual void updateTimeStep(float timeStep) = 0;
	virtual std::vector<vec2d> getAgentPositions() = 0;

	void startNavigating();
	void loopUpdate();

	float timeStep; // em segundos

	bool drawDestinationLines, running;
	float navigatorLastTickTimestamp;
	float navigatorAccTs;
};