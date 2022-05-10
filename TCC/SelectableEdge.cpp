#include "Pch.hpp"
#include "SelectableEdge.hpp"

SelectableEdge::SelectableEdge(const vec2d& A, const vec2d& B) :
    A(A),
    B(B)
{
    auto dir = B - A;
    if (dir)
        this->dir = normalize(dir);
}
