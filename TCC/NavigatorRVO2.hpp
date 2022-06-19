#pragma once
#include "NavigatorInterface.hpp"
#include "Simulator2D.hpp"

class NavigatorRVO2 : public NavigatorInterface
{
	std::vector<vec2d> getAgentPositions() override;

	float timeStep;
	void addAgentImpl(const Agent2D& agent);

	std::vector<const Agent2D*> orgAgents;

	bool drawGoals;

	static constexpr float RADIUS_MULTIPLIER_FACTOR = 1.f;
	static constexpr float DEFAULT_TIME_HORIZON = 5;
	static constexpr float DEFAULT_TIME_HORIZON_OBST = DEFAULT_TIME_HORIZON;

	float neighbourDist;
	size_t maxNeighbours = 1;
	float timeHorizon = DEFAULT_TIME_HORIZON;
	float timeHorizonObst = DEFAULT_TIME_HORIZON_OBST;

	float maxSpeed = DEFAULT_AGENT_MAX_VELOCITY;

	float accel = maxSpeed / .4f;
	float decel = maxSpeed / .4f;

	std::unique_ptr<RVO::RVOSimulator> sim;
	Simulator2D& simulator2D;

	void restart();
	void init();
	void addAgent(const Agent2D& agent) override;
	void tick() override;
	void draw() override;
	void updateTimeStep(float timeStep) override;

public:
	NavigatorRVO2(Simulator2D& s, float neighbourDist);
};