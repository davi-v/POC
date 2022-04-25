#include "Pch.hpp"
#include "Hopcroft.hpp"

bool bipartite_matching::dfs(size_t u)
{
    for (const auto& v : g[u])
    {
        auto& srcOnLeft = rightVertexNeighbour[v];
        if (!~srcOnLeft)
        {
            leftVertexNeighbour[u] = v;
            srcOnLeft = u;
            return true;
        }
    }
    for (const auto& v : g[u])
    {
        auto& srcOnLeft = rightVertexNeighbour[v];
        if (dist[srcOnLeft] == dist[u] + 1 && dfs(srcOnLeft))
        {
            leftVertexNeighbour[u] = v;
            srcOnLeft = u;
            return true;
        }
    }
    return false;
}

bipartite_matching::bipartite_matching(size_t nL, size_t nR) :
    flow(0),
    g(nL),
    leftVertexNeighbour(nL, -1),
    rightVertexNeighbour(nR, -1),
    dist(nL)
{

}

void bipartite_matching::add(size_t u, size_t v)
{
    g[u].emplace_back(v);
}

size_t bipartite_matching::get_max_matching()
{
    auto nL = leftVertexNeighbour.size();
    while (true)
    {
        // BFS step
        std::queue<size_t> q;
        for (size_t u = 0; u != nL; u++)
            if (~leftVertexNeighbour[u])
                dist[u] = -1;
            else
            {
                q.emplace(u);
                dist[u] = 0;
            }
        while (!q.empty())
        {
            auto u = q.front();
            q.pop();
            for (const auto& v : g[u])
            {
                const auto& srcOnLeft = rightVertexNeighbour[v];
                if (~srcOnLeft && !~dist[srcOnLeft])
                {
                    dist[srcOnLeft] = dist[u] + 1;
                    q.emplace(srcOnLeft);
                }
            }
        }

        size_t augment = 0;
        for (size_t u = 0; u != nL; u++)
            if (!~leftVertexNeighbour[u])
                augment += dfs(u);

        if (!augment)
            break;

        flow += augment;
    }
    return flow;
}

std::vector<std::pair<size_t, size_t>> bipartite_matching::get_edges()
{
    std::vector<std::pair<size_t, size_t>> ans;
    for (size_t u = 0, nL = leftVertexNeighbour.size(); u != nL; u++)
    {
        const auto& neighbourOnRight = leftVertexNeighbour[u];
        if (~neighbourOnRight)
            ans.emplace_back(u, neighbourOnRight);
    }
    return ans;
}