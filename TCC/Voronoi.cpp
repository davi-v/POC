#include "Pch.hpp"
#include "Voronoi.hpp"

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

Voronoi::Voronoi(const sf::Rect<double>& area)
{
    rect = { area.left, area.top, area.left + area.width, area.top + area.height };
    rect.min = { (float)area.left,(float)area.top };
    rect.max = { (float)area.left + (float)area.width,(float)area.top + (float)area.height };
    MakeUniquePtr(diagram, VecCoords(), rect);
}

void Voronoi::update(const VecCoords& coords)
{
    MakeUniquePtr(diagram, coords, rect);
}

std::vector<Edge> Voronoi::getEdges() const
{
    return diagram->getEdges();
}

void Voronoi::fillCellEdges(std::vector<Cell>& v) const
{
    auto& d = diagram->diagram;
    const jcv_site* sites = jcv_diagram_get_sites(&d);
    for (int i = 0; i != d.numsites; i++)
    {
        const jcv_site* site = &sites[i];
        const jcv_graphedge* e = site->edges;
        auto& cell = v[site->index];
        cell.clear();
        while (e)
        {
            const auto& [x, y] = e->pos[0];
            cell.emplace_back(x, y);
            e = e->next;
        }
    }
}

void DrawVoronoiEdges(const Voronoi& voronoi, const sf::Color& voronoiLineColor, sf::RenderWindow& w)
{
    for (const auto& [p0, p1] : voronoi.getEdges())
    {
        sf::Vertex v[2]
        {
            { { (float)p0.x, (float)p0.y}, voronoiLineColor },
            { { (float)p1.x, (float)p1.y}, voronoiLineColor },
        };
        w.draw(v, 2, sf::Lines);
    }
}

Voronoi::DiagramWrapper::DiagramWrapper(const VecCoords& coords, const jcv_rect& rect)
{
    memset(&diagram, 0, sizeof(jcv_diagram));
    const auto n = coords.size();
    jcv_diagram_generate((int)n, (jcv_point*)coords.data(), &rect, 0, &diagram);
}

std::vector<Edge> Voronoi::DiagramWrapper::getEdges() const
{
    std::vector<Edge> ret;
    auto edge = jcv_diagram_get_edges(&diagram);
    while (edge)
    {
        const auto& p0 = edge->pos[0];
        const auto& p1 = edge->pos[1];
        ret.emplace_back(vec2d{ p0.x, p0.y }, vec2d{ p1.x, p1.y });
        edge = jcv_diagram_get_next_edge(edge);
    }
    return ret;
}

Voronoi::DiagramWrapper::~DiagramWrapper()
{
    jcv_diagram_free(&diagram);
}