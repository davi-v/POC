#include "Pch.hpp"
#include "Voronoi.hpp"

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"
// https://github.com/JCash/voronoi

vec2d Voronoi::clipToRect(const vec2d& org, const vec2d& dir) const
{
    if (dir.x)
    {
        double dSide;
        if (dir.x > 0)
            dSide = rect.max.x;
        else
            dSide = rect.min.x;
        dSide -= org.x;

        double dVert;
        if (dir.y > 0)
            dVert = rect.max.y;
        else
            dVert = rect.min.y;
        dVert -= org.y;

        const auto tan = dir.y / dir.x;
        const auto wouldWalkYWithoutWall = tan * dSide;
        if (abs(wouldWalkYWithoutWall) < abs(dVert))
            return org + vec2d{ dSide, wouldWalkYWithoutWall };
        return org + vec2d{ dVert / tan, dVert };
    }

    double dVert;
    if (dir.y > 0)
        dVert = rect.max.y;
    else
        dVert = rect.min.y;
    return org + vec2d{ 0, dVert - org.y };
}

Voronoi::Voronoi(const sf::Rect<double>& area)
{
    brect_ = { area.left, area.top, area.left + area.width, area.top + area.height };
    rect.min = { (float)area.left,(float)area.top };
    rect.max = { (float)area.left + (float)area.width,(float)area.top + (float)area.height };
}

void Voronoi::update(const VecCoords& coords)
{
    point_data_.clear();
    for (const auto& c : coords)
        point_data_.emplace_back(c.x, c.y);
    MakeUniquePtr(diagram, coords, rect);
}

Voronoi::~Voronoi()
{
}

std::vector<Edge> Voronoi::getEdges() const
{
    std::vector<Edge> ret;
    //auto& vd = diagram->vd;
    //for (auto it = vd.edges().begin(); it != vd.edges().end(); ++it)
    //{
    //    auto& e = *it;
    //    if (e.is_infinite())
    //    {
    //        const auto& cell1 = *e.cell();
    //        const auto& cell2 = *e.twin()->cell();
    //        vec2d origin, direction;
    //        auto retrieve_point = [&](auto&& cell)
    //        {
    //            auto index = cell.source_index();
    //            //auto category = cell.source_category();
    //            //if (category == SOURCE_CATEGORY_SINGLE_POINT) {
    //            return point_data_[index];
    //        };
    //        //if (cell1.contains_point() && cell2.contains_point())
    //        {
    //            auto p1 = retrieve_point(cell1);
    //            auto p2 = retrieve_point(cell2);
    //            origin.x =((p1.x() + p2.x()) * 0.5);
    //            origin.y=((p1.y() + p2.y()) * 0.5);
    //            direction.x=(p1.y() - p2.y());
    //            direction.y=(p2.x() - p1.x());
    //        }
    //        coordinate_type side = xh(brect_) - xl(brect_);
    //        vec2d p1, p2;
    //        if (e.vertex0())
    //        {
    //            p1 = {
    //                e.vertex0()->x(), e.vertex0()->y()
    //            };
    //            if (p1.x > rect.max.x || p1.x < rect.min.x || p1.y > rect.max.y || p1.y < rect.min.y)
    //                p1 = clipToRect(origin, p1 - origin);
    //        }
    //        else
    //            p1 = clipToRect(origin, -direction);
    //        if (e.vertex1())
    //        {
    //
    //            p2 = {
    //                e.vertex1()->x(), e.vertex1()->y()
    //            };
    //            if (p2.x > rect.max.x || p2.x < rect.min.x || p2.y > rect.max.y || p2.y < rect.min.y)
    //              p2 = clipToRect(origin, p2 - origin);
    //        }
    //            //p2 = clipToRect(origin, -direction);
    //        else
    //            p2 = clipToRect(origin, direction);
    //        ret.emplace_back(p1, p2);
    //    }
    //    else
    //    //if (e.is_primary())
    //    {
    //        auto v0Ptr = e.vertex0();
    //        auto v1Ptr = e.vertex1();
    //        //if (v0Ptr && v1Ptr)
    //        {
    //            auto& v0 = *v0Ptr;
    //            auto& v1 = *v1Ptr;
    //            ret.emplace_back(vec2d{ v0.x(), v0.y() }, vec2d{ v1.x(), v1.y() });
    //        }
    //    }
    //}
    // If all you need are the edges
    if (diagram)
    {
        auto& d = diagram->diagram;
        const jcv_edge* edge = jcv_diagram_get_edges(&d);
        while (edge)
        {
            auto& p0 = edge->pos[0];
            auto& p1 = edge->pos[1];
            ret.emplace_back(vec2d{ p0.x, p0.y }, vec2d{ p1.x, p1.y });
            edge = jcv_diagram_get_next_edge(edge);
        }
    }
    return ret;
}

void Voronoi::fillCellEdges(std::vector<Cell>& v) const
{
    //auto& vd = diagram->vd;
    //unsigned int cell_index = 0;
    //for (voronoi_diagram<double>::const_cell_iterator it = vd.cells().begin();
    //    it != vd.cells().end(); ++it) {
    //    if (it->contains_point()) {
    //        std::size_t index = it->source_index();
    //        auto&
    //        Point p = points[index];
    //        printf("Cell #%ud contains a point: (%d, %d).\n",
    //            cell_index, x(p), y(p));
    //    }
    //    ++cell_index;
    //}

    //size_t i = 0;
    //for (auto it = vd.cells().begin(); it != vd.cells().end(); ++it, i++)
    //{
    //    auto& retCell = v[i];
    //    retCell.clear();
    //
    //    auto& cell = *it;
    //    auto edge = cell.incident_edge();
    //    while (edge != cell.incident_edge())
    //    {
    //        if (edge->is_primary())
    //        {
    //            auto v = *edge->vertex0();
    //            auto& x = v.x();
    //            auto& y = v.y();
    //            retCell.emplace_back(x, y);
    //        }
    //        edge = edge->next();
    //    }
    //}
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

struct Point {
    int a;
    int b;
    Point() : a(0), b(0)
    {

    }
    Point(int x, int y) : a(x), b(y) {}
};

namespace boost {
    namespace polygon {

        template <>
        struct geometry_concept<Point> {
            typedef point_concept type;
        };

        template <>
        struct point_traits<Point> {
            typedef int coordinate_type;

            static inline coordinate_type get(
                const Point& point, orientation_2d orient) {
                return (orient == HORIZONTAL) ? point.a : point.b;
            }
        };
    }  // polygon
}  // boost

// Traversing Voronoi edges using edge iterator.
int iterate_primary_edges1(const voronoi_diagram<double>& vd)
{
    int result = 0;
    for (auto it = vd.edges().begin(); it != vd.edges().end(); ++it)
        if (it->is_primary())
            ++result;
    return result;
}

// Traversing Voronoi edges using cell iterator.
int iterate_primary_edges2(const voronoi_diagram<double>& vd)
{
    int result = 0;
    for (auto it = vd.cells().begin(); it != vd.cells().end(); ++it)
    {
        auto& cell = *it;
        auto edge = cell.incident_edge();
        do
        {
            if (edge->is_primary())
                ++result;
            edge = edge->next();
        } while (edge != cell.incident_edge());
    }
    return result;
}

// Traversing Voronoi edges using vertex iterator.
// As opposite to the above two functions this one will not iterate through
// edges without finite endpoints and will iterate only once through edges
// with single finite endpoint.
int iterate_primary_edges3(const voronoi_diagram<double>& vd)
{
    int result = 0;
    for (auto it = vd.vertices().begin(); it != vd.vertices().end(); it++)
    {
        auto& vertex = *it;
        auto edge = vertex.incident_edge();
        do
        {
            if (edge->is_primary())
                ++result;
            edge = edge->rot_next();
        } while (edge != vertex.incident_edge());
    }
    return result;
}

Voronoi::DiagramWrapper::DiagramWrapper(const std::vector<vec2d>& coords, const jcv_rect& rect)
{
    //std::vector<Point> points;
    //auto mapToInt = [&](const vec2d& c) -> Point
    //{
    //    return { (int)c.x, (int)c.y };
    //    auto W = rect.max.x - rect.min.x;
    //    auto H = rect.max.y - rect.min.y;
    //    auto offx = c.x - rect.min.x;
    //    auto offy = c.y - rect.min.y;
    //    static constexpr auto DIFF = (long long)std::numeric_limits<int>::max() - std::numeric_limits<int>::min();
    //    return {
    //        lround(std::numeric_limits<int>::min() + offx * DIFF),
    //        lround(std::numeric_limits<int>::min() + offy * DIFF)
    //    };
    //};
    //for (const auto& c : coords)
    //    points.emplace_back(mapToInt(c));

    //construct_voronoi(points.begin(), points.end(), &vd);

    memset(&diagram, 0, sizeof(jcv_diagram));
    const auto n = coords.size();
    //std::vector<jcv_point> points(n);
    //for (size_t i = 0; i != n; i++)
    //{
    //    auto& p = points[i];
    //    const auto& c = coords[i];
    //    p.x = (float)c.x;
    //    p.y = (float)c.y;
    //}
    jcv_diagram_generate((int)n, (jcv_point*)coords.data(), &rect, 0, &diagram);
}

Voronoi::DiagramWrapper::~DiagramWrapper()
{
    jcv_diagram_free(&diagram);
}

