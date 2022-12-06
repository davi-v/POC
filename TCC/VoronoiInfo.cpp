#include "Pch.hpp"
#include "VoronoiInfo.hpp"

VoronoiInfo::VoronoiInfo(const ViewerBase& imgViewer) :
	CellAreaCalculator(imgViewer),
	voronoi(imgViewer.getBB())
{
}

void VoronoiInfo::update(const std::vector<vec2d>& coords)
{
	auto n = coords.size();
	voronoiCells.resize(n);

	auto& v = voronoi;

	v.update(coords);
	v.fillCellEdges(voronoiCells);

	centroids = CalculateCentroids(voronoiCells);
	targets = getTargets(coords);
}
std::vector<vec2d> VoronoiInfo::getTargets(const VecCoords& coords)
{
	const auto n = voronoiCells.size();
	std::vector<vec2d> ret(n);
	for (size_t i = 0; i != n; i++)
		if (auto optCenterOfGravity = tryGetCenterOfGravity(voronoiCells[i]))
			ret[i] = *optCenterOfGravity;
		else
			ret[i] = coords[i];
	return ret;
}

size_t VoronoiInfo::findCell(double x, double y)
{
	const auto n = voronoiCells.size();
	for (size_t i = 0; i != n; i++)
		if (IsInsideConvexShape({ x, y }, voronoiCells[i]))
			return i;
	return -1;
}