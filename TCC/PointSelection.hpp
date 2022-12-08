#pragma once
#include "vec2.hpp"

template<class T>
struct Triangle_t
{
	using type = T;
	std::array<vec2_t<T>, 3> p;
	const vec2_t<T>& operator[](size_t i) const;
	vec2_t<T>& operator[](size_t i);
	Triangle_t() = default;
	Triangle_t(const vec2_t<T>& p0, const vec2_t<T>& p1, const vec2_t<T>& p2);
	Triangle_t(const std::array<vec2_t<T>, 3>& p);
	constexpr auto size() const
	{
		return p.size();
	}
};
typedef Triangle_t<double> TriangleD;

template<class T>
vec2_t<T> Centroid(const Triangle_t<T>& t, T a2)
{
	return CalculateCentroid<T>(t, a2);
}

// don't call with empty triangles
std::pair<std::vector<vec2f>, std::deque<std::array<vec2f, 2>>>
SelectPoints(
	std::deque<TriangleD>& triangles,
	size_t nPoints,
	bool quadCentroid);

template<class T>
inline const vec2_t<T>& Triangle_t<T>::operator[](size_t i) const
{
	return p[i];
}

template<class T>
inline vec2_t<T>& Triangle_t<T>::operator[](size_t i)
{
	return p[i];
}

template<class T>
inline Triangle_t<T>::Triangle_t(const vec2_t<T>& p0, const vec2_t<T>& p1, const vec2_t<T>& p2) :
	p{p0, p1, p2}
{
}

template<class T>
inline Triangle_t<T>::Triangle_t(const std::array<vec2_t<T>, 3>& p) :
	p(p)
{
}
