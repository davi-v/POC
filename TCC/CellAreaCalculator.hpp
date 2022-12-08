#pragma once
#include "ViewerBase.hpp"
#include "Utilities.hpp"

class CellAreaCalculator
{
	std::vector<std::vector<std::pair<double, double>>> prefixCols;
	const ViewerBase* viewerBase;
	double getPixelWCheckingRange(unsigned x, unsigned y);
	std::pair<vec2d, double> getCentroidTimesMassAndMassBelowSegment(vec2d a, const vec2d& b);
	void mergePrevious(vec2d& a, double& b, const vec2d& newA, const double& newB);
	std::pair< vec2d, double> getCoordsTimesMassAndMass(const Cell& cell);
	double getPixelNotCheckingRange(unsigned x, unsigned y);
public:
	CellAreaCalculator(const ViewerBase& viewerBase);
	std::optional<vec2d> tryGetCenterOfGravity(const Cell& cell);
	double getArea(const Cell& cell);
};