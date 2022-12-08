#pragma once
#include "SampleBased.hpp"
#include "vec2.hpp"
#include "PointSelection.hpp"
#include "CGALStuff.hpp"

static constexpr auto POLYGONIZATION = "Polygonization";

class PreviewPolygonize : public SampleBased
{
	void drawUISpecific() override;
	void samplesExtraDrawUI() override;

	
	

	void recalculateSamplesAndAllocation() override;
	

	
	
	
	

	sf::Color
		circleColor, polylineColor;
	
	

	std::unique_ptr<Alpha_shape_2> alphaShaperPtr;
	CDTP2 cdtp;
	sf::Color
		orgTriangleColor,
		simplifiedTriangleColor,
		alphaVertColor;
	std::vector<sf::Vertex> trigs, trigsSimplified;
	std::deque<TriangleD> trianglesForSample;
	
	std::deque<std::array<vec2f, 2>> sampleLines;
	double alphaShapeRadius;
	
	std::list<sf::Vector2f> alphaVertices;
	std::vector<sf::Vector2f> verticesSimplified;

	size_t
		nComponents,
		trigsOrgCnt,
		trigsSimplifiedCnt;

	enum class SampleMode
	{
		TrigCentroid,
		QuadCentroid
	} sampleMode;

	bool
		drawVerticesSimplified,
		drawAlphaShapePolylines,
		drawSimplifiedPolylines,
		drawTrigs,
		drawTrigsSimplified,
		drawAlphaVertices,
		drawExtraSampleLines;

	double simplificationRatio; // (0, 1]
	struct Seg
	{
		sf::Vector2f p1, p2;
	};
	std::map<Point, std::deque<Point>> g;
	std::deque<Seg> segments;
	std::vector<sf::Vertex> segsSimplified;

	void recalculateTriangulations();
	void updateSimplification();
	void drawExtraSampleBased() override;
	const char* getTitle() override;
	
public:
	PreviewPolygonize(ViewerBase& viewerBase);
};