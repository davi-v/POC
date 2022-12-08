#pragma once
#include "ViewerBase.hpp"

class Previewer
{
protected:
	virtual void drawUIImpl() = 0;
	ViewerBase& viewerBase;
public:

	Previewer(ViewerBase& viewerBase);
	virtual ~Previewer() = default;

	bool drawUI();

	virtual const sf::Image& accessColorMapImg();
	virtual void draw() = 0;
	virtual void pollEvent(const sf::Event& e) = 0;
	virtual void onImgChangeImpl() = 0;
	virtual const char* getTitle() = 0;
};