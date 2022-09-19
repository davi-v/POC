#pragma once
#include "Voronoi.hpp"
#include "Application.hpp"

struct VoronoiInfo
{
	//std::vector<vec2d[4]> fixme;
	//std::vector<std::array<vec2d, 4>> fixme;

	void mergePrevious(vec2d& a, double& b, const vec2d& newA, const double& newB);
	size_t findCell(double x, double y);
	sf::Rect<double> border;
	std::vector<vec2d> getTargets();
	Voronoi voronoi;
	std::vector<vec2d> targets, centroids;
	std::vector<std::vector<std::pair<vec2d, double>>> prefixCols;
	std::vector<Cell> voronoiCells;
	const ImgViewer& imgViewer;
	VoronoiInfo(const ImgViewer& imgViewer);
	void update(const std::vector<vec2d>& robotCoords);
	vec2d getWeightedMeanOfCoords(size_t cell);
	double getPixelW(unsigned i, unsigned j);
	std::pair<vec2d, double> getCentroidAndMassBelowSegment(vec2d a, vec2d b);
};