#pragma once
#include "Utilities.hpp"

// no momento, a biblioteca tem bug https://github.com/JCash/voronoi/issues/68 mas to usando assim mesmo
// Idealmente, mudamos para a implementação do boost ou cgal
#define JCV_REAL_TYPE double
#define JCV_ATAN2 atan2
#define JCV_SQRT sqrt
#define JCV_FLT_MAX DBL_MAX
#define JCV_PI 3.141592653589793115997963468544185161590576171875
#include "jc_voronoi.h"

typedef std::pair<vec2d, vec2d> Edge;

class Voronoi
{
	struct DiagramWrapper
	{
		jcv_diagram diagram;
		DiagramWrapper(const std::vector<vec2d>& coords, const jcv_rect& rect);
		std::vector<Edge> getEdges() const;
		~DiagramWrapper();
	};
	std::unique_ptr<DiagramWrapper> diagram;
	jcv_rect rect;
public:
	Voronoi(const sf::Rect<double>& rect);
	void update(const VecCoords& coords);
	std::vector<Edge> getEdges() const;
	void fillCellEdges(std::vector<Cell>& v) const;
};

void DrawVoronoiEdges(const Voronoi& voronoi, const sf::Color& c, sf::RenderWindow& w);