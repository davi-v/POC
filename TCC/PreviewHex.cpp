#include "Pch.hpp"
#include "PreviewHex.hpp"

#include "Application.hpp"

void PreviewHex::draw()
{
	auto& w = viewerBase.app.window;
	const auto& img = viewerBase.accessImgViewer();
	const auto& imageOffset = img.imageOffsetF;
	if (showHexGrid)
	{
		for (int x = 0; x != hexGridXMax; x++)
			for (int y = 0; y != hexGridYMax; y++)
			{
				auto coords = getHexagonCoordsInLocalSpace(x, y);
				for (auto& coord : coords)
					coord += imageOffset;
				sf::Vertex vertices[12]
				{
					{coords[0], hexGridColor},
					{coords[1], hexGridColor},
					{coords[1], hexGridColor},
					{coords[2], hexGridColor},
					{coords[2], hexGridColor},
					{coords[3], hexGridColor},
					{coords[3], hexGridColor},
					{coords[4], hexGridColor},
					{coords[4], hexGridColor},
					{coords[5], hexGridColor},
					{coords[5], hexGridColor},
					{coords[0], hexGridColor},
				};
				w.draw(vertices, 12, sf::Lines);
			}
	}
	if (showGoals)
	{
		auto& r = viewerBase.radius;
		sf::CircleShape circle{ r };
		if (showCircleBorderOnly)
		{
			circle.setOutlineColor(circleBorderColor);
			circle.setOutlineThickness(-r * .1f);
		}
		circle.setOrigin(r, r);
		for (const auto& circleCenter : goalsPositions)
		{
			circle.setPosition(circleCenter);
			circle.setFillColor(viewerBase.getColor(circleCenter));
			w.draw(circle);
		}
	}
	if (showSmallerHexagon)
	{
		auto localCoords = getHexagonOffsets();
		for (auto& coord : localCoords)
			coord = coord * smallerHexRatio + imageOffset;

		for (int x = 0; x != hexGridXMax; x++)
			for (int y = 0; y != hexGridYMax; y++)
			{
				auto center = getHexagonCenterInLocalSpace(x, y);
				auto coords = localCoords;
				for (auto& coord : coords)
					coord += center;
				if (false)
				{

					sf::Vertex vertices[]
					{
						{coords[0], smallHexColor},
						{coords[1], smallHexColor},
						{coords[1], smallHexColor},
						{coords[2], smallHexColor},
						{coords[2], smallHexColor},
						{coords[3], smallHexColor},
						{coords[3], smallHexColor},
						{coords[4], smallHexColor},
						{coords[4], smallHexColor},
						{coords[5], smallHexColor},
						{coords[5], smallHexColor},
						{coords[0], smallHexColor},
					};
					w.draw(vertices, sizeof(vertices) / sizeof(decltype(*vertices)), sf::Lines);
				}
				else
				{
					sf::Vertex vertices[]
					{
						{coords[0], smallHexColor},
						{coords[1], smallHexColor},
						{coords[2], smallHexColor},
						{coords[3], smallHexColor},
						{coords[4], smallHexColor},
						{coords[5], smallHexColor},
					};
					w.draw(vertices, sizeof(vertices) / sizeof(decltype(*vertices)), sf::TriangleFan);
				}
			}
	}
	if (highlightHexHovered)
	{
		sf::VertexArray hexagon(sf::TriangleFan, 6);
		auto point = sf::Mouse::getPosition(w);
		auto coord = w.mapPixelToCoords(point);
		coord -= imageOffset;
		if (auto optHexagon = tryGetHexagonHovered(coord))
		{
			auto& [x, y] = *optHexagon;
			auto coords = getHexagonCoordsInLocalSpace(x, y);
			for (int i = 0; i != 6; i++)
			{
				auto& p = hexagon[i];
				p.position = coords[i] + imageOffset;
				p.color = hexHighlightColor;
			}
			w.draw(hexagon);
		}
	}
	if (drawOrgOffsets)
		w.draw(offsetsLines.data(), offsetsLines.size(), sf::Lines);
	if (drawHexGridBorder)
	{
		sf::RectangleShape borderDrawing{ mapBorderSize };

		borderDrawing.setPosition(mapBorderCoord);
		borderDrawing.setFillColor(sf::Color{ 0 });
		borderDrawing.setOutlineColor(sf::Color::White);
		borderDrawing.setOutlineThickness(2.f);
		w.draw(borderDrawing);
	}
	if (sim)
		sim->draw();
}

void PreviewHex::pollEvent(const sf::Event& e)
{
}

const char* PreviewHex::getTitle()
{
	return "Hexagonal Grid";
}

PreviewHex::PreviewHex(ViewerBase& viewerBase) :
	Previewer(viewerBase),
	allocationType(AllocationType::Unlimited),
	nRobotsBudget(DEFAULT_N_ROBOTS),
	drawHexGridBorder{false},
	showGoals{ true },
	bestFitCircles{ true },
	hexPlotReady{ false },
	showCircleBorderOnly{ false },
	usePixelsInConfigurationSpaceOnly{false},
	clipToConfigurationSpace{ true },
	drawOrgOffsets{false},
	showHexGrid{ true },
	highlightHexHovered{false},
	showSmallerHexagon{false},
	numberOfBars{20},
	numberOfBarsPrev{numberOfBars - 1}
{
	updatedRadius();
	onImgChangeImpl();
	recalculateGoalPositions();
}

void PreviewHex::updatedRadiusCallback()
{
	updatedRadius();
	recalculateGoalPositions();
}

void PreviewHex::updatedRadius()
{
	curMinHexagonSide = viewerBase.radius * INV_SIN_60;
	updateSmallHexagonDataThatDependOnHexHeightAndRadius();

	if (usePixelsInConfigurationSpaceOnly) // mudar o raio não muda o número de círculos alocados se estamos ignorando o espaço de configuração
		signalNeedToUpdateHexPlot();
}

std::array<sf::Vector2f, 6> PreviewHex::getHexagonOffsets()
{
	return
	{
		sf::Vector2f{ +hexagonHalfSide, -hexHeight },
		sf::Vector2f{ +hexagonSide, 0 },
		sf::Vector2f{ +hexagonHalfSide, +hexHeight },
		sf::Vector2f{ -hexagonHalfSide, +hexHeight },
		sf::Vector2f{ -hexagonSide, 0 },
		sf::Vector2f{ -hexagonHalfSide, -hexHeight },
	};
}

std::array<sf::Vector2f, 6> PreviewHex::getHexagonCoordsInLocalSpace(int x, int y)
{
	auto [cx, cy] = getHexagonCenterInLocalSpace(x, y);
	auto offsets = getHexagonOffsets();
	for (auto& coord : offsets)
	{
		coord.x += cx;
		coord.y += cy;
	}
	return offsets;
}

void PreviewHex::onImgChangeImpl()
{
	signalNeedToUpdateHexPlot();
	const auto& img = viewerBase.accessImgViewer();
	const auto [C, H] = img.currentImage->getSize();
	const auto rowsF = static_cast<float>(H);
	vec2f p{ static_cast<float>(C), rowsF };
	const auto proj = projPointUAxis(p, HEX_AXIS[0]);
	curMaxHexagonSide = std::max(rowsF, length(proj)) / SIN_60;
	hexagonSide = std::max(img.imgWF * 0.05f, curMinHexagonSide);
	updateHexVars();
}

size_t PreviewHex::getNumRobotsAllocated()
{
	return goalsPositions.size();
}

void PreviewHex::drawUIImpl()
{
	if (ImGui::BeginMenu("Goals"))
	{
		ImGui::Checkbox("Show", &showGoals);
		if (ImGui::ColorEdit3("Color", viewerBase.circleColorF3))
			viewerBase.circleColorSFML = ToSFMLColor(viewerBase.circleColorF3);

		ImGui::Checkbox("Border Only", &showCircleBorderOnly);

		auto& r = viewerBase.radius;
		if (ShowRadiusOption(r, hexHeight))
			updatedRadiusCallback();

		ImGui::Begin("Filter");
		auto& img = viewerBase.accessImgViewer();
		ImGui::Image(img.currentFilterSprite);
		ImGui::End();

		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Hexagon"))
	{
		ImGui::Checkbox("Grid", &showHexGrid);
		ImGui::Checkbox("Rect Around Hex Grid", &drawHexGridBorder);
		ImGui::Checkbox("Configuration Space", &showSmallerHexagon);
		ImGui::Checkbox("Highlight Hovered", &highlightHexHovered);

		if (ImGui::SliderFloat("Side", &hexagonSide, curMinHexagonSide, curMaxHexagonSide))
			updateVarsThatDependOnCircleAndHexagon();

		ImGui::LabelText("Number of Circles Alocated", "%zu", goalsPositions.size());

		static constexpr int MAX_BARS = 1000;
		if (ImGui::DragInt("Number of Bars", &numberOfBars, 1, 2, MAX_BARS, "%d", ImGuiSliderFlags_AlwaysClamp))
		{
			numberOfBarsPrev = numberOfBars - 1;
			signalNeedToUpdateHexPlot();
		}

		if (!hexPlotReady)
		{
			if (ImGui::MenuItem("Create Hexagon Side Plot"))
			{
				hexPlotReady = true;

				auto hexSideUsing = hexagonSide;

				barData.resize(numberOfBars);
				for (int i = 0; i != numberOfBars; i++)
				{
					auto curOff = static_cast<float>(i) / numberOfBarsPrev;
					auto curHexSide = curMinHexagonSide * (1.f - curOff) + curMaxHexagonSide * curOff;
					barData[i] = getNumCirclesUsingThisHexagonSide(curHexSide);
				}

				hexagonSide = hexSideUsing;
				updateHexVars();
			}
		}
		if (hexPlotReady)
		{
			if (ImPlot::BeginPlot("Total Number of Allocated Robots per Hexagon Side"))
			{
				ImPlot::PlotBars("", barData.data(), static_cast<int>(barData.size()));
				ImPlot::EndPlot();
			}
		}

		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Allocation"))
	{
		static constexpr const char* ALLOCATION_TYPES_NAMES[] = {
			"Unlimited",
			"Limited"
		};
		if (ImGui::Combo("Type", (int*)&allocationType, ALLOCATION_TYPES_NAMES, IM_ARRAYSIZE(ALLOCATION_TYPES_NAMES)))
			if (allocationType == AllocationType::Limited)
				calculateBestHexagonSideBasedOnNRobotsAndR();

		ImGui::Checkbox("Draw Original Offsets", &drawOrgOffsets);

		if (ImGui::Checkbox("Use Only Pixels on Configuration Space", &usePixelsInConfigurationSpaceOnly))
			recalculateCircleCentersAndPlot();

		if (!usePixelsInConfigurationSpaceOnly)
			if (ImGui::Checkbox("Clip To Configuration Space", &clipToConfigurationSpace))
				if (!usePixelsInConfigurationSpaceOnly)
					recalculateGoalPositions();

		if (allocationType == AllocationType::Unlimited)
		{
			if (ImGui::Checkbox("Fit", &bestFitCircles))
				recalculateCircleCentersAndPlot();
		}
		else
			showNRobotsOption();

		ImGui::EndMenu();
	}
	if (sim)
	{
		if (!sim->drawUI())
		{
			sim.reset();
			showGoals = true;
		}
	}
	else
		if (ImGui::MenuItem("Navigator"))
		{
			showGoals = false;

			size_t curBudget;
			if (allocationType == AllocationType::Unlimited)
				curBudget = goalsPositions.size();
			else
				curBudget = nRobotsBudget;

			MakeUniquePtr(sim, viewerBase.app, viewerBase.radius);
			// posição inicial dos robôs
			size_t cnt = 0;
			for (int i = 0; i != hexGridXMax; i++)
				for (int j = 0; j != hexGridYMax; j++, cnt++)
				{
					if (cnt == curBudget)
						break;
					sim->addAgent(getHexagonCenterInWorldSpace(i, j));
				}
			for (const auto& v : goalsPositions)
				sim->addGoal({ static_cast<double>(v.x), static_cast<double>(v.y) });
			sim->updateGraphEdges();
			sim->editModeType = Simulator2D::EditModeType::Metric;
		}
}

sf::Vector2f PreviewHex::getHexagonCenterInLocalSpace(int x, int y)
{
	auto cx = x * hexagonStride;
	auto cy = y * hex2Heights;
	if (x % 2)
		cy += hexHeight;
	return { cx, cy };
}

sf::Vector2f PreviewHex::getHexagonCenterInWorldSpace(int x, int y)
{
	return getHexagonCenterInLocalSpace(x, y) + viewerBase.accessImgViewer().imageOffsetF;
}

const vec2f& PreviewHex::accessHexagonUAxis(const vec2f& off)
{
	auto angle = atan2(off.y, off.x);
	if (angle < 0)
		angle += 2 * PI;

	// angle \in [0, 2*pi]

	int i = 0;
	for (; i != 5; i++)
		if (angle < HEX_ANGLES[i])
			break;
	return HEX_AXIS[i];
}

bool PreviewHex::isPixelInsideSmallerHexagon(const sf::Vector2f& pixelCenter, int hx, int hy)
{
	auto hc = getHexagonCenterInLocalSpace(hx, hy);
	vec2f off = pixelCenter - hc;
	//off.y = -off.y; // not necessary because it's symmetric
	auto& uAxis = accessHexagonUAxis(off);
	auto proj = projPointUAxis(off, uAxis);
	auto len2 = dot(proj, proj);
	return len2 < smallerHexHeightSquared;
}

void PreviewHex::updateSmallHexagonDataThatDependOnHexHeightAndRadius()
{
	const auto& a = hexHeight;
	const auto& r = viewerBase.radius;
	smallerHexRatio = (a - r) / a;
	smallerHexHeight = hexHeight * smallerHexRatio;
	smallerHexHeightSquared = square(smallerHexHeight);
}

void PreviewHex::updateHexagonAuxiliarVariables()
{
	const auto& img = viewerBase.accessImgViewer();
	hexagonHalfSide = hexagonSide * .5f;
	hexagonStride = hexagonSide + hexagonHalfSide;
	hexHeight = hexagonSide * SIN_60;
	hex2Heights = hexHeight + hexHeight;
	hexGridXMax = 1 + static_cast<size_t>(ceil((img.imgWF - hexagonHalfSide) / hexagonStride));
	hexGridYMax = 1 + static_cast<size_t>(ceil((img.imgHF - hexHeight) / hex2Heights));
	nHexagons = hexGridXMax * hexGridYMax;
}

void PreviewHex::signalNeedToUpdateHexPlot()
{
	hexPlotReady = false;
}

void PreviewHex::updateHexVars()
{
	updateHexagonAuxiliarVariables();
	updateSmallHexagonDataThatDependOnHexHeightAndRadius();

	const auto& img = viewerBase.accessImgViewer();
	mapBorderCoord = img.imageOffsetF;
	mapBorderCoord.x -= hexagonSide;
	mapBorderCoord.y -= hexHeight;

	mapBorderSize = {
		(hexGridXMax - 1) * hexagonStride + 2 * hexagonSide,
		hexHeight * (hexGridYMax * 2 + 1),
		//hexGridYMax * 2 * hexHeight + hexHeight,
	};
}

void PreviewHex::updateVarsThatDependOnCircleAndHexagon()
{
	updateHexVars();
	recalculateGoalPositions();
}

sf::Vector2i PreviewHex::getHexagonHovered(sf::Vector2f& coord)
{
	coord.x += hexagonHalfSide;
	coord.y += hexHeight;

	auto mathMod = [](float a, float b)
	{
		auto mod = fmod(a, b);
		if (mod < 0)
			mod += b;
		return mod;
	};

	int x, y;
	x = static_cast<int>(floor(coord.x / hexagonStride));
	auto mod = mathMod(coord.x, hexagonStride);
	auto xMod = static_cast<bool>(x % 2);
	if (xMod)
		coord.y -= hexHeight;
	y = static_cast<int>(floor(coord.y / hex2Heights));

	coord.x = mathMod(coord.x, hexagonStride);
	if (coord.x > hexagonSide)
	{
		coord.x -= hexagonSide;
		coord.y = mathMod(coord.y, hex2Heights);
		auto checkIsOnTheOtherSideOfTheSegment = [&]
		{
			auto tanAngle = coord.y / coord.x; // coord.x can't be 0 because we used >
			if (tanAngle < TAN_60)
			{
				x++;
				return true;
			}
			return false;
		};
		if (coord.y < hexHeight)
		{
			if (checkIsOnTheOtherSideOfTheSegment())
				if (!xMod)
					y--;
		}
		else
		{
			coord.y = hex2Heights - coord.y;
			if (checkIsOnTheOtherSideOfTheSegment())
				if (xMod)
					y++;
		}
	}

	return sf::Vector2i{ x, y };
}

sf::Vector2i PreviewHex::getHexagonHoveredConserving(const sf::Vector2f& coord)
{
	sf::Vector2f copy = coord;
	return getHexagonHovered(copy);
}

std::optional<sf::Vector2i> PreviewHex::tryGetHexagonHovered(sf::Vector2f& coord)
{
	auto v = getHexagonHovered(coord);
	auto& [x, y] = v;
	if (x >= 0 && x < hexGridXMax &&
		y >= 0 && y < hexGridYMax)
		return v;
	return {};
}

size_t PreviewHex::getNumCirclesUsingThisHexagonSide(float curHexSide)
{
	hexagonSide = curHexSide;
	updateHexVars();

	// código retirado e cropado de recalculateCircleCenters
	size_t r = 0;
	if (bestFitCircles)
	{
		std::vector<std::vector<size_t>> hexagonsPixels(hexGridXMax, std::vector<size_t>(hexGridYMax));
		const auto& img = viewerBase.accessImgViewer();
		auto [rows, cols] = img.currentImage->getSize();
		for (unsigned i = 0; i != rows; i++)
			for (unsigned j = 0; j != cols; j++)
			{
				auto px = img.currentImage->getPixel(i, j);
				if (viewerBase.isMarked(px))
				{
					// local coordinates of the hex grid
					sf::Vector2f pixelCenter{ i + .5f, j + .5f };
					auto [hx, hy] = getHexagonHoveredConserving(pixelCenter);
					if (!usePixelsInConfigurationSpaceOnly || isPixelInsideSmallerHexagon(pixelCenter, hx, hy))
					{
						auto& cnt = hexagonsPixels[hx][hy];
						cnt++;
					}
				}
			}


		for (int x = 0; x != hexGridXMax; x++)
			for (int y = 0; y != hexGridYMax; y++)
			{
				const auto& cnt = hexagonsPixels[x][y];
				if (cnt)
					r++;
			}
	}
	else
		for (int x = 0; x != hexGridXMax; x++)
			for (int y = 0; y != hexGridYMax; y++)
				r++;
	return r;
}

void PreviewHex::recalculateGoalPositions()
{
	const auto& img = viewerBase.accessImgViewer();
	if (
		//allocationType == AllocationType::RobotBased || 
		bestFitCircles)
	{
		goalsPositions.clear();

		// para cada pixel, calcula o quanto ele influencia cada smaller hexagon
		// hexagonsPixels[x][y] guarda os pixels marcados que "pertencem" a esse hexágono pequeno
		// guarda apenas a soma das coordenadas e a quantidade deles para depois fazermos a média
		std::vector<std::vector<std::pair<sf::Vector2f, size_t>>> hexagonsPixels(hexGridXMax,
			std::vector<std::pair<sf::Vector2f, size_t>>(hexGridYMax));
		auto [width, height] = img.currentImage->getSize();
		for (unsigned i = 0; i != width; i++)
			for (unsigned j = 0; j != height; j++)
			{
				auto px = img.currentImage->getPixel(i, j);
				if (viewerBase.isMarked(px))
				{
					// local coordinates of the hex grid
					sf::Vector2f pixelCenter{ i + .5f, j + .5f };
					auto [hx, hy] = getHexagonHoveredConserving(pixelCenter);
					if (!usePixelsInConfigurationSpaceOnly || isPixelInsideSmallerHexagon(pixelCenter, hx, hy))
					{
						auto& [sum, cnt] = hexagonsPixels[hx][hy];
						sum += pixelCenter;
						cnt++;
						isPixelInsideSmallerHexagon(pixelCenter, hx, hy);
					}
				}
			}


		offsetsLines.clear();
		for (int x = 0; x != hexGridXMax; x++)
			for (int y = 0; y != hexGridYMax; y++)
			{
				const auto& v = hexagonsPixels[x][y];
				const auto& sumOfCoords = v.first;
				const auto& cnt = v.second;

				if (cnt)
				{
					vec2f mean = sumOfCoords / static_cast<float>(cnt);
					auto dst = mean + img.imageOffsetF;
					auto hexCenter = getHexagonCenterInLocalSpace(x, y);

					// dentro do hexágono amarelo com certeza está dentro do hexágono amarelo, pois é convexo então a média dá ali dentro

					if (!usePixelsInConfigurationSpaceOnly)
						if (clipToConfigurationSpace)
							if (vec2f local = mean - hexCenter)
							{
								local.y = -local.y; // esse menos é porque o sistema estava com y positivo para baixo (que é o sistema da imagem), mas eu codei o sistema do hexágono com y para cima
								auto& hexUAxisApotema = accessHexagonUAxis(local);
								auto curLen = length(local);
								auto uOff = local / curLen;
								auto cosAngle = dot(uOff, hexUAxisApotema);
								auto maxLen = smallerHexHeight / cosAngle;
								dst = std::min(maxLen, curLen) * vec2f { uOff.x, -uOff.y } + hexCenter + img.imageOffsetF;
							}
					goalsPositions.emplace_back(dst);

					offsetsLines.emplace_back(hexCenter + img.imageOffsetF, offsetLinesColor);
					offsetsLines.emplace_back(dst, offsetLinesColor);
				}
			}
	}
	else
	{
		goalsPositions.resize(hexGridXMax * hexGridYMax);
		for (int x = 0; x != hexGridXMax; x++)
			for (int y = 0; y != hexGridYMax; y++)
				goalsPositions[x * hexGridYMax + y] = { getHexagonCenterInLocalSpace(x, y) + img.imageOffsetF };
	}
}

void PreviewHex::recalculateCircleCentersAndPlot()
{
	recalculateGoalPositions();
	signalNeedToUpdateHexPlot();
}

void PreviewHex::showNRobotsOption()
{
	if (ImGui::InputScalar("Number of Robots", ImGuiDataType_U64, &nRobotsBudget))
		calculateBestHexagonSideBasedOnNRobotsAndR();
}

void PreviewHex::calculateBestHexagonSideBasedOnNRobotsAndR()
{
	if (!nRobotsBudget)
	{
		Clear(offsetsLines, goalsPositions);
		return;
	}

	auto m = curMinHexagonSide;
	auto M = curMaxHexagonSide;
	while (M - m > EPS_HEX_BIN_SEARCH)
	{
		hexagonSide = (M + m) * .5f;
		updateVarsThatDependOnCircleAndHexagon();
		if (getNumRobotsAllocated() <= nRobotsBudget)
			M = hexagonSide;
		else
			m = hexagonSide;
	}

	hexagonSide = M;
	updateVarsThatDependOnCircleAndHexagon();
}
