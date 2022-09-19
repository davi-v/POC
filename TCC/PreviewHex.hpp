#pragma once
#include "Previewer.hpp"
#include "vec2.hpp"
#include "Utilities.hpp"
#include "Simulator2D.hpp"

class PreviewHex : public Previewer
{
	std::unique_ptr<Simulator2D> sim;
	enum class AllocationType
	{
		Unlimited,
		Limited
	} allocationType;

	void drawUIImpl() override;

	static constexpr auto SQRT_3 = 1.7320508075688772f;
	static constexpr auto DEFAULT_CIRCLE_RADIUS = 15.f;
	static constexpr auto SIN_60 = 0.8660254037844386f;
	static constexpr auto COS_60 = .5;
	static constexpr auto TAN_60 = SQRT_3;
	static constexpr auto SIN_30 = COS_60;
	static constexpr auto COS_30 = SIN_60;
	static constexpr auto INV_SIN_60 = 1 / SIN_60;
	static constexpr auto SIXTY_DEG = PI / 3;
	static constexpr float HEX_ANGLES[]
	{
		SIXTY_DEG,
		SIXTY_DEG * 2,
		SIXTY_DEG * 3,
		SIXTY_DEG * 4,
		SIXTY_DEG * 5,
	};
	static constexpr vec2f HEX_AXIS[]
	{
		{COS_30, SIN_30},
		{0, 1},
		{-COS_30, SIN_30},
		{-COS_30, -SIN_30},
		{0, -1},
		{COS_30, -SIN_30},
	};

	// in local space (i.e. before translating)
	sf::Vector2f getHexagonCenterInLocalSpace(int x, int y);
	sf::Vector2f getHexagonCenterInWorldSpace(int x, int y);

	const vec2f& accessHexagonUAxis(const vec2f& off);

	// pixelCenter in local coordinates of hex grid
	// hx, hy : precomputed hexagon coord that pixelCenter is in
	bool isPixelInsideSmallerHexagon(const sf::Vector2f& pixelCenter, int hx, int hy);

	// depends hexHeight and radius
	void updateSmallHexagonDataThatDependOnHexHeightAndRadius();
	
	void updateHexagonAuxiliarVariables();
	void signalNeedToUpdateHexPlot();
	void updateHexVars();
	void updateVarsThatDependOnCircleAndHexagon();

	sf::Vector2i getHexagonHovered(sf::Vector2f& coord);

	sf::Vector2i getHexagonHoveredConserving(const sf::Vector2f& coord);

	std::optional<sf::Vector2i> tryGetHexagonHovered(sf::Vector2f& coord);
	
	sf::Vector2f mapBorderCoord, mapBorderSize;
	
	void updatedRadiusCallback();
	void updatedRadius();
	
	void onImgChangeImpl() override;
	void pollEvent(const sf::Event& e) override;
	
	size_t getNumRobotsAllocated();

	// changes hexagon variables internally
	size_t getNumCirclesUsingThisHexagonSide(float curHexSide);

	void recalculateGoalPositions(); // should be called after updateVarsThatDependOnHexagonSideAndCircleRadius, since updateVarsThatDependOnHexagonSideAndCircleRadius can modify hexGridXMax and hexGridYMax

	void recalculateCircleCentersAndPlot();

	// in local coordinates
	std::array<sf::Vector2f, 6> getHexagonOffsets();

	// hexagon (0, 0) center is on (0, 0)
	std::array<sf::Vector2f, 6> getHexagonCoordsInLocalSpace(int x, int y);
	void showNRobotsOption();
	void calculateBestHexagonSideBasedOnNRobotsAndR();


	void draw() override;
	const char* getTitle() override;

	static constexpr float EPS_HEX_BIN_SEARCH = .1f;
	static constexpr size_t DEFAULT_N_ROBOTS = 100;

	bool
		showGoals,
		hexPlotReady,
		showCircleBorderOnly;
	
	bool drawHexGridBorder;
	float
		hexagonSide,
		hexHeight,
		hexagonHalfSide,
		hexagonStride, // 1.5 * hexagonSide
		hex2Heights,
		curMinHexagonSide, // r / 2 / (sin(60))
		curMaxHexagonSide,
		smallerHexRatio,
		smallerHexHeight,
		smallerHexHeightSquared;
	sf::Color hexGridColor = sf::Color::Red;
	sf::Color smallHexColor = sf::Color(255, 255, 0, 100);
	sf::Color hexHighlightColor = sf::Color(0, 0, 255, 170);
	size_t nRobotsBudget;
	sf::Color circleBorderColor = sf::Color(255, 0, 0, 170);
	size_t
		hexGridXMax,
		hexGridYMax;
	size_t nHexagons;
	int
		numberOfBars,
		numberOfBarsPrev;
	std::vector<size_t> barData;
	bool
		bestFitCircles,
		usePixelsInConfigurationSpaceOnly,
		clipToConfigurationSpace,
		drawOrgOffsets,
		showHexGrid,
		highlightHexHovered,
		showSmallerHexagon;
	std::vector<sf::Vertex> offsetsLines;
	sf::Color offsetLinesColor = sf::Color::Green;
	std::vector<sf::Vector2f> goalsPositions;

public:
	PreviewHex(ViewerBase& viewerBase);
};