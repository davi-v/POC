#pragma once
#include "vec2.hpp"
#include "Simulator2D.hpp"
#include "ViewerBase.hpp"

static constexpr float DEFAULT_ZOOM_LEVEL = 0;
class Application
{
	static constexpr auto N_SCROLS_TO_DOUBLE = 4;

	void tryCreateSequencer();

	sf::Font font;

	static constexpr auto MSG_DURATION = 3.f;
	sf::Text textPopUpMessages;
	std::deque<std::pair<sf::String, sf::Time>> warnings;

public:
	void addMessage(const sf::String& msg);

	sf::Color backgroundColor;

	float
		zoomFac,
		zoomLevel;

	const sf::Color& getSFBackgroundColor();
	std::unique_ptr<ViewerBase> viewerBase;
	std::unique_ptr<Simulator2D> editor;
	Application(sf::RenderWindow& window);
	void pollEvent(const sf::Event& event);
	void draw();

	sf::RenderWindow& window;
	sf::Vector2f getWindowSizeF();

	// according to SFML's documentation, it supports
	// bmp, png, tga, jpg, gif, psd, hdr and pic
	// mas nossa lógica precisa da componente alpha, então alguns formatos tipo jpg não suportam
#define x(s) L"*."#s,
	static constexpr wchar_t const* SFML_SUPPORTED_IMAGE_FORMATS[] = {
		//x(bmp)
		x(png)
		//x(tga)
		//x(jpg)
		//x(gif)
		//x(psd)
		//x(hdr)
		//x(pic)
	};
#undef x
	static constexpr auto SFML_SUPPORTED_IMAGE_FORMATS_SIZE = sizeof(SFML_SUPPORTED_IMAGE_FORMATS) / sizeof(*SFML_SUPPORTED_IMAGE_FORMATS);

	int lastPxClickedX, lastPxClickedY; // for moving the view
	void tryReadImg(const wchar_t* path);
	void openImg();
	void updateZoomFacAndViewSizeFromZoomLevel(sf::View& view);

	void drawCircles(const std::vector<vec2d>& coords, const std::deque<float>& r, const sf::Color& color);
};