#pragma once
#include "Voronoi.hpp"
#include "Application.hpp"

struct VoronoiInfo
{
	void mergePrevious(vec2d& a, double& b, const vec2d& newA, const double& newB);
	size_t findCell(double x, double y);
	sf::Rect<double> border;
	std::vector<vec2d> getTargets();
	Voronoi voronoi;
	std::vector<vec2d> targets, centroids;
	std::vector<std::vector<std::pair<double, double>>> prefixCols;
	std::vector<Cell> voronoiCells;
	const ImgViewer& imgViewer;
	VoronoiInfo(const ImgViewer& imgViewer);
	void update(const std::vector<vec2d>& robotCoords);
	vec2d getWeightedMeanOfCoords(size_t cell);
	double getPixelW(unsigned i, unsigned j);
	std::pair<vec2d, double> getCentroidTimesMassAndMassBelowSegment(vec2d a, const vec2d& b);
};