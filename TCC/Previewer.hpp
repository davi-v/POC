#pragma once

class ViewerBase;

class Previewer
{
protected:
	ViewerBase& viewerBase;
	virtual void drawUIImpl() = 0;

public:
	Previewer(ViewerBase& viewerBase);
	virtual ~Previewer() = default;

	bool drawUI();
	virtual void draw() = 0;

	virtual void pollEvent(const sf::Event& e) = 0;
	virtual void onImgChangeImpl() = 0;
	virtual const char* getTitle() = 0;
};