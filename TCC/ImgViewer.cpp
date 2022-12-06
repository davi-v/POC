#include "Pch.hpp"
#include "ImgViewer.hpp"

#include "Previewer.hpp"

ImgViewer::ImgViewer(Application* app, std::unique_ptr<sf::Image>&& img) :
	ViewerBase(app, img.get()),
	img(std::move(img))
{
}
