#pragma once
#include "Agent.hpp"
#include "Goal.hpp"
#include "NavigatorInterface.hpp"
#include "SelectableEdge.hpp"

static constexpr float DEFAULT_RADIUS = 15.f;

#define SIMULATOR_EXT "tcc"

static constexpr wchar_t const* lFilterPatterns[] = { L"*." SIMULATOR_EXT};

double distanceAgent(const Agent2D& agent, const Goal& goal);
double distanceSquaredAgent(const Agent2D& agent, const Goal& goal);
typedef double(*DistanceFunc)(const Agent2D& agent, const Goal& goal);

static constexpr auto DEFAULT_AGENT_MAX_VELOCITY = 20.f; // em px/s
static constexpr auto DEFAULT_TICKS_PER_SECOND = 144; // >= 1
static constexpr float DEFAULT_TIME_STEP = 1.f / DEFAULT_TICKS_PER_SECOND;

typedef std::vector<std::vector<double>> CostMatrix;
typedef std::vector<std::pair<size_t, size_t>> Edges;
class Application;
class Simulator2D
{
	void updateOutlineColor();
	void updateOutlineThickness();

	static constexpr auto WARNING_DURATION = 3.f;
	static const sf::Color DEFAULT_GOAL_COLOR;

	bool canSaveFile();
	void saveFile();
	void tryOpenFile();

	void defaultDraw();

	bool usingDistancesSquared = false;
	void updateDistanceSquaredVars();
	bool hasAtLeast1Edge();

	void createSelectedNavigator();

	int tickRate;
	float curTimeStep; // em segundos

	sf::Font font;


	enum class LeftClickAction
	{
		Select,
		PlaceAgent,
		PlaceGoal,
		AddEdge
	} leftClickAction;
	
	enum class NavigatorCheckbox
	{
		rvo2,
		CamposPotenciais
	} curNavigator;

	std::unique_ptr<NavigatorInterface> navigator;
	float navigatorLastTickTimestamp;
	float navigatorAccTs;

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

	float outlineColor[3]{1, 1, 1};

	bool loadAgentsAndGoals(const std::wstring& path);
	void saveAgentsAndGoals(const std::wstring& path);

	double getDistanceSquaredToEdge(const SelectableEdge& e, const vec2d& c);

	sf::Text textPopUpMessages;
	
	std::vector<std::unique_ptr<Goal>> goals;
	bool usingOutline;

	vec2d getCoordDouble(int x, int y);
	void addEdge(Agent2D& agent, const Goal& goal);


	struct LeftClickInterface
	{
		Simulator2D& sim;
		LeftClickInterface(Simulator2D& sim);

		virtual void draw();
		virtual bool canMoveView();

		virtual bool pollEvent(const sf::Event& event) = 0;
	};

	class AddEdgeAction : public LeftClickInterface
	{
		void clear();
		ElemSelected* elemHoveredPtr;
		enum class Type
		{
			Agent,
			Goal
		} typeHovered;

		Agent2D* curAgent;
		Goal* curGoal;

		bool goalSelected;

		bool pollEvent(const sf::Event& event) override;
		void draw() override;

	public:
		AddEdgeAction(Simulator2D& sim);
	};

	struct SelectAction : LeftClickInterface
	{
		bool pollEvent(const sf::Event& event) override;
		void draw() override;
		bool canMoveView() override;
		SelectAction(Simulator2D& sim);

		ElemSelected* elemSelected;

		vec2d curOff; // distância de onde clicamos até o centro do círculo
		bool isHoldingLeftClick;
	};

	struct AddItemAction : LeftClickInterface
	{
		bool pollEvent(const sf::Event& event) override;
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



	static constexpr float N_DIAMETERS_SAFER = 3;

public:
	std::vector<std::unique_ptr<Agent2D>> agents; // usamos ponteiros para não mudar o endereço do objeto apontado quando o vetor é resized
	Application* app;
	Simulator2D(Application* app, float r);
	void addAgent(const vec2d& c);
	void addAgent(float x, float y);
	void addGoal(const vec2d& coord);
	void draw();
	bool pollEvent(const sf::Event& event);
	void clear();
	void updateGraphEdges();

	sf::Color getColor(float x, float y);

	enum class EditModeType
	{
		Free,
		Metric,
		Navigation
	} editModeType;

	float percentageOutline = .1f;

	float r;

	sf::CircleShape circle;

	void leaveNavigatorUpdatingAgentsCoordinates();
};