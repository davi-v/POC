#include "Pch.hpp"
#include "Application.hpp"

#include <imgui/imgui_internal.h>

#include "FileExplorer.hpp"
#include "OpenCVSFML.hpp"
#include "Utilities.hpp"
#include "Sequencer.hpp"

void Application::createSequencer()
{
	//viewerBase = std::make_unique<Sequencer>(*this);
}

ImgViewer& Application::accessImgViewer()
{
	return viewerBase->accessImgViewer();
}

sf::Color Application::getSFBackgroundColor()
{
	return ToSFMLColor(backgroundColor);
}

Application::Application(sf::RenderWindow& window) :
	backgroundColor{},
	window(window),
	lastPxClickedX(-1),
	zoomFac(1),
	zoomLevel(DEFAULT_ZOOM_LEVEL)
{
	//tryReadImg(L"exemplos/listra.png");
	//tryReadImg(L"exemplos/i4.png");
	//viewerBase->accessImgViewer().startVoronoiPreview();
}

void Application::pollEvent(const sf::Event& e)
{
	switch (e.type)
	{
	case sf::Event::KeyPressed:
	{
		switch (e.key.code)
		{
		case sf::Keyboard::O:
		{
			if (e.key.control)
				controlO();
		}
		break;
		}
	}
	break;
	}

	if (viewerBase)
		viewerBase->pollEvent(e);

	if (NotHoveringIMGui())
	{
		switch (e.type)
		{
		case sf::Event::MouseButtonPressed:
		{
			auto& data = e.mouseButton;
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

	switch (e.type)
	{
	case sf::Event::MouseWheelScrolled:
	{
		if (NotHoveringIMGui())
		{
			auto& scrollData = e.mouseWheelScroll;
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
	}
	break;
	case sf::Event::MouseMoved:
	{
		if (lastPxClickedX != -1) // if we are holding left click, move view of the world
		{
			auto& data = e.mouseMove;
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
		switch (e.mouseButton.button)
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
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open Image", "Ctrl+O"))
				controlO();

			if (ImGui::MenuItem("Create Sequence"))
				createSequencer();

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
		if (viewerBase)
			viewerBase->drawUI();
		ImGui::EndMainMenuBar();
	}
	if (viewerBase)
		viewerBase->draw();
	ImGui::SFML::Render(window);
	window.display();
}

sf::Vector2f Application::getWindowSizeF()
{
	auto [x, y] = window.getSize();
	return sf::Vector2f{ static_cast<float>(x), static_cast<float>(y) };
}

void Application::tryReadImg(const wchar_t* path)
{
	const sf::String s{ path };
	std::unique_ptr<sf::Image> currentImage;
	MakeUniquePtr(currentImage);
	if (currentImage->loadFromFile(s))
		viewerBase = std::make_unique<ImgViewer>(*this, std::move(currentImage));
	else
		LOG("Error importing image ", path, '\n');
}

void Application::controlO()
{
	if (const auto path = TryOpenFile(L"Choose Image to Load", SFML_SUPPORTED_IMAGE_FORMATS_SIZE, SFML_SUPPORTED_IMAGE_FORMATS))
		tryReadImg(path);
}

void Application::updateZoomFacAndViewSizeFromZoomLevel(sf::View& view)
{
	zoomFac = powf(2, static_cast<float>(zoomLevel) / N_SCROLS_TO_DOUBLE);
	view.setSize(sf::Vector2f(window.getSize()) * zoomFac);
}