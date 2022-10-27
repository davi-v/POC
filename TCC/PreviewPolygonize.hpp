#pragma once
#include "Previewer.hpp"
#include "vec2.hpp"

struct FaceInfo2
{
	int nesting_level;
	bool in_domain();
};

namespace PS = CGAL::Polyline_simplification_2;

typedef CGAL::Exact_predicates_inexact_constructions_kernel       K;
typedef K::Segment_2                                         Segment;
//typedef CGAL::Triangulation_vertex_base_2<K>                      Vb;
typedef PS::Vertex_base_2<K> Vb;
typedef CGAL::Triangulation_face_base_with_info_2<FaceInfo2, K>    Fbb;
typedef CGAL::Constrained_triangulation_face_base_2<K, Fbb>        Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb>               TDS;
typedef CGAL::Exact_predicates_tag                                Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, TDS, Itag>  CDT2;
typedef CDT2::Point                                                Point;
typedef CGAL::Polygon_2<K>                                        Polygon_2;
typedef CDT2::Face_handle                                          Face_handle;
typedef CGAL::Constrained_triangulation_plus_2<CDT2>                       CDTP2;

typedef CGAL::Delaunay_triangulation_2<
	K,
	CGAL::Triangulation_data_structure_2<CGAL::Alpha_shape_vertex_base_2<K>,
	CGAL::Alpha_shape_face_base_2<K>>
	>
	Triangulation_2;
typedef CGAL::Alpha_shape_2<Triangulation_2>                 Alpha_shape_2;
//typedef CGAL::Triangulation_data_structure_2<CGAL::Triangulation_vertex_base_2<K>, CGAL::Constrained_triangulation_face_base_2<K, CGAL::Triangulation_face_base_with_info_2<FaceInfo2, K>>>               TDS;
typedef CGAL::Exact_predicates_tag                                Itag;

class PreviewPolygonize : public Previewer
{
	std::unique_ptr<Alpha_shape_2> alphaShaperPtr;
	CDTP2 cdtp;
	sf::Color trigColor, orgVertexColor, alphaVertColor;
	std::vector<sf::Vertex> trigs, trigsSimplified;
	double alpha;
	std::deque<sf::Vector2f> orgVertices;
	std::list<sf::Vector2f> alphaVertices, vertices;
	size_t nComponents, trigCnt;
	bool showOrgVertices, showVerticesSimplified,
		drawAlphaShapePolylines,
		drawSimplifiedPolylines,
		drawTrigs, drawTrigsSimplified, showAlphaVertices;
	double simplificationRatio; // (0, 1]
	struct Seg
	{
		sf::Vector2f p1, p2;
	};
	std::map<Point, std::deque<Point>> g;
	std::deque<Seg> segments;
	std::vector<sf::Vertex> segsSimplified;

	void recalculate();
	void updateSimplification();
	void pollEvent(const sf::Event& e) override;
	void drawUIImpl() override;
	void draw() override;
	void onImgChangeImpl() override;
	const char* getTitle() override;
public:
	PreviewPolygonize(ViewerBase& viewerBase);
};