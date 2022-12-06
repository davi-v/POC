#pragma once
// adaptado de https://github.com/brunomaletta/Biblioteca/blob/90a6a6a1eac5ad710ff68b7e3d9841c73a76a326/Codigo/Problemas/hungarian.cpp

template<class T>
struct Hungarian
{
	size_t n;

	using M = std::vector<std::vector<T>>;
	M m;

	std::vector<T> u, v;
	std::vector<size_t> p, way;
	T inf;

	Hungarian(const M& m) :
		n(m.size()),
		m(m),
		u(n + 1),
		v(n + 1),
		p(n + 1),
		way(n + 1),
		inf(std::numeric_limits<T>::max())
	{
	}
	std::pair<T, std::vector<size_t>> assignment()
	{
		for (size_t i = 1; i <= n; i++)
		{
			p[0] = i;
			size_t j0 = 0;
			std::vector<T> minv(n + 1, inf);
			std::vector<size_t> used(n + 1, 0);
			do
			{
				used[j0] = true;
				size_t i0 = p[j0], j1 = -1;
				T delta = inf;
				for (size_t j = 1; j <= n; j++)
					if (!used[j])
					{
						T cur = m[i0 - 1][j - 1] - u[i0] - v[j];
						if (cur < minv[j])
							minv[j] = cur, way[j] = j0;
						if (minv[j] < delta)
							delta = minv[j], j1 = j;
					}
				for (size_t j = 0; j <= n; j++)
					if (used[j])
						u[p[j]] += delta, v[j] -= delta;
					else
						minv[j] -= delta;
				j0 = j1;
			} while (p[j0]);
			do
			{
				const auto& j1 = way[j0];
				p[j0] = p[j1];
				j0 = j1;
			} while (j0);
		}
		std::vector<size_t> ans(n);
		for (size_t j = 1; j <= n; j++)
			ans[p[j] - 1] = j - 1;
		return std::make_pair(-v[0], ans);
	}
};