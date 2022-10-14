#include "Pch.hpp"
#include "PreviewPolygonize.hpp"

#include "Application.hpp"
#include "ImgViewer.hpp"

#define CGAL_NO_GMP 1
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Alpha_shape_2.h>
#include <CGAL/Alpha_shape_vertex_base_2.h>
#include <CGAL/Alpha_shape_face_base_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/algorithm.h>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Constrained_triangulation_plus_2.h>
#include <CGAL/Polygon_2.h>

//typedef CGAL::Exact_predicates_exact_constructions_kernel       K;
typedef CGAL::Exact_predicates_inexact_constructions_kernel       K;
typedef K::Segment_2                                         Segment;

struct FaceInfo2
{
	FaceInfo2() {}
	int nesting_level;
	bool in_domain() {
		return nesting_level % 2 == 1;
	}
};
typedef CGAL::Triangulation_vertex_base_2<K>                      Vb;
typedef CGAL::Triangulation_face_base_with_info_2<FaceInfo2, K>    Fbb;
typedef CGAL::Constrained_triangulation_face_base_2<K, Fbb>        Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb>               TDS;
typedef CGAL::Exact_predicates_tag                                Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, TDS, Itag>  CDT;
typedef CDT::Point                                                Point;
typedef CGAL::Polygon_2<K>                                        Polygon_2;
typedef CDT::Face_handle                                          Face_handle;


typedef CGAL::Constrained_triangulation_plus_2<CDT>                       CDTP;

void
mark_domains(CDT& ct,
	Face_handle start,
	int index,
	std::list<CDT::Edge>& border)
{
	if (start->info().nesting_level != -1) {
		return;
	}
	std::list<Face_handle> queue;
	queue.push_back(start);
	while (!queue.empty()) {
		Face_handle fh = queue.front();
		queue.pop_front();
		if (fh->info().nesting_level == -1) {
			fh->info().nesting_level = index;
			for (int i = 0; i < 3; i++) {
				CDT::Edge e(fh, i);
				Face_handle n = fh->neighbor(i);
				if (n->info().nesting_level == -1) {
					if (ct.is_constrained(e)) border.push_back(e);
					else queue.push_back(n);
				}
			}
		}
	}
}
void mark_domains(CDT& cdt)
{
	for (CDT::Face_handle f : cdt.all_face_handles())
		f->info().nesting_level = -1;
	std::list<CDT::Edge> border;
	mark_domains(cdt, cdt.infinite_face(), 0, border);
	while (!border.empty()) {
		CDT::Edge e = border.front();
		border.pop_front();
		Face_handle n = e.first->neighbor(e.second);
		if (n->info().nesting_level == -1) {
			mark_domains(cdt, n, e.first->info().nesting_level + 1, border);
		}
	}
}

typedef CGAL::Delaunay_triangulation_2<
	K,
	CGAL::Triangulation_data_structure_2<CGAL::Alpha_shape_vertex_base_2<K>,
	CGAL::Alpha_shape_face_base_2<K>>
	>
	Triangulation_2;
typedef CGAL::Alpha_shape_2<Triangulation_2>                 Alpha_shape_2;
//typedef CGAL::Triangulation_data_structure_2<CGAL::Triangulation_vertex_base_2<K>, CGAL::Constrained_triangulation_face_base_2<K, CGAL::Triangulation_face_base_with_info_2<FaceInfo2, K>>>               TDS;
typedef CGAL::Exact_predicates_tag                                Itag;

void PreviewPolygonize::recalculate()
{
	const auto& imgViewer = viewerBase.accessImgViewer();
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
	Alpha_shape_2 alphaShaper(points.begin(), points.end(),
		alpha//,
		//Alpha_shape_2::GENERAL
	);
	auto
		it = alphaShaper.alpha_shape_edges_begin(),
		lim = alphaShaper.alpha_shape_edges_end();

	CDTP cdtp;

	segments.clear();
	for (; it != lim; ++it)
	{
		auto s = alphaShaper.segment(*it);
		auto& [a, b] = segments.emplace_back();
		auto& p0 = s.source(); a = { (float)p0.x(), (float)p0.y() };
		auto& p1 = s.target(); b = { (float)p1.x(), (float)p1.y() };
		cdtp.insert_constraint(p0, p1);
	}
	nComponents = alphaShaper.number_solid_components();
	LOG("num ", segments.size(), '\n');
	cdtp.split_subconstraint_graph_into_constraints();
	mark_domains(cdtp);

	Clear(alphaVertices);
	for (auto it = alphaShaper.Alpha_shape_vertices_begin(),
		lim = alphaShaper.Alpha_shape_vertices_end(); it != lim; it++)
	{
		auto& p = (**it).point();
		alphaVertices.emplace_back((float)p.x(), (float)p.y());
	}

	trigs.clear();
	for (Face_handle vIt : cdtp.finite_face_handles())
	{
		if (!vIt->info().in_domain())
			continue;

		auto& v0 = vIt->vertex(0)->point();
		auto& v1 = vIt->vertex(1)->point();
		auto& v2 = vIt->vertex(2)->point();

		{
			auto& vert = trigs.emplace_back();
			vert.position = { (float)v0.x(),(float)v0.y() };
			vert.color = trigColor;
		}

		{
			auto& vert = trigs.emplace_back();
			vert.position = { (float)v1.x(),(float)v1.y() };
			vert.color = trigColor;
		}

		{
			auto& vert = trigs.emplace_back();
			vert.position = { (float)v2.x(),(float)v2.y() };
			vert.color = trigColor;
		}
	}
	trigCnt = trigs.size() / 3;
	////std::deque<std::deque<Point>> p;
	//{
	//	//Marked_face_set marked_face_set(false);
	//	//Finite_faces_iterator face_it;
	//	//size_type nb_solid_components = 0;
	//	//
	//	//if (number_of_vertices() == 0)
	//	//	return 0;
	//	auto vIt = alphaShaper.faces_begin();
	//	auto vLim = alphaShaper.faces_end();
	//	int cnt = 0;
	//	trigs.clear();
	//	for (; vIt != vLim; vIt++)
	//	{
	//		if (alphaShaper.classify(vIt) != alphaShaper.INTERIOR)
	//			continue;
	//
	//		cnt++;
	//		auto& v0 = vIt->vertex(0)->point();
	//		auto& v1 = vIt->vertex(1)->point();
	//		auto& v2 = vIt->vertex(2)->point();
	//
	//		{
	//			auto& vert = trigs.emplace_back();
	//			vert.position = { (float)v0.x(),(float)v0.y() };
	//			vert.color = trigColor;
	//		}
	//
	//		{
	//			auto& vert = trigs.emplace_back();
	//			vert.position = { (float)v1.x(),(float)v1.y() };
	//			vert.color = trigColor;
	//		}
	//
	//		{
	//			auto& vert = trigs.emplace_back();
	//			vert.position = { (float)v2.x(),(float)v2.y() };
	//			vert.color = trigColor;
	//		}
	//
	//		{
	//			auto& vert = trigs.emplace_back();
	//			vert.position = { (float)v0.x(),(float)v0.y() };
	//			vert.color = trigColor;
	//		}
	//	}
	//	LOG("cnt ", cnt, '\n');
	//}

	////Insert the polygons into a constrained triangulation
	//for (const auto& a : p)
	//	cdt.insert_constraint(a.begin(), a.end(), true);
	//
	////Mark facets that are inside the domain bounded by the polygon
	//mark_domains(cdt);
	////const auto nTriangles = cdt.all_face_handles().size();
	//LOG("n tr ", nTriangles, '\n');
	//trigs.resize(nTriangles * 4);
	//size_t cnt = 0;
	//for (auto& faceHandle : cdt.finite_face_handles())
	//{
	//	auto& f = *faceHandle;
	//	for (int i = 0; i <= 3; i++)
	//	{
	//		auto& v = *f.vertex(i % 3);
	//		auto& p = v.point();
	//		trigs[cnt].position = { (float)p.x(),(float)p.y() };
	//		trigs[cnt].color = trigColor;
	//		cnt++;
	//	}
	//}

//construct two non-intersecting nested polygons
//Polygon_2 polygon1;
//polygon1.push_back(Point(0, 0));
//polygon1.push_back(Point(2, 0));
//polygon1.push_back(Point(2, 2));
//polygon1.push_back(Point(0, 2));
//Polygon_2 polygon2;
//polygon2.push_back(Point(0.5, 0.5));
//polygon2.push_back(Point(1.5, 0.5));
//polygon2.push_back(Point(1.5, 1.5));
//polygon2.push_back(Point(0.5, 1.5));
////Insert the polygons into a constrained triangulation
//CDT cdt;
//cdt.insert_constraint(polygon1.vertices_begin(), polygon1.vertices_end(), true);
//cdt.insert_constraint(polygon2.vertices_begin(), polygon2.vertices_end(), true);
////Mark facets that are inside the domain bounded by the polygon
//mark_domains(cdt);
//int count = 0;
//for (Face_handle vIt : cdt.finite_face_handles())
//{
//	if (vIt->info().in_domain())
//	{
//		++count;
//
//		auto& v0 = vIt->vertex(0)->point();
//		auto& v1 = vIt->vertex(1)->point();
//		auto& v2 = vIt->vertex(2)->point();
//
//		{
//			auto& vert = trigs.emplace_back();
//			vert.position = { (float)v0.x(),(float)v0.y() };
//			vert.color = trigColor;
//		}
//
//		{
//			auto& vert = trigs.emplace_back();
//			vert.position = { (float)v1.x(),(float)v1.y() };
//			vert.color = trigColor;
//		}
//
//		{
//			auto& vert = trigs.emplace_back();
//			vert.position = { (float)v2.x(),(float)v2.y() };
//			vert.color = trigColor;
//		}
//	}
//}
//std::cout << "There are " << count << " facets in the domain." << std::endl;
}

void PreviewPolygonize::pollEvent(const sf::Event& e)
{
}

void PreviewPolygonize::drawUIImpl()
{
	ImGui::Checkbox("Org Vertices", &showOrgVertices);
	ImGui::Checkbox("Alpha Shape", &showAlphaShape);
	ImGui::Checkbox("Alpha Vertices", &showAlphaVertices);
	ImGui::Checkbox("Triangles", &drawTrigs);
	static constexpr double
		vmin = .5,
		//vmin = .25,
		vmax = 100000.;
	if (ImGui::DragScalar("Alpha", ImGuiDataType_Double, &alpha, 1.f, &vmin, &vmax, "%.3f", 0)) recalculate();
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
		c.setFillColor(vertexColor);
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
	if (showAlphaShape)
	{
		//int cnt = 0;
		for (const auto& [a, b] : segments)
		{
			sf::Vertex v[2]
			{
				//{a, cnt&1?sf::Color::Green:sf::Color::Blue},
				//{b, cnt&1?sf::Color::Green:sf::Color::Blue}
				{a, sf::Color::Cyan },
				{b, sf::Color::Cyan }
			};
			w.draw(v, 2, sf::Lines);
			//cnt++;
		}
	}
	if (drawTrigs)
	{
		const auto len = trigs.size();
		for (size_t i = 0; i != len; i += 3)
		{
			auto d = [&](int x, int y)
			{
				sf::Vertex v[2]
				{
					trigs[i + x],
					trigs[i + y],
				};
				w.draw(v, 2, sf::Lines);
			};
			for (int j = 0; j != 3; j++)
				d(j, (j + 1) % 3);
		}
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
	vertexColor(sf::Color::Black),
	showOrgVertices(false),
	showAlphaShape(true),
	showAlphaVertices(true),
	drawTrigs(true),
	trigColor(sf::Color ::Yellow),
	alphaVertColor(sf::Color::Red)
{
	recalculate();
}
