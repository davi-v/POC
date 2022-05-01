#pragma once
#include "NavigatorInterface.hpp"

class NavigatorRVO2 : public NavigatorInterface
{
	static constexpr float EXTRA = 1.2f;
	static constexpr float DEFAULT_NEIGHBOUR_DIST = 150;
	static constexpr size_t DEFAULT_MAX_NEIGHBOURS = 100;
	static constexpr float DEFAULT_TIME_HORIZON = 100;
	static constexpr float DEFAULT_TIME_HORIZON_OBST = DEFAULT_TIME_HORIZON;

	RVO::RVOSimulator sim;
	std::vector<Goal*> goals;
	sf::RenderWindow& window;

	void addAgent(const Agent2D& agent) override;
	void tick() override;
	void draw() override;
	void updateTimeStep(float timeStep) override;

public:
	NavigatorRVO2(sf::RenderWindow& window);
};