#include "Pch.hpp"
#include "Utilities.hpp"
#include "FileExplorer.hpp"
#include "vec2.hpp"

#include "Application.hpp"
#include "OpenCVSFML.hpp"

#include <Windows.h>

static const std::string SCREENSHOTS_DIR = "screenshots/";
static constexpr auto SCREENSHOT_EXT = ".png";

#define USE_CUSTOM_FONT_IN_IMGUI

void run()
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
		//sf::Style::Fullscreen
#endif
	};
	window.setVerticalSyncEnabled(true);

#ifdef USE_CUSTOM_FONT_IN_IMGUI
	ImGui::SFML::Init(window, false);
	auto& IO = ImGui::GetIO();
	IO.Fonts->AddFontFromFileTTF("segoeui.ttf", 28);
	ImGui::SFML::UpdateFontTexture();
#else
	ImGui::SFML::Init(window);
#endif

	ImPlot::CreateContext();
	Application app{ window };
	sf::Event event;
	sf::Clock clk;
	while (window.isOpen())
	{
		while (window.pollEvent(event))
		{
			app.pollEvent(event);
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
					const auto path = GetUniqueNameWithCurrentTime(SCREENSHOTS_DIR, SCREENSHOT_EXT);
					img.saveToFile(path);
					app.addMessage(path);
				}
				break;
				}
			}
			break;
			}
		}
		ImGui::SFML::Update(window, clk.restart());
		app.draw();
	}
	ImPlot::DestroyContext();
	ImGui::SFML::Shutdown();
}

int main()
{
	NFD_Init();
	try
	{
		run();
	}
	catch (const std::exception& e)
	{
		MessageBoxA(0, "Um erro irrecuperável ocorreu, como falta de memória", e.what(), MB_OK);
	}
	NFD_Quit();
}