#include "Pch.hpp"
#include "PreviewHex.hpp"

#include "Application.hpp"
#include "OpenCVSFML.hpp"

void PreviewHex::draw()
{
	auto& w = viewerBase.app->window;
	const auto& img = viewerBase;
	const auto& imageOffset = img.imageOffsetF;
	if (drawHexGrid)
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
	if (drawGoals)
	{
		auto& r = viewerBase.radius;
		sf::CircleShape circle{ r };
		if (drawCircleBorderOnly)
		{
			circle.setFillColor(sf::Color(0));
			circle.setOutlineColor(viewerBase.circleColor);
			circle.setOutlineThickness(-r * .1f);
		}
		circle.setOrigin(r, r);
		for (const auto& circleCenter : goalsPositions)
		{
			circle.setPosition(circleCenter);
			if (!drawCircleBorderOnly)
				circle.setFillColor(viewerBase.getColor(circleCenter));
			w.draw(circle);
		}
	}
	if (drawSmallerHexagon)
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
		borderDrawing.setOutlineColor(hexGridBorderColor);
		borderDrawing.setOutlineThickness(2.f);
		w.draw(borderDrawing);
	}
	if (sim)
		sim->draw();
}

void PreviewHex::pollEvent(const sf::Event& e)
{
	if (sim)
		sim->pollEvent(e);
}

const char* PreviewHex::getTitle()
{
	return "Hexagonal Grid";
}

PreviewHex::PreviewHex(ViewerBase& viewerBase) :
	Previewer(viewerBase),
	allocationType(AllocationType::Unlimited),
	nRobotsBudget(50),
	drawHexGridBorder{false},
	drawGoals{ true },
	bestFitCircles{ true },
	hexPlotReady{ false },
	drawCircleBorderOnly{ false },
	usePixelsInConfigurationSpaceOnly(true),
	clipToConfigurationSpace{ true },
	drawOrgOffsets{false},
	drawHexGrid{ true },
	highlightHexHovered{false},
	drawSmallerHexagon{false},
	numberOfBars{20},
	numberOfBarsPrev{numberOfBars - 1},
	hexGridColor(sf::Color::Red),
	offsetLinesColor(sf::Color::Green),
	hexGridBorderColor(sf::Color::White)
{
	hexSide = viewerBase.imgWF * 0.05f;
	onImgChangeImpl();
}

void PreviewHex::setHexSmallData()
{
	curMinHexSide = viewerBase.radius * INV_SIN_60;
	updateSmallHexagonDataThatDependOnHexHeightAndRadius();
}

void PreviewHex::onUpdateRadius()
{
	setHexSmallData();
	if (usePixelsInConfigurationSpaceOnly) // mudar o raio não muda o número de círculos alocados se estamos ignorando o espaço de configuração
		signalNeedToUpdateHexPlot();
}

std::array<sf::Vector2f, 6> PreviewHex::getHexagonOffsets()
{
	return
	{
		sf::Vector2f{ +hexagonHalfSide, -hexHeight },
		sf::Vector2f{ +hexSide, 0 },
		sf::Vector2f{ +hexagonHalfSide, +hexHeight },
		sf::Vector2f{ -hexagonHalfSide, +hexHeight },
		sf::Vector2f{ -hexSide, 0 },
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
	recreateFilterTextureAndColorMap();

	const auto [X, Y] = viewerBase.currentImage->getSize();
	const auto rowsF = static_cast<double>(Y);
	vec2d p{ static_cast<double>(X), rowsF };
	const auto proj = projPointUAxis(p, HEX_AXIS[0]);
	curMaxHexSide = float(std::max(rowsF, length(proj)) / SIN_60);

	setHexSmallData();
	hexSide = std::clamp(hexSide, curMinHexSide, curMaxHexSide);

	onUpdateHexVars();
	calculateBestHexagonSideBasedOnNRobotsAndR();
	recalculateGoalsAndPlot();
	if (sim)
	{
		sim->clearGoals();
		sim->addGoalsAndRecalculate(goalsPositions);
	}
}

size_t PreviewHex::getNumRobotsAllocated()
{
	return goalsPositions.size();
}

const sf::Image& PreviewHex::accessColorMapImg()
{
	return colorMapImage;
}

void PreviewHex::drawUIImpl()
{
	if (ImGui::CollapsingHeader("Robots"))
	{
		ImGui::Checkbox("Draw", &drawGoals);
		ColorPicker3U32("Color", viewerBase.circleColor);

		ImGui::Checkbox("Border Only", &drawCircleBorderOnly);

		if (ImGui::SliderFloat("Radius", &viewerBase.radius, .5f, hexHeight))
		{
			onUpdateRadius();
			recalculateGoalPositions();
			recreateFilterTextureAndColorMap();
		}

		if (ImGui::TreeNode("Filter"))
		{
			ImGui::Image(viewerBase.currentFilterSprite);
			ImGui::TreePop();
		}
	}
	if (ImGui::CollapsingHeader("Hexagon"))
	{
		ImGui::LabelText("Number of Goals", "%zu", goalsPositions.size());

		if (ImGui::TreeNode("Grid"))
		{
			ImGui::Checkbox("Draw", &drawHexGrid);
			ColorPicker3U32("Color", hexGridColor);
			ImGui::TreePop();
		}

		ImGui::Checkbox("Configuration Space", &drawSmallerHexagon);

		ImGui::Checkbox("Highlight Hexagon Hovered", &highlightHexHovered);

		if (allocationType == AllocationType::Unlimited)
			if (ImGui::SliderFloat("Side", &hexSide, curMinHexSide, curMaxHexSide))
				updateVarsThatDependOnCircleAndHexagon();

		if (ImGui::TreeNode("Bounding Box"))
		{
			ImGui::Checkbox("Rect Around Hex Grid", &drawHexGridBorder);
			ColorPicker3U32("Rect Around Hex Grid Color", hexGridBorderColor);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Plot"))
		{
			static constexpr int MAX_BARS = 1000;
			if (ImGui::DragInt("Number of Bars", &numberOfBars, 1, 2, MAX_BARS, "%d", ImGuiSliderFlags_AlwaysClamp))
			{
				numberOfBarsPrev = numberOfBars - 1;
				signalNeedToUpdateHexPlot();
			}

			if (!hexPlotReady)
			{
				if (ImGui::Button("Create Hexagon Side Plot"))
				{
					hexPlotReady = true;

					auto hexSideUsing = hexSide;

					barData.resize(numberOfBars);
					for (int i = 0; i != numberOfBars; i++)
					{
						auto curOff = static_cast<float>(i) / numberOfBarsPrev;
						auto curHexSide = curMinHexSide * (1.f - curOff) + curMaxHexSide * curOff;
						barData[i] = getNumCirclesUsingThisHexagonSide(curHexSide);
					}

					hexSide = hexSideUsing;
					onUpdateHexVars();
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
			ImGui::TreePop();
		}
	}
	if (ImGui::CollapsingHeader("Allocation"))
	{
		static constexpr const char* ALLOCATION_TYPES_NAMES[] = {
			"Unlimited",
			"Limited"
		};
		if (ImGui::Combo("Type", (int*)&allocationType, ALLOCATION_TYPES_NAMES, IM_ARRAYSIZE(ALLOCATION_TYPES_NAMES)))
			if (allocationType == AllocationType::Limited)
				calculateBestHexagonSideBasedOnNRobotsAndR();

		ImGui::Checkbox("Draw Original Offsets", &drawOrgOffsets);
		if (ColorPicker3U32("Offsets Color", offsetLinesColor))
			for (auto& v : offsetsLines)
				v.color = offsetLinesColor;

		if (ImGui::Checkbox("Use Only Pixels on Configuration Space", &usePixelsInConfigurationSpaceOnly))
			recalculateGoalsAndPlot();

		if (!usePixelsInConfigurationSpaceOnly)
			if (ImGui::Checkbox("Clip To Configuration Space", &clipToConfigurationSpace))
				if (!usePixelsInConfigurationSpaceOnly)
					recalculateGoalPositions();

		if (allocationType == AllocationType::Unlimited)
		{
			if (ImGui::Checkbox("Fit", &bestFitCircles))
				recalculateGoalsAndPlot();
		}
		else
			showNRobotsOption();
	}
	if (sim)
	{
		if (!sim->drawUI())
		{
			sim.reset();
			drawGoals = true;
		}
	}
	else
		if (ImGui::Button("Initial Placement / Editor"))
		{
			drawGoals = false;

			size_t curBudget;
			if (allocationType == AllocationType::Unlimited)
				curBudget = goalsPositions.size();
			else
				curBudget = nRobotsBudget;

			MakeUniquePtr(sim, viewerBase.app, viewerBase.radius);

			// posição inicial dos agentes
			size_t cnt = 0;
			for (int i = 0; i != hexGridXMax; i++)
				for (int j = 0; j != hexGridYMax; j++, cnt++)
				{
					if (cnt == curBudget)
						break;
					sim->addAgent(getHexagonCenterInWorldSpace(i, j));
				}

			sim->addGoalsAndRecalculate(goalsPositions);
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
	return getHexagonCenterInLocalSpace(x, y) + viewerBase.imageOffsetF;
}

const vec2d& PreviewHex::accessHexagonUAxis(const vec2d& off)
{
	auto angle = atan2(off.y, off.x);
	if (angle < 0)
		angle += 2 * std::numbers::pi;

	// angle \in [0, 2*pi]

	int i = 0;
	for (; i != 5; i++)
		if (angle < HEX_ANGLES[i])
			break;
	return HEX_AXIS[i];
}

bool PreviewHex::isPixelInsideSmallerHexagon(const vec2d& pixelCenter, int hx, int hy)
{
	auto hc = getHexagonCenterInLocalSpace(hx, hy);
	auto off = pixelCenter - hc;
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
	const auto& img = viewerBase;
	hexagonHalfSide = hexSide * .5f;
	hexagonStride = hexSide + hexagonHalfSide;
	hexHeight = hexSide * (float)SIN_60;
	hex2Heights = hexHeight + hexHeight;
	hexGridXMax = 1 + static_cast<size_t>(ceil((img.imgWF - hexagonHalfSide) / hexagonStride));
	hexGridYMax = 1 + static_cast<size_t>(ceil((img.imgHF - hexHeight) / hex2Heights));
	nHexagons = hexGridXMax * hexGridYMax;
}

void PreviewHex::signalNeedToUpdateHexPlot()
{
	hexPlotReady = false;
}

void PreviewHex::onUpdateHexVars()
{
	updateHexagonAuxiliarVariables();
	updateSmallHexagonDataThatDependOnHexHeightAndRadius();

	const auto& img = viewerBase;
	mapBorderCoord = img.imageOffsetF;
	mapBorderCoord.x -= hexSide;
	mapBorderCoord.y -= hexHeight;

	mapBorderSize = {
		(hexGridXMax - 1) * hexagonStride + 2 * hexSide,
		hexHeight * (hexGridYMax * 2 + 1), //hexGridYMax * 2 * hexHeight + hexHeight,
	};
}

void PreviewHex::updateVarsThatDependOnCircleAndHexagon()
{
	onUpdateHexVars();
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
	if (coord.x > hexSide)
	{
		coord.x -= hexSide;
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
	hexSide = curHexSide;
	onUpdateHexVars();
	size_t r = 0;
	if (bestFitCircles)
	{
		std::vector<std::vector<size_t>> hexagonsPixels(hexGridXMax, std::vector<size_t>(hexGridYMax));
		const auto& img = viewerBase;
		auto [rows, cols] = img.currentImage->getSize();
		for (unsigned i = 0; i != rows; i++)
			for (unsigned j = 0; j != cols; j++)
			{
				auto px = img.currentImage->getPixel(i, j);
				if (px.a)
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
	const auto& img = viewerBase;
	if (bestFitCircles)
	{
		goalsPositions.clear();

		std::vector hexagonsPixels(hexGridXMax, std::vector<std::pair<vec2d, double>>(hexGridYMax));
		const auto [X, Y] = img.currentImage->getSize();
		for (unsigned i = 0; i != X; i++)
			for (unsigned j = 0; j != Y; j++)
			{
				const auto w = img.currentImage->getPixel(i, j).a / 255.;
				vec2d pixelCenter{ i + .5, j + .5 };
				auto [hx, hy] = getHexagonHoveredConserving(pixelCenter);
				if (!usePixelsInConfigurationSpaceOnly || isPixelInsideSmallerHexagon(pixelCenter, hx, hy))
				{
					auto& [cs, s] = hexagonsPixels[hx][hy];
					cs += pixelCenter * w;
					s += w;
				}
			}

		offsetsLines.clear();
		for (int x = 0; x != hexGridXMax; x++)
			for (int y = 0; y != hexGridYMax; y++)
			{
				const auto& v = hexagonsPixels[x][y];
				const auto& [cs, s] = v;
				if (s)
				{
					const auto hexCenter = getHexagonCenterInLocalSpace(x, y);
					const auto mean = cs / s;
					auto dst = mean + img.imageOffsetF;
					if (!usePixelsInConfigurationSpaceOnly)
						if (clipToConfigurationSpace)
							if (auto local = mean - hexCenter)
							{
								local.y = -local.y;
								auto& hexUAxisApotema = accessHexagonUAxis(local);
								auto curLen = length(local);
								auto uOff = local / curLen;
								auto cosAngle = dot(uOff, hexUAxisApotema);
								auto maxLen = smallerHexHeight / cosAngle;
								dst = std::min(maxLen, curLen) * vec2d{ uOff.x, -uOff.y } + hexCenter + img.imageOffsetF;
							}
					goalsPositions.emplace_back((sf::Vector2f)dst);

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

void PreviewHex::recalculateGoalsAndPlot()
{
	recalculateGoalPositions();
	signalNeedToUpdateHexPlot();
}

void PreviewHex::showNRobotsOption()
{
	if (DragScalarMax("Number of Robots", nRobotsBudget, size_t(1000)))
		calculateBestHexagonSideBasedOnNRobotsAndR();
}

void PreviewHex::calculateBestHexagonSideBasedOnNRobotsAndR()
{
	if (!nRobotsBudget)
	{
		Clear(offsetsLines, goalsPositions);
		return;
	}

	auto
		m = curMinHexSide,
		M = curMaxHexSide;
	while (M - m > EPS_HEX_BIN_SEARCH)
	{
		hexSide = (M + m) * .5f;
		updateVarsThatDependOnCircleAndHexagon();
		if (getNumRobotsAllocated() <= nRobotsBudget)
			M = hexSide;
		else
			m = hexSide;
	}

	hexSide = M;
	updateVarsThatDependOnCircleAndHexagon();
}

void PreviewHex::recreateFilterTextureAndColorMap()
{
	auto& v = viewerBase;
	const auto side = static_cast<unsigned>(ceil(v.radius * 2));

	sf::Image currentFilterImage;
	currentFilterImage.create(side, side, sf::Color::White);

	std::vector<float> filterValues(square(static_cast<size_t>(side)));
	if (side == 1)
		filterValues.front() = 1.f;
	else
	{
		currentFilterImage.create(side, side);
		auto mid = vec2d{ (double)side } *.5f;

		auto std = static_cast<double>(side) * .5f / 3.f;
		auto var = std * std;
		auto G = [&](double x) // gaussiana com média 0
		{
			return static_cast<float>(1 / (std * sqrt(2 * std::numbers::pi)) * pow(std::numbers::e, -.5 * square(x) / var));
		};

		auto M = G(0);

		for (unsigned x = 0; x != side; x++)
			for (unsigned y = 0; y != side; y++)
			{
				auto off = vec2d(x + .5, y + .5) - mid;

				auto strength = G(length(off));
				filterValues[(size_t)x * side + y] = strength;

				sf::Color color{
				static_cast<sf::Uint8>(strength / M * 255),
				static_cast<sf::Uint8>(strength / M * 255),
				static_cast<sf::Uint8>(strength / M * 255)
				};
				currentFilterImage.setPixel(x, y, color);
			}

		auto sum = std::accumulate(filterValues.begin(), filterValues.end(), 0.f);
		for (auto& f : filterValues)
			f /= sum;
	}
	v.currentFilterTexture.loadFromImage(currentFilterImage);
	v.currentFilterSprite.setTexture(v.currentFilterTexture, true);

	colorMapImage = *v.currentImage; // hard bug to find. Opencv does things with pointers
	auto mat = SFMLImageToOpenCVBGRA(colorMapImage);
	cv::Mat kernel(side, side, CV_32F, (void*)filterValues.data());
	cv::filter2D(mat, mat, -1, kernel);
	CVBGRAToSFMLTextureAndImage(mat, v.colorMapTexture, colorMapImage);
	v.colorMapSprite.setTexture(v.colorMapTexture, true);
}
