#include "Pch.hpp"
#include "ViewerBase.hpp"

#include "Previewer.hpp"

#include "PreviewHex.hpp"
#include "PreviewVoronoi.hpp"
#include "PreviewPolygonize.hpp"
#include "SampleRandom.hpp"

ViewerBase::ViewerBase(Application* app, sf::Image* imgPtr) :
	app{ app },
	circleColorSFML{ sf::Color::Green },
	highlightPixelHovered{ false },
	drawBorder{ false },
	radius{ 10.f },
	borderColor(sf::Color::Red)
{
	onImgChangeBase(imgPtr);
	currentFilterSprite.setScale(PX_FILTER_SCALE, PX_FILTER_SCALE); // pixel side of filter preview
}

void ViewerBase::draw()
{
	auto& w = app->window;
	switch (renderType)
	{
	case RenderType::RGBA:
	{
		w.draw(currentImageSprite);
	}
	break;
	case RenderType::A:
	{
		w.draw(currentImageASprite);
	}
	break;
	case RenderType::ColorMap:
	{
		w.draw(colorMapSprite);
	}
	break;
	}
	if (previewer)
		previewer->draw();
	if (highlightPixelHovered)
	{
		auto point = sf::Mouse::getPosition(w);
		auto curPos = w.mapPixelToCoords(point);
		curPos -= imageOffsetF;
		const auto size = currentImage->getSize();
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
			//rect.setOutlineThickness(-0.1f);
			rect.setPosition(
				static_cast<float>(floor(curPos.x)) + imageOffsetF.x,
				static_cast<float>(floor(curPos.y)) + imageOffsetF.y
			);
			w.draw(rect);
		}
	}
	if (drawBorder)
	{
		sf::RectangleShape rect{ ToFloat(currentImage->getSize()) };
		rect.setFillColor({ 0, 0, 0, 0 });
		rect.setOutlineColor(borderColor);
		rect.setOutlineThickness(1.5f);
		rect.setPosition(imageOffsetF);
		w.draw(rect);
	}
}

sf::Color ViewerBase::getRightColor(float x, float y)
{
	auto
		wx = x - imageOffsetF.x,
		wy = y - imageOffsetF.y;

	auto
		xi = lround(wx),
		yi = lround(wy);

	if (xi >= 0 and yi >= 0)
	{
		unsigned
			xu = xi,
			yu = yi;

		const auto& img = previewer->accessColorMapImg();
		const auto [C, R] = img.getSize();
		if (xu < C && yu < R)
			return img.getPixel(xi, yi);
	}
	return app->getSFBackgroundColor();
}

sf::Color ViewerBase::getRightColor(const vec2f& c)
{
	return getRightColor(c.x, c.y);
}

bool ViewerBase::drawUI()
{
	bool ret = true;
	auto& w = app->window;
	if (ImGui::BeginMenu("Image"))
	{
		//if (ImGui::BeginMenu("Rendering"))
		{
			static constexpr const char* RENDER_TYPES_NAMES[] = {
					"None",
					"RGBA",
					"A",
					"Color Map",
					"Right Colors"
			};
			ImGui::Combo("Render Type", reinterpret_cast<int*>(&renderType), RENDER_TYPES_NAMES, IM_ARRAYSIZE(RENDER_TYPES_NAMES));
			ImGui::Checkbox("Image Border", &drawBorder);
			//ImGui::EndMenu();
		}
		ImGui::Checkbox("Pixel Highlight Hovered", &highlightPixelHovered);
		if (ImGui::MenuItem("Centralize"))
		{
			w.setView(w.getDefaultView());
			app->zoomLevel = DEFAULT_ZOOM_LEVEL;
		}
		if (ImGui::MenuItem("Close"))
			ret = false;
		ImGui::EndMenu();
	}

	if (previewer)
	{
		if (!previewer->drawUI())
			previewer.reset();
	}
	else
	{
		drawPreviewerSelectorMenu();
	}

	drawUIExtra();

	return ret;
}

sf::Color ViewerBase::getColor(float x, float y)
{
	if (renderType == RenderType::RightColor)
		return getRightColor(x, y);
	return circleColorSFML;
}

sf::Color ViewerBase::getColor(const vec2f& c)
{
	return getColor(c.x, c.y);
}

void ViewerBase::pollEvent(const sf::Event& e)
{
	if (previewer)
		previewer->pollEvent(e);
	pollEventExtra(e);
}

void ViewerBase::drawPreviewerSelectorMenu()
{
	if (ImGui::BeginMenu("Previewer"))
	{
		if (ImGui::MenuItem("Hexagon"))
			previewer = std::make_unique<PreviewHex>(*this);
		if (ImGui::MenuItem("Voronoi"))
			previewer = std::make_unique<PreviewVoronoi>(*this);
		if (ImGui::MenuItem(POLYGONIZATION))
			previewer = std::make_unique<PreviewPolygonize>(*this);
		if (ImGui::MenuItem(RANDOM_SAMPLING))
			previewer = std::make_unique<SampleRandom>(*this);
		ImGui::EndMenu();
	}
}

void ViewerBase::onImgChange(sf::Image* imgPtr)
{
	onImgChangeBase(imgPtr);
	if (previewer)
		previewer->onImgChangeImpl();
}

void ViewerBase::pollEventExtra(const sf::Event& e)
{
}

void ViewerBase::drawUIExtra()
{
}

void ViewerBase::onImgChangeBase(sf::Image* imgPtr)
{
	currentImage = imgPtr;
	currentImageTexture.loadFromImage(*currentImage);
	currentImageSprite.setTexture(currentImageTexture);

	const auto& img = *currentImage;
	sf::Image imgA = *currentImage;
	const auto [W, H] = imgA.getSize();
	for (unsigned i = 0; i != H; i++)
		for (unsigned j = 0; j != W; j++)
		{
			auto px = imgA.getPixel(j, i);
			sf::Color newC(px.a, px.a, px.a);
			imgA.setPixel(j, i, newC);
		}
	currentImageATexture.loadFromImage(imgA);
	currentImageASprite.setTexture(currentImageATexture);

	const auto [imgW, imgH] = currentImage->getSize();
	imgWF = (float)imgW,
		imgHF = (float)imgH;
	auto halfX = imgWF * .5f;
	auto halfY = imgHF * .5f;
	const auto newImgCenter = app->getWindowSizeF() * .5f;
	auto centralizeSprite = [&](auto&& s)
	{
		s.setOrigin(halfX, halfY);
		s.setPosition(newImgCenter);
	};
	centralizeSprite(currentImageSprite);
	centralizeSprite(colorMapSprite);
	centralizeSprite(currentImageASprite);

	sf::Vector2f prevImgCenter(
		imgWF * .5f,
		imgHF * .5f
	);
	imageOffsetF = newImgCenter - prevImgCenter;
}

sf::Rect<double> ViewerBase::getBB() const
{
	const auto& off = imageOffsetF;
	const auto [X, Y] = currentImage->getSize();
	return { off.x, off.y, (double)X, (double)Y };
}
