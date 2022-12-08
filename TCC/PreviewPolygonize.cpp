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
			{
				static constexpr double
					vmin = .5,
					vmax = 100000.;
				if (ImGui::DragScalar("Radius", ImGuiDataType_Double, &alphaShapeRadius, 1.f, &vmin, &vmax, "%.3f", 0))
					recalculateTriangulations();
			}
		}

		if (helper("1", "Simplification", verticesSimplified.size(), trigsSimplifiedCnt, drawVerticesSimplified, drawSimplifiedPolylines, drawTrigsSimplified, simplifiedTriangleColor, trigsSimplified))
		{
			{
				static constexpr double
					vmin = .01,
					vmax = 1;
				if (ImGui::DragScalar("Simplification Ratio", ImGuiDataType_Double, &simplificationRatio, .01f, &vmin, &vmax, "%.3f", 0))
					updateSimplification();
			}
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
		recalculateSamplesAndAllocation();
	ImGui::Checkbox("Draw Sampled Triangles", &drawExtraSampleLines);
}

void PreviewPolygonize::recalculateSamplesAndAllocation()
{
	if (trigsSimplifiedCnt)
	{
		trianglesForSample.resize(trigsSimplifiedCnt);
		for (size_t i = 0, j = 0; i != trigsSimplifiedCnt; i++, j += 3)
			for (size_t x = 0; x != 3; x++)
			{
				const auto& c = trigsSimplified[j + x].position;
				trianglesForSample[i][x] = { (double)c.x, (double)c.y };
			}
		std::tie(samples, sampleLines) = SelectPoints(trianglesForSample, limitedNSamples, sampleMode == SampleMode::QuadCentroid);

		recalculateGreedyMethod();
	}
}

void PreviewPolygonize::recalculateTriangulations()
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

	recalculateSamplesAndAllocation();
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
	recalculateTriangulations();
}