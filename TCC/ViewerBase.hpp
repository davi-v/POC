#pragma once
#include "vec2.hpp"

class ImgViewer;
class Application;

class ViewerBase
{
public:
	sf::CircleShape circle;
	enum class RenderType
	{
		None,
		Image,
		ColorMap,
		RightColor
	} renderType = RenderType::Image;
	bool isMarked(const sf::Color& c);
	Application& app;
	float
		circleColorF3[3],
		radius;
	bool
		highlightPixelHovered,
		drawBorder;
	sf::Color circleColorSFML;

	ViewerBase(Application& app);
	virtual ~ViewerBase() = default;

	void drawUI();
	void draw();
	sf::Color getColor(float x, float y);
	sf::Color getColor(const sf::Vector2f& c);

	virtual void pollEvent(const sf::Event& e) = 0;
	virtual ImgViewer& accessImgViewer() = 0;
};