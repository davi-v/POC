#include "Pch.hpp"
#include "PreviewPolygonize.hpp"

#include "Application.hpp"
#include "ImgViewer.hpp"

void
mark_domains(CDTP2& ct,
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

void PreviewPolygonize::recalculate()
{
	const auto& imgViewer = viewerBase.accessImgViewer();
	const auto& img = *imgViewer.currentImage;
	const auto& [ox, oy] = imgViewer.imageOffsetF;
	const auto [W, H] = img.getSize();
	std::deque<Point> points;
	Clear(orgVertices);
	auto prevW = W - 1;
	auto prevH = H - 1;
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

	MakeUniquePtr(alphaShaperPtr, points.begin(), points.end(),
		alpha//,
		//Alpha_shape_2::GENERAL
	);

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
		//LOG(p0, ' ', p1, '\n');
		cdtp.insert_constraint(p0, p1);
	}

	nComponents = alphaShaper.number_solid_components();

	//cdtp.split_subconstraint_graph_into_constraints();

	mark_domains(cdtp);
	trigs.clear();
	for (auto& vIt : cdtp.finite_face_handles())
		if (vIt->info().in_domain())
			for (int i = 0; i != 3; i++)
			{
				auto& v = vIt->vertex(i)->point();
				auto& t = trigs.emplace_back();
				t.position = { (float)v.x(),(float)v.y() };
				t.color = trigColor;
			}

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

	//LOG(ct.number_of_vertices(), '\n');
	PS::simplify(ct, Cost(), Stop(simplificationRatio));
	//LOG(ct.number_of_vertices(), '\n');

	mark_domains(ct);
	trigsSimplified.clear();
	trigCnt = 0;
	for (auto& vIt : ct.finite_face_handles())
		if (vIt->info().in_domain())
		{
			trigCnt++;
			for (int i = 0; i != 3; i++)
			{
				auto& v = vIt->vertex(i)->point();
				auto& vert = trigsSimplified.emplace_back();
				vert.position = { (float)v.x(),(float)v.y() };
				vert.color = trigColor;
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
				v.color = sf::Color::Cyan;
				return v;
			};
			auto off = segsSimplified.size();
			segsSimplified.emplace_back(to(d.front()));
			for (size_t i = 1; i != lim; i++)
			{
				segsSimplified.emplace_back(to(d[i]));
				segsSimplified.emplace_back(segsSimplified.back());
			}
			segsSimplified.emplace_back(segsSimplified[off]);
		}
	}
}

void PreviewPolygonize::pollEvent(const sf::Event& e)
{
}

void PreviewPolygonize::drawUIImpl()
{
	ImGui::Checkbox("Original Vertices", &showOrgVertices);
	ImGui::Checkbox("Alpha Shape Polylines", &drawAlphaShapePolylines);
	ImGui::Checkbox("Alpha Shape Vertices", &showAlphaVertices);
	ImGui::Checkbox("Alpha Shape Triangles", &drawTrigs);
	ImGui::Checkbox("Simplified Vertices", &showVerticesSimplified);
	ImGui::Checkbox("Simplified Triangles", &drawTrigsSimplified);
	ImGui::Checkbox("Simplified Polylines", &drawSimplifiedPolylines);
	{
		static constexpr double
			vmin = .5,
			//vmin = .25,
			vmax = 100000.;
		if (ImGui::DragScalar("Alpha", ImGuiDataType_Double, &alpha, 1.f, &vmin, &vmax, "%.3f", 0)) recalculate();
	}
	{
		static constexpr double
			vmin = .01,
			vmax = 1;
		if (ImGui::SliderScalar("Simplification Ratio", ImGuiDataType_Double, &simplificationRatio, &vmin, &vmax, "%.3f", 0)) updateSimplification();
	}
	ImGui::Text("Vertices: %zu", alphaVertices.size());
	ImGui::Text("Components: %zu", nComponents);
	ImGui::Text("Triangles: %zu", trigCnt);
}

void PreviewPolygonize::draw()
{
	auto& w = viewerBase.app.window;
	if (showOrgVertices)
	{
		auto& imgViewer = viewerBase.accessImgViewer();
		auto& c = imgViewer.circle;
		c.setFillColor(orgVertexColor);
		constexpr auto R = .25f;
		c.setRadius(R);
		c.setOrigin(R, R);
		for (const auto& coord : orgVertices)
		{
			c.setPosition(coord);
			w.draw(c);
		}
	}
	if (showAlphaVertices)
	{
		auto& imgViewer = viewerBase.accessImgViewer();
		auto& c = imgViewer.circle;
		c.setFillColor(alphaVertColor);
		constexpr auto R = .25f;
		c.setRadius(R);
		c.setOrigin(R, R);
		for (const auto& coord : alphaVertices)
		{
			c.setPosition(coord);
			w.draw(c);
		}
	}
	if (showVerticesSimplified)
	{
		auto& imgViewer = viewerBase.accessImgViewer();
		auto& c = imgViewer.circle;
		c.setFillColor(alphaVertColor);
		constexpr auto R = .25f;
		c.setRadius(R);
		c.setOrigin(R, R);
		for (const auto& coord : vertices)
		{
			c.setPosition(coord);
			w.draw(c);
		}
	}
	if (drawAlphaShapePolylines)
	{
		for (const auto& [a, b] : segments)
		{
			sf::Vertex v[2]
			{
				{a, sf::Color::Cyan },
				{b, sf::Color::Cyan }
			};
			w.draw(v, 2, sf::Lines);
		}
	}
	auto drawTrigsH = [&](auto&& t)
	{
		const auto len = t.size();
		for (size_t i = 0; i != len; i += 3)
		{
			auto d = [&](int x, int y)
			{
				sf::Vertex v[2]
				{
					t[i + x],
					t[i + y],
				};
				w.draw(v, 2, sf::Lines);
			};
			for (int j = 0; j != 3; j++)
				d(j, (j + 1) % 3);
		}
	};
	if (drawTrigs)
		drawTrigsH(trigs);
	if (drawTrigsSimplified)
		drawTrigsH(trigsSimplified);
	if (drawSimplifiedPolylines)
	{
		//for (const auto& [a, b] : segsSimplified)
		//{
		//	sf::Vertex v[2]
		//	{
		//		{a, sf::Color::Cyan },
		//		{b, sf::Color::Cyan }
		//	};
		//}
			w.draw(segsSimplified.data(), segsSimplified.size(), sf::Lines);
	}
}

void PreviewPolygonize::onImgChangeImpl()
{
}

const char* PreviewPolygonize::getTitle()
{
	return "Polygonize";
}

PreviewPolygonize::PreviewPolygonize(ViewerBase& viewerBase) :
	Previewer(viewerBase),
	alpha(1),
	orgVertexColor(sf::Color(34, 33, 169)),
	showOrgVertices(false),
	drawAlphaShapePolylines(false),
	showAlphaVertices(false),
	showVerticesSimplified(false),
	drawTrigs(false),
	drawTrigsSimplified(false),
	drawSimplifiedPolylines(true),
	trigColor(sf::Color::Yellow),
	alphaVertColor(sf::Color::Red),
	simplificationRatio(1.)
{
	recalculate();
}

bool FaceInfo2::in_domain()
{
	return nesting_level % 2 == 1;
}