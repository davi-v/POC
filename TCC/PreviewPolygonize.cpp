#include "Pch.hpp"
#include "PreviewPolygonize.hpp"

#include "FileExplorer.hpp"
#include "OpenCVSFML.hpp"
#include "Application.hpp"
#include "ImgViewer.hpp"

void
mark_domains(
	CDTP2& ct,
	Face_handle start,
	int index,
	std::queue<CDT2::Edge>& border)
{
	if (start->info().nesting_level != -1)
		return;
	std::queue<Face_handle> queue;
	queue.emplace(start);
	while (!queue.empty())
	{
		Face_handle fh = queue.front();
		queue.pop();
		if (fh->info().nesting_level == -1)
		{
			fh->info().nesting_level = index;
			for (int i = 0; i != 3; i++)
			{
				CDT2::Edge e(fh, i);
				Face_handle n = fh->neighbor(i);
				if (n->info().nesting_level == -1)
				{
					if (ct.is_constrained(e))
						border.emplace(e);
					else
						queue.emplace(n);
				}
			}
		}
	}
}
void mark_domains(CDTP2& cdt)
{
	for (auto& f : cdt.all_face_handles())
		f->info().nesting_level = -1;
	std::queue<CDT2::Edge> border;
	mark_domains(cdt, cdt.infinite_face(), 0, border);
	while (!border.empty())
	{
		CDT2::Edge e = border.front();
		border.pop();
		auto n = e.first->neighbor(e.second);
		if (n->info().nesting_level == -1)
			mark_domains(cdt, n, e.first->info().nesting_level + 1, border);
	}
}

void PreviewPolygonize::recalculateSamplesRandom()
{
	samples.clear();
	const auto& img = *viewerBase.currentImage;
	const auto [X, Y] = img.getSize();
	std::deque<std::pair<unsigned, unsigned>> candidates;
	for (unsigned x = 0; x != X; x++)
		for (unsigned y = 0; y != Y; y++)
			if (img.getPixel(x, y).a)
				candidates.emplace_back(x, y);

	const auto nCandidates = candidates.size();
	
	std::mt19937 mt(samplesSeed);
	std::uniform_int_distribution<size_t> dIndex(0, nCandidates - 1);
	std::uniform_real_distribution<float> dOff(0, 1);

	samples.resize(limitedNSamples);
	auto off = viewerBase.imageOffsetF;
	for (size_t i = 0; i != limitedNSamples; i++)
	{
		auto& p = samples[i];
		const auto& ip = candidates[dIndex(mt)];
		p.x = (float)ip.first + off.x + dOff(mt);
		p.y = (float)ip.second + off.y + dOff(mt);
	}

	recalculateGreedyMethod();
}

double PreviewPolygonize::getPolArea(double r)
{
	return curPolArea * square(r);
}

void PreviewPolygonize::recalculateCirclePointCount()
{
	curCirclePointCount.resize(pointCount);
	const auto angle = std::numbers::pi * 2 / pointCount;
	for (size_t i = 0; i != pointCount; i++)
	{
		auto& p = curCirclePointCount[i];
		p.x = cos(angle * i);
		p.y = sin(angle * i);
	}
	curPolArea = sin(angle) * .5 * pointCount; // l*l*sin(angle)/2 = area of a single trig
}

bool PreviewPolygonize::shouldDrawExtraLines()
{
	return drawExtraSampleLines && sampleMode != SampleMode::Random;
}

Cell PreviewPolygonize::getCellFromCoordAndRadius(const vec2d& coord, double r)
{
	Cell cell(pointCount);
	for (size_t i = 0; i != pointCount; i++)
		cell[i] = coord + curCirclePointCount[i] * r;
	return cell;
}

double PreviewPolygonize::getRobotArea(size_t robotIdx) const
{
	return std::numbers::pi * square(robotRadius[robotIdx]);
}

double PreviewPolygonize::getAreaCovered(size_t robotIdx, size_t pointIdx)
{
	return cellCalculator.getArea(getCellFromCoordAndRadius((vec2d)samples[pointIdx], robotRadius[robotIdx]));
	//const auto& r = robotRadius[robotIdx];
	//auto& img = radiusToW.at(r).first;
	//auto& coord = samples[pointIdx];
	//auto& o = viewerBase.imageOffsetF;
	//return img.getPixel((unsigned)(coord.x-o.x), (unsigned)(coord.y-o.y)).a / 255. * getRobotArea(robotIdx);
}

bool PreviewPolygonize::shouldAddEdge(size_t i, size_t j) const
{
	const auto
		& v1 = gV[i],
		& v2 = gV[j];
	if (v1.first == v2.first) // mesmo robô não pode aparecer em 2 lugares
		return true;
	const auto
		& r1 = robotRadius[v1.first],
		& r2 = robotRadius[v2.first];
	const auto
		& p1 = samples[v1.second],
		& p2 = samples[v2.second];
	return distanceSquared(p1, p2) < square(r1 + r2); // não colidindo com o outro robô
}

void PreviewPolygonize::dumpGraph()
{
	if (const auto path = TryWriteFile(L"Choose Path", GRAPH_FORMAT_LENS, GRAPH_FORMAT_EXTS))
	{
		std::ofstream f(path);
		f.precision(std::numeric_limits<double>::max_digits10);
		const auto R = robotCoords.size();
		const auto P = samples.size();
		Clear(gV, gE);
		for (size_t r = 0; r != R; r++)
			for (size_t p = 0; p != P; p++)
			{
				const auto w = getAreaCovered(r, p);
				if (w >= getRobotArea(r) * ratioMinArea)
				{
					gV.emplace_back(r, p);
					f << w << '\n';
				}
			}
		const auto V = gV.size();
		for (size_t i = 0; i != V; i++)
			for (size_t j = i + 1; j != V; j++)
				if (shouldAddEdge(i, j))
				{
					gE.emplace_back(i, j);
					f << i << ' ' << j << '\n';
				}
	}
}

void PreviewPolygonize::dumpGraph2()
{
	if (const auto path = TryWriteFile(L"Choose Path", GRAPH_FORMAT_LENS, GRAPH_FORMAT_EXTS))
	{
		std::ofstream f(path);
		const auto R = robotCoords.size();
		const auto P = samples.size();
		f << R << ' ' << P << ' ' << ratioMinArea << '\n';
		f.precision(std::numeric_limits<float>::max_digits10);
		for (const auto& r : robotRadius)
			f << r << '\n';
		for (const auto& p : samples)
			f << p.x << ' ' << p.y << '\n';
		std::deque<std::pair<size_t, size_t>> invalidPoints;
		for (size_t r = 0; r != R; r++)
		{
			for (size_t p = 0; p != P; p++)
				f << getAreaCovered(r, p) << ' ';
			f << '\n';
		}
	}
}

void PreviewPolygonize::importAssignment()
{
	if (const auto path = TryOpenFile(L"Choose Assignment File", GRAPH_FORMAT_LENS, GRAPH_FORMAT_EXTS))
	{
		std::ifstream f(path);
		size_t x;
		std::fill(targets.begin(), targets.end(), size_t(-1));
		const auto V = gV.size();
		while (f >> x)
		{
			if (x >= V)
			{
				LOG("Assignment out of range\n");
				break;
			}
			const auto& v = gV[x];
			targets[v.first] = v.second;
		}
	}
}

void PreviewPolygonize::importAssignment2()
{
	if (const auto path = TryOpenFile(L"Choose Assignment File", GRAPH_FORMAT_LENS, GRAPH_FORMAT_EXTS))
	{
		std::ifstream f(path);
		size_t a, b;
		std::fill(targets.begin(), targets.end(), size_t(-1));
		const auto
			R = robotCoords.size(),
			P = samples.size();
		while (f >> a >> b)
		{
			if (a >= R || b >= P)
			{
				LOG("Assignment out of range\n");
				break;
			}
			targets[a] = b;
		}
	}
}

void PreviewPolygonize::onChangeRadius(float r1, float r2)
{
	recalculateGreedyMethod();
}

void PreviewPolygonize::recalculateSamples()
{
	if (sampleMode == SampleMode::Random)
		recalculateSamplesRandom();
	else
		recalculateSamplesCentroid();
}

void PreviewPolygonize::recalculateSamplesCentroid()
{
	trianglesForSample.resize(trigsSimplifiedCnt);
	for (size_t i = 0, j = 0; i != trigsSimplifiedCnt; i++, j += 3)
		for (size_t x = 0; x != 3; x++)
		{
			auto& c = trigsSimplified[j + x].position;
			trianglesForSample[i][x] = { (double)c.x, (double)c.y };
		}
	std::tie(samples, sampleLines) = SelectPoints(trianglesForSample, limitedNSamples, sampleMode == SampleMode::QuadCentroid);
	recalculateGreedyMethod();
}

void PreviewPolygonize::recalculateGreedyMethod()
{
	const auto n = robotCoords.size();

	targets.resize(n);
	std::fill(targets.begin(), targets.end(), size_t(-1));

	std::vector<size_t> biggerIdxToSmaller(n);
	std::iota(biggerIdxToSmaller.begin(), biggerIdxToSmaller.end(), 0);
	std::sort(biggerIdxToSmaller.begin(), biggerIdxToSmaller.end(), [&](size_t a, size_t b)
		{
			return robotRadius[a] > robotRadius[b];
		});
	const auto curNSamples = samples.size();
	std::vector<bool> used(limitedNSamples);
	std::deque<std::pair<size_t, size_t>> idxUsed;
	for (const auto& idxRobot : biggerIdxToSmaller)
	{
		const auto& coord = robotCoords[idxRobot];
		const auto& r = robotRadius[idxRobot];
		const auto rd = (double)r;
		const auto r2 = square(r);
		auto curBestSample = (size_t)-1;
		auto bestRatio = ratioMinArea;
		const auto polArea = getPolArea(rd);
		for (size_t j = 0; j != limitedNSamples; j++)
			if (!used[j])
			{
				auto notCollidingWithAlreadyAllocatedRobots = [&]
				{
					for (const auto& [nRobot, nSample] : idxUsed)
						if (square(samples[nSample] - samples[j]) < square(r + robotRadius[nRobot]))
							return false;
					return true;
				};
				if (notCollidingWithAlreadyAllocatedRobots())
				{
					auto area = cellCalculator.getArea(getCellFromCoordAndRadius(samples[j], rd));
					if (area >= bestRatio * polArea)
					{
						curBestSample = j;
						bestRatio = area / polArea;
					}
				}
			}
		if (curBestSample != -1)
		{
			idxUsed.emplace_back(idxRobot, curBestSample);

			// já marcando os samples dentro do círculo
			for (size_t j = 0; j != limitedNSamples; j++)
				if (square(samples[j] - samples[curBestSample]) < r2)
					used[j] = true;

			targets[idxRobot] = curBestSample;
		}
	}
}

void PreviewPolygonize::recalculate()
{
	const auto& imgViewer = viewerBase;
	const auto& img = *imgViewer.currentImage;
	const auto& [ox, oy] = imgViewer.imageOffsetF;
	const auto [W, H] = img.getSize();
	std::deque<Point> points;
	Clear(orgVertices);
	for (unsigned x = 0; x != W; x++)
		for (unsigned y = 0; y != H; y++)
		{
			auto px = img.getPixel(x, y);
			if (px.a)
			{
				float
					bx = (float)x + ox,
					by = (float)y + oy;
				auto add = [&](float a, float b)
				{
					orgVertices.emplace_back(a, b);
					points.emplace_back(a, b);
				};
				add(bx, by);
				if (x + 1 < W && !img.getPixel(x + 1, y).a) add(bx + 1, by);
				if (y + 1 < H && !img.getPixel(x, y + 1).a) add(bx, by + 1);
				if (x + 1 < W && y + 1 < H && !img.getPixel(x + 1, y + 1).a) add(bx + 1, by + 1);
			}
		}

	MakeUniquePtr(alphaShaperPtr, points.begin(), points.end(),
		alphaShapeRadius//,
		//Alpha_shape_2::GENERAL
	);

	auto& alphaShaper = *alphaShaperPtr;
	alphaVertices.clear();
	for (auto it = alphaShaper.alpha_shape_vertices_begin(), lim = alphaShaper.alpha_shape_vertices_end(); it != lim; it++)
	{
		auto& p = (**it).point();
		alphaVertices.emplace_back((float)p.x(), (float)p.y());
	}

	segments.clear();
	cdtp.clear();
	g.clear();
	for (auto it = alphaShaper.alpha_shape_edges_begin(), lim = alphaShaper.alpha_shape_edges_end(); it != lim; ++it)
	{
		auto s = alphaShaper.segment(*it);
		auto& [a, b] = segments.emplace_back();
		auto& p0 = s.source(); a = { (float)p0.x(), (float)p0.y() };
		auto& p1 = s.target(); b = { (float)p1.x(), (float)p1.y() };
		g[p0].emplace_back(p1);
		g[p1].emplace_back(p0);
		//LOG(p0, ' ', p1, '\n');
		cdtp.insert_constraint(p0, p1);
	}

	nComponents = alphaShaper.number_solid_components();

	//cdtp.split_subconstraint_graph_into_constraints();

	mark_domains(cdtp);
	trigs.clear();
	trigsOrgCnt = 0;
	for (auto& vIt : cdtp.finite_face_handles())
		if (vIt->info().in_domain())
		{
			trigsOrgCnt++;
			for (int i = 0; i != 3; i++)
			{
				auto& v = vIt->vertex(i)->point();
				auto& t = trigs.emplace_back();
				t.position = { (float)v.x(),(float)v.y() };
				t.color = orgTriangleColor;
			}
		}

	static constexpr size_t DEFAULT_N_SAMPLES = 100;
	limitedNSamples = DEFAULT_N_SAMPLES;

	// ter no máximo uns 100 triângulos na malha simplificada
	simplificationRatio = std::min(1., static_cast<double>(DEFAULT_N_SAMPLES) / trigsOrgCnt);
	updateSimplification();
}

void PreviewPolygonize::updateSimplification()
{
	namespace PS = CGAL::Polyline_simplification_2;
	typedef PS::Stop_below_count_ratio_threshold Stop;
	typedef PS::Squared_distance_cost Cost;

	CDTP2 ct;
	auto& alphaShaper = *alphaShaperPtr;
	std::deque<std::deque<Point>> Ps;
	std::set<Point> visited;
	for (const auto& [start, _] : g)
	{
		if (visited.emplace(start).second)
		{
			auto& P = Ps.emplace_back();
			auto get = [&](const Point& p) -> std::optional<Point>
			{
				auto& ns = g[p];
				for (size_t i = 0, lim = ns.size(); i != lim; i++)
				{
					const auto& n = ns[i];
					if (visited.emplace(n).second)
						return n;
				}
				return {};
			};
			Point pIt = start;
			P.emplace_back(start);
			while (const auto pOpt = get(pIt))
			{
				const auto& nxt = *pOpt;
				P.emplace_back(nxt);
				pIt = nxt;
			}
		}
	}

	for (auto& P : Ps)
		ct.insert_constraint(P.begin(), P.end(), true);

	//LOG(ct.number_of_vertices(), '\n');
	PS::simplify(ct, Cost(), Stop(simplificationRatio));
	//LOG(ct.number_of_vertices(), '\n');

	mark_domains(ct);
	trigsSimplified.clear();
	trigsSimplifiedCnt = 0;
	for (auto& vIt : ct.finite_face_handles())
		if (vIt->info().in_domain())
		{
			trigsSimplifiedCnt++;
			for (int i = 0; i != 3; i++)
			{
				auto& v = vIt->vertex(i)->point();
				auto& vert = trigsSimplified.emplace_back();
				vert.position = { (float)v.x(),(float)v.y() };
				vert.color = simplifiedTriangleColor;
			}
		}

	recalculateSamples();

	segsSimplified.clear();
	for (auto& cit : ct.constraints())
	{
		std::deque<Point> d;
		for (auto& p : ct.points_in_constraint(cit))
			d.emplace_back(p);
		if (auto lim = d.size())
		{
			auto to = [&](auto&& p)
			{
				sf::Vertex v;
				v.position = { (float)p.x(), (float)p.y() };
				v.color = polylineColor;
				return v;
			};
			auto off = segsSimplified.size();
			auto p0 = to(d.front());
			segsSimplified.emplace_back(p0);
			verticesSimplified.resize(lim);
			verticesSimplified[0] = p0.position;
			for (size_t i = 1; i != lim; i++)
			{
				auto newP = to(d[i]);
				segsSimplified.emplace_back(newP);
				segsSimplified.emplace_back(segsSimplified.back());
				verticesSimplified[i] = newP.position;
			}
			segsSimplified.emplace_back(segsSimplified[off]);
		}
	}
}

void PreviewPolygonize::drawUIImpl()
{
	if (ImGui::CollapsingHeader("Polygonization"))
	{
		ImGui::Checkbox("Draw Original Vertices", &drawOrgVertices);

		auto colorPick = [](const char* str, sf::Color& color, std::vector<sf::Vertex>& container)
		{
			if (ColorPicker3U32(str, color))
				for (auto& v : container)
					v.color = color;
		};
		auto helper = [&](const std::string& imguiId, auto&& title, size_t nv, size_t nt, bool& drawV, bool& drawP, bool& drawT, sf::Color& color, std::vector<sf::Vertex>& trigs)
		{
			if (ImGui::TreeNode(title))
			{
				ImGui::Text("Number of Vertices: %zu", nv);
				ImGui::Text("Number of Triangles: %zu", nt);
				ImGui::Checkbox(("Vertices##" + imguiId).c_str(), &drawV);
				ImGui::Checkbox(("Polylines##" + imguiId).c_str(), &drawP);
				ImGui::Checkbox(("Triangles##" + imguiId).c_str(), &drawT);
				colorPick(("Triangles Color##" + imguiId).c_str(), color, trigs);
				ImGui::TreePop();
				return true;
			}
			return false;
		};

		if (helper("0", "Alpha Shape", alphaVertices.size(), trigsOrgCnt, drawAlphaVertices, drawAlphaShapePolylines, drawTrigs, orgTriangleColor, trigs))
		{
			{
				static constexpr double
					vmin = .5,
					vmax = 100000.;
				if (ImGui::DragScalar("Radius", ImGuiDataType_Double, &alphaShapeRadius, 1.f, &vmin, &vmax, "%.3f", 0))
					recalculate();
			}
		}

		if (helper("1", "Simplification", verticesSimplified.size(), trigsSimplifiedCnt, drawVerticesSimplified, drawSimplifiedPolylines, drawTrigsSimplified, simplifiedTriangleColor, trigsSimplified))
		{
			{
				static constexpr double
					vmin = .01,
					vmax = 1;
				if (ImGui::DragScalar("Simplification Ratio", ImGuiDataType_Double, &simplificationRatio, .01f, &vmin, &vmax, "%.3f", 0)) updateSimplification();
			}
		}

		ImGui::Text("Components: %zu", nComponents);

		colorPick("Polyline Color", polylineColor, segsSimplified);
	}

	if (ImGui::CollapsingHeader("Samples"))
	{
		ImGui::Checkbox("Draw Samples", &drawVerticesSampled);
		{
			static constexpr float
				vmin = .25,
				vmax = 50;
			ImGui::SliderScalar("Radius", ImGuiDataType_Float, &samplePointRadius, &vmin, &vmax, "%.3f", 0);
		}
		static constexpr const char* SAMPLE_MODES[] = {
			"TrigCentroid",
			"QuadCentroid",
			"Random"
		};
		if (ImGui::Combo("Sample Mode", reinterpret_cast<int*>(&sampleMode), SAMPLE_MODES, IM_ARRAYSIZE(SAMPLE_MODES)))
			recalculateSamples();
		static constexpr const char* SAMPLE_RANDOM_PIXEL_MODES[] = {
				"Limited Number",
				"Use All Nonempty Pixels",
				"Use All Pixels"
		};
		if (sampleMode == SampleMode::Random)
		{
			if (ImGui::InputScalar("Seed", ImGuiDataType_U32, &samplesSeed))
				recalculateSamplesRandom();
		}
		else
			ImGui::Checkbox("Draw Sampled Triangles", &drawExtraSampleLines);

		static constexpr size_t
			VMAX = 1000,
			STEP = 1;
		if (ImGui::InputScalar("Number of Samples", ImGuiDataType_U64, &limitedNSamples, &STEP))
		{
			limitedNSamples = std::min(limitedNSamples, VMAX);
			recalculateSamples();
		}
		ColorPicker3U32("Color##1", sampleColor);
	}

	if (ImGui::CollapsingHeader("Robots"))
	{
		ImGui::Checkbox("Draw Robots", &drawRobots);
		ImGui::Checkbox("Draw Goals", &drawGoals);
		ColorPicker3U32("Color", robotColor);
		ColorPicker3U32("Goals Color", viewerBase.circleColorSFML);

		ImGui::Checkbox("Fill Area", &fillArea);

		if (!fillArea)
		{
			ImGui::Checkbox("Highlight Hovered", &highlightHovered);
			static constexpr size_t
				vmin = 3,
				vmax = 100;
			if (ImGui::SliderScalar("Circle Point Count", ImGuiDataType_U64, &pointCount, &vmin, &vmax))
				recalculateCirclePointCount();

			if (circleHovered != -1)
			{
				const auto r = (double)robotRadius[circleHovered];
				const auto cell = getCellFromCoordAndRadius(robotCoords[circleHovered], r);
				double area = cellCalculator.getArea(cell);
				ImGui::Text("Hovered Polygon Area: %.3f", area);
				ImGui::Text("Hovered Polygon Area Ratio: %.3f", area / getPolArea(r));
			}
		}
	}

	if (ImGui::CollapsingHeader("Allocation"))
	{
		{
			static constexpr double
				vmin = 0,
				vmax = 1;
			if (ImGui::SliderScalar("Ratio Min Area", ImGuiDataType_Double, &ratioMinArea, &vmin, &vmax))
				recalculateGreedyMethod();
		}
	}

	if (ImGui::CollapsingHeader("Dump/Import Allocation Graph"))
	{
		auto helper = [&](auto&& title, auto&& f1, auto&& f2)
		{
			if (ImGui::TreeNode(title))
			{
				if (ImGui::MenuItem("Dump Graph"))
					(this->*f1)();
				if (ImGui::MenuItem("Import Assignment"))
					(this->*f2)();
				ImGui::TreePop();
			}
		};
		helper("Format 1", &PreviewPolygonize::dumpGraph, &PreviewPolygonize::importAssignment);
		helper("Format 2", &PreviewPolygonize::dumpGraph2, &PreviewPolygonize::importAssignment2);
	}

	if (nav)
	{
		if (!nav->drawUI())
			nav.reset();
	}
	else
	{
		if (ImGui::CollapsingHeader("Random Initial Robots"))
		{
			static constexpr unsigned
				MIN_N = 0,
				MAX_N = 100;
			static constexpr double
				MIN_R = .5,
				MAX_R = 100;
			ImGui::DragScalar("Number of Robots", ImGuiDataType_U32, &nRobots, 1.f, &MIN_N, &MAX_N);
			ImGui::DragScalar("Min R", ImGuiDataType_Double, &minR, 1.f, &MIN_R, &maxR);
			ImGui::DragScalar("Max R", ImGuiDataType_Double, &maxR, 1.f, &minR, &MAX_R);
			ImGui::InputScalar("Seed##2", ImGuiDataType_U32, &robotsSeed);
			if (ImGui::MenuItem("Generate"))
			{
				robotCoords.resize(nRobots);
				robotRadius.resize(nRobots);
				robotRadiusOffs.resize(nRobots);
				std::fill(robotRadiusOffs.begin(), robotRadiusOffs.end(), 1.f);
				if (nRobots)
				{
					auto lenRow = (size_t)ceil(sqrt(nRobots));
					auto div = nRobots / lenRow;
					auto mod = nRobots % lenRow;
					std::mt19937 mt(robotsSeed);
					std::uniform_real_distribution d((float)minR, (float)maxR);
					const auto [X, Y] = viewerBase.currentImage->getSize();
					const auto& off = viewerBase.imageOffsetF;
					const auto stride = maxR * 2;
					auto
						xStart = (double)off.x + maxR,
						y = (double)off.y + Y + maxR;
					size_t curId = 0;
					auto doRow = [&](size_t len)
					{
						auto x = xStart;
						for (size_t j = 0; j != len; j++, x += stride)
						{
							robotCoords[curId] = { x, y };
							robotRadius[curId] = d(mt);
							curId++;
						}
						y += stride;
					};
					for (size_t i = 0; i != div; i++)
						doRow(lenRow);
					doRow(mod);
				}
				recalculateGreedyMethod();
			}
		}
		if (ImGui::Button("Navigator"))
		{
			const auto n = robotCoords.size();
			if (n)
			{
				goals.clear();
				const auto nGoals = std::count_if(targets.begin(), targets.end(), [](size_t v)
					{
						return v != -1;
					});
				goals.reserve(nGoals);
				for (size_t i = 0; i != n; i++)
				{
					if (targets[i] != -1)
						goals.emplace_back(samples[targets[i]], robotRadius[i]);
				}

				agents.clear();
				agents.reserve(n);
				auto curGPtr = goals.data();
				for (size_t i = 0; i != n; i++)
				{
					Goal* gPtr;
					const auto& t = targets[i];
					if (t == -1)
						gPtr = nullptr;
					else
						gPtr = curGPtr++;
					agents.emplace_back(robotCoords[i], robotRadius[i], gPtr);
				}
				MakeUniquePtr(nav, viewerBase, agents);
			}
			else
			{
				viewerBase.app->addMessage("Não há robôs");
			}
		}
	}
}

void PreviewPolygonize::drawExtra()
{
	auto& w = viewerBase.app->window;
	auto drawV = [&](auto&& container, const sf::Color& color)
	{
		auto& imgViewer = viewerBase;
		auto& c = imgViewer.circle;
		c.setFillColor(color);
		PrepareCircleRadius(c, samplePointRadius);
		for (const auto& coord : container)
		{
			c.setPosition(coord);
			w.draw(c);
		}
	};
	if (drawOrgVertices)
		drawV(orgVertices, orgVertexColor);
	if (drawAlphaVertices)
		drawV(alphaVertices, alphaVertColor);
	if (drawVerticesSimplified)
		drawV(verticesSimplified, alphaVertColor);
	if (drawAlphaShapePolylines)
	{
		for (const auto& [a, b] : segments)
		{
			sf::Vertex v[2]
			{
				{a, polylineColor },
				{b, polylineColor }
			};
			w.draw(v, 2, sf::Lines);
		}
	}
	auto drawTrigsH = [&](auto&& t)
	{
		const auto len = t.size();
		for (size_t i = 0; i != len; i += 3)
		{
			auto d = [&](size_t x, size_t y)
			{
				sf::Vertex v[2]
				{
					t[i + x],
					t[i + y]
				};
				w.draw(v, 2, sf::Lines);
			};
			for (size_t j = 0; j != 3; j++)
				d(j, (j + 1) % 3);
		}
	};
	if (drawTrigs)
		drawTrigsH(trigs);
	const auto shouldDraw = shouldDrawExtraLines();
	if (drawTrigsSimplified || shouldDrawExtraLines())
		drawTrigsH(trigsSimplified);
	if (shouldDraw)
	{
		for (auto& [a, b] : sampleLines)
		{
			sf::Vertex v[2]
			{
				{a, simplifiedTriangleColor},
				{b, simplifiedTriangleColor}
			};
			w.draw(v, 2, sf::Lines);
		}
	}
	if (drawSimplifiedPolylines)
		w.draw(segsSimplified.data(), segsSimplified.size(), sf::Lines);
	if (!fillArea)
	{
		const auto n = robotCoords.size();
		for (size_t i = 0; i != n; i++)
		{
			const auto& r = robotRadius[i];
			const auto& coord = robotCoords[i];
			std::vector<sf::Vertex> vert(pointCount + 1);
			sf::Color color;
			if (highlightHovered && circleHovered == i)
				color = highlightColor;
			else
				color = robotColor;
			for (size_t j = 0; j != pointCount; j++)
			{
				auto& v = vert[j];
				v.position.x = (float)coord.x + (float)curCirclePointCount[j].x * r;
				v.position.y = (float)coord.y - (float)curCirclePointCount[j].y * r;
				v.color = color;
			}
			vert.back() = vert.front(); // SFML não tem LineFan...
			w.draw(vert.data(), pointCount + 1, sf::LineStrip);
		}
	}
	if (drawRobots)
	{
		viewerBase.app->drawCircles(robotCoords, robotRadius, robotColor);
	}
	if (drawGoals)
	{
		const auto n = robotCoords.size();
		auto& c = viewerBase.circle;
		for (size_t i = 0; i != n; i++)
		{
			const auto& target = targets[i];
			if (target != -1)
			{
				const auto& goal = samples[target];
				PrepareCircleRadius(c, robotRadius[i]);
				c.setFillColor(viewerBase.getColor(goal));
				c.setPosition(goal);
				w.draw(c);
			}
		}
	}
	if (drawVerticesSampled)
		drawV(samples, sampleColor);

	if (nav)
	{
		nav->loopUpdate();
		nav->draw();
	}
}

void PreviewPolygonize::onImgChangeImpl()
{
}

const char* PreviewPolygonize::getTitle()
{
	return "Polygonize";
}

void PreviewPolygonize::onAdd(float r)
{
	recalculateGreedyMethod();
}

void PreviewPolygonize::onDelete(float r)
{
	recalculateGreedyMethod();
}

PreviewPolygonize::PreviewPolygonize(ViewerBase& viewerBase) :
	CommonEditor(viewerBase),
	cellCalculator(viewerBase),
	ratioMinArea(.5),
	alphaShapeRadius(1),
	samplesSeed(0),
	robotsSeed(0),
	samplePointRadius(.5),
	orgVertexColor(sf::Color(34, 33, 169)),
	orgTriangleColor(sf::Color::Yellow),
	simplifiedTriangleColor(sf::Color::Red),
	polylineColor(sf::Color::Cyan),
	sampleColor(sf::Color::Blue),
	alphaVertColor(sf::Color::Red),
	drawOrgVertices(false),
	drawAlphaShapePolylines(false),
	drawAlphaVertices(false),
	drawVerticesSimplified(false),
	drawTrigs(false),
	drawTrigsSimplified(false),
	drawSimplifiedPolylines(false),
	drawVerticesSampled(false),
	drawExtraSampleLines(false),
	drawRobots(true),
	drawGoals(true),
	fillArea(true),
	pointCount(10),
	minR(0.5),
	maxR(50),
	nRobots(10),
	highlightHovered(true),
	highlightColor(sf::Color::Yellow),
	sampleMode(SampleMode::Random)
{
	recalculate();
	recalculateCirclePointCount();
}