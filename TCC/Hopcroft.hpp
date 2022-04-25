#pragma once

// reference: https://judge.yosupo.jp/submission/52112
class bipartite_matching
{
    size_t flow;
    std::vector<std::vector<size_t>> g;
    std::vector<size_t>
        leftVertexNeighbour, // i-th index is the neighbour of the i-th vertex on the left side, which is in the right side, 0-indexed
        rightVertexNeighbour;
    std::vector<size_t> dist;

    bool dfs(size_t u);

public:
    bipartite_matching(size_t n_left, size_t n_right);
    void add(size_t u, size_t v);
    size_t get_max_matching();
    std::vector<std::pair<size_t, size_t>> get_edges();
};