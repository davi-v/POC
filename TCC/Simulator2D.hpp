#pragma once
#include "NavigatorInterface.hpp"
#include "SelectableEdge.hpp"
#include "Voronoi.hpp"

static constexpr float DEFAULT_RADIUS = 3.f;

#define SIMULATOR_EXT "tcc"
static constexpr wchar_t const* lFilterPatterns[] = { L"*." SIMULATOR_EXT};

typedef double(*DistanceFunc)(const ElementInteractable&, const ElementInteractable&);

double distance1(const ElementInteractable& e1, const ElementInteractable& e2);
double distance2(const ElementInteractable& e1, const ElementInteractable& e2);

static constexpr auto DEFAULT_AGENT_MAX_VELOCITY = 40.f; // em px/s

typedef std::vector<std::vector<double>> CostMatrix;
typedef std::vector<std::pair<size_t, size_t>> Edges;
class Application;

class Simulator2D
{
	sf::Clock clock;

	sf::Color
		colorEdge,
		colorEdgeMaxDistance,
		goalColor,
		defaultAgentColor;

	ElementInteractable* elemHovered;
	std::list<ElementInteractable>::iterator elemHoveredIt;
	enum class TypeHovered
	{
		None,
		Agent,
		Goal
	} typeHovered;
	std::list<ElementInteractable>::iterator
		agentSel,
		goalSel;

	void recalculateElemHovered(const vec2d& coordDouble);
	
	void updateOutlineColor();
	void updateOutlineThickness();


	bool canSaveFile();
	void saveFile();
	void tryOpenFile();

	bool
		useDistancesSquared,
		metricBasedAllocation;
	void updateDistanceSquaredVars();
	bool hasAtLeast1Edge();

	void tryCreateSelectedNavigator();


	enum class AddAction
	{
		Agent,
		Goal,
		Edge
	} addAction;
	
	enum class NavigatorCheckbox
	{
		rvo2,
		CamposPotenciais
	} curNavigator;

public:
	void tryUpdateAllocation();
	
	sf::Color getColor(float x, float y) const;
	sf::Color getColor(const sf::Vector2f& v) const;
	std::unique_ptr<NavigatorInterface> nav;
private:
	typedef bool DistanceFunctionsUnderlyingType; // bool bc only 2
	enum class DistanceFunctionsEnum : DistanceFunctionsUnderlyingType
	{
		SumOfEachEdgeDistance,
		SumOfEachEdgeDistanceSquared
	} distanceFunctionUsingType;
	static constexpr DistanceFunc DISTANCE_FUNCTIONS[]
	{
		distance1,
		distance2
	};
	DistanceFunc distanceFuncUsingCallback;

	double currentMaxEdge, currentMaxEdgeSquared;
	double currentTotalSumOfEachEdgeDistance, currentTotalSumOfEachEdgeDistanceSquared, variance;

	// this function updates currentTotalSumOfEachEdgeDistance and currentTotalSumOfEachEdgeDistanceSquared
	// if needsToUpdateMaxEdge, also updates currentMaxEdge
	void updateAgentsGoalsAndInternalVariablesWithHungarianAlgorithm(const CostMatrix& costMatrix, bool needsToUpdateMaxEdge);

	// depending on the internal distance function using set, takes each distance squared or not
	CostMatrix getHungarianCostMatrix() const;

	Edges updateInternalVariablesMetricEdgesMinimizingBiggestEdge();

	enum class Metric // must be int bc of imgui
	{
		MinimizeTotalSumOfEdges,
		MinimizeBiggestEdge,
		MinMaxEdgeThenSum
	} metric;

	float outlineColor[3]{1, 1, 1};

	bool loadAgentsAndGoals(const std::wstring& path);
	void saveAgentsAndGoals(const std::wstring& path);

	double getDistanceSquaredToEdge(const SelectableEdge& e, const vec2d& c);

	bool usingOutline;

	vec2d getCoordDouble(int x, int y);
	void addEdge(Agent& agent, Goal& goal);

	struct EventInterface
	{
		using It = std::list<ElementInteractable>::iterator;
		std::optional<std::pair<It, TypeHovered>> tryGetElementHovered(const vec2d& coordDouble);
		vec2d curOff; // distância de onde clicamos até o centro do círculo
		bool
			isHoldingLeftClick,
			isHoldingRightClick;

		Simulator2D& sim;
		EventInterface(Simulator2D& sim);

		virtual void draw();

		virtual void pollEventExtra(const sf::Event& event) = 0;
		void pollEvent(const sf::Event& event);
	};

	class AddEdgeAction : public EventInterface
	{
		void clear();
		ElementInteractable* elemSelected;
		Agent* curAgent;
		Goal* curGoal;

		void pollEventExtra(const sf::Event& event) override;
		void draw() override;

	public:
		AddEdgeAction(Simulator2D& sim);
	};

	struct AddItemAction : EventInterface
	{
		void pollEventExtra(const sf::Event& event) override;
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

	std::unique_ptr<EventInterface> eventInterface;

public:
	// ponteiros pq por exemplo, a adição das arestas salva ponteiros, e a deleção pode mudar
	std::list<Agent> agents;
	std::list<Goal> goals;

	Application& app;
	Simulator2D(Application* app, float radius);
	void addAgent(const vec2d& c);
	void addAgent(float x, float y);
	void addAgent(const sf::Vector2f& c);
	void addGoal(const vec2d& coord);
	

	bool drawUI();
	void draw();

	void pollEvent(const sf::Event& event);
	void updateGraphEdges();


	float percentageOutline = .1f;
	sf::CircleShape circle;
	float radius;

	std::vector<vec2d> getAgentsCoordinatesFromNavigator();

	void recreateNavigatorAndPlay();
	void quitNavigator();
	void clearAll();

	
};