#pragma once
#include "NavigatorInterface.hpp"
#include "Simulator2D.hpp"

class NavigatorCamposPotenciais : public NavigatorInterface
{
	std::vector<vec2d> getAgentPositions() override;

	Simulator2D& sim;

	double maxVel;

	vec2d getAttractionVec(const vec2d& c1, const vec2d& c2);

	struct Object
	{
		double radius;
		vec2d cur, dst;
	};
	std::vector<Object> agents;
	// @distance : precomputed distance between the centers
	vec2d getRepulsionVec(const Object& c1, const Object& c2, double distance);

	bool drawRadius;
	float maxRadius;

	void addAgent(const Agent& agent) override;
	void tick() override;
	void drawUIExtra() override;
	void draw() override;
	void updateTimeStep(float timeStep) override;

public:
	NavigatorCamposPotenciais(Simulator2D& simulator2D, float r);
};