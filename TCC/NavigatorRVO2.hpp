#pragma once
#include "NavigatorInterface.hpp"
#include "Simulator2D.hpp"

class NavigatorRVO2 : public NavigatorInterface
{
	static constexpr float EXTRA = 1.2f;
	static constexpr float DEFAULT_NEIGHBOUR_DIST = 150;
	static constexpr size_t DEFAULT_MAX_NEIGHBOURS = 100;
	static constexpr float DEFAULT_TIME_HORIZON = 100;
	static constexpr float DEFAULT_TIME_HORIZON_OBST = DEFAULT_TIME_HORIZON;

	RVO::RVOSimulator sim;
	std::vector<const Goal*> goals;
	Simulator2D& simulator2D;

	void addAgent(const Agent2D& agent) override;
	void tick() override;
	void draw() override;
	void updateTimeStep(float timeStep) override;

public:
	NavigatorRVO2(Simulator2D& simulator2D);
};