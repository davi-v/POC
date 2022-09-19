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
		const auto xCoord = j + .5;
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
			a += w * vec2d{ xCoord, yCoord };
			b += w;
		}
	}
}

void VoronoiInfo::update(const std::vector<vec2d>& robotCoords)
{
	auto n = robotCoords.size();
	voronoiCells.resize(n);

	auto& v = voronoi;

	v.update(robotCoords);
	v.fillCellEdges(voronoiCells);

	centroids = CalculateCentroids(voronoiCells);
	targets = getTargets();
}
std::vector<vec2d> VoronoiInfo::getTargets()
{
	//fixme.clear();


	const auto n = voronoiCells.size();
	std::vector<vec2d> ret(n);
	for (size_t i = 0; i != n; i++)
		ret[i] = getWeightedMeanOfCoords(i);
	return ret;
}

std::pair<vec2d, double> VoronoiInfo::getCentroidAndMassBelowSegment(vec2d a, vec2d b)
{
	const auto& img = imgViewer.currentImage;
	const auto size = img->getSize();
	const int C = (int)size.x, R = size.y;

	auto inImg = [&](int x, int y)
	{
		return x >= 0 && x < C && y >= 0 && y < R;
	};

	// doesn't include x, y
	auto getSumOfCoordsTimesMassAndSumBelow = [&](int cx, int cy) -> std::pair<vec2d, double>
	{
		if (cx >= 0 && cx < C)
		{
			const auto& curCol = prefixCols[cx];
			if (cy >= R)
				return {};
			const auto& last = curCol.back();
			if (cy >= 0)
			{
				const auto& cur = curCol[cy];
				return { last.first - cur.first, last.second - cur.second };
			}
			return last;
		}
		return {};
	};

	vec2d centroid;
	double tsm = 0;

	auto handleSum = [&](double b1, double b2, double h, const int cx, const int cy)
	{
		vec2d nxt = a + vec2d{ h, b1 - b2 };
		if (h)
		{
			const auto [scm, sm] = getSumOfCoordsTimesMassAndSumBelow(cx, cy);
			if (sm)
			{
				const auto lim = a.x + h;
				const double avgX = (lim + a.x) / 2;
				const double offXFromCenter = fmod(avgX, 1) - .5;
				auto newCentroid = scm / sm;
				newCentroid.x += offXFromCenter;
				const double newMass = sm * h;
				mergePrevious(centroid, tsm, newCentroid, newMass);
			}

			vec2d bot1{ a.x, a.y + 1 - fmod(a.y, 1) };
			vec2d bot2 = bot1 + vec2d{ h, 0 };

			// merge with trapezoid centroid
			auto handle = [&](auto&& p1, auto&& p2, auto&& p3, double strength)
			{
				if (const auto trigMass = AreaTrig(p1, p2, p3) * strength)
				{
					const auto trigCentroid = (p1 + p2 + p3) / 3;
					mergePrevious(centroid, tsm, trigCentroid, trigMass);
				}
			};

			//auto& t = fixme.emplace_back();
			//t = {a, bot1, bot2, nxt};

			const auto strength = getPixelW(cx, cy);

			handle(a, bot2, bot1, strength);
			handle(a, nxt, bot2, strength);
		}
		a = nxt;
	};

	const auto getPx = [&](double v)
	{
		return (int)floor(v);
	};

	int
		cx = getPx(a.x),
		cy = getPx(a.y);
	double b1 = 1 - fmod(a.y, 1);
	double totalDiffX = b.x - a.x;
	const bool up = a.y > b.y;
	if (totalDiffX)
	{
		auto totalDiffY = a.y - b.y;
		auto tan = totalDiffY / totalDiffX;
		bool running = true;
		while (running)
		{
			double dyToNextVertWall;
			if (up)
				dyToNextVertWall = 1 - b1;
			else
				dyToNextVertWall = b1;

			const auto curOffXInCell = fmod(a.x, 1);
			const auto dxToNextRightWall = 1 - curOffXInCell;

			const auto curTanToNextCorner = dyToNextVertWall / dxToNextRightWall;

			if (abs(tan) > abs(curTanToNextCorner)) // foi pra cima
			{
				const double faltay = abs(a.y - b.y);
				double h;
				if (faltay < dyToNextVertWall)
				{
					h = b.x - a.x;
					running = false;
				}
				else
					h = dyToNextVertWall / abs(tan);

				const double b2 = b1 + tan * h;

				handleSum(b1, b2, h, cx, cy);
				if (up)
				{
					b1 = 0;
					cy--;
				}
				else
				{
					b1 = 1;
					cy++;
				}
			}
			else // foi pra direita
			{
				const double faltax = b.x - a.x;
				double h;
				if (faltax < dxToNextRightWall)
				{
					h = faltax;
					running = false;
				}
				else
					h = dxToNextRightWall;

				const double b2 = b1 + tan * h;

				handleSum(b1, b2, h, cx, cy);
				b1 = b2;
				cx++;
			}
		}
	}
	return { centroid, tsm };
}

vec2d VoronoiInfo::getWeightedMeanOfCoords(size_t cellIndex)
{
	const auto& off = imgViewer.imageOffsetF;
	vec2d coordsOff{ off.x, off.y };

	const auto& cell = voronoiCells[cellIndex];

	vec2d centroid;
	double m = 0;

	const auto n = cell.size();
	for (size_t i = 0; i != n; i++)
	{
		const auto
			curCoord = cell[i] - coordsOff,
			nxtCoord = cell[(i + 1) % n] - coordsOff;
		if (curCoord.x < nxtCoord.x)
		{
			auto [newCentroid, newMass] = getCentroidAndMassBelowSegment(curCoord, nxtCoord);
			if (newMass)
				mergePrevious(centroid, m, newCentroid, newMass);
		}
		else
		{
			auto [c2, m2] = getCentroidAndMassBelowSegment(nxtCoord, curCoord);
			if (m2)
			{
				auto m1 = m - m2;
				centroid = (centroid * m - c2 * m2) / m1;
				m = m1;
			}
		}
	}
	if (abs(m) > EPS_DBS) // uma fonte de erros é no cálculo de h, por exemplo
		return centroid + coordsOff;
	return centroids[cellIndex];
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
	const auto tm = m1 + m2;
	c1 = (c1 * m1 + c2 * m2) / tm;
	m1 = tm;
}

size_t VoronoiInfo::findCell(double x, double y)
{
	const auto n = voronoiCells.size();
	for (size_t i = 0; i != n; i++)
		if (IsInsideConvexShape({ x, y }, voronoiCells[i]))
			return i;
	return -1;
}