#include "Pch.hpp"
#include "VoronoiInfo.hpp"

#include "Utilities.hpp"

VoronoiInfo::VoronoiInfo(const ImgViewer& imgViewer) :
	imgViewer(imgViewer),
	voronoi(imgViewer.getBB())
{
	const auto& img = *imgViewer.currentImage;
	const auto [C, R] = img.getSize();
	prefixCols.resize(C);
	for (unsigned j = 0; j != C; j++)
	{
		auto& col = prefixCols[j];
		col.resize(R);
		for (unsigned i = 0; i != R; i++)
		{
			auto px = img.getPixel(j, i);
			double w;
			if (px.a)
				w = 1;
			else
				w = 0;
			if (i)
				col[i] = col[i - 1];
			const auto yCoord = i + .5;
			auto& [a, b] = col[i];
			a += w * yCoord;
			b += w;
		}
	}
}

void VoronoiInfo::update(const std::vector<vec2d>& coords)
{
	auto n = coords.size();
	voronoiCells.resize(n);

	auto& v = voronoi;

	v.update(coords);
	v.fillCellEdges(voronoiCells);

	centroids = CalculateCentroids(voronoiCells);
	targets = getTargets();
}
std::vector<vec2d> VoronoiInfo::getTargets()
{
	const auto n = voronoiCells.size();
	std::vector<vec2d> ret(n);
	for (size_t i = 0; i != n; i++)
		ret[i] = getWeightedMeanOfCoords(i);
	return ret;
}

std::pair<vec2d, double> VoronoiInfo::getCentroidTimesMassAndMassBelowSegment(vec2d a, const vec2d& b)
{
	const auto& img = imgViewer.currentImage;
	const auto size = img->getSize();
	const int C = (int)size.x, R = size.y;

	vec2d cm;
	double tsm = 0;
	double totalDiffX = b.x - a.x;
	if (totalDiffX)
	{
		auto
			dfx = floor(a.x),
			dfy = floor(a.y);
		auto
			cx = (int)dfx,
			cy = (int)dfy;
		auto totalDiffY = a.y - b.y;
		const bool up = totalDiffY > 0;
		auto tan = totalDiffY / totalDiffX;
		double absTan = abs(tan);
		bool running = true;
		double dxToNextRightWall = 1 - a.x + dfx;
		double b1 = 1 - a.y + dfy, b2, h;
		do
		{
			double dyToNextVertWall;
			if (up)
				dyToNextVertWall = 1 - b1;
			else
				dyToNextVertWall = b1;
			auto handleSum = [&]
			{
				b2 = b1 + tan * h;
				vec2d nxt = a + vec2d{ h, b1 - b2 };
				if (h)
				{
					// doesn't include cx, cy
					auto getSumOfCoordsTimesMassAndSumBelow = [&]() -> std::pair<double, double>
					{
						if (cx >= 0 && cx < C)
						{
							const auto& curCol = prefixCols[cx];
							if (cy >= R)
								return {};
							const auto& last = curCol.back();
							const auto& [yl, ml] = last;
							if (cy >= 0)
							{
								const auto& [yc, mc] = curCol[cy];
								return { yl - yc, ml - mc};
							}
							return { yl, ml };
						}
						return {};
					};
					const auto [yc, sm] = getSumOfCoordsTimesMassAndSumBelow();
					const double avgX = (a.x + nxt.x) * .5;
					const double m = sm * h;
					mergePrevious(cm, tsm, vec2d{ avgX * m, yc * h }, m);

					const auto strength = getPixelW(cx, cy);
					// merge with trapezoid centroid (aqui dividimos o trapézio em 2 triângulos)
					auto handle = [&](const vec2d& p2, const vec2d& p3)
					{
						const auto trigCentroid = (a + p2 + p3) / 3;
						const auto trigMass = AreaTrig(a, p2, p3) * strength;
						mergePrevious(cm, tsm, trigCentroid * trigMass, trigMass);
					};
					vec2d bot1{ a.x, a.y + b1 };
					vec2d bot2 = bot1; bot2.x += h;
					handle(bot2, bot1);
					handle(nxt, bot2);
				}
				a = nxt;
			};
			if (dyToNextVertWall < absTan * dxToNextRightWall) // mudou verticalmente
			{
				if (abs(a.y - b.y) < dyToNextVertWall)
				{
					h = b.x - a.x;
					running = false;
				}
				else
					h = dyToNextVertWall / absTan;
				handleSum();
				static constexpr double NEW_B[]{ 1, 0 };
				static constexpr int CY_OFF[]{ 1, -1 };
				b1 = NEW_B[up];
				cy += CY_OFF[up];
				dxToNextRightWall -= h;
			}
			else // mudou pro quadrado da direita
			{
				const double faltax = b.x - a.x;
				if (faltax < dxToNextRightWall)
				{
					h = faltax;
					running = false;
				}
				else
					h = dxToNextRightWall;
				handleSum();
				b1 = b2;
				cx++;
				dxToNextRightWall = 1;
			}
		} while (running);
	}
	return { cm, tsm };
}

vec2d VoronoiInfo::getWeightedMeanOfCoords(size_t cellIndex)
{
	const auto& off = imgViewer.imageOffsetF;
	vec2d coordsOff{ off.x, off.y };

	const auto& cell = voronoiCells[cellIndex];

	vec2d cmTotal;
	double mTotal = 0;

	const auto n = cell.size();
	for (size_t i = 0; i != n; i++)
	{
		const auto
			curCoord = cell[i] - coordsOff,
			nxtCoord = cell[(i + 1) % n] - coordsOff;
		vec2d cm; double m;
		if (curCoord.x < nxtCoord.x)
			std::tie(cm, m) = getCentroidTimesMassAndMassBelowSegment(curCoord, nxtCoord);
		else
		{
			std::tie(cm, m) = getCentroidTimesMassAndMassBelowSegment(nxtCoord, curCoord);
			cm = -cm;
			m = -m;
		}
		mergePrevious(cmTotal, mTotal, cm, m);
	}
	if (abs(mTotal) > EPS_DBS) // um exemplo de fonte de erros é no cálculo das massas dos triângulos
	{
		auto c = cmTotal / mTotal;
		return c + coordsOff;
	}
	return voronoi.getCoord(cellIndex);
}

double VoronoiInfo::getPixelW(unsigned i, unsigned j)
{
	const auto [C, R] = imgViewer.currentImage->getSize();
	if (i >= 0 and i < C and j >= 0 and j < R)
	{
		auto px = imgViewer.currentImage->getPixel(i, j);
		if (px.a)
			return 1.;
	}
	return 0.;
}

void VoronoiInfo::mergePrevious(vec2d& c1, double& m1, const vec2d& c2, const double& m2)
{
	c1 += c2;
	m1 += m2;
}

size_t VoronoiInfo::findCell(double x, double y)
{
	const auto n = voronoiCells.size();
	for (size_t i = 0; i != n; i++)
		if (IsInsideConvexShape({ x, y }, voronoiCells[i]))
			return i;
	return -1;
}