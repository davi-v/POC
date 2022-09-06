#include "Pch.hpp"
#include "Voronoi.hpp"

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

Voronoi::Voronoi(const std::vector<vec2d>& coords, const sf::Rect<double>& area)
{
    memset(&diagram, 0, sizeof(jcv_diagram));

    const auto count = (int)coords.size();
    const auto points = coords.data();
    jcv_rect rect;
    rect.min = { area.left, area.top };
    rect.max = { area.left + area.width, area.top + area.height };
    jcv_diagram_generate(count, (const jcv_point*)points, &rect, 0, &diagram);
}

std::vector<Edge> Voronoi::getEdges() const
{
    std::vector<Edge> ret;
    // If all you need are the edges
    const jcv_edge* edge = jcv_diagram_get_edges(&diagram);
    while (edge)
    {
        auto& p0 = edge->pos[0];
        auto& p1 = edge->pos[1];
        ret.emplace_back(vec2d{ p0.x, p0.y }, vec2d{ p1.x, p1.y });
        edge = jcv_diagram_get_next_edge(edge);
    }
    return ret;
}
