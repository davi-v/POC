#include "Pch.hpp"
#include "Pch.hpp"
#include "Application.hpp"

//#include <imgui/imgui_internal.h>

#include "ImgViewer.hpp"
#include "FileExplorer.hpp"
#include "OpenCVSFML.hpp"
#include "Utilities.hpp"
#include "Sequencer.hpp"

void Application::tryCreateSequencer()
{
	auto dir = TrySelectDirectory(L"Selecione a pasta com as imagens 1.png, 2.png, ... , n.png");
	auto dirName = sf::String(dir);
	size_t cnt = 1;
	SequenceImg seqImgs;
	while (true)
	{
		std::unique_ptr<sf::Image> content;
		MakeUniquePtr(content);
		auto& img = *content;
		static constexpr auto SEP = '\\';
		if (!img.loadFromFile(dirName + SEP + std::to_wstring(cnt) + ".png"))
			break;
		cnt++;
		seqImgs.emplace_back(std::move(content));
	}
	if (cnt == 1)
	{
		if (dir.isOk())
			addMessage(dirName + " sem imagens no formato esperado");
	}
	else
	{
		editor.reset();
		viewerBase = std::make_unique<Sequencer>(this, std::move(seqImgs));
	}
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
	font.loadFromFile("segoeui.ttf");

	textPopUpMessages.setFont(font);
	textPopUpMessages.setFillColor(sf::Color::Red);
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
			{
				if (e.key.shift)
					tryCreateSequencer();
				else
					openImg();
			}
		}
		break;
		}
	}
	break;
	}

	if (viewerBase)
		viewerBase->pollEvent(e);
	else if (editor)
		editor->pollEvent(e);

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
				openImg();

			if (ImGui::MenuItem("Sequencer", "Ctrl+Shift+O"))
				tryCreateSequencer();

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
		{
			if (!viewerBase->drawUI())
				viewerBase.reset();
		}
		else
		{
			if (editor)
			{
				if (!editor->drawUI())
				{
					editor.reset();
				}
			}
			else
			{
				if (ImGui::MenuItem("Editor"))
				{
					MakeUniquePtr(editor, this, DEFAULT_RADIUS);
					//ImGui::EndMenu();
				}
			}
		}

		ImGui::EndMainMenuBar();
	}
	if (viewerBase)
		viewerBase->draw();
	else if (editor)
		editor->draw();

	ImGui::SFML::Render(window);

	// draw UI in screen space
	sf::View curView = window.getView(); // to restore later
	sf::View orgView = window.getDefaultView();
	window.setView(orgView);

	// pop-up messages
	std::string str;
	for (auto it = warnings.begin(); it != warnings.end(); )
	{
		auto& msg = it->first;
		auto& ts = it->second;
		auto now = globalClock.getElapsedTime();
		if ((now - ts).asSeconds() > MSG_DURATION)
			it = warnings.erase(it);
		else
		{
			str += msg;
			str += '\n';
			it++;
		}
	}
	textPopUpMessages.setString(str);
	auto bounds = textPopUpMessages.getLocalBounds();
	textPopUpMessages.setOrigin(bounds.width * 0.5f, 0);
	auto [w, h] = window.getSize();
	textPopUpMessages.setPosition(w * 0.5f, h * 0.1f);
	window.draw(textPopUpMessages);

	window.setView(curView); // restore current view

	window.display();
}

sf::Vector2f Application::getWindowSizeF()
{
	auto [x, y] = window.getSize();
	return sf::Vector2f{ static_cast<float>(x), static_cast<float>(y) };
}

void Application::tryReadImg(const wchar_t* path)
{
	const sf::String s(path);
	std::unique_ptr<sf::Image> currentImage;
	MakeUniquePtr(currentImage);
	if (currentImage->loadFromFile(s))
	{
		viewerBase = std::make_unique<ImgViewer>(this, std::move(currentImage));
		editor.reset();
	}
	else
		addMessage(s + " erro na importação");
}

void Application::openImg()
{
	if (const auto path = TryOpenFile(L"Choose Image to Load", SFML_SUPPORTED_IMAGE_FORMATS_SIZE, SFML_SUPPORTED_IMAGE_FORMATS))
		tryReadImg(path);
}

void Application::updateZoomFacAndViewSizeFromZoomLevel(sf::View& view)
{
	zoomFac = powf(2, static_cast<float>(zoomLevel) / N_SCROLS_TO_DOUBLE);
	view.setSize(sf::Vector2f(window.getSize()) * zoomFac);
}

void Application::drawCircles(const std::vector<vec2d>& coords, const std::deque<float>& r, const sf::Color& color)
{
	sf::CircleShape circle;
	circle.setFillColor(color);
	for (size_t i = 0, lim = coords.size(); i != lim; i++)
	{
		PrepareCircleRadius(circle, r[i]);
		circle.setPosition(coords[i]);
		window.draw(circle);
	}
}

void Application::addMessage(const sf::String& msg)
{
	warnings.emplace_back(msg, globalClock.getElapsedTime());
}