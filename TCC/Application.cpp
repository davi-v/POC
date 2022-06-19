#include "Pch.hpp"
#include "Application.hpp"

#include <imgui/imgui_internal.h>

#include "FileExplorer.hpp"
#include "OpenCVSFML.hpp"
#include "Utilities.hpp"

void Application::ImgViewer::showNRobotsOption()
{
	if (ImGui::InputScalar("Number of Robots", ImGuiDataType_U64, &nRobotsBudget))
		callbackRobotBased();
}

void Application::createSequence()
{
	viewer = std::make_unique<Sequencer>(*this);
}

sf::Color Application::getSFBackgroundColor()
{
	return ToSFMLColor(backgroundColor);
}

bool Application::ImgViewer::isMarked(const sf::Color& c)
{
	return c.a;
}



void Application::ImgViewer::callbackRobotBased()
{
	if (!nRobotsBudget)
	{
		Clear(offsetsLines, circleCenters);
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

void Application::ImgViewer::drawImpl(bool showOptions)
{
	auto& window = accessWindow();

	if (ImGui::BeginMenu("Image"))
	{
		if (ImGui::MenuItem("Centralize"))
		{
			window.setView(window.getDefaultView());
			app.zoomLevel = DEFAULT_ZOOM_LEVEL;
		}

		if (ImGui::BeginMenu("Rendering"))
		{
			static constexpr const char* RENDER_TYPES_NAMES[] = {
					"None",
					"Image",
					"Color Map",
					"Right Colors"
			};
			ImGui::Combo("Render Type", reinterpret_cast<int*>(&renderType), RENDER_TYPES_NAMES, IM_ARRAYSIZE(RENDER_TYPES_NAMES));

			ImGui::EndMenu();
		}

		if (showOptions)
		{
			if (ImGui::BeginMenu("Pixel"))
			{
				ImGui::Checkbox("Highlight Hovered", &highlightPixelHovered);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Circle"))
			{
				ImGui::Checkbox("Show", &showCircleCenters);
				ImGui::Checkbox("Border Only", &showCircleBorderOnly);
				if (ImGui::SliderFloat("Radius", &r, .5f, hexHeight))
				{
					curMinHexagonSide = r * INV_SIN_60;
					sim.r = r;
					circleRadiusUpdateCallback();
					updateSmallHexagonData();
					if (yellowHexagonOnly) // mudar o raio não muda o número de círculos alocados se estamos ignorando o espaço de configuração
						signalNeedToUpdateHexPlot();
					recalculateCircleCenters();
				}
				ImGui::Image(currentFilterSprite);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Hexagon"))
			{
				ImGui::Checkbox("Show Grid", &showHexGrid);
				ImGui::Checkbox("Show Configuration Space", &showSmallerHexagon);
				ImGui::Checkbox("Highlight Hovered", &highlightHexHovered);

				if (ImGui::SliderFloat("Side", &hexagonSide, curMinHexagonSide, curMaxHexagonSide))
					updateVarsThatDependOnCircleAndHexagon();

				ImGui::LabelText("Number of Circles Alocated", "%zu", circleCenters.size());

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
							barData[i] = getNumCircles(curHexSide);
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
					"Edit Mode",
					"Robot Based"
				};
				if (ImGui::Combo("Type", reinterpret_cast<int*>(&allocationType), ALLOCATION_TYPES_NAMES, IM_ARRAYSIZE(ALLOCATION_TYPES_NAMES)))
				{
					if (allocationType == AllocationType::RobotBased)
						callbackRobotBased();
				}

				ImGui::Checkbox("Draw Original Offsets", &drawOrgOffsets);

				if (ImGui::Checkbox("Use Only Pixels on Configuration Space", &yellowHexagonOnly))
					recalculateCircleCentersAndPlot();

				if (!yellowHexagonOnly)
					if (ImGui::Checkbox("Clip To Configuration Space", &clipToConfigurationSpace))
						if (!yellowHexagonOnly)
							recalculateCircleCenters();

				if (allocationType == AllocationType::EditMode)
				{
					if (ImGui::Checkbox("Fit", &bestFitCircles))
					{
						recalculateCircleCentersAndPlot();
					}
				}
				else
					showNRobotsOption();
				ImGui::EndMenu();
			}
		}

		ImGui::EndMenu();
	}


	if (ImGui::Checkbox("Navigation", &isNavigatorMenuOpened))
	{
		if (isNavigatorMenuOpened)
		{
			if (showOptions)
			{
				// posição inicial dos robôs
				size_t cnt = 0;
				for (int i = 0; i != hexGridXMax; i++)
					for (int j = 0; j != hexGridYMax; j++, cnt++)
					{
						if (cnt == nRobotsBudget)
							break;
						addAgent(getHexagonCenterInWorldSpace(i, j));
					}
			}
			else
				callbackRobotBased();

			addGoalsAndCalculateEdges();
		}
		else
		{
			if (showOptions)
				sim.clear();
			else
				sim.leaveNavigatorUpdatingAgentsCoordinates();
		}
	}

	switch (renderType)
	{
	case RenderType::Image:
	{
		window.draw(currentImageSprite);
	}
	break;
	case RenderType::ColorMap:
	{
		window.draw(colorMapSprite);
	}
	break;
	}

	if (isNavigatorMenuOpened)
		sim.draw();
	else
	{
		if (showHexGrid)
		{
			for (int x = 0; x != hexGridXMax; x++)
				for (int y = 0; y != hexGridYMax; y++)
				{
					auto coords = getHexagonCoords(x, y);
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
					window.draw(vertices, 12, sf::Lines);
				}
		}
		if (showCircleCenters)
		{
			sf::CircleShape circle{ r };
			if (renderType != RenderType::RightColor)
			{
				if (showCircleBorderOnly)
				{
					circle.setOutlineColor(circleBorderColor);
					circle.setOutlineThickness(-r * .1f);
					circle.setFillColor(sf::Color{}); // a gente só liga pro alpha = 0, mas a API não deixa	mexer só no alpha
				}
				else
					circle.setFillColor(circleBorderColor);
			}

			circle.setOrigin(r, r);
			if (showOptions)
				for (auto& circleCenter : circleCenters)
				{
					circle.setPosition(circleCenter);
					circle.setFillColor(getColor(circleCenter));
					window.draw(circle);
				}
			else
			{
				for (auto& agent : sim.agents)
				{
					vec2f circleCenter(
						static_cast<float>(agent->coord.x),
						static_cast<float>(agent->coord.y));
					circle.setPosition(circleCenter);
					circle.setFillColor(getColor(circleCenter));
					window.draw(circle);
				}
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
						window.draw(vertices, sizeof(vertices) / sizeof(decltype(*vertices)), sf::Lines);
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
						window.draw(vertices, sizeof(vertices) / sizeof(decltype(*vertices)), sf::TriangleFan);
					}
				}
		}
		if (highlightPixelHovered)
		{
			auto point = sf::Mouse::getPosition(window);
			auto curPos = window.mapPixelToCoords(point);
			curPos -= imageOffset;
			auto size = currentImage->getSize();
			auto wf = static_cast<float>(size.x);
			auto hf = static_cast<float>(size.y);
			if (curPos.x >= 0 && curPos.x < wf &&
				curPos.y >= 0 && curPos.y < hf
				)
			{

				auto pixelHoverColor = sf::Color(255, 0, 0, 120);
				//auto pixelHoverColor = sf::Color(255 - c.r, 255 - c.g, 255 - c.b);

				sf::RectangleShape rect{ {1,1} };
				rect.setFillColor(pixelHoverColor);
				rect.setOutlineColor(sf::Color::Red);
				rect.setOutlineThickness(-0.1f);
				rect.setPosition(
					static_cast<float>(floor(curPos.x)) + imageOffset.x,
					static_cast<float>(floor(curPos.y)) + imageOffset.y
				);
				window.draw(rect);
			}
		}
		if (highlightHexHovered)
		{
			sf::VertexArray hexagon(sf::TriangleFan, 6);
			auto point = sf::Mouse::getPosition(window);
			auto coord = window.mapPixelToCoords(point);
			coord -= imageOffset;
			if (auto optHexagon = tryGetHexagonHovered(coord))
			{
				auto& [x, y] = *optHexagon;
				auto coords = getHexagonCoords(x, y);
				for (int i = 0; i != 6; i++)
				{
					auto& p = hexagon[i];
					p.position = coords[i] + imageOffset;
					p.color = hexHighlightColor;
				}
				window.draw(hexagon);
			}
		}
		if (drawOrgOffsets)
			window.draw(offsetsLines.data(), offsetsLines.size(), sf::Lines);
	}
}

void Application::ImgViewer::addGoalsAndCalculateEdges()
{
	for (auto& circleCenter : circleCenters)
		addGoal(circleCenter);

	sim.updateGraphEdges();
	sim.editModeType = Simulator2D::EditModeType::Metric;
}

bool Application::ImgViewer::pollEvent(const sf::Event& e)
{
	return sim.pollEvent(e);
}

sf::Color Application::ImgViewer::getColor(const vec2f& c)
{
	auto& [x, y] = c;

	auto
		wx = x - imageOffset.x,
		wy = y - imageOffset.y;

	if (renderType == RenderType::RightColor)
	{
		auto
			xi = lround(wx),
			yi = lround(wy);

		if (xi >= 0 and yi >= 0)
		{
			unsigned
				xu = xi,
				yu = yi;
			auto [width, height] = colorMapImage.getSize();
			if (xu < width && yu < height)
				return colorMapImage.getPixel(xi, yi);
		}
		return app.getSFBackgroundColor();
	}
	return sf::Color::Red;
}

void Application::ImgViewer::addGoal(const sf::Vector2f& v)
{
	sim.addGoal(vec2d{ static_cast<double>(v.x), static_cast<double>(v.y) });
}

void Application::ImgViewer::addAgent(const sf::Vector2f& v)
{
	sim.addAgent(v.x, v.y);
}

size_t Application::ImgViewer::getNumRobotsAllocated()
{
	return circleCenters.size();
}

Application::Application(sf::RenderWindow& window) :
	window(window),
	lastPxClickedX(-1),
	zoomFac(1),
	zoomLevel(DEFAULT_ZOOM_LEVEL)
{

}

void Application::pollEvent(const sf::Event& event)
{
	switch (event.type)
	{
	case sf::Event::KeyPressed:
	{
		switch (event.key.code)
		{
		case sf::Keyboard::O:
		{
			if (event.key.control)
				controlO();
		}
		break;
		}
	}
	break;
	}

	if (viewer && viewer->pollEvent(event))
		return;

	if (NotHoveringIMGui())
	{
		switch (event.type)
		{
		case sf::Event::MouseButtonPressed:
		{
			auto& data = event.mouseButton;
			switch (data.button)
			{
			case sf::Mouse::Left:
			{
				lastPxClickedX = data.x;
				lastPxClickedY = data.y;
			}
			break;
			}
		}
		break;
		}
	}

	switch (event.type)
	{
	case sf::Event::MouseWheelScrolled:
	{
		auto& scrollData = event.mouseWheelScroll;
		zoomLevel -= scrollData.delta;

		auto& px = scrollData.x;
		auto& py = scrollData.y;

		auto pixelCoord = sf::Vector2i{ px, py };

		sf::View view = window.getView();
		auto prevCoord = window.mapPixelToCoords(pixelCoord, view);
		updateZoomFacAndViewSizeFromZoomLevel(view);
		auto newCoord = window.mapPixelToCoords(pixelCoord, view);
		view.move(prevCoord - newCoord);

		window.setView(view);
	}
	break;
	case sf::Event::MouseMoved:
	{
		if (lastPxClickedX != -1) // if we are holding left click, move view of the world
		{
			auto& data = event.mouseMove;
			auto& newX = data.x;
			auto& newY = data.y;

			sf::View view = window.getView();
			view.move((lastPxClickedX - newX) * zoomFac, (lastPxClickedY - newY) * zoomFac);
			window.setView(view);

			lastPxClickedX = newX;
			lastPxClickedY = newY;
		}
	}
	break;
	case sf::Event::MouseButtonReleased:
	{
		switch (event.mouseButton.button)
		{
		case sf::Mouse::Left:
		{
			lastPxClickedX = -1; // no longer moving the scene
		}
		break;
		}
	}
	break;
	}
}

void Application::draw()
{
	window.clear(getSFBackgroundColor());

	//ImPlot::ShowDemoWindow();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open Image", "Ctrl+O"))
				controlO();

			if (ImGui::MenuItem("Create Sequence"))
				createSequence();

			if (ImGui::MenuItem("Quit", "Alt+F4"))
				window.close();

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Settings"))
		{
			ImGui::ColorEdit3("Background Color", backgroundColor);
			ImGui::SameLine(); HelpMarker(
				"Click on the color square to open a color picker.\n"
				"Click and hold to use drag and drop.\n"
				"Right-click on the color square to show options.\n"
				"CTRL+click on individual component to input value.");
			ImGui::EndMenu();
		}

		if (viewer)
			viewer->draw();

		ImGui::EndMainMenuBar();
	}

	ImGui::SFML::Render(window);
	window.display();
}

sf::Color Application::getColor(const vec2f& c)
{
	return viewer->getColor(c);
}

sf::Vector2f Application::ImgViewer::getHexagonCenterInLocalSpace(int x, int y)
{
	auto cx = x * hexagonStride;
	auto cy = y * hex2Heights;
	if (x % 2)
		cy += hexHeight;
	return { cx, cy };
}

sf::Vector2f Application::ImgViewer::getHexagonCenterInWorldSpace(int x, int y)
{
	return getHexagonCenterInLocalSpace(x, y) + imageOffset;
}

const vec2f& Application::ImgViewer::accessHexagonUAxis(const vec2f& off)
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

bool Application::ImgViewer::isPixelInsideSmallerHexagon(const sf::Vector2f& pixelCenter, int hx, int hy)
{
	auto hc = getHexagonCenterInLocalSpace(hx, hy);
	vec2f off = pixelCenter - hc;
	//off.y = -off.y; // not necessary because it's symmetric
	auto& uAxis = accessHexagonUAxis(off);
	auto proj = projPointUAxis(off, uAxis);
	auto len2 = dot(proj, proj);
	return len2 < smallerHexHeightSquared;
}

void Application::ImgViewer::updateSmallHexagonData()
{
	const auto& a = hexHeight;
	smallerHexRatio = (a - r) / a;
	smallerHexHeight = hexHeight * smallerHexRatio;
	smallerHexHeightSquared = square(smallerHexHeight);
}

void Application::ImgViewer::updateHexagonAuxiliarVariables()
{
	hexagonHalfSide = hexagonSide * .5f;
	hexagonStride = hexagonSide + hexagonHalfSide;
	hexHeight = hexagonSide * SIN_60;
	hex2Heights = hexHeight + hexHeight;
	hexGridXMax = 1 + static_cast<size_t>(ceil((imgWF - hexagonHalfSide) / hexagonStride));
	hexGridYMax = 1 + static_cast<size_t>(ceil((imgHF - hexHeight) / hex2Heights));
	nHexagons = hexGridXMax * hexGridYMax;
}

void Application::ImgViewer::circleRadiusUpdateCallback()
{
	auto side = static_cast<unsigned>(ceil(r));

	sf::Image currentFilterImage;
	currentFilterImage.create(side, side, sf::Color::White);
	if (side % 2 == 0)
		side++;
	std::vector<float> filterValues(square(static_cast<size_t>(side)));
	if (side == 1)
		filterValues.front() = 1.f;
	else
	{
		currentFilterImage.create(side, side);
		auto mid = vec2f{ static_cast<float>(side) } *.5f;
		auto max = square(mid - vec2f{ .5, .5 });

		auto den = square(side * .2f);
		auto invDen = 1.f / den;

		for (unsigned x = 0; x != side; x++)
			for (unsigned y = 0; y != side; y++)
			{
				auto off = vec2f(x + .5f, y + .5f) - mid;
				auto d2 = dot(off, off);
				float strength = 1.f / (1.f + d2 * invDen);
				filterValues[x * side + y] = strength;
				sf::Color color{
					static_cast<sf::Uint8>(strength * 255),
					static_cast<sf::Uint8>(strength * 255),
					static_cast<sf::Uint8>(strength * 255)
				};
				currentFilterImage.setPixel(x, y, color);
			}
		auto sum = std::accumulate(filterValues.begin(), filterValues.end(), 0.f);
		for (auto& f : filterValues)
			f /= sum;
	}
	currentFilterTexture.loadFromImage(currentFilterImage);
	currentFilterSprite.setTexture(currentFilterTexture, true);

	colorMapImage = *currentImage; // hard bug to find. Opencv does things with pointers
	auto mat = SFMLImageToOpenCVBGRA(colorMapImage);

	cv::Mat kernel(side, side, CV_32F, (void*)filterValues.data());
	cv::filter2D(mat, mat, -1, kernel);
	CVBGRAToSFMLTextureAndImage(mat, colorMapTexture, colorMapImage);
	colorMapSprite.setTexture(colorMapTexture, true);
}

void Application::ImgViewer::signalNeedToUpdateHexPlot()
{
	hexPlotReady = false;
}

void Application::ImgViewer::updateHexVars()
{
	updateHexagonAuxiliarVariables();
	updateSmallHexagonData();
}

void Application::ImgViewer::updateVarsThatDependOnCircleAndHexagon()
{
	updateHexVars();
	recalculateCircleCenters();
}

sf::Vector2i Application::ImgViewer::getHexagonHovered(sf::Vector2f& coord)
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

sf::Vector2i Application::ImgViewer::getHexagonHoveredConserving(const sf::Vector2f& coord)
{
	sf::Vector2f copy = coord;
	return getHexagonHovered(copy);
}

std::optional<sf::Vector2i> Application::ImgViewer::tryGetHexagonHovered(sf::Vector2f& coord)
{
	auto v = getHexagonHovered(coord);
	auto& [x, y] = v;
	if (x >= 0 && x < hexGridXMax &&
		y >= 0 && y < hexGridYMax)
		return v;
	return {};
}

size_t Application::ImgViewer::getNumCircles(float curHexSide)
{
	hexagonSide = curHexSide;
	updateHexVars();

	// código retirado e cropado de recalculateCircleCenters
	size_t r = 0;
	if (bestFitCircles)
	{
		std::vector<std::vector<size_t>> hexagonsPixels(hexGridXMax, std::vector<size_t>(hexGridYMax));
		auto [rows, cols] = currentImage->getSize();
		for (unsigned i = 0; i != rows; i++)
			for (unsigned j = 0; j != cols; j++)
			{
				auto px = currentImage->getPixel(i, j);
				if (isMarked(px))
				{
					// local coordinates of the hex grid
					sf::Vector2f pixelCenter{ i + .5f, j + .5f };
					auto [hx, hy] = getHexagonHoveredConserving(pixelCenter);
					if (!yellowHexagonOnly || isPixelInsideSmallerHexagon(pixelCenter, hx, hy))
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

void Application::ImgViewer::recalculateCircleCenters()
{
	circleCenters.clear();
	if (allocationType == AllocationType::RobotBased || bestFitCircles)
	{
		// para cada pixel, calcula o quanto ele influencia cada smaller hexagon
		// hexagonsPixels[x][y] guarda os pixels marcados que "pertencem" a esse hexágono pequeno
		// guarda apenas a soma das coordenadas e a quantidade deles para depois fazermos a média
		std::vector<std::vector<std::pair<sf::Vector2f, size_t>>> hexagonsPixels(hexGridXMax,
			std::vector<std::pair<sf::Vector2f, size_t>>(hexGridYMax));
		auto [width, height] = currentImage->getSize();
		for (unsigned i = 0; i != width; i++)
			for (unsigned j = 0; j != height; j++)
			{
				auto px = currentImage->getPixel(i, j);
				if (isMarked(px))
				{
					// local coordinates of the hex grid
					sf::Vector2f pixelCenter{ i + .5f, j + .5f };
					auto [hx, hy] = getHexagonHoveredConserving(pixelCenter);
					if (!yellowHexagonOnly || isPixelInsideSmallerHexagon(pixelCenter, hx, hy))
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
					auto dst = mean + imageOffset;
					auto hexCenter = getHexagonCenterInLocalSpace(x, y);

					// dentro do hexágono amarelo com certeza está dentro do hexágono amarelo, pois é convexo então a média dá ali dentro

					if (!yellowHexagonOnly)
						if (clipToConfigurationSpace)
							if (vec2f local = mean - hexCenter)
							{
								local.y = -local.y; // esse menos é porque o sistema estava com y positivo para baixo (que é o sistema da imagem), mas eu codei o sistema do hexágono com y para cima
								auto& hexUAxisApotema = accessHexagonUAxis(local);
								auto curLen = length(local);
								auto uOff = local / curLen;
								auto cosAngle = dot(uOff, hexUAxisApotema);
								auto maxLen = smallerHexHeight / cosAngle;
								dst = std::min(maxLen, curLen) * vec2f { uOff.x, -uOff.y } + hexCenter + imageOffset;
							}
					circleCenters.emplace_back(dst);

					offsetsLines.emplace_back(hexCenter + imageOffset, offsetLinesColor);
					offsetsLines.emplace_back(dst, offsetLinesColor);
				}
			}
	}
	else
		for (int x = 0; x != hexGridXMax; x++)
			for (int y = 0; y != hexGridYMax; y++)
				circleCenters.emplace_back(getHexagonCenterInLocalSpace(x, y) + imageOffset);
}

void Application::ImgViewer::recalculateCircleCentersAndPlot()
{
	recalculateCircleCenters();
	signalNeedToUpdateHexPlot();
}


std::array<sf::Vector2f, 6> Application::ImgViewer::getHexagonOffsets()
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

std::array<sf::Vector2f, 6> Application::ImgViewer::getHexagonCoords(int x, int y)
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

sf::Vector2f Application::getWindowSizeF()
{
	auto [x, y] = window.getSize();
	return sf::Vector2f{ static_cast<float>(x), static_cast<float>(y) };
}

void Application::controlO()
{
	if (auto path = TryOpenFile(L"Choose Image to Load", SFML_SUPPORTED_IMAGE_FORMATS_SIZE, SFML_SUPPORTED_IMAGE_FORMATS))
	{
		sf::String s{ path };

		std::unique_ptr<sf::Image> currentImage;
		MakeUniquePtr(currentImage);

		if (currentImage->loadFromFile(s))
			viewer = std::make_unique<ImgViewer>(*this, std::move(currentImage), DEFAULT_RADIUS);
		else
		{
			LOG("Error importing image ", path, '\n');
		}
	}
}

bool Application::Sequencer::pollEvent(const sf::Event& e)
{
	if (imgViewer)
		return imgViewer->sim.pollEvent(e);
	return false;
}

sf::Color Application::Sequencer::getColor(const vec2f& c)
{
	return imgViewer->getColor(c);
}

Application::Sequencer::Sequencer(Application& app) :
	ViewerBase(app)
{
	static constexpr const char* HARDCODED_PATHS[] = {
		"exemplos/circle.png",
		"exemplos/rect.png",
		"exemplos/triangle.png",
		"exemplos/horse.png",
		"exemplos/happy.png",
		"exemplos/horse-skel2.png",
	};
	for (auto& p : HARDCODED_PATHS)
	{
		ImgContent content;
		MakeUniquePtr(content);
		auto& img = *content;
		img.loadFromFile(p);
		seqImgs.emplace_back(std::move(content));
	}
	updateImg(true);
}

void Application::Sequencer::draw()
{
	if (ImGui::BeginMenu("Sequence"))
	{
		ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);
		ImGui::InputScalar("Number of Robots", ImGuiDataType_U64, &n);
		ImGui::SliderFloat("Radius", &r, .5f, 1000.f);

		auto hasSomething = !seqImgs.empty();
		if (ImGui::MenuItem("Reset Positions", NULL, false, hasSomething))
			updateImg(true);

		if (ImGui::MenuItem("Prev", NULL, false, imgIndex))
		{
			imgIndex--;
			updateImg(false);
		}
		if (ImGui::MenuItem("Next", NULL, false, seqImgs.size() && imgIndex != seqImgs.size() - 1))
		{
			imgIndex++;
			updateImg(false);
		}

		ImGui::PopItemFlag();
		ImGui::EndMenu();
	}

	//if (!seqImgs.empty())
	//{
	//	auto& s = std::get<2>(*seqImgs[imgIndex]);
	//	auto& window = accessWindow();
	//	window.draw(s);
	//}

	if (imgViewer)
		imgViewer->drawImpl(false);
}

void Application::Sequencer::updateImg(bool resetPositions)
{
	auto& img = *seqImgs[imgIndex];
	if (resetPositions)
	{
		std::unique_ptr<sf::Image> imgPtr;
		MakeUniquePtr(imgPtr, img);
		MakeUniquePtr(imgViewer, app, std::move(imgPtr), r, n, false);

		auto& sim = imgViewer->sim;
		auto H = static_cast<size_t>(floor(sqrt(n)));
		auto W = n / H;
		auto mod = n % H;
		float spacing = 3.f;
		auto off = r * spacing;
		size_t cnt = 0;
		for (size_t i = 0; i != W; i++)
			for (size_t j = 0; j != H; j++)
				sim.addAgent(i * off, j * off);

		auto lastRowY = H * off;
		for (size_t i = 0; i != mod; i++)
			sim.addAgent(i * off, lastRowY);
	}
	else
		imgViewer->updateImg(img);
}

Application::ImgViewer::ImgViewer(Application& app, std::unique_ptr<sf::Image>&& c, float newR, size_t nRobotsBudget, bool calculateCircleCenters) :
	ViewerBase(app),
	currentImage(std::move(c)),
	isNavigatorMenuOpened(false),
	sim(&app, newR),
	r(newR),
	nRobotsBudget(nRobotsBudget)
{
	currentFilterSprite.setScale(8, 8); // make each pixel 8 pixels

	auto [width, height] = currentImage->getSize();
	curMinHexagonSide = r / SIN_60;
	{
		auto rowsF = static_cast<float>(height);
		vec2f p{ static_cast<float>(width), rowsF };
		auto proj = projPointUAxis(p, HEX_AXIS[0]);
		curMaxHexagonSide = std::max(rowsF, length(proj)) / SIN_60;
	}
	signalNeedToUpdateHexPlot();

	updateImgImpl(calculateCircleCenters);
}

void Application::ImgViewer::updateImg(const sf::Image& img)
{
	*currentImage = img;
	updateImgImpl(false);
}

void Application::ImgViewer::updateImgImpl(bool calculateCircleCenters)
{
	auto [width, height] = currentImage->getSize();
	currentImageTexture.loadFromImage(*currentImage);
	currentImageSprite.setTexture(currentImageTexture, true);


	imgWF = static_cast<float>(width);
	imgHF = static_cast<float>(height);

	auto newImgCenter = app.getWindowSizeF() * .5f;

	auto halfX = imgWF * .5f;
	auto halfY = imgHF * .5f;

	auto fixSprite = [&](auto&& s)
	{
		s.setOrigin(halfX, halfY);
		s.setPosition(newImgCenter);
	};
	fixSprite(currentImageSprite);
	fixSprite(colorMapSprite);

	sf::Vector2f prevImgCenter(
		imgWF * .5f,
		imgHF * .5f
	);
	imageOffset = newImgCenter - prevImgCenter;
	hexagonSide = std::max(imgWF * 0.05f, curMinHexagonSide);

	//updateVarsThatDependOnCircleAndHexagon();
	updateHexVars();
	if (calculateCircleCenters)
		recalculateCircleCenters();

	circleRadiusUpdateCallback();
}

void Application::ImgViewer::draw()
{
	drawImpl(true);
}

Application::ViewerBase::ViewerBase(Application& app) :
	app(app)
{
}


void Application::updateZoomFacAndViewSizeFromZoomLevel(sf::View& view)
{
	zoomFac = powf(2, static_cast<float>(zoomLevel) / N_SCROLS_TO_DOUBLE);
	view.setSize(sf::Vector2f(window.getSize()) * zoomFac);
}
