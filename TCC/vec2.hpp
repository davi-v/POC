#pragma once

template<class T>
struct vec2_t
{
	vec2_t();
	vec2_t(T x, T y);
	T x, y;
	operator sf::Vector2f() const;
	operator RVO::Vector2() const;
	operator bool() const;
	vec2_t operator-(const vec2_t& other) const;
	vec2_t operator+(const vec2_t& other) const;
	vec2_t operator*(T other) const;
	vec2_t operator/(T other) const;
	vec2_t<T>& operator+=(const vec2_t<T>& other);
	vec2_t<T>& operator*=(T other);
	bool tryNormalize();
};

typedef vec2_t<double> vec2d;

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
vec2_t<T> normalize(const vec2_t<T>& v);

template<class T>
vec2_t<T> operator*(T s, const vec2_t<T>& v);

template<class T>
inline vec2_t<T>::vec2_t(T x, T y) :
	x(x),
	y(y)
{
}

template<class T>
inline vec2_t<T>::vec2_t() :
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
inline vec2_t<T>& vec2_t<T>::operator*=(T other)
{
	return x *= other, y *= other, *this;
}

template<class T>
inline bool vec2_t<T>::tryNormalize()
{
	if (*this)
	{
		*this = normalize(*this);
		return true;
	}
	return false;
}

template<class T>
inline vec2_t<T> normalize(const vec2_t<T>& v)
{
	auto d2 = dot(v, v);
	auto len = sqrt(d2);
	return v / len;
}

template<class T>
inline vec2_t<T> operator*(T s, const vec2_t<T>& v)
{
	return v * s;
}
