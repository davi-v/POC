#pragma once
#include "vec2.hpp"
#include "Simulator2D.hpp"

class Application
{
	class ImgViewer;
	struct ViewerBase
	{
		enum class RenderType
		{
			None,
			Image,
			ColorMap,
			RightColor
		} renderType = RenderType::Image;
		virtual sf::Color getColor(const vec2f& c) = 0;
		virtual void draw() = 0;
		virtual bool pollEvent(const sf::Event& e) = 0;
		virtual ImgViewer& accessImgViewer() = 0;
	};
	class ImgViewer : public ViewerBase
	{
		sf::Color circleColorSFML;
		float circleColorF3[3];

		bool border;

	public:
		ImgViewer& accessImgViewer() override;
		void updatedRadiusCallback();

		// advanced options are the options to open navigator
		void drawUI(bool showAdvancedOptions);

		void drawOpenNavigator();

		void updatedRadiusCallback(float r);
		void drawOverlays(bool justInfo, bool showGoalsExtraCond);
		void updateNewGoalsAndCalculateEdges();
		sf::Vector2f imageOffset, mapBorderCoord, mapBorderSize;
		bool pollEvent(const sf::Event& e) override;
		sf::Color getColor(const vec2f& c) override;
		sf::Image colorMapImage;
		
		void addAgent(const sf::Vector2f& v);

		size_t getNumRobotsAllocated();

		static constexpr auto PI = std::numbers::pi_v<float>;

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



		enum class AllocationType
		{
			EditMode,
			RobotBased
		} allocationType = AllocationType::EditMode;


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

		// updates filter and color map
		void recreateFilterTexture();

		void signalNeedToUpdateHexPlot();
		void updateHexVars();
		void updateVarsThatDependOnCircleAndHexagon();

		sf::Vector2i getHexagonHovered(sf::Vector2f& coord);

		sf::Vector2i getHexagonHoveredConserving(const sf::Vector2f& coord);

		std::optional<sf::Vector2i> tryGetHexagonHovered(sf::Vector2f& coord);

		// changes hexagon variables internally
		size_t getNumCircles(float curHexSide);

		void recalculateGoalPositions(); // should be called after updateVarsThatDependOnHexagonSideAndCircleRadius, since updateVarsThatDependOnHexagonSideAndCircleRadius can modify hexGridXMax and hexGridYMax

		void recalculateCircleCentersAndPlot();

		// in local coordinates
		std::array<sf::Vector2f, 6> getHexagonOffsets();

		// hexagon (0, 0) center is on (0, 0)
		std::array<sf::Vector2f, 6> getHexagonCoords(int x, int y);
		void showNRobotsOption();
		bool highlightPixelHovered = false;
		bool showGoals = true;
		bool showCircleBorderOnly = false;

		float hexagonSide, hexHeight, hexagonHalfSide,
			hexagonStride, // 1.5 * hexagonSide
			hex2Heights,
			r,
			curMinHexagonSide, // r / 2 / (sin(60))
			curMaxHexagonSide,
			smallerHexRatio,
			smallerHexHeight,
			smallerHexHeightSquared;
		Simulator2D sim;
		void calculateBestHexagonSideBasedOnNRobotsAndR();
		

		sf::Color hexGridColor = sf::Color::Red;
		sf::Color smallHexColor = sf::Color(255, 255, 0, 100);
		sf::Color hexHighlightColor = sf::Color(0, 0, 255, 170);
		static constexpr float EPS_HEX_BIN_SEARCH = .1f;
		size_t nRobotsBudget;
		static constexpr size_t DEFAULT_N_ROBOTS = 100;
		sf::Color circleBorderColor = sf::Color(255, 0, 0, 170);
		size_t
			hexGridXMax,
			hexGridYMax;
		float imgWF, imgHF;
	
		std::unique_ptr<sf::Image> currentImage;
		size_t nHexagons;
		int numberOfBars = 20;
		int numberOfBarsPrev = numberOfBars - 1;
		std::vector<size_t> barData;
		bool bestFitCircles = true;
		bool yellowHexagonOnly = true;
		bool clipToConfigurationSpace = true;
		bool drawOrgOffsets = false;
		std::vector<sf::Vertex> offsetsLines;
		sf::Color offsetLinesColor = sf::Color::Green;
		std::vector<sf::Vector2f> goalsPositions;
		bool showHexGrid = false;
		bool highlightHexHovered = false;
		bool showSmallerHexagon = false;
		sf::Texture currentImageTexture, currentFilterTexture, colorMapTexture;
		sf::Sprite currentImageSprite, currentFilterSprite, colorMapSprite;

		ImgViewer(Application& app, std::unique_ptr<sf::Image>&& currentImage, float r, size_t nR = DEFAULT_N_ROBOTS);

		void updateRadiusNRobotsImg(float r, size_t n, const sf::Image& img);
		void updateImg(const sf::Image& img);
		void updateImgImpl();

		void draw() override;
		bool hexPlotReady = false;
		bool isMarked(const sf::Color& c);
	};
	struct Sequencer : ViewerBase
	{
		ImgViewer& accessImgViewer() override;
		void handleAutoPlayToggled();
		
		Application& app;
		void calculateGoals(bool resetPositions);
		

		bool canPressPrev();
		bool canPressNext();
		void updateNewImageAndTryStartNavigator(size_t off);
		void prev();
		void next();

		float r = DEFAULT_RADIUS;

		bool autoPlay = false;

		std::unique_ptr<ImgViewer> imgViewer;
		bool pollEvent(const sf::Event& e) override;
		sf::Color getColor(const vec2f& c) override;
		Sequencer(Application& app);
		size_t n = 50; // number of robots
		std::vector<std::unique_ptr<sf::Image>> seqImgs;
		size_t imgIndex = 0;
		void draw() override;

		void processNewImage(bool resetPositions);
	};
	void createSequence();

	sf::Color getSFBackgroundColor();

	float backgroundColor[3]{};




public:
	std::unique_ptr<ViewerBase> viewerBase;
	Application(sf::RenderWindow& window);
	void pollEvent(const sf::Event& event);
	void draw();

	sf::RenderWindow& window;

	sf::Color getColor(const vec2f& c);

	

	sf::Vector2f getWindowSizeF();

	// according to SFML's documentation, it supports
	// bmp, png, tga, jpg, gif, psd, hdr and pic
	// mas nossa l�gica precisa da componente alpha, ent�o alguns formatos tipo jpg n�o suportam
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

	float zoomFac;
	float zoomLevel;
	static constexpr float DEFAULT_ZOOM_LEVEL = 0;
	int lastPxClickedX, lastPxClickedY; // for moving the view
	void controlO();
	void updateZoomFacAndViewSizeFromZoomLevel(sf::View& view);
	static constexpr auto N_SCROLS_TO_DOUBLE = 4;
};