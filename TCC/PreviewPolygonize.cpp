#include "Pch.hpp"
#include "PreviewPolygonize.hpp"

#include "OpenCVSFML.hpp"
#include "Application.hpp"
#include "ImgViewer.hpp"

void
mark_domains(
	CDTP2& ct,
	Face_handle start,
	int index,
	std::queue<CDT2::Edge>& border)
{
	if (start->info().nesting_level != -1)
		return;
	std::queue<Face_handle> queue;
	queue.emplace(start);
	while (!queue.empty())
	{
		Face_handle fh = queue.front();
		queue.pop();
		if (fh->info().nesting_level == -1)
		{
			fh->info().nesting_level = index;
			for (int i = 0; i != 3; i++)
			{
				CDT2::Edge e(fh, i);
				Face_handle n = fh->neighbor(i);
				if (n->info().nesting_level == -1)
				{
					if (ct.is_constrained(e))
						border.emplace(e);
					else
						queue.emplace(n);
				}
			}
		}
	}
}
void mark_domains(CDTP2& cdt)
{
	for (auto& f : cdt.all_face_handles())
		f->info().nesting_level = -1;
	std::queue<CDT2::Edge> border;
	mark_domains(cdt, cdt.infinite_face(), 0, border);
	while (!border.empty())
	{
		CDT2::Edge e = border.front();
		border.pop();
		auto n = e.first->neighbor(e.second);
		if (n->info().nesting_level == -1)
			mark_domains(cdt, n, e.first->info().nesting_level + 1, border);
	}
}

void PreviewPolygonize::drawUISpecific()
{
	if (ImGui::CollapsingHeader(POLYGONIZATION))
	{
		ImGui::Checkbox("Draw Original Vertices", &drawOrgVertices);

		auto colorPick = [](const char* str, sf::Color& color, std::vector<sf::Vertex>& container)
		{
			if (ColorPicker3U32(str, color))
				for (auto& v : container)
					v.color = color;
		};
		auto helper = [&](const std::string& imguiId, auto&& title, size_t nv, size_t nt, bool& drawV, bool& drawP, bool& drawT, sf::Color& color, std::vector<sf::Vertex>& trigs)
		{
			if (ImGui::TreeNode(title))
			{
				ImGui::Text("Number of Vertices: %zu", nv);
				ImGui::Text("Number of Triangles: %zu", nt);
				ImGui::Checkbox(("Vertices##" + imguiId).c_str(), &drawV);
				ImGui::Checkbox(("Polylines##" + imguiId).c_str(), &drawP);
				ImGui::Checkbox(("Triangles##" + imguiId).c_str(), &drawT);
				colorPick(("Triangles Color##" + imguiId).c_str(), color, trigs);
				ImGui::TreePop();
				return true;
			}
			return false;
		};

		if (helper("0", "Alpha Shape", alphaVertices.size(), trigsOrgCnt, drawAlphaVertices, drawAlphaShapePolylines, drawTrigs, orgTriangleColor, trigs))
		{
			if (DragScalarMinMax("Radius##alpha", alphaShapeRadius, .5, 100000.))
				initSamplesAndAllocation();
		}

		if (helper("1", "Simplification", verticesSimplified.size(), trigsSimplifiedCnt, drawVerticesSimplified, drawSimplifiedPolylines, drawTrigsSimplified, simplifiedTriangleColor, trigsSimplified))
		{
			if (DragScalarMinMax("Simplification Ratio", simplificationRatio, 0.01f,  .01, 1.))
				updateSimplification();
		}

		ImGui::Text("Components: %zu", nComponents);

		colorPick("Polyline Color", polylineColor, segsSimplified);
	}
}

void PreviewPolygonize::samplesExtraDrawUI()
{
	static constexpr const char* SAMPLE_MODES[] = {
			"TrigCentroid",
			"QuadCentroid"
	};
	if (ImGui::Combo("Sample Mode", reinterpret_cast<int*>(&sampleMode), SAMPLE_MODES, IM_ARRAYSIZE(SAMPLE_MODES)))
		recalculateSamplesOnly();
	ImGui::Checkbox("Draw Sampled Triangles", &drawExtraSampleLines);
}

void PreviewPolygonize::initSamplesAndAllocation()
{
	const auto& imgViewer = viewerBase;
	const auto& img = *imgViewer.currentImage;
	const auto& [ox, oy] = imgViewer.imageOffsetF;
	const auto [W, H] = img.getSize();
	std::deque<Point> points;
	Clear(orgVertices);
	for (unsigned x = 0; x != W; x++)
		for (unsigned y = 0; y != H; y++)
		{
			auto px = img.getPixel(x, y);
			if (px.a)
			{
				float
					bx = (float)x + ox,
					by = (float)y + oy;
				auto add = [&](float a, float b)
				{
					orgVertices.emplace_back(a, b);
					points.emplace_back(a, b);
				};
				add(bx, by);
				if (x + 1 < W && !img.getPixel(x + 1, y).a) add(bx + 1, by);
				if (y + 1 < H && !img.getPixel(x, y + 1).a) add(bx, by + 1);
				if (x + 1 < W && y + 1 < H && !img.getPixel(x + 1, y + 1).a) add(bx + 1, by + 1);
			}
		}

	MakeUniquePtr(alphaShaperPtr, points.begin(), points.end(), alphaShapeRadius);

	auto& alphaShaper = *alphaShaperPtr;
	alphaVertices.clear();
	for (auto it = alphaShaper.alpha_shape_vertices_begin(), lim = alphaShaper.alpha_shape_vertices_end(); it != lim; it++)
	{
		auto& p = (**it).point();
		alphaVertices.emplace_back((float)p.x(), (float)p.y());
	}

	segments.clear();
	cdtp.clear();
	g.clear();
	for (auto it = alphaShaper.alpha_shape_edges_begin(), lim = alphaShaper.alpha_shape_edges_end(); it != lim; ++it)
	{
		auto s = alphaShaper.segment(*it);
		auto& [a, b] = segments.emplace_back();
		auto& p0 = s.source(); a = { (float)p0.x(), (float)p0.y() };
		auto& p1 = s.target(); b = { (float)p1.x(), (float)p1.y() };
		g[p0].emplace_back(p1);
		g[p1].emplace_back(p0);
		cdtp.insert_constraint(p0, p1);
	}

	nComponents = alphaShaper.number_solid_components();

	mark_domains(cdtp);
	trigs.clear();
	trigsOrgCnt = 0;
	for (auto& vIt : cdtp.finite_face_handles())
		if (vIt->info().in_domain())
		{
			trigsOrgCnt++;
			for (int i = 0; i != 3; i++)
			{
				auto& v = vIt->vertex(i)->point();
				auto& t = trigs.emplace_back();
				t.position = { (float)v.x(),(float)v.y() };
				t.color = orgTriangleColor;
			}
		}

	// ter no máximo uns 100 triângulos na malha simplificada
	if (DEFAULT_N_SAMPLES > trigsOrgCnt)
		simplificationRatio = 1.;
	else
		simplificationRatio = (double)DEFAULT_N_SAMPLES / trigsOrgCnt;

	updateSimplification();
}

void PreviewPolygonize::recalculateSamplesOnly()
{
	if (trigsSimplifiedCnt)
	{
		std::deque<TriangleD> trianglesForSample(trigsSimplifiedCnt);
		for (size_t i = 0, j = 0; i != trigsSimplifiedCnt; i++, j += 3)
			for (size_t x = 0; x != 3; x++)
			{
				const auto& c = trigsSimplified[j + x].position;
				trianglesForSample[i][x] = { (double)c.x, (double)c.y };
			}
		std::tie(samples, sampleLines) = selectPoints(trianglesForSample);
		recalculateTargetsGreedyMethod();
	}
}

void PreviewPolygonize::updateSimplification()
{
	namespace PS = CGAL::Polyline_simplification_2;
	typedef PS::Stop_below_count_ratio_threshold Stop;
	typedef PS::Squared_distance_cost Cost;

	CDTP2 ct;
	auto& alphaShaper = *alphaShaperPtr;
	std::deque<std::deque<Point>> Ps;
	std::set<Point> visited;
	for (const auto& [start, _] : g)
	{
		if (visited.emplace(start).second)
		{
			auto& P = Ps.emplace_back();
			auto get = [&](const Point& p) -> std::optional<Point>
			{
				auto& ns = g[p];
				for (size_t i = 0, lim = ns.size(); i != lim; i++)
				{
					const auto& n = ns[i];
					if (visited.emplace(n).second)
						return n;
				}
				return {};
			};
			Point pIt = start;
			P.emplace_back(start);
			while (const auto pOpt = get(pIt))
			{
				const auto& nxt = *pOpt;
				P.emplace_back(nxt);
				pIt = nxt;
			}
		}
	}

	for (auto& P : Ps)
		ct.insert_constraint(P.begin(), P.end(), true);

	PS::simplify(ct, Cost(), Stop(simplificationRatio));

	mark_domains(ct);
	trigsSimplified.clear();
	trigsSimplifiedCnt = 0;
	for (auto& vIt : ct.finite_face_handles())
		if (vIt->info().in_domain())
		{
			trigsSimplifiedCnt++;
			for (int i = 0; i != 3; i++)
			{
				auto& v = vIt->vertex(i)->point();
				auto& vert = trigsSimplified.emplace_back();
				vert.position = { (float)v.x(),(float)v.y() };
				vert.color = simplifiedTriangleColor;
			}
		}

	segsSimplified.clear();
	for (auto& cit : ct.constraints())
	{
		std::deque<Point> d;
		for (auto& p : ct.points_in_constraint(cit))
			d.emplace_back(p);
		if (auto lim = d.size())
		{
			auto to = [&](auto&& p)
			{
				sf::Vertex v;
				v.position = { (float)p.x(), (float)p.y() };
				v.color = polylineColor;
				return v;
			};
			auto off = segsSimplified.size();
			auto p0 = to(d.front());
			segsSimplified.emplace_back(p0);
			verticesSimplified.resize(lim);
			verticesSimplified[0] = p0.position;
			for (size_t i = 1; i != lim; i++)
			{
				auto newP = to(d[i]);
				segsSimplified.emplace_back(newP);
				segsSimplified.emplace_back(segsSimplified.back());
				verticesSimplified[i] = newP.position;
			}
			segsSimplified.emplace_back(segsSimplified[off]);
		}
	}

	recalculateSamplesOnly();
}

void PreviewPolygonize::drawExtraSampleBased()
{
	auto& w = viewerBase.app->window;
	if (drawAlphaVertices)
		drawV(alphaVertices, alphaVertColor);
	if (drawVerticesSimplified)
		drawV(verticesSimplified, alphaVertColor);
	if (drawAlphaShapePolylines)
	{
		for (const auto& [a, b] : segments)
		{
			sf::Vertex v[2]
			{
				{a, polylineColor },
				{b, polylineColor }
			};
			w.draw(v, 2, sf::Lines);
		}
	}
	auto drawTrigsH = [&](const std::vector<sf::Vertex>& t)
	{
		const auto len = t.size();
		for (size_t i = 0; i != len; i += 3)
		{
			auto d = [&](size_t x, size_t y)
			{
				sf::Vertex v[2]
				{
					t[i + x],
					t[i + y]
				};
				w.draw(v, 2, sf::Lines);
			};
			for (size_t j = 0; j != 3; j++)
				d(j, (j + 1) % 3);
		}
	};
	if (drawTrigs)
		drawTrigsH(trigs);
	if (drawTrigsSimplified || drawExtraSampleLines)
		drawTrigsH(trigsSimplified);
	if (drawExtraSampleLines)
	{
		for (auto& [a, b] : sampleLines)
		{
			sf::Vertex v[2]
			{
				{a, simplifiedTriangleColor},
				{b, simplifiedTriangleColor}
			};
			w.draw(v, 2, sf::Lines);
		}
	}
	if (drawSimplifiedPolylines)
		w.draw(segsSimplified.data(), segsSimplified.size(), sf::Lines);
}

const char* PreviewPolygonize::getTitle()
{
	return POLYGONIZATION;
}

PreviewPolygonize::PreviewPolygonize(ViewerBase& viewerBase) :
	SampleBased(viewerBase),
	alphaShapeRadius(1),
	orgTriangleColor(sf::Color::Yellow),
	simplifiedTriangleColor(sf::Color::Red),
	polylineColor(sf::Color::Cyan),
	alphaVertColor(sf::Color::Red),
	drawAlphaShapePolylines(false),
	drawAlphaVertices(false),
	drawVerticesSimplified(false),
	drawTrigs(false),
	drawTrigsSimplified(false),
	drawSimplifiedPolylines(false),
	drawExtraSampleLines(false),
	sampleMode(SampleMode::TrigCentroid)
{
	initSamplesAndAllocation();
}

std::pair<std::vector<vec2f>, std::deque<std::array<vec2f, 2>>> PreviewPolygonize::selectPoints(
	std::deque<TriangleD>& triangles)
{
	std::vector<vec2f> samples(limitedNSamples);
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
	for (size_t i = 0; i != limitedNSamples; i++)
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

		if (sampleMode == SampleMode::QuadCentroid)
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