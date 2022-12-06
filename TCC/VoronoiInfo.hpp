#pragma once
#include "Voronoi.hpp"
#include "Application.hpp"
#include "CellAreaCalculator.hpp"

struct VoronoiInfo : public CellAreaCalculator
{
	size_t findCell(double x, double y);
	VecCoords getTargets(const VecCoords& coords);
	Voronoi voronoi;
	VecCoords targets, centroids;
	std::vector<Cell> voronoiCells;
	
	VoronoiInfo(const ViewerBase& imgViewer);
	void update(const VecCoords& robotCoords);
};