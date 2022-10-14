#pragma once
#include "Previewer.hpp"
#include "vec2.hpp"

class PreviewPolygonize : public Previewer
{
	sf::Color trigColor, vertexColor, alphaVertColor;
	std::vector<sf::Vertex> trigs;
	double alpha;
	std::deque<sf::Vector2f> orgVertices, alphaVertices;
	size_t nComponents, trigCnt;
	bool showOrgVertices, showAlphaShape, drawTrigs, showAlphaVertices;
	struct Seg
	{
		sf::Vector2f p1, p2;
	};
	std::deque<Seg> segments;

	void recalculate();
	void pollEvent(const sf::Event& e) override;
	void drawUIImpl() override;
	void draw() override;
	void onImgChangeImpl() override;
	const char* getTitle() override;
public:
	PreviewPolygonize(ViewerBase& viewerBase);
};