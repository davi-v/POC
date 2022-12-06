#pragma once
#include "CommonEditor.hpp"
#include "vec2.hpp"
#include "PointSelection.hpp"
#include "CGALStuff.hpp"
#include "NavigatorRVO2.hpp"

class PreviewPolygonize : public CommonEditor
{
	std::unique_ptr<NavigatorRVO2> nav;
	std::vector<Agent> agents;
	std::vector<Goal> goals;

	void recalculateCirclePointCount();
	bool shouldDrawExtraLines();
	void recalculateSamples();

	void recalculateSamplesCentroid();
	void recalculateSamplesRandom();

	double getPolArea(double r);
	CellAreaCalculator cellCalculator;
	std::vector<vec2d> curCirclePointCount;
	double curPolArea;

	sf::Color
		circleColor,
		highlightColor;
	static constexpr wchar_t const* GRAPH_FORMAT_EXTS[]{ L"*.txt" };
	static constexpr auto GRAPH_FORMAT_LENS = sizeof(GRAPH_FORMAT_EXTS) / sizeof(*GRAPH_FORMAT_EXTS);
	using P = std::pair<size_t, size_t>;
	typedef std::deque<P> GraphV; // índice do robô, índice do ponto
	typedef GraphV GraphE; // índices do GraphV
	GraphV gV, gE;
	Cell getCellFromCoordAndRadius(const vec2d& c, double r);
	double getRobotArea(size_t robotIdx) const;
	double getAreaCovered(size_t i, size_t j);
	
	bool shouldAddEdge(size_t i, size_t j) const;
	void dumpGraph();
	void dumpGraph2();
	void importAssignment();
	void importAssignment2();
	sf::Color polylineColor;
	float samplePointRadius;
	void onChangeRadius(float r1, float r2) override;
	
	double ratioMinArea;
	void recalculateGreedyMethod();

	std::vector<size_t> targets;
	std::unique_ptr<Alpha_shape_2> alphaShaperPtr;
	CDTP2 cdtp;
	sf::Color orgTriangleColor, simplifiedTriangleColor, orgVertexColor, alphaVertColor, sampleColor;
	std::vector<sf::Vertex> trigs, trigsSimplified;
	std::deque<TriangleD> trianglesForSample;
	std::vector<vec2f> samples;
	std::deque<std::array<vec2f, 2>> sampleLines;
	double alphaShapeRadius;
	std::deque<sf::Vector2f> orgVertices;
	std::list<sf::Vector2f> alphaVertices;
	std::vector<sf::Vector2f> verticesSimplified;

	size_t
		nComponents,
		trigsOrgCnt,
		trigsSimplifiedCnt,
		limitedNSamples;

	enum class SampleMode
	{
		TrigCentroid,
		QuadCentroid,
		Random
	} sampleMode;
	unsigned
		samplesSeed,
		robotsSeed,
		nRobots;

	double
		minR,
		maxR;

	bool
		drawOrgVertices,
		drawVerticesSimplified,
		drawAlphaShapePolylines,
		drawSimplifiedPolylines,
		drawTrigs,
		drawTrigsSimplified,
		drawAlphaVertices,
		drawExtraSampleLines,
		drawVerticesSampled,
		drawRobots,
		drawGoals,
		fillArea,
		highlightHovered;

	double simplificationRatio; // (0, 1]
	struct Seg
	{
		sf::Vector2f p1, p2;
	};
	std::map<Point, std::deque<Point>> g;
	std::deque<Seg> segments;
	std::vector<sf::Vertex> segsSimplified;

	size_t pointCount;

	void recalculate();
	void updateSimplification();
	void drawUIImpl() override;
	void drawExtra() override;
	void onImgChangeImpl() override;
	const char* getTitle() override;
	void onAdd(float r) override;
	void onDelete(float r) override;
public:
	PreviewPolygonize(ViewerBase& viewerBase);
};