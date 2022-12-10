#pragma once
#include "CommonEditor.hpp"
#include "vec2.hpp"
#include "VoronoiInfo.hpp"

class PreviewVoronoi : public CommonEditor
{
	const char* getTitle() override;
	void onImgChangeImpl() override;
	void drawExtraCommonEditor() override;
	void drawUIImpl() override;

	bool drawRobots;

public:
	// usamos aleatoriedade só para escolher um vetor aleatório para corrigir agentes nas mesmas coordenadas
	static constexpr unsigned SEED = 0;
	std::mt19937 mt;

	bool running;
	bool recalculate;
	float speed;
	float tickOff;
	float dt;
	double
		maxStep,
		invMaxStep;
	sf::Clock clk;
	void tick();
	void recalculateMaxStep();
	sf::Color lineColor;
	std::unique_ptr<VoronoiInfo> voronoiInfo;
	bool
		drawVoronoi,
		drawCenters,
		drawTargets,
		highlightHoveredRobot,
		highlightHoveredCell;

	void clear();
	void recalculateVoronoi();

public:
	PreviewVoronoi(ViewerBase& viewerBase);
	void onAdd() override;
	void onDelete() override;
	void onMove() override;
};