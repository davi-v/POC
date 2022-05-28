#pragma once
#include "vec2.hpp"
#include "Simulator2D.hpp"

class Application
{
	sf::RenderWindow& window;
	Simulator2D simulator;

	bool hexPlotReady = false;

	float backgroundColor[3]{};

	bool isMarked(const sf::Color& c);

public:
	Application(sf::RenderWindow& window);
	void pollEvent(const sf::Event& event);
	void draw();

	static constexpr auto PI = std::numbers::pi_v<float>;

	static constexpr auto SQRT_3 = 1.7320508075688772f;

	static constexpr auto DEFAULT_CIRCLE_RADIUS = 1.f;
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

	// according to SFML's documentation, it supports
// bmp, png, tga, jpg, gif, psd, hdr and pic
	// mas nossa lógica precisa da componente alpha, então alguns formatos tipo jpg não suportam
#define x(s) L"*."#s,
	static constexpr wchar_t const* SFML_SUPPORTED_IMAGE_FORMATS[] = {
		//x(bmp)
		x(png)
		//x(tga)
		//x(jpg)
		x(gif)
		//x(psd)
		//x(hdr)
		//x(pic)
	};
#undef x
	static constexpr auto SFML_SUPPORTED_IMAGE_FORMATS_SIZE = sizeof(SFML_SUPPORTED_IMAGE_FORMATS) / sizeof(decltype(*SFML_SUPPORTED_IMAGE_FORMATS));


	sf::Color hexGridColor = sf::Color::Red;
	sf::Color smallHexColor = sf::Color(255, 255, 0, 100);
	sf::Color hexHighlightColor = sf::Color(0, 0, 255, 170);

	float hexagonSide, hexHeight, hexagonHalfSide,
		hexagonStride, // 1.5 * hexagonSide
		hex2Heights,
		circleRadius,
		curMinHexagonSide, // r / 2 / (sin(60))
		curMaxHexagonSide,
		smallerHexRatio,
		smallerHexHeight,
		smallerHexHeightSquared;

	sf::Color circleBorderColor = sf::Color(255, 0, 0, 170);

	size_t
		hexGridXMax,
		hexGridYMax;
	float imgWF, imgHF;
	sf::Vector2f imageOffset;
	sf::Image currentImage;

	size_t nHexagons;

	int numberOfBars = 20;
	int numberOfBarsPrev = numberOfBars - 1;

	std::vector<size_t> barData;

	bool bestFitCircles = false;
	bool yellowHexagonOnly = true;
	bool clipToConfigurationSpace = false;
	bool drawOrgOffsets = false;
	std::vector<sf::Vertex> offsetsLines;
	sf::Color offsetLinesColor = sf::Color::Green;
	std::vector<sf::Vector2f> circleCenters;

	bool isImageEditorActive = false;
	bool showHexGrid = false;
	bool highlightPixelHovered = false;
	bool highlightHexHovered = false;
	bool showCircleCenters = false;
	bool showCircleBorderOnly = false;

	bool showSmallerHexagon = false;
	sf::Image colorMapImage;
	sf::Texture currentImageTexture, currentFilterTexture, colorMapTexture;
	sf::Sprite currentImageSprite, currentFilterSprite, colorMapSprite;
	bool showImage = true;

	enum class RenderType
	{
		None,
		Image,
		ColorMap,
		CircleColor
	} renderType = RenderType::Image;


	// in local space (i.e. before translating)
	sf::Vector2f getHexagonCenter(int x, int y);

	const vec2f& accessHexagonUAxis(const vec2f& off);

	// pixelCenter in local coordinates of hex grid
	// hx, hy : precomputed hexagon coord that pixelCenter is in
	bool isPixelInsideSmallerHexagon(const sf::Vector2f& pixelCenter, int hx, int hy);

	// depends on circle and hexagon
	void updateSmallHexagonData();

	void updateHexagonAuxiliarVariables();

	void circleRadiusUpdateCallback();

	void signalNeedToUpdateHexPlot();

	void updateHexVars();
	void updateVarsThatDependOnCircleAndHexagon();

	sf::Vector2i getHexagonHovered(sf::Vector2f& coord);

	sf::Vector2i getHexagonHoveredConserving(const sf::Vector2f& coord);

	std::optional<sf::Vector2i> tryGetHexagonHovered(sf::Vector2f& coord);

	// changes hexagon variables internally
	size_t getNumCircles(float curHexSide);

	void recalculateCircleCenters(); // should be called after updateVarsThatDependOnHexagonSideAndCircleRadius, since updateVarsThatDependOnHexagonSideAndCircleRadius can modify hexGridXMax and hexGridYMax

	void recalculateCircleCentersAndPlot();
	
	// in local coordinates
	std::array<sf::Vector2f, 6> getHexagonOffsets();

	// hexagon (0, 0) center is on (0, 0)
	std::array<sf::Vector2f, 6> getHexagonCoords(int x, int y);

	sf::Vector2f getWindowSizeF();

	void controlO();
};