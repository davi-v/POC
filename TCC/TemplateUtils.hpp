#pragma once

template<class...Rest>auto FileAppend(std::ofstream& f, Rest&&... rest)
{
	(f.write((char*)&rest, sizeof(rest)), ...);
}
template<class...Rest>auto FileRead(std::ifstream& f, Rest&&... rest)
{
	(f.read((char*)&rest, sizeof(rest)), ...);
}

template<class...Rest>auto Clear(Rest&&... rest)
{
	(rest.clear(), ...);
}

template<class T, class...Rest>auto MakeUniquePtr(std::unique_ptr<T>& ptr, Rest&&... rest)
{
	ptr = std::make_unique<T>(rest...);
}

template<class T>constexpr auto square(T x)
{
	return x * x;
}