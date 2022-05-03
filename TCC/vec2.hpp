#pragma once

template<class T>
struct vec2
{
	vec2(T x, T y);
	vec2();
	T x, y;
	operator sf::Vector2f() const;
	operator RVO::Vector2() const;
	operator bool() const;
	vec2 operator-(const vec2& other) const;
	vec2 operator+(const vec2& other) const;
	vec2 operator*(T other) const;
	vec2 operator/(T other) const;
	vec2<T>& operator+=(const vec2<T>& other);
	vec2<T>& operator*=(T other);
	bool tryNormalize();
};

typedef vec2<double> Coord;

template<class T>
T dot(const vec2<T>& v1, const vec2<T>& v2)
{
	auto x2 = v1.x * v2.x;
	auto y2 = v1.y * v2.y;
	return x2 + y2;
}

template<class T>
auto square(const vec2<T>& v)
{
	return dot(v, v);
}

template<class T>
vec2<T> normalize(const vec2<T>& v);

template<class T>
vec2<T> operator*(T s, const vec2<T>& v);

template<class T>
inline vec2<T>::vec2(T x, T y) :
	x(x),
	y(y)
{
}

template<class T>
inline vec2<T>::vec2() :
	x(0),
	y(0)
{
}

template<class T>
vec2<T>::operator sf::Vector2f() const
{
	return { static_cast<float>(x), static_cast<float>(y) };
}

template<class T>
vec2<T>::operator RVO::Vector2() const
{
	return RVO::Vector2(static_cast<float>(x), static_cast<float>(y));
}

template<class T>
inline vec2<T>::operator bool() const
{
	return x || y;
}

template<class T>
inline vec2<T> vec2<T>::operator-(const vec2& other) const
{
	return vec2(x - other.x, y - other.y);
}

template<class T>
inline vec2<T> vec2<T>::operator+(const vec2& other) const
{
	return vec2(x + other.x, y + other.y);
}

template<class T>
inline vec2<T> vec2<T>::operator*(T other) const
{
	return vec2(x * other, y * other);
}

template<class T>
inline vec2<T> vec2<T>::operator/(T other) const
{
	return vec2(x / other, y / other);
}

template<class T>
inline vec2<T>& vec2<T>::operator+=(const vec2<T>& other)
{
	x += other.x;
	y += other.y;
	return *this;
}

template<class T>
inline vec2<T>& vec2<T>::operator*=(T other)
{
	return x *= other, y *= other, *this;
}

template<class T>
inline bool vec2<T>::tryNormalize()
{
	if (*this)
	{
		*this = normalize(*this);
		return true;
	}
	return false;
}

template<class T>
inline vec2<T> normalize(const vec2<T>& v)
{
	auto d2 = dot(v, v);
	auto len = sqrt(d2);
	return v / len;
}

template<class T>
inline vec2<T> operator*(T s, const vec2<T>& v)
{
	return v * s;
}
