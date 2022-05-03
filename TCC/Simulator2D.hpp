#pragma once
#include "Agent.hpp"
#include "Goal.hpp"
#include "NavigatorInterface.hpp"

#define SIMULATOR_EXT "tcc"

static constexpr wchar_t const* lFilterPatterns[] = { L"*." SIMULATOR_EXT};


double distanceAgent(const Agent2D& agent, const Goal& goal);
double distanceSquaredAgent(const Agent2D& agent, const Goal& goal);
typedef double(*DistanceFunc)(const Agent2D& agent, const Goal& goal);

static constexpr auto DEFAULT_AGENT_MAX_VELOCITY = 20.f; // em metros/s
static constexpr auto DEFAULT_TICKS_PER_SECOND = 144; // >= 1
static constexpr float DEFAULT_TIME_STEP = 1.f / DEFAULT_TICKS_PER_SECOND;

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

	void createNavigator();

	int tickRate;
	float curTimeStep; // em segundos

	sf::Font font;

	bool editMode;

	enum class LeftClickAction
	{
		Select,
		PlaceAgent,
		PlaceGoal
	} leftClickAction;
	
	enum class NavigatorCheckbox
	{
		rvo2,
		CamposPotenciais
	} curNavigator;

	std::unique_ptr<NavigatorInterface> navigator;
	float navigatorLastTick;

	typedef int DistanceFunctionsUnderlyingType;
	enum class DistanceFunctionsEnum : DistanceFunctionsUnderlyingType
	{
		SumOfEachEdgeDistance,
		SumOfEachEdgeDistanceSquared,
		Count
	} distanceFunctionUsingType;
	static constexpr DistanceFunc DISTANCE_FUNCTIONS[]
	{
		distanceAgent,
		distanceSquaredAgent
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

	sf::Clock globalClock;
	std::deque<std::pair<const char*, sf::Time>> warnings;
	void addMessage(const char* msg);

	bool loadAgentsAndGoals(const std::wstring& path);
	void saveAgentsAndGoals(const std::wstring& path);

	sf::Text textPopUpMessages;
	std::vector<Agent2D> agents;
	std::vector<Goal> goals;
	sf::CircleShape circle;
	sf::RenderWindow& window;
	float zoomLevel;

	int lastPxClickedX, lastPxClickedY; // for moving the view

	struct LeftClickInterface
	{
		virtual void pollEvent(const sf::Event& event) = 0;
		Simulator2D& sim;
		LeftClickInterface(Simulator2D& sim);
		virtual void draw();
		virtual bool canMoveView();
	};

	struct SelectAction : LeftClickInterface
	{
		void pollEvent(const sf::Event& event) override;
		void draw() override;
		bool canMoveView() override;
		SelectAction(Simulator2D& sim);
		Agent2D* curAgentSelected;
		Coord curOff; // distância de onde clicamos até o centro do círculo
		bool isHoldingLeftClick;
	};

	struct AddItemAction : LeftClickInterface
	{
		void pollEvent(const sf::Event& event) override;
		AddItemAction(Simulator2D& sim);
		bool didNotMoveSinceLastPress;

		// world coordinates x, y
		virtual void placeItem(double x, double y) = 0;
	};
	struct AddAgentAction : AddItemAction
	{
		void placeItem(double x, double y) override;
		AddAgentAction(Simulator2D& sim);
	};
	struct AddGoalAction : AddItemAction
	{
		void placeItem(double x, double y) override;
		AddGoalAction(Simulator2D& sim);
	};

	std::unique_ptr<LeftClickInterface> leftClickInterface;

	float zoomFac;
	void updateZoomFacAndViewSizeFromZoomLevel(sf::View& view);

	static constexpr auto N_SCROLS_TO_DOUBLE = 4;

public:
	Simulator2D(sf::RenderWindow& window);
	void addAgent(const Agent2D& agent);
	void addGoal(const Goal& coord);
	void draw();
	void pollEvent(const sf::Event& event);
};