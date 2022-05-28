#include "Pch.hpp"
#include "Application.hpp"

#include "FileExplorer.hpp"
#include "OpenCVSFML.hpp"
#include "Utilities.hpp"

bool Application::isMarked(const sf::Color& c)
{
	return c.a;
}

Application::Application(sf::RenderWindow& window) :
	window(window),
	simulator(window)
{
	currentFilterSprite.setScale(8, 8); // make each pixel 8 pixels
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

	simulator.pollEvent(event);
}

void Application::draw()
{
	window.clear(sf::Color{
		static_cast<sf::Uint8>(backgroundColor[0] * 255),
		static_cast<sf::Uint8>(backgroundColor[1] * 255),
		static_cast<sf::Uint8>(backgroundColor[2] * 255),
		});

	//ImPlot::ShowDemoWindow();
	//simulator.draw();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open Image", "Ctrl+O"))
				controlO();

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

		if (isImageEditorActive)
		{
			if (ImGui::BeginMenu("Image"))
			{
				if (ImGui::MenuItem("Centralize"))
				{
					window.setView(window.getDefaultView());
					simulator.zoomLevel = Simulator2D::DEFAULT_ZOOM_LEVEL;
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

				if (ImGui::BeginMenu("Pixel"))
				{
					if (ImGui::Checkbox("Highlight Hovered", &highlightPixelHovered))
					{

					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Circle"))
				{
					if (ImGui::Checkbox("Show", &showCircleCenters))
					{
					}
					if (ImGui::Checkbox("Border Only", &showCircleBorderOnly))
					{

					}
					if (ImGui::SliderFloat("Radius", &circleRadius, .5f, hexHeight))
					{
						curMinHexagonSide = circleRadius * INV_SIN_60;
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
					if (ImGui::Checkbox("Show Grid", &showHexGrid))
					{

					}
					if (ImGui::Checkbox("Show Available Area", &showSmallerHexagon))
					{
					}
					if (ImGui::Checkbox("Highlight Hovered", &highlightHexHovered))
					{
					}
					if (ImGui::SliderFloat("Side", &hexagonSide, curMinHexagonSide, curMaxHexagonSide))
					{
						updateVarsThatDependOnCircleAndHexagon();
					}
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

				if (ImGui::BeginMenu("Alocation"))
				{
					if (ImGui::BeginMenu("Objective Points"))
					{

						if (ImGui::Checkbox("Fit", &bestFitCircles))
						{
							recalculateCircleCentersAndPlot();
						}
						if (bestFitCircles)
							if (ImGui::Checkbox("Draw Original Offsets", &drawOrgOffsets))
							{

							}

						if (ImGui::Checkbox("Use Only Pixels on Configuration Space", &yellowHexagonOnly))
						{
							recalculateCircleCentersAndPlot();
						}

						//if (!yellowHexagonOnly)
						if (ImGui::Checkbox("Clip To Configuration Space", &clipToConfigurationSpace))
						{
							if (!yellowHexagonOnly)
								recalculateCircleCenters();
						}


						ImGui::EndMenu();
					}
					//if (ImGui::BeginMenu("Assignment"))
					//{
					//
					//	ImGui::EndMenu();
					//}

					ImGui::EndMenu();
				}

				//if (ImGui::BeginMenu("Navigation"))
				//{
				//	ImGui::EndMenu();
				//}

				if (ImGui::MenuItem("Close"))
				{
					isImageEditorActive = false;
				}

				ImGui::EndMenu();
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
				sf::CircleShape circle(circleRadius);

				if (renderType != RenderType::CircleColor)
				{
					if (showCircleBorderOnly)
					{
						circle.setOutlineColor(circleBorderColor);
						circle.setOutlineThickness(-circleRadius * .1f);
						circle.setFillColor(sf::Color(0)); // a gente só liga pro alpha = 0, mas a API não deixa	mexer só no alpha
					}
					else
						circle.setFillColor(circleBorderColor);
				}

				circle.setOrigin(circleRadius, circleRadius);
				for (auto& circleCenter : circleCenters)
				{
					circle.setPosition(circleCenter);

					if (renderType == RenderType::CircleColor)
					{
						auto pixelCoord = circleCenter - imageOffset;
						auto x = static_cast<unsigned>(floor(pixelCoord.x));
						auto y = static_cast<unsigned>(floor(pixelCoord.y));
						auto [w, h] = colorMapImage.getSize();
						if (x < w && y < h)
							circle.setFillColor(colorMapImage.getPixel(x, y));
						else
						{
							//circle.setFillColor(sf::Color::Cyan);
							continue; // ignore o círculo que está em um hexágono fora do mapa
						}
					}

					window.draw(circle);
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
						auto center = getHexagonCenter(x, y);
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
				auto size = currentImage.getSize();
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
		else
		{
			if (ImGui::BeginMenu("Free Edit"))
			{
				ImGui::EndMenu();
			}
		}

		ImGui::EndMainMenuBar();
	}

	ImGui::SFML::Render(window);
	window.display();
}

sf::Vector2f Application::getHexagonCenter(int x, int y)
{
	auto cx = x * hexagonStride;
	auto cy = y * hex2Heights;
	if (x % 2)
		cy += hexHeight;
	return { cx, cy };
}

const vec2f& Application::accessHexagonUAxis(const vec2f& off)
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

bool Application::isPixelInsideSmallerHexagon(const sf::Vector2f& pixelCenter, int hx, int hy)
{
	auto hc = getHexagonCenter(hx, hy);
	vec2f off = pixelCenter - hc;
	//off.y = -off.y; // not necessary because it's symmetric
	auto& uAxis = accessHexagonUAxis(off);
	auto proj = projPointUAxis(off, uAxis);
	auto len2 = dot(proj, proj);
	return len2 < smallerHexHeightSquared;
}

void Application::updateSmallHexagonData()
{
	const auto& a = hexHeight;
	const auto& r = circleRadius;
	smallerHexRatio = (a - r) / a;
	smallerHexHeight = hexHeight * smallerHexRatio;
	smallerHexHeightSquared = square(smallerHexHeight);
}

void Application::updateHexagonAuxiliarVariables()
{
	hexagonHalfSide = hexagonSide * .5f;
	hexagonStride = hexagonSide + hexagonHalfSide;
	hexHeight = hexagonSide * SIN_60;
	hex2Heights = hexHeight + hexHeight;
	hexGridXMax = 1 + static_cast<size_t>(ceil((imgWF - hexagonHalfSide) / hexagonStride));
	hexGridYMax = 1 + static_cast<size_t>(ceil((imgHF - hexHeight) / hex2Heights));
	nHexagons = hexGridXMax * hexGridYMax;
}

void Application::circleRadiusUpdateCallback()
{
	auto side = static_cast<unsigned>(ceil(circleRadius));

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

	colorMapImage = currentImage; // hard bug to find. Opencv does things with pointers
	auto mat = SFMLImageToOpenCVBGRA(colorMapImage);

	cv::Mat kernel(side, side, CV_32F, (void*)filterValues.data());
	cv::filter2D(mat, mat, -1, kernel);
	CVBGRAToSFMLTextureAndImage(mat, colorMapTexture, colorMapImage);
	colorMapSprite.setTexture(colorMapTexture, true);
}

void Application::signalNeedToUpdateHexPlot()
{
	hexPlotReady = false;
}

void Application::updateHexVars()
{
	updateHexagonAuxiliarVariables();
	updateSmallHexagonData();
}

void Application::updateVarsThatDependOnCircleAndHexagon()
{
	updateHexVars();
	recalculateCircleCenters();
}

sf::Vector2i Application::getHexagonHovered(sf::Vector2f& coord)
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

sf::Vector2i Application::getHexagonHoveredConserving(const sf::Vector2f& coord)
{
	sf::Vector2f copy = coord;
	return getHexagonHovered(copy);
}

std::optional<sf::Vector2i> Application::tryGetHexagonHovered(sf::Vector2f& coord)
{
	auto v = getHexagonHovered(coord);
	auto& [x, y] = v;
	if (x >= 0 && x < hexGridXMax &&
		y >= 0 && y < hexGridYMax)
		return v;
	return {};
}

size_t Application::getNumCircles(float curHexSide)
{
	hexagonSide = curHexSide;
	updateHexVars();

	// código retirado e cropado de recalculateCircleCenters
	size_t r = 0;
	if (bestFitCircles)
	{
		std::vector<std::vector<size_t>> hexagonsPixels(hexGridXMax, std::vector<size_t>(hexGridYMax));
		auto [w, h] = currentImage.getSize();
		for (unsigned i = 0; i != w; i++)
			for (unsigned j = 0; j != h; j++)
			{
				auto px = currentImage.getPixel(i, j);
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

void Application::recalculateCircleCenters()
{
	circleCenters.clear();
	if (bestFitCircles)
	{
		// para cada pixel, calcula o quanto ele influencia cada smaller hexagon
		// hexagonsPixels[x][y] guarda os pixels marcados que "pertencem" a esse hexágono pequeno
		// guarda apenas a soma das coordenadas e a quantidade deles para depois fazermos a média
		std::vector<std::vector<std::pair<sf::Vector2f, size_t>>> hexagonsPixels(hexGridXMax,
			std::vector<std::pair<sf::Vector2f, size_t>>(hexGridYMax));
		auto [w, h] = currentImage.getSize();
		for (unsigned i = 0; i != w; i++)
			for (unsigned j = 0; j != h; j++)
			{
				auto px = currentImage.getPixel(i, j);
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
					auto hexCenter = getHexagonCenter(x, y);

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
				circleCenters.emplace_back(getHexagonCenter(x, y) + imageOffset);
}

void Application::recalculateCircleCentersAndPlot()
{
	recalculateCircleCenters();
	signalNeedToUpdateHexPlot();
}


std::array<sf::Vector2f, 6> Application::getHexagonOffsets()
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

std::array<sf::Vector2f, 6> Application::getHexagonCoords(int x, int y)
{
	auto [cx, cy] = getHexagonCenter(x, y);
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
		sf::String s(path);
		if (currentImage.loadFromFile(s))
		{
			isImageEditorActive = true;

			currentImageTexture.loadFromImage(currentImage);
			currentImageSprite.setTexture(currentImageTexture, true);

			auto [w, h] = currentImage.getSize();

			imgWF = static_cast<float>(w);
			imgHF = static_cast<float>(h);

			auto newImgCenter = getWindowSizeF() * .5f;

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
			hexagonSide = imgWF * 0.05f;
			circleRadius = DEFAULT_CIRCLE_RADIUS;
			curMinHexagonSide = DEFAULT_CIRCLE_RADIUS / SQRT_3; // r / 2 / (sin(60))
			curMaxHexagonSide = static_cast<float>(currentImage.getSize().x);
			circleRadiusUpdateCallback();
			updateVarsThatDependOnCircleAndHexagon();
			signalNeedToUpdateHexPlot();
		}
		else
		{
			LOG("Error importing image ", path, '\n');
		}
	}
}
