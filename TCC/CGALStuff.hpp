#pragma once

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