#pragma once
#include "vec2.hpp"

#define JCV_REAL_TYPE double
#define JCV_ATAN2 atan2
#define JCV_SQRT sqrt
#define JCV_FLT_MAX DBL_MAX
#define JCV_PI 3.141592653589793115997963468544185161590576171875
#include "jc_voronoi.h"

typedef std::pair<vec2d, vec2d> Edge;

class Voronoi
{
	jcv_diagram diagram;
public:
	Voronoi(const std::vector<vec2d>& coords, const sf::Rect<double>& rect);
	std::vector<Edge> getEdges() const;
};