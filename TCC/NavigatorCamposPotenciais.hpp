#pragma once
#include "NavigatorInterface.hpp"

class NavigatorCamposPotenciais : public NavigatorInterface
{
	float timeStep;
	sf::RenderWindow& window;

	Coord getAttractionVec(const Coord& c1, const Coord& c2);

	struct Object
	{
		double radius;
		Coord cur, dst;
	};
	std::vector<Object> agents;
	// @distance : precomputed distance between the centers
	Coord getRepulsionVec(const Object& c1, const Object& c2, double distance);

	bool drawRadius;
	float maxRadius;
	static constexpr float DEFAULT_CAMPOS_POTENCIAIS_RADIUS = Agent2D::DEFAULT_RADIUS * 6;

	void addAgent(const Agent2D& agent) override;
	void tick() override;
	void draw() override;
	void updateTimeStep(float timeStep) override;

public:
	NavigatorCamposPotenciais(sf::RenderWindow& window);
};