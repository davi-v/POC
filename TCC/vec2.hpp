#pragma once

template<class T>
struct vec2_t;

typedef vec2_t<float> vec2f;
typedef vec2_t<double> vec2d;

template<class T>
struct vec2_t
{
	constexpr vec2_t();
	constexpr vec2_t(T x, T y);
	constexpr explicit vec2_t(T val);

	template<class U>
	constexpr vec2_t(const sf::Vector2<U>& v);
	T x, y;
	operator sf::Vector2f() const;
	operator vec2f() const;
	operator vec2d() const;
	operator RVO::Vector2() const;
	explicit operator bool() const;
	vec2_t operator-(const vec2_t& other) const;
	vec2_t operator+(const vec2_t& other) const;
	vec2_t operator+(T o) const;
	vec2_t operator*(T other) const;
	vec2_t operator/(T other) const;
	vec2_t<T>& operator+=(const vec2_t<T>& other);
	vec2_t<T>& operator-=(const vec2_t<T>& other);
	vec2_t<T>& operator*=(T other);
	vec2_t& normalize();
	bool tryNormalize();
};

template<class T>bool operator==(const vec2_t<T>& a, const vec2_t<T>& b);
template<class T>vec2_t<T> operator-(const vec2_t<T>& v);


template<class T>
vec2_t<T> normalize(const vec2_t<T>& v);

template<class T>
T dot(const vec2_t<T>& v1, const vec2_t<T>& v2)
{
	auto x2 = v1.x * v2.x;
	auto y2 = v1.y * v2.y;
	return x2 + y2;
}

template<class T>
T cross(const vec2_t<T>& v1, const vec2_t<T>& v2)
{
	return v1.x * v2.y - v1.y * v2.x;
}

template<class T>
auto square(const vec2_t<T>& v)
{
	return dot(v, v);
}

template<class T>
auto length(const vec2_t<T>& v)
{
	return sqrt(dot(v, v));
}

template<class T>
vec2_t<T> operator*(T s, const vec2_t<T>& v);

template<class T>
inline constexpr vec2_t<T>::vec2_t(T x, T y) :
	x(x),
	y(y)
{
}

template<class T>
inline constexpr vec2_t<T>::vec2_t(T val) :
	x(val),
	y(val)
{
}

template<class T>
template<class U>
inline constexpr vec2_t<T>::vec2_t(const sf::Vector2<U>& v) :
	x((T)v.x),
	y((T)v.y)
{
}

template<class T>
inline constexpr vec2_t<T>::vec2_t() :
	x(0),
	y(0)
{
}

template<class T>
vec2_t<T>::operator sf::Vector2f() const
{
	return { static_cast<float>(x), static_cast<float>(y) };
}

template<class T>
inline vec2_t<T>::operator vec2f() const
{
	return { (float)x, (float)y };
}

template<class T>
inline vec2_t<T>::operator vec2d() const
{
	return { (double)x, (double)y };
}

template<class T>
vec2_t<T>::operator RVO::Vector2() const
{
	return RVO::Vector2(static_cast<float>(x), static_cast<float>(y));
}

template<class T>
inline vec2_t<T>::operator bool() const
{
	return x || y;
}

template<class T>
inline vec2_t<T> vec2_t<T>::operator-(const vec2_t& other) const
{
	return vec2_t(x - other.x, y - other.y);
}

template<class T>
inline vec2_t<T> vec2_t<T>::operator+(const vec2_t& other) const
{
	return vec2_t(x + other.x, y + other.y);
}

template<class T>
inline vec2_t<T> vec2_t<T>::operator+(T o) const
{
	return { x + o, y + o };
}

template<class T>
inline vec2_t<T> vec2_t<T>::operator*(T other) const
{
	return vec2_t(x * other, y * other);
}

template<class T>
inline vec2_t<T> vec2_t<T>::operator/(T other) const
{
	return vec2_t(x / other, y / other);
}

template<class T>
inline vec2_t<T>& vec2_t<T>::operator+=(const vec2_t<T>& other)
{
	x += other.x;
	y += other.y;
	return *this;
}

template<class T>
inline vec2_t<T>& vec2_t<T>::operator-=(const vec2_t<T>& other)
{
	x -= other.x;
	y -= other.y;
	return *this;
}

template<class T>
inline vec2_t<T>& vec2_t<T>::operator*=(T other)
{
	return x *= other, y *= other, *this;
}

template<class T>
inline vec2_t<T>& vec2_t<T>::normalize()
{
	auto& v = *this;
	v = v / length(v);
	return v;
}

template<class T>
inline bool vec2_t<T>::tryNormalize()
{
	if (*this)
	{
		normalize();
		return true;
	}
	return false;
}

template<class T>
inline bool operator==(const vec2_t<T>& a, const vec2_t<T>& b)
{
	return a.x == b.x && a.y == b.y;
}

template<class T>
inline vec2_t<T> operator-(const vec2_t<T>& v)
{
	return vec2_t<T>(-v.x, -v.y);
}

template<class T>
inline vec2_t<T> normalize(const vec2_t<T>& v)
{
	auto ret = v;
	return ret.normalize();
}

template<class T>
inline vec2_t<T> operator*(T s, const vec2_t<T>& v)
{
	return v * s;
}

template<class T>
vec2_t<T> projPointUAxis(const vec2_t<T>& p, const vec2_t<T>& uAxis)
{
	return uAxis * dot(p, uAxis);
}

///// Returns projection of @P in segment formed by points @A and @B
//template<typename T>
//inline T projPointSegment(const T& P, const T& A, const T& B)
//{
//	auto d = B - A;
//	return A + dot(P - A, d) / square(d) * d;
//}

template<class T>
vec2_t<T> mean(const vec2_t<T>& a, const vec2_t<T>& b)
{
	return (a + b) * static_cast<T>(.5);
}

template<class T>
T distanceSquared(const vec2_t<T>& c1, const  vec2_t<T>& c2)
{
	return square(c1 - c2);
}

template<class T>
T distance(const vec2_t<T>& c1, const vec2_t<T>& c2)
{
	return sqrt(distanceSquared(c1, c2));
}