#pragma once
#include "Utilities.hpp"

#define JCV_REAL_TYPE double
#define JCV_ATAN2 atan2
#define JCV_SQRT sqrt
#define JCV_FLT_MAX DBL_MAX
#define JCV_PI 3.141592653589793115997963468544185161590576171875
#include "jc_voronoi.h"
typedef std::pair<vec2d, vec2d> Edge;
typedef double coordinate_type;
typedef point_data<coordinate_type> point_type;
typedef rectangle_data<coordinate_type> rect_type;
class Voronoi
{
	struct DiagramWrapper
	{
		//voronoi_diagram<double> vd;
		jcv_diagram diagram;
		DiagramWrapper(const std::vector<vec2d>& coords, const jcv_rect& rect);
		~DiagramWrapper();
	};
	std::unique_ptr<DiagramWrapper> diagram;
	jcv_rect rect;
	std::vector<point_type> point_data_;
	vec2d clipToRect(const vec2d& org, const vec2d& dir) const;
	rect_type brect_;
public:
	Voronoi(const sf::Rect<double>& rect);
	void update(const VecCoords& coords);
	~Voronoi();
	std::vector<Edge> getEdges() const;
	void fillCellEdges(std::vector<Cell>& v) const;
};

void DrawVoronoiEdges(const Voronoi& voronoi, const sf::Color& c, sf::RenderWindow& w);