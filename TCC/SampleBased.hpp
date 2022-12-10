#pragma once
#include "CommonEditor.hpp"
#include "NavigatorRVO2.hpp"

class SampleBased : public CommonEditor
{
	static constexpr wchar_t const* GRAPH_FORMAT_EXTS[]{ L"*.txt" };
	static constexpr auto GRAPH_FORMAT_LENS = sizeof(GRAPH_FORMAT_EXTS) / sizeof(*GRAPH_FORMAT_EXTS);

	void onAdd() override;
	void onDelete() override;
	void onChangeRadius() override;
	void drawExtraCommonEditor() override;
	void onImgChangeImpl() override;

	bool shouldAddEdge(size_t i, size_t j) const;

	Agents recalculateGoalsGetAgents();

	void recreateNav();

	void saveCoordsFromNav();
	void dumpGraph();
	void dumpGraph2();
	void importAssignment();
	void importAssignment2();

	sf::RenderWindow& accessW();

	using P = std::pair<size_t, size_t>;
	typedef std::deque<P> GraphV; // índice do robô, índice do ponto
	typedef GraphV GraphE; // índices do GraphV

	std::vector<Goal> goals;
	sf::Color goalColor;

protected:

	static constexpr size_t DEFAULT_N_SAMPLES = 1000; // > 0

	std::deque<sf::Vector2f> orgVertices;
	unsigned
		robotsSeed,
		nRobots;
	GraphV gV, gE;
	CellAreaCalculator cellAreaCalculator;
	std::vector<vec2f> samples;
	size_t
		limitedNSamples,
		pointCount;
	double
		ratioMinArea,
		curPolArea,
		minR,
		maxR;
	float samplePointRadius;
	bool
		drawVerticesSampled,
		drawOrgVertices,
		drawRobots,
		drawGoals,
		fillArea,
		highlightHovered;
	sf::Color
		sampleColor,
		highlightColor,
		orgVertexColor;
	std::vector<size_t> targets;
	std::vector<vec2d> curCirclePointCount;
	std::unique_ptr<NavigatorRVO2> nav;

	virtual void drawExtraSampleBased();
	virtual void recalculateSamplesOnly() = 0;
	void recalculateTargetsGreedyMethod();
	double getRobotArea(size_t robotIdx) const;
	double getAreaCovered(size_t i, size_t j);
	void recalculateCirclePointCount();
	Cell getCellFromCoordAndRadius(const vec2d& c, double r);
	double getPolArea(double r);
	virtual void initSamplesAndAllocation() = 0;
	void drawUIImpl() override;

	virtual void drawUISpecific();


	template<class C>
	auto drawV(const C& container, const sf::Color& color)
	{
		auto& w = accessW();
		auto& c = viewerBase.circle;
		c.setFillColor(color);
		PrepareCircleRadius(c, samplePointRadius);
		for (const auto& coord : container)
		{
			c.setPosition(coord);
			w.draw(c);
		}
	}

public:
	SampleBased(ViewerBase& viewerBase);
	virtual ~SampleBased() = default;

	// valores extras dentro da subcategoria Samples
	virtual void samplesExtraDrawUI() = 0;
};