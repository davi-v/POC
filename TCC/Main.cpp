#include "Pch.hpp"
#include "Simulator2D.hpp"
#include "Utilities.hpp"
#include "FileExplorer.hpp"
#include "vec2.hpp"

#include "OpenCVSFML.hpp"

static const std::string SCREENSHOTS_DIR = "screenshots/";
static constexpr auto SCREENSHOT_EXT = ".png";
static constexpr auto PI = std::numbers::pi_v<float>;

constexpr auto SQRT_3 = 1.7320508075688772f;

const auto MARKED_COLOR = sf::Color::White;

constexpr auto DEFAULT_CIRCLE_RADIUS = 1.f;
constexpr auto SIN_60 = 0.8660254037844386f;
constexpr auto COS_60 = .5;
constexpr auto TAN_60 = SQRT_3;
constexpr auto SIN_30 = COS_60;
constexpr auto COS_30 = SIN_60;
constexpr auto INV_SIN_60 = 1 / SIN_60;
const auto hexGridColor = sf::Color::Red;
const auto smallHexColor = sf::Color(255, 255, 0, 100);
const auto hexHighlightColor = sf::Color(0, 0, 255, 170);

constexpr auto SIXTY_DEG = PI / 3;
constexpr float HEX_ANGLES[]
{
	SIXTY_DEG,
	SIXTY_DEG * 2,
	SIXTY_DEG * 3,
	SIXTY_DEG * 4,
	SIXTY_DEG * 5,
};

constexpr vec2f HEX_AXIS[]
{
	{COS_30, SIN_30},
	{0, 1},
	{-COS_30, SIN_30},
	{-COS_30, -SIN_30},
	{0, -1},
	{COS_30, -SIN_30},
};

float hexagonSide, hexHeight, hexagonHalfSide,
hexagonStride, // 1.5 * hexagonSide
hex2Heights,
circleRadius,
curMinHexagonSide, // r / 2 / (sin(60))
smallerHexRatio,
smallerHexHeight,
smallerHexHeightSquared;

auto circleBorderColor = sf::Color(255, 0, 0, 170);

size_t
hexGridXMax,
hexGridYMax;
float imgWF, imgHF;
sf::Vector2f imageOffset;
sf::Image currentImage;

// in local space (i.e. before translating)
sf::Vector2f GetHexagonCenter(int x, int y)
{
	auto cx = x * hexagonStride;
	auto cy = y * hex2Heights;
	if (x % 2)
		cy += hexHeight;
	return { cx, cy };
}

// pixelCenter in local coordinates of hex grid
// hx, hy : precomputed hexagon coord that pixelCenter is in
bool IsPixelInsideSmallerHexagon(const sf::Vector2f& pixelCenter, int hx, int hy)
{
	auto hc = GetHexagonCenter(hx, hy);
	auto off = pixelCenter - hc;
	auto angle = atan2(off.y, off.x) + PI; // [0, 2*pi]
	int i = 0;
	for (; i != 5; i++)
	{
		if (angle < HEX_ANGLES[i])
			break;
	}
	vec2f o{ off.x, off.y };
	auto proj = projPointUAxis(o, HEX_AXIS[i]);
	auto len2 = dot(proj, proj);
	return len2 < smallerHexHeightSquared;
}

void updateVarsOnCircleThatDependOnHex()
{
	const auto& a = hexHeight;
	const auto& r = circleRadius;
	smallerHexRatio = (a - r) / a;
	smallerHexHeight = hexHeight * smallerHexRatio;
	smallerHexHeightSquared = square(smallerHexHeight);
}

size_t nHexagons;
void updateVarsOnHex()
{
	hexagonHalfSide = hexagonSide * .5f;
	hexagonStride = hexagonSide + hexagonHalfSide;
	hexHeight = hexagonSide * SIN_60;
	hex2Heights = hexHeight + hexHeight;
	hexGridXMax = 1 + static_cast<size_t>(ceil((imgWF - hexagonHalfSide) / hexagonStride));
	hexGridYMax = 1 + static_cast<size_t>(ceil((imgHF - hexHeight) / hex2Heights));
	nHexagons = hexGridXMax * hexGridYMax;
}


void updateVarsThatDependOnHexagonSideAndCircleRadius()
{
	updateVarsOnHex();
	updateVarsOnCircleThatDependOnHex();
}

bool bestFitCircles = false;
std::vector<sf::Vector2f> circleCenters;

sf::Vector2i GetHexagonHovered(sf::Vector2f& coord)
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

sf::Vector2i GetHexagonHoveredConserving(const sf::Vector2f& coord)
{
	sf::Vector2f copy = coord;
	return GetHexagonHovered(copy);
}
std::optional<sf::Vector2i> TryGetHexagonHovered(sf::Vector2f& coord)
{
	auto v = GetHexagonHovered(coord);
	auto& [x, y] = v;
	if (x >= 0 && x < hexGridXMax &&
		y >= 0 && y < hexGridYMax)
		return v;
	return {};
}

void recalculateCircleCenters() // should be called after updateVarsThatDependOnHexagonSideAndCircleRadius, since updateVarsThatDependOnHexagonSideAndCircleRadius can modify hexGridXMax and hexGridYMax
{
	circleCenters.clear();
	if (bestFitCircles)
	{
		// para cada pixel, calcula o quanto ele influencia cada smaller hexagon
		// hexagonsPixels[x][y] guarda os pixels marcados que "pertencem" a esse hexágono pequeno
		// guarda apenas a soma das coordenadas, e a quantidade deles para depois fazermos a média
		std::vector<std::vector<std::pair<sf::Vector2f, size_t>>> hexagonsPixels(hexGridXMax,
			std::vector<std::pair<sf::Vector2f, size_t>>(hexGridYMax));
		auto [w, h] = currentImage.getSize();
		for (unsigned i = 0; i != w; i++)
			for (unsigned j = 0; j != h; j++)
			{
				auto px = currentImage.getPixel(i, j);
				if (px == MARKED_COLOR) // pixel marcado
				{
					// local coordinates of the hex grid
					sf::Vector2f pixelCenter{
						static_cast<float>(i) + .5f,
						static_cast<float>(j) + .5f,
					};
					auto [hx, hy] = GetHexagonHoveredConserving(pixelCenter);
					if (IsPixelInsideSmallerHexagon(pixelCenter, hx, hy))
					{
						auto& v = hexagonsPixels[hx][hy];
						v.first += pixelCenter;
						v.second++;
					}
				}
			}


		for (int x = 0; x != hexGridXMax; x++)
			for (int y = 0; y != hexGridYMax; y++)
			{
				const auto& v = hexagonsPixels[x][y];
				const auto& coord = v.first;
				const auto& n = v.second;
				if (n)
					circleCenters.emplace_back(sf::Vector2f{ coord.x / n, coord.y / n } + imageOffset);
			}
	}
	else
		for (int x = 0; x != hexGridXMax; x++)
			for (int y = 0; y != hexGridYMax; y++)
			{
				auto center = GetHexagonCenter(x, y);
				center += imageOffset;
				circleCenters.emplace_back(center);
			}
}

// in local coordinates
std::array<sf::Vector2f, 6> GetHexagonOffsets()
{
	return
	{
		sf::Vector2f{ + hexagonHalfSide, - hexHeight },
		sf::Vector2f{ + hexagonSide, 0 },
		sf::Vector2f{ + hexagonHalfSide, + hexHeight },
		sf::Vector2f{ - hexagonHalfSide, + hexHeight },
		sf::Vector2f{ - hexagonSide, 0 },
		sf::Vector2f{ - hexagonHalfSide, - hexHeight },
	};
}

// hexagon (0, 0) center is on (0, 0)
std::array<sf::Vector2f, 6> GetHexagonCoords(int x, int y)
{
	auto [cx, cy] = GetHexagonCenter(x, y);
	auto coords = GetHexagonOffsets();
	for (auto& coord : coords)
	{
		coord.x += cx;
		coord.y += cy;
	}
	return coords;
}

void updateHexAndCircle()
{
	updateVarsThatDependOnHexagonSideAndCircleRadius();
	recalculateCircleCenters();
}

// according to documentation
// bmp, png, tga, jpg, gif, psd, hdr and pic
#define x(s) L"*."#s,
static constexpr wchar_t const* SFML_SUPPORTED_IMAGE_FORMATS[] = {
	x(bmp)
	x(png)
	x(tga)
	x(jpg)
	x(gif)
	x(psd)
	x(hdr)
	x(pic)
};
#undef x
static constexpr auto SFML_SUPPORTED_IMAGE_FORMATS_SIZE = sizeof(SFML_SUPPORTED_IMAGE_FORMATS) / sizeof(decltype(*SFML_SUPPORTED_IMAGE_FORMATS));

#define USE_CUSTOM_FONT_IN_IMGUI

int main()
{
	sf::RenderWindow window{
#ifdef _DEBUG
		sf::VideoMode{ 1280, 720 }
#else
		sf::VideoMode::getDesktopMode()
#endif
		,"Simulator",
#ifdef _DEBUG
		sf::Style::Close
#else
		sf::Style::Fullscreen
#endif
	};

	//sf::Image image;
	//image.loadFromFile("happy.png");
	//auto mBGR = SFMLImageToOpenCVNative(image);
	//
	////const int T = 3;cv::blur(mBGR, mBGR, cv::Size(T, T));
	//
	//cv::Mat mRGBA;
	//cv::cvtColor(mBGR, mRGBA, cv::COLOR_RGB2BGRA);
	//sf::Texture texture;
	//UpdateTextureFromCVRGBA(texture, mRGBA, image);
	//sf::Sprite sprite;
	//sprite.setTexture(texture, true);
	//
	//sf::Event event;
	//while (window.isOpen())
	//{
	//	while (window.pollEvent(event))
	//	{
	//		switch (event.type)
	//		{
	//		case sf::Event::Closed:
	//		{
	//			window.close();
	//		}
	//		break;
	//		}
	//	}
	//	window.clear();
	//	window.draw(sprite);
	//	window.display();
	//}
	//
	//return -1;

#ifdef USE_CUSTOM_FONT_IN_IMGUI
	ImGui::SFML::Init(window, false);
	auto& IO = ImGui::GetIO();
	IO.Fonts->AddFontFromFileTTF("segoeui.ttf", 28);
	ImGui::SFML::UpdateFontTexture();
#else
	ImGui::SFML::Init(window);
#endif

	bool isImageEditorActive = false;
	bool showHexGrid = false;
	bool highlightPixelHovered = false;
	bool highlightHexHovered = false;
	bool showCircleCenters = false;
	bool showCircleBorderOnly = false;

	bool showSmallerHexagon = false;
	sf::Texture currentTexture;
	sf::Sprite currentSprite;

	Simulator2D simulator{ window };

	auto getWindowSizeF = [&]
	{
		auto [x, y] = window.getSize();
		return sf::Vector2f{ static_cast<float>(x), static_cast<float>(y) };
	};

	auto ControlO = [&]
	{
		if (auto path = TryOpenFile(L"Choose Image to Load", SFML_SUPPORTED_IMAGE_FORMATS_SIZE, SFML_SUPPORTED_IMAGE_FORMATS))
		{
			sf::String s(path);
			if (currentImage.loadFromFile(s))
			{
				currentTexture.loadFromImage(currentImage);
				currentSprite.setTexture(currentTexture, true);
				auto [w, h] = currentImage.getSize();
				currentSprite.setOrigin(w * .5f, h * .5f);
				auto newImgCenter = getWindowSizeF() * .5f;
				currentSprite.setPosition(newImgCenter);

				imgWF = static_cast<float>(w),
					imgHF = static_cast<float>(h);

				sf::Vector2f prevImgCenter(
					imgWF * .5f,
					imgHF * .5f
				);
				imageOffset = newImgCenter - prevImgCenter;
				isImageEditorActive = true;
				hexagonSide = imgWF * 0.05f;
				circleRadius = DEFAULT_CIRCLE_RADIUS;
				curMinHexagonSide = DEFAULT_CIRCLE_RADIUS / SQRT_3; // r / 2 / (sin(60))
				updateHexAndCircle();
			}
			else
			{
				LOG("Error importing image ", path, '\n');
			}
		}
	};

	sf::Event event;
	sf::Clock c;
	while (window.isOpen())
	{
		while (window.pollEvent(event))
		{
			ImGui::SFML::ProcessEvent(window, event);

			switch (event.type)
			{
				case sf::Event::Closed:
				{
					window.close();
				}
				break;
				case sf::Event::KeyPressed:
				{
					switch (event.key.code)
					{
					case sf::Keyboard::F2: // screenshot
					{
						std::filesystem::create_directory(SCREENSHOTS_DIR);
						sf::Texture tex;
						auto [x, y] = window.getSize();
						tex.create(x, y);
						tex.update(window);
						auto img = tex.copyToImage();
						auto f = GetUniqueNameWithCurrentTime(SCREENSHOTS_DIR, SCREENSHOT_EXT);
						img.saveToFile(f);
					}
					break;
					case sf::Keyboard::O:
					{
						if (event.key.control)
							ControlO();
					}
					break;
					}
				}
				break;
			}

			simulator.pollEvent(event);
		}
		
		ImGui::SFML::Update(window, c.restart());

		window.clear();
		//simulator.draw();

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open Image", "Ctrl+O"))
					ControlO();
			
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
						if (ImGui::SliderFloat("Radius", &circleRadius,
							0.5f,
							hexHeight))
						{
							curMinHexagonSide = circleRadius * INV_SIN_60;
							if (hexagonSide < curMinHexagonSide)
							{
								hexagonSide = curMinHexagonSide;
								updateVarsOnHex();
							}
							updateVarsOnCircleThatDependOnHex();
							recalculateCircleCenters();
						}
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
						if (ImGui::SliderFloat("Side", &hexagonSide,
							curMinHexagonSide,
							static_cast<float>(currentImage.getSize().x)))
						{
							updateHexAndCircle();
						}
						ImGui::EndMenu();
					}

					if (ImGui::Checkbox("Best Fit Circles", &bestFitCircles))
					{
						recalculateCircleCenters();
					}

					ImGui::EndMenu();
				}

				window.draw(currentSprite);
				if (showHexGrid)
				{
					for (int x = 0; x != hexGridXMax; x++)
						for (int y = 0; y != hexGridYMax; y++)
						{
							auto coords = GetHexagonCoords(x, y);
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
					if (showCircleBorderOnly)
					{
						circle.setOutlineColor(circleBorderColor);
						circle.setOutlineThickness(-circleRadius * .1f);
						circle.setFillColor(sf::Color(0)); // a gente só liga pro alpha = 0, mas a API não deixa mexer só no alpha
					}
					else
						circle.setFillColor(circleBorderColor);

					circle.setOrigin(circleRadius, circleRadius);
					for (auto& circleCenter : circleCenters)
					{
						circle.setPosition(circleCenter);
						window.draw(circle);
					}
				}
				if (showSmallerHexagon)
				{
					auto localCoords = GetHexagonOffsets();
					for (auto& coord : localCoords)
						coord = coord * smallerHexRatio + imageOffset;

					for (int x = 0; x != hexGridXMax; x++)
						for (int y = 0; y != hexGridYMax; y++)
						{
							auto center = GetHexagonCenter(x, y);
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
					auto w = static_cast<float>(size.x);
					auto h = static_cast<float>(size.y);
					if (curPos.x >= 0 && curPos.x < w &&
						curPos.y >= 0 && curPos.y < h
						)
					{
						unsigned
							x = static_cast<int>(floor(curPos.x)),
							y = static_cast<int>(floor(curPos.y));
						auto c = currentImage.getPixel(x, y);
						auto oppositeColor = sf::Color(255, 0, 0, 120);
						//auto oppositeColor = sf::Color(255 - c.r, 255 - c.g, 255 - c.b);
						sf::RectangleShape rect{{1,1}};
						rect.setFillColor(oppositeColor);
						rect.setOutlineColor(sf::Color::Red);
						rect.setOutlineThickness(-0.1f);
						rect.setPosition(
							static_cast<float>(x) + imageOffset.x,
							static_cast<float>(y) + imageOffset.y
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
					if (auto optHexagon = TryGetHexagonHovered(coord))
					{
						auto& [x, y] = *optHexagon;
						auto coords = GetHexagonCoords(x, y);
						for (int i = 0; i != 6; i++)
						{
							auto& p = hexagon[i];
							p.position = coords[i] + imageOffset;
							p.color = hexHighlightColor;
						}
						window.draw(hexagon);
					}
				}
			}

			ImGui::EndMainMenuBar();
		}

		ImGui::SFML::Render(window);
		window.display();
	}
	ImGui::SFML::Shutdown();
}