#include "Pch.hpp"
#include "SampleBased.hpp"

#include "Application.hpp"
#include "FileExplorer.hpp"

SampleBased::SampleBased(ViewerBase& viewerBase) :
	CommonEditor(viewerBase),
	drawOrgVertices(false),
	orgVertexColor(sf::Color(34, 33, 169)),
	goalColor(sf::Color::Green),
	highlightColor(sf::Color::Yellow),
	cellAreaCalculator(viewerBase),
	ratioMinArea(.8),
	pointCount(10),
	limitedNSamples(DEFAULT_N_SAMPLES),
	samplePointRadius(.5),
	drawRobots(true),
	drawGoals(true),
	fillArea(true),
	highlightHovered(true),
	drawVerticesSampled(false),
	sampleColor(sf::Color::Blue),
	minR(.5),
	maxR(50),
	robotsSeed(0),
	nRobots(10)
{
	viewerBase.circleColor = sf::Color::Red;
	recalculateCirclePointCount();
}

void SampleBased::drawExtraSampleBased()
{
}

void SampleBased::recalculateTargetsGreedyMethod()
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
					auto area = cellAreaCalculator.getArea(getCellFromCoordAndRadius(samples[j], rd));
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

double SampleBased::getPolArea(double r)
{
	return curPolArea * square(r);
}

Cell SampleBased::getCellFromCoordAndRadius(const vec2d& coord, double r)
{
	Cell cell(pointCount);
	for (size_t i = 0; i != pointCount; i++)
		cell[i] = coord + curCirclePointCount[i] * r;
	return cell;
}

void SampleBased::recalculateCirclePointCount()
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

void SampleBased::drawUIImpl()
{
	if (ImGui::CollapsingHeader("Samples"))
	{
		ImGui::Checkbox("Draw Samples", &drawVerticesSampled);
		{
			static constexpr float
				vmin = .25,
				vmax = 50;
			ImGui::SliderScalar("Radius", ImGuiDataType_Float, &samplePointRadius, &vmin, &vmax, "%.3f", 0);
		}
		if (DragScalarMax("Number of Samples", limitedNSamples, size_t(100000)))
			recalculateSamplesOnly();
		ColorPicker3U32("Color##1", sampleColor);

		samplesExtraDrawUI();
	}

	if (ImGui::CollapsingHeader("Robots"))
	{
		ImGui::Checkbox("Draw Robots", &drawRobots);
		ImGui::Checkbox("Draw Goals", &drawGoals);
		ColorPicker3U32("Robots Color", viewerBase.circleColor);
		ColorPicker3U32("Goals Color", goalColor);

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
				double area = cellAreaCalculator.getArea(cell);
				if (area <= 0) // == tbm pro caso de -0
					area = 0;
				ImGui::Text("Hovered Polygon Area: %.3f", area);
				ImGui::Text("Hovered Polygon Area Ratio: %.3f", area / getPolArea(r));
			}
		}
	}

	if (ImGui::CollapsingHeader("Allocation"))
	{
		if (DragScalarMinMax("Ratio Min Area", ratioMinArea, .01f, 0., 1.))
			recalculateTargetsGreedyMethod();
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
		helper("Format 1", &SampleBased::dumpGraph, &SampleBased::importAssignment);
		helper("Format 2", &SampleBased::dumpGraph2, &SampleBased::importAssignment2);
	}

	if (nav)
	{
		if (!nav->drawUI())
		{
			saveCoordsFromNav();
			nav.reset();
		}
	}
	else
	{
		if (ImGui::CollapsingHeader("Random Initial Robots"))
		{
			static constexpr unsigned
				MAX_N = 1000;
			static constexpr double
				MIN_R = .5,
				MAX_R = 100;
			DragScalarMax("Number of Robots", nRobots, MAX_N);
			DragScalarMinMax("Min R", minR, MIN_R, maxR);
			DragScalarMinMax("Max R", maxR, minR, MAX_R);
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
				recalculateTargetsGreedyMethod();
			}
		}
		if (ImGui::Button("Navigator"))
		{
			drawRobots = drawGoals = false;
			const auto n = robotCoords.size();
			if (n)
				recreateNav();
			else
				viewerBase.app->addMessage("Não há robôs");
		}
	}

	drawUISpecific();
}

void SampleBased::drawUISpecific()
{
}

void SampleBased::saveCoordsFromNav()
{
	const auto coords = nav->getAgentPositions();
	const auto n = robotCoords.size();
	for (size_t i = 0; i != n; i++)
		robotCoords[i] = coords[i];
}

void SampleBased::dumpGraph()
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

void SampleBased::dumpGraph2()
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

void SampleBased::importAssignment()
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

void SampleBased::importAssignment2()
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

sf::RenderWindow& SampleBased::accessW()
{
	return viewerBase.app->window;
}

double SampleBased::getRobotArea(size_t robotIdx) const
{
	return std::numbers::pi * square(robotRadius[robotIdx]);
}

double SampleBased::getAreaCovered(size_t robotIdx, size_t pointIdx)
{
	return cellAreaCalculator.getArea(getCellFromCoordAndRadius((vec2d)samples[pointIdx], robotRadius[robotIdx]));
}

void SampleBased::drawExtraCommonEditor()
{
	auto& w = viewerBase.app->window;
	if (drawOrgVertices)
		drawV(orgVertices, orgVertexColor);
	if (drawRobots)
	{
		if (fillArea)
			viewerBase.app->drawCircles(robotCoords, robotRadius, viewerBase.circleColor);
		else
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
					color = viewerBase.circleColor;
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
				c.setFillColor(goalColor);
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
	drawExtraSampleBased();
}

void SampleBased::onImgChangeImpl()
{
	cellAreaCalculator = viewerBase;
	initSamplesAndAllocation();
	recalculateTargetsGreedyMethod();
	if (nav)
	{
		saveCoordsFromNav();
		nav->agents = recalculateGoalsGetAgents();
	}
}

void SampleBased::recreateNav()
{
	const auto agents = recalculateGoalsGetAgents();
	MakeUniquePtr(nav, viewerBase, agents);
}

bool SampleBased::shouldAddEdge(size_t i, size_t j) const
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

Agents SampleBased::recalculateGoalsGetAgents()
{
	goals.clear();
	const auto nGoals = std::count_if(targets.begin(), targets.end(), [](size_t v)
		{
			return v != -1;
		});
	goals.reserve(nGoals);
	const auto n = robotCoords.size();
	for (size_t i = 0; i != n; i++)
		if (targets[i] != -1)
			goals.emplace_back(samples[targets[i]], robotRadius[i]);

	std::vector<Agent> agents;
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
	return agents;
}

void SampleBased::onAdd()
{
	recalculateTargetsGreedyMethod();
}

void SampleBased::onDelete()
{
	recalculateTargetsGreedyMethod();
}


void SampleBased::onChangeRadius()
{
	recalculateTargetsGreedyMethod();
}