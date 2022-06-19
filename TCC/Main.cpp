#include "Pch.hpp"
#include "Utilities.hpp"
#include "FileExplorer.hpp"
#include "vec2.hpp"

#include "Application.hpp"
#include "OpenCVSFML.hpp"

static const std::string SCREENSHOTS_DIR = "screenshots/";
static constexpr auto SCREENSHOT_EXT = ".png";

#define USE_CUSTOM_FONT_IN_IMGUI

int main()
{
	sf::RenderWindow window{
#ifdef _DEBUG
		sf::VideoMode{ 1600, 900 }
		//sf::VideoMode{ 1280, 720 }
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

#ifdef USE_CUSTOM_FONT_IN_IMGUI
	ImGui::SFML::Init(window, false);
	auto& IO = ImGui::GetIO();
	IO.Fonts->AddFontFromFileTTF("segoeui.ttf", 28);
	ImGui::SFML::UpdateFontTexture();
#else
	ImGui::SFML::Init(window);
#endif

	ImPlot::CreateContext();

	Application application{ window };

	sf::Event event;
	sf::Clock c;
	while (window.isOpen())
	{
		while (window.pollEvent(event))
		{
			application.pollEvent(event);
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
					}
				}
				break;
			}
		}
		
		ImGui::SFML::Update(window, c.restart());

		application.draw();
	}
	ImPlot::DestroyContext();
	ImGui::SFML::Shutdown();
}