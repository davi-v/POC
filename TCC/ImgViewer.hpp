#pragma once
#include "ViewerBase.hpp"

class ImgViewer : public ViewerBase
{
	std::unique_ptr<sf::Image> img;
public:
	ImgViewer(Application* app, std::unique_ptr<sf::Image>&& img);
};