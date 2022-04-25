#include "Pch.hpp"
#include "Simulator2D.hpp"
#include "Utilities.hpp"

static const std::string SCREENSHOTS_DIR = "screenshots/";
static constexpr auto SCREENSHOT_EXT = ".png";

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
	Simulator2D simulator{ window };
	//for (int i = 0; i != 10; i++)
	//	for (int j = 0; j != 10; j++)
	//		simulator.addAgent({
	//			{i * 40., j * 40.},
	//			0.2,
	//			sf::Color{ 36 + rand() % 220u, 36 + rand() % 220u, 36 + rand() % 220u}
	//			});

	sf::Event event;
	while (window.isOpen())
	{
		while (window.pollEvent(event))
		{
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
					case sf::Keyboard::F2:
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

			simulator.pollEvent(event);
		}

		window.clear();

		simulator.tickAndDraw();

		window.display();
	}
}