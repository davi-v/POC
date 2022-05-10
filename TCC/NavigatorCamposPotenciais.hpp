#pragma once
#include "NavigatorInterface.hpp"
#include "Simulator2D.hpp"

class NavigatorCamposPotenciais : public NavigatorInterface
{
	float timeStep;
	Simulator2D& simulator2D;

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
	static constexpr float DEFAULT_CAMPOS_POTENCIAIS_RADIUS = Agent2D::DEFAULT_RADIUS * 6;

	void addAgent(const Agent2D& agent) override;
	void tick() override;
	void draw() override;
	void updateTimeStep(float timeStep) override;

public:
	NavigatorCamposPotenciais(Simulator2D& simulator2D);
};