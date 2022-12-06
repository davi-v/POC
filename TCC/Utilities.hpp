#pragma once
#include "vec2.hpp"
#include "ElemSelected.hpp"

typedef std::vector<vec2d> VecCoords;
typedef VecCoords Cell;

static constexpr auto DEFAULT_TICKS_PER_SECOND = 144; // >= 1
static constexpr double EPSILON = .0001;

extern sf::Clock globalClock;
time_t GetCurTime();

sf::Vector2f ToFloat(const sf::Vector2u& p);

bool TimeToFormat(time_t t, const char* format, std::string& out);

std::string TimeToDay(time_t t);

std::string TimeToHour(time_t t);

// ext with '.' (.png)
std::string GetUniqueNameWithCurrentTime(const std::string& prepend, const std::string& ext);

bool TryUpdateClosestCircle(const vec2d& coordDouble, float& curMinD2, const vec2d& coord, float radius);
bool TryUpdateClosestCircle(const vec2d& coordDouble, float& curMinD2, ElementInteractable& e);

void HelpMarker(const char* desc);

bool NotHoveringIMGui();

sf::Color ToSFMLColor(float colors[3]);

double cross(const vec2d& a, const vec2d& b);

// v ccw
bool IsInsideConvexShape(const vec2d& p, const std::vector<vec2d>& v);

// shoelace formula
template<class T = double, class V>
T TwiceAreaPolygonSigned(const V& cell)
{
	T ret = 0;
	const auto n = cell.size();
	for (size_t i = 0; i != n; i++)
	{
		const auto& cur = cell[i];
		const auto& nxt = cell[(i + 1) % n];
		ret += cross(cur, nxt);
	}
	return ret;
}

// CCW
double AreaTrig(const vec2d& p1, const vec2d& p2, const vec2d& p3);

// a2 is twice the area signed
template<class T, class V>
vec2_t<T> CalculateCentroid(const V& cell, T a2)
{
	T sx = 0, sy = 0;
	const auto n = cell.size();
	for (size_t i = 0; i != n; i++)
	{
		const auto& cur = cell[i];
		const auto& nxt = cell[(i + 1) % n];
		const auto c = cross(cur, nxt);
		sx += (cur.x + nxt.x) * c;
		sy += (cur.y + nxt.y) * c;
	}
	auto invM = static_cast<T>(1) / (3 * a2);
	return { sx * invM, sy * invM };
}

template<class T = double, class V>
vec2_t<T> CalculateCentroid(const V& cell)
{
	auto a2 = TwiceAreaPolygonSigned<T>(cell);
	return CalculateCentroid(cell, a2);
}

std::vector<vec2d> CalculateCentroids(const std::vector<Cell>& cells);
bool ColorPicker3U32(const char* label, sf::Color& c);

static constexpr double EPS_DBS = .0001;

sf::Vector2f ToSFML(const vec2f& v);

void PrepareCircleRadius(sf::CircleShape& circle, float r);

sf::Vector2f ToSFML(const RVO::Vector2& v);