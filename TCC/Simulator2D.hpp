#pragma once

struct Coord
{
	double x, y;
	operator sf::Vector2f() const;
	operator RVO::Vector2() const;
};

#define SIMULATOR_EXT "tcc"
static constexpr wchar_t const* lFilterPatterns[] = { L"*." SIMULATOR_EXT};

static constexpr auto DEFAULT_RADIUS = 30.f; // em metros
static const auto DEFAULT_COLOR = sf::Color::Red; // em metros
static const auto DEFAULT_VELOCITY = 20.f; // em metros/s

typedef Coord Goal;

class Agent2D
{
public:
	Agent2D(const Coord& coord, double radius = DEFAULT_RADIUS, const sf::Color& color = DEFAULT_COLOR);
	Coord coord;
	Goal* goalPtr; // se nullptr, o agente está ocioso (não assigned a nenhum goal)
	double radius;
	sf::Color color;
	void updateGoal(Goal& goal);
};


double distance(const Agent2D& agent, const Goal& goal);
double distanceSquared(const Agent2D& agent, const Goal& goal);
typedef double(*DistanceFunc)(const Agent2D& agent, const Goal& goal);

static constexpr auto WARNING_DURATION = 3.f;
static const auto GOAL_COLOR = sf::Color::Green;

static constexpr float DEFAULT_ZOOM_LEVEL = 0;

typedef std::vector<std::vector<double>> CostMatrix;
typedef std::vector<std::pair<size_t, size_t>> Edges;
class Simulator2D
{
	void restartRVO2();

	static constexpr auto TICKS_PER_SECOND = 128;
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

	bool condCanCompute();
	void tryUpdateGraph();

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
		MinimizeTotalSumOfEdges,
		MinimizeBiggestEdge,
		Both,
		
		Count
	} metric;

	sf::Clock clock;
	std::deque<std::pair<const char*, sf::Time>> warnings;
	void addMessage(const char* msg);

	bool loadAgentsAndGoals(const std::wstring& path);
	void saveAgentsAndGoals(const std::wstring& path);

	sf::Font font;
	sf::Text textPopUpMessages, textMetric;
	std::vector<Agent2D> agents;
	std::vector<Coord> goals;
	sf::CircleShape circle;
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
	void addGoal(const Coord& coord);
	void tick();
	void tickAndDraw();
	void pollEvent(const sf::Event& event);
};