#include "Pch.hpp"
#include "ViewerBase.hpp"

#include "Application.hpp"

ViewerBase::ViewerBase(Application& app) :
	app{ app },
	circleColorSFML{ sf::Color::Red },
	circleColorF3{ 1, 0, 0 },
	highlightPixelHovered{ false },
	drawBorder{ true },
	radius{ 10.f }
{
}

void ViewerBase::draw()
{
	auto& w = app.window;
	auto& img = accessImgViewer();
	switch (renderType)
	{
	case RenderType::Image:
	{
		w.draw(img.currentImageSprite);
	}
	break;
	case RenderType::ColorMap:
	{
		w.draw(img.colorMapSprite);
	}
	break;
	}
	if (const auto& previewer = img.previewer)
		previewer->draw();
	if (highlightPixelHovered)
	{
		auto point = sf::Mouse::getPosition(w);
		auto curPos = w.mapPixelToCoords(point);
		curPos -= img.imageOffsetF;
		const auto size = img.currentImage->getSize();
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
				static_cast<float>(floor(curPos.x)) + img.imageOffsetF.x,
				static_cast<float>(floor(curPos.y)) + img.imageOffsetF.y
			);
			w.draw(rect);
		}
	}
	if (drawBorder)
	{
		sf::RectangleShape rect{ ToFloat(img.currentImage->getSize()) };
		rect.setFillColor({ 0, 0, 0, 0 });
		rect.setOutlineColor(img.borderColor);
		rect.setOutlineThickness(1.5f);
		rect.setPosition(img.imageOffsetF);
		w.draw(rect);
	}
}

void ViewerBase::drawUI()
{
	auto& w = app.window;
	if (ImGui::BeginMenu("Image"))
	{
		//if (ImGui::BeginMenu("Rendering"))
		{
			static constexpr const char* RENDER_TYPES_NAMES[] = {
					"None",
					"Image",
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
			app.zoomLevel = DEFAULT_ZOOM_LEVEL;
		}
		ImGui::EndMenu();
	}

	auto& img = accessImgViewer();
	if (auto& previewer = img.previewer)
	{
		if (!previewer->drawUI())
			previewer.reset();
	}
	else
	{
		if (ImGui::BeginMenu("Previewer"))
		{
			if (ImGui::MenuItem("Hexagon"))
				img.startHexagonPreview();
			if (ImGui::MenuItem("Voronoi"))
				img.startVoronoiPreview();
			ImGui::EndMenu();
		}
	}
}

sf::Color ViewerBase::getColor(float x, float y)
{
	if (renderType == RenderType::RightColor)
	{
		const auto& img = accessImgViewer();

		auto
			wx = x - img.imageOffsetF.x,
			wy = y - img.imageOffsetF.y;

		auto
			xi = lround(wx),
			yi = lround(wy);

		if (xi >= 0 and yi >= 0)
		{
			unsigned
				xu = xi,
				yu = yi;

			const auto [C, R] = img.colorMapImage.getSize();
			if (xu < C && yu < R)
				return img.colorMapImage.getPixel(xi, yi);
		}
		return app.getSFBackgroundColor();
	}
	return circleColorSFML;
}

sf::Color ViewerBase::getColor(const sf::Vector2f& c)
{
	return getColor(c.x, c.y);
}

bool ViewerBase::isMarked(const sf::Color& c)
{
	return c.a;
}