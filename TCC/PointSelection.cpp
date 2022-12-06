#include "Pch.hpp"
#include "PointSelection.hpp"

#include "Utilities.hpp"

std::pair<std::vector<vec2f>, std::deque<std::array<vec2f, 2>>> SelectPoints(
	std::deque<TriangleD>& triangles,
	size_t nPoints,
	bool quadCentroid)
{
	std::vector<vec2f> samples(nPoints);
	std::deque<std::array<vec2f, 2>> lines;
	using T = TriangleD::type;
	struct P
	{
		P(T a2, size_t index) :
			a2(a2),
			index(index)
		{

		}
		T a2;
		size_t index;
	};
	auto cmp = [&](const P& a, const P& b)
	{
		return a.a2 < b.a2;
	};
	std::priority_queue<P, std::vector<P>, decltype(cmp)> q(cmp);
	for (size_t i = 0, len = triangles.size(); i != len; i++)
	{
		const auto& t = triangles[i];
		auto a2 = TwiceAreaPolygonSigned<T>(t);
		q.emplace(a2, i);
	}
	for (size_t i = 0; i != nPoints; i++)
	{
		const auto& e = q.top();
		const auto& index = e.index;
		const auto& t = triangles[index];
		const auto& a2 = e.a2;
		const auto trigC = Centroid(t, a2);
		const auto& curSample = samples[i] = (vec2f)trigC;
		q.pop();
		std::array<vec2_t<T>, 3> mids;
		for (size_t i = 0; i != 3; i++)
		{
			mids[i] = mean(t[i], t[(i + 1) % 3]);
			lines.emplace_back(std::array<vec2f, 2>{ (vec2f)trigC, (vec2f)mids[i] });
		}

		auto addNewT = [&](const TriangleD& newT)
		{
			auto a2 = TwiceAreaPolygonSigned<T>(newT);
			assert(a2 > 0);
			auto newIndex = triangles.size();
			q.emplace(a2, newIndex);
			triangles.emplace_back(newT);
		};

		if (quadCentroid)
		{
			for (size_t i = 0; i != 3; i++)
			{
				const auto
					& ml = mids[i],
					& m = t[i],
					& mr = mids[(i + 2) % 3];

				const auto cell = std::array<vec2_t<T>, 4>{ trigC, mr, m, ml };
				assert(cross(mr - trigC, m - trigC) > 0);
				assert(cross(m - trigC, ml - trigC) > 0);
				assert(cross(mr - trigC, ml - trigC) > 0);
				const auto aCell = TwiceAreaPolygonSigned<T>(cell);

#ifdef _DEBUG
				// quando usando float, deu erro em algum(ns) dos asserts, então pq não deixar aqui
				for (int i = 0; i != 4; i++)
				{
					auto& a = cell[i];
					auto& b = cell[(i + 1) % 4];
					auto& c = cell[(i + 2) % 4];
					assert(cross(b - a, c - a) > 0);
				}
				assert(aCell > 0);
#endif

				const auto cCell = CalculateCentroid<T>(cell, aCell);

#ifdef _DEBUG
				for (int i = 0; i != 4; i++)
				{
					auto& a = cell[i];
					auto& b = cell[(i + 1) % 4];
					assert(cross(cCell - a, cCell - b) > 0);
				}
#endif

				auto add = [&](auto& a, auto& b)
				{
					lines.emplace_back(std::array<vec2f, 2>{ (vec2f)cCell, (vec2f)a });
					auto newT = std::array<vec2_t<T>, 3>{ cCell, a, b };
					assert(cross(a - cCell, b - cCell) > 0);

					addNewT(newT);
				};
				// calculate new 4 triangles
				add(m, ml);
				add(ml, trigC);
				add(trigC, mr);
				add(mr, m);
			}
		}
		else
		{
			for (size_t i = 0; i != 3; i++)
			{
				const auto
					& t0 = t[i],
					& t1 = t[(i + 1) % 3],
					& m = mids[i];
				lines.emplace_back(std::array<vec2f, 2>{ curSample, (vec2f)mids[i] });
				lines.emplace_back(std::array<vec2f, 2>{ curSample, t0 });
				addNewT({ trigC, t0, m });
				addNewT({ trigC, m, t1 });
			}
		}
	}
	return { samples, lines };
}