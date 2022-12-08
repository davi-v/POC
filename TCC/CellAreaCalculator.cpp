#include "Pch.hpp"
#include "CellAreaCalculator.hpp"

CellAreaCalculator::CellAreaCalculator(const ViewerBase& viewerBase) :
	viewerBase(&viewerBase)
{
	const auto& img = *viewerBase.currentImage;
	const auto [C, R] = img.getSize();
	prefixCols.resize(C);
	for (unsigned j = 0; j != C; j++)
	{
		auto& col = prefixCols[j];
		col.resize(R);
		for (unsigned i = 0; i != R; i++)
		{
			auto w = getPixelNotCheckingRange(j, i);
			if (i)
				col[i] = col[i - 1];
			const auto yCoord = i + .5;
			auto& [a, b] = col[i];
			a += w * yCoord;
			b += w;
		}
	}
}


std::pair<vec2d, double> CellAreaCalculator::getCentroidTimesMassAndMassBelowSegment(vec2d a, const vec2d& b)
{
	const auto& img = viewerBase->currentImage;
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
								return { yl - yc, ml - mc };
							}
							return { yl, ml };
						}
						return {};
					};
					const auto [yc, sm] = getSumOfCoordsTimesMassAndSumBelow();
					const double avgX = (a.x + nxt.x) * .5;
					const double m = sm * h;
					mergePrevious(cm, tsm, vec2d{ avgX * m, yc * h }, m);

					const auto w = getPixelWCheckingRange(cx, cy);
					// merge with trapezoid centroid (aqui dividimos o trapézio em 2 triângulos)
					auto handle = [&](const vec2d& p2, const vec2d& p3)
					{
						const auto trigCentroid = (a + p2 + p3) / 3;
						const auto trigMass = AreaTrig(a, p2, p3) * w;
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

std::optional<vec2d> CellAreaCalculator::tryGetCenterOfGravity(const Cell& cell)
{
	auto [cmTotal, mTotal] = getCoordsTimesMassAndMass(cell);
	if (abs(mTotal) > EPS_DBS)
	{
		auto c = cmTotal / mTotal;

		const auto& off = viewerBase->imageOffsetF;
		vec2d coordsOff{ off.x, off.y };

		return c + coordsOff;
	}
	return {};
}

double CellAreaCalculator::getArea(const Cell& cell)
{
	return getCoordsTimesMassAndMass(cell).second;
}

double CellAreaCalculator::getPixelWCheckingRange(unsigned x, unsigned y)
{
	const auto [C, R] = viewerBase->currentImage->getSize();
	if (x >= 0 and x < C and y >= 0 and y < R)
		return getPixelNotCheckingRange(x, y);
	return 0.;
}

void CellAreaCalculator::mergePrevious(vec2d& c1, double& m1, const vec2d& c2, const double& m2)
{
	c1 += c2;
	m1 += m2;
}

std::pair<vec2d, double> CellAreaCalculator::getCoordsTimesMassAndMass(const Cell& cell)
{
	const auto& off = viewerBase->imageOffsetF;
	vec2d coordsOff{ off.x, off.y };

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
	return { cmTotal, mTotal };
}

double CellAreaCalculator::getPixelNotCheckingRange(unsigned x, unsigned y)
{
	auto& img = *viewerBase->currentImage;
	auto px = img.getPixel(x, y);
	double w = px.a / 255.;
	return w;
}
