#pragma once
#include "Agent.hpp"
#include "Goal.hpp"

#define SIMULATOR_EXT "tcc"

static constexpr wchar_t const* lFilterPatterns[] = { L"*." SIMULATOR_EXT};

static const auto DEFAULT_VELOCITY = 20.f; // em metros/s

double distance(const Agent2D& agent, const Goal& goal);
double distanceSquared(const Agent2D& agent, const Goal& goal);
typedef double(*DistanceFunc)(const Agent2D& agent, const Goal& goal);


typedef std::vector<std::vector<double>> CostMatrix;
typedef std::vector<std::pair<size_t, size_t>> Edges;
class Simulator2D
{
	static constexpr auto WARNING_DURATION = 3.f;
	static constexpr float DEFAULT_ZOOM_LEVEL = 0;
	static const sf::Color DEFAULT_GOAL_COLOR;

	bool usingDistancesSquared;
	void updateDistanceSquaredVars();
	bool hasAtLeast1Edge();
	void restartRVO2();

	int tickRate;

	sf::Font font;

	static constexpr auto TICKS_PER_SECOND = 144;
	static constexpr float TIME_STEP = 1.f / TICKS_PER_SECOND;

	void updatePreferredVelocities();
	bool reachedGoal();

	std::unique_ptr<RVO::RVOSimulator> sim;

	typedef int DistanceFunctionsUnderlyingType;
	enum class DistanceFunctionsEnum : DistanceFunctionsUnderlyingType
	{
		SumOfEachEdgeDistance,
		SumOfEachEdgeDistanceSquared,
		Count
	} distanceFunctionUsingType;
	static constexpr DistanceFunc DISTANCE_FUNCTIONS[]
	{
		distance,
		distanceSquared
	};
	DistanceFunc distanceFuncUsingCallback;

	// handles graph with no edges
	void tryUpdateGraph();

	// doesn't handle graph with no edges
	void updateGraph();

	double currentMaxEdge, currentMaxEdgeSquared;
	double currentTotalSumOfEachEdgeDistance, currentTotalSumOfEachEdgeDistanceSquared;

	// this function updates currentTotalSumOfEachEdgeDistance and currentTotalSumOfEachEdgeDistanceSquared
	// if needsToUpdateMaxEdge, also updates currentMaxEdge
	void updateAgentsGoalsAndInternalVariablesWithHungarianAlgorithm(CostMatrix& costMatrix, bool needsToUpdateMaxEdge);

	// depending on the internal distance function using set, takes each distance squared or not
	CostMatrix getHungarianCostMatrix();

	Edges getEdgesMinimizingBiggestEdgeAndUpdateInternalVariables();

	enum class Metric
	{
		MinimizeBiggestEdge,
		MinimizeTotalSumOfEdges,
		MinMaxEdgeThenSum,
		
		Count
	} metric;

	sf::Clock clock;
	std::deque<std::pair<const char*, sf::Time>> warnings;
	void addMessage(const char* msg);

	bool loadAgentsAndGoals(const std::wstring& path);
	void saveAgentsAndGoals(const std::wstring& path);

	sf::Text textPopUpMessages;
	std::vector<Agent2D> agents;
	std::vector<Goal> goals;
	sf::CircleShape circle;
	static constexpr auto DEFAULT_GOAL_RADIUS = 15.f;
	sf::RenderWindow& window;
	float zoomLevel;

	int lastLeftX, lastLeftY;
	int lastPxClickedX, lastPxClickedY;
	int lastRightX, lastRightY;

	float zoomFac;
	void updateZoomFacAndViewSizeFromZoomLevel(sf::View& view);

	static constexpr auto N_SCROLS_TO_DOUBLE = 4;

public:
	Simulator2D(sf::RenderWindow& window);
	void addAgent(const Agent2D& agent);
	void addGoal(const Goal& coord);
	void tickAndDraw();
	void pollEvent(const sf::Event& event);
};