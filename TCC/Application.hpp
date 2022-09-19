#pragma once
#include "vec2.hpp"
#include "Simulator2D.hpp"
#include "ImgViewer.hpp"

static constexpr float PX_FILTER_SCALE = 8;
static constexpr float DEFAULT_ZOOM_LEVEL = 0;
class Application
{
	static constexpr auto N_SCROLS_TO_DOUBLE = 4;

	void createSequencer();

public:

	float
		backgroundColor[3],
		zoomFac,
		zoomLevel;

	ImgViewer& accessImgViewer();
	sf::Color getSFBackgroundColor();
	std::unique_ptr<ViewerBase> viewerBase;
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
		x(gif)
		//x(psd)
		//x(hdr)
		//x(pic)
	};
#undef x
	static constexpr auto SFML_SUPPORTED_IMAGE_FORMATS_SIZE = sizeof(SFML_SUPPORTED_IMAGE_FORMATS) / sizeof(*SFML_SUPPORTED_IMAGE_FORMATS);

	int lastPxClickedX, lastPxClickedY; // for moving the view
	void tryReadImg(const wchar_t* path);
	void controlO();
	void updateZoomFacAndViewSizeFromZoomLevel(sf::View& view);
};