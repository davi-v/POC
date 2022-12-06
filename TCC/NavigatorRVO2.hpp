#pragma once
#include "NavigatorInterface.hpp"
#include "Simulator2D.hpp"
#include "ViewerBase.hpp"

typedef std::vector<Agent> Agents;

class NavigatorRVO2 : public NavigatorInterface
{
	sf::Color agentColor, goalColor, dstLineColor;
	sf::Color getColor(float x, float y) const;
	void readd();
	ViewerBase* viewerBase;
	sf::RenderWindow& w;
	void dumpCSV();

	std::vector<vec2d> getAgentPositions() override;

	void addAgentImpl(const Agent& agent);

	Agents agents;

	bool
		drawGoals,
		drawTrajectories;

	std::vector<std::vector<vec2f>> coordsThroughTime;

	static constexpr float DEFAULT_TIME_HORIZON = 60;
	static constexpr float DEFAULT_TIME_HORIZON_OBST = DEFAULT_TIME_HORIZON;

	float neighbourDist;
	size_t maxNeighbours = 1;
	float timeHorizon = DEFAULT_TIME_HORIZON;
	float timeHorizonObst = DEFAULT_TIME_HORIZON_OBST;

	float maxSpeed = DEFAULT_AGENT_MAX_VELOCITY;

	float accel = maxSpeed / .4f;
	float decel = maxSpeed / .4f;

	std::unique_ptr<RVO::RVOSimulator> rvoSim;

	void init();

	NavigatorRVO2(sf::RenderWindow& w, ViewerBase* viewerBase);

public:
	void restart();
	void addAgent(const Agent& agent) override;
	void tick() override;
	void drawUIExtra() override;
	void draw() override;
	void updateTimeStep(float timeStep) override;

	// não chame com agents vazio
	NavigatorRVO2(Simulator2D& sim);
	NavigatorRVO2(ViewerBase& viewerBase, Agents& agents);

	template<class C>
	NavigatorRVO2(sf::RenderWindow& w, ViewerBase* v, const C& a) :
		NavigatorRVO2(w, v)
	{
		agents.assign(a.begin(), a.end());
		init();
	}
};