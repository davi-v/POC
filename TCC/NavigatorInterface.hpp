#pragma once
#include "Agent.hpp"
#include "Goal.hpp"

class NavigatorInterface
{
public:
	NavigatorInterface();
	virtual void addAgent(const Agent2D& agent) = 0;
	virtual void tick() = 0;
	virtual void drawUI() = 0;
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