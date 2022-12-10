#pragma once
#include "NavigatorInterface.hpp"
#include "Simulator2D.hpp"
#include "ViewerBase.hpp"

typedef std::vector<Agent> Agents;

class NavigatorRVO2 : public NavigatorInterface
{
	void setObservationRadii();

	sf::Color
		agentColor,
		goalColor,
		dstLineColor;
	ViewerBase* viewerBase;
	sf::RenderWindow& w;
public:
	Agents agents;
private:
	bool
		usingPredefinedObservationRadius,
		drawGoals,
		drawTrajectories;
	std::vector<std::vector<vec2f>> coordsThroughTime;
	size_t maxNeighbours;
	float
		maxRadius,
		neighbourDist,
		timeHorizon,
		maxSpeed,
		accel,
		decel;
	std::unique_ptr<RVO::RVOSimulator> rvoSim;

	sf::Color getColor(float x, float y) const;
	void readd();
	void dumpCSV();
	void addAgentImpl(const Agent& agent);
	void init();
	NavigatorRVO2(sf::RenderWindow& w, ViewerBase* viewerBase);

public:
	std::vector<vec2d> getAgentPositions() override;
	void restart();
	void addAgent(const Agent& agent) override;
	void tick() override;
	void drawUIExtra() override;
	void draw() override;
	void updateTimeStep(float timeStep) override;

	// não chame com agents vazio
	NavigatorRVO2(Simulator2D& sim);
	NavigatorRVO2(ViewerBase& viewerBase, const Agents& agents);

	template<class C>
	NavigatorRVO2(sf::RenderWindow& w, ViewerBase* v, const C& a) :
		NavigatorRVO2(w, v)
	{
		agents.assign(a.begin(), a.end());
		init();
	}
};