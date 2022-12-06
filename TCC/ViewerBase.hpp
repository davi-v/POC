#pragma once
#include "vec2.hpp"

class Previewer;
class Application;
class ViewerBase
{
	static constexpr float PX_FILTER_SCALE = 8;
	void onImgChangeBase(sf::Image* imgPtr);

public:
	sf::Rect<double> getBB() const;
	sf::Color borderColor;
	std::unique_ptr<Previewer> previewer;
	sf::Image* currentImage;
	sf::Vector2f imageOffsetF;
	sf::Texture
		currentImageTexture,
		currentImageATexture,
		currentFilterTexture,
		colorMapTexture;
	sf::Sprite
		currentImageSprite,
		currentImageASprite,
		currentFilterSprite,
		colorMapSprite;
	sf::CircleShape circle;
	enum class RenderType
	{
		None,
		RGBA,
		A,
		ColorMap,
		RightColor
	} renderType = RenderType::RGBA;

	Application* app;
	float
		radius;
	bool
		highlightPixelHovered,
		drawBorder;
	sf::Color circleColorSFML;

	ViewerBase(Application* app, sf::Image* imgPtr);
	virtual ~ViewerBase() = default;

	bool drawUI();
	void draw();
	sf::Color getRightColor(float x, float y);
	sf::Color getRightColor(const vec2f& c);
	sf::Color getColor(float x, float y);
	sf::Color getColor(const vec2f& c);

	float
		imgWF,
		imgHF;

	void pollEvent(const sf::Event& e);

	void drawPreviewerSelectorMenu();
	void onImgChange(sf::Image* imgPtr);

	virtual void pollEventExtra(const sf::Event& e);
	virtual void drawUIExtra();
};