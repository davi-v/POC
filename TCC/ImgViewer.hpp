#pragma once
#include "ViewerBase.hpp"
#include "Simulator2D.hpp"
#include "Previewer.hpp"

class Application;

class ImgViewer : public ViewerBase
{
public:
	sf::Color borderColor;
	std::unique_ptr<Previewer> previewer;
	std::unique_ptr<sf::Image> currentImage;
	sf::Vector2f imageOffsetF;
	sf::Image colorMapImage;
	sf::Texture currentImageTexture, currentFilterTexture, colorMapTexture;
	sf::Sprite currentImageSprite, currentFilterSprite, colorMapSprite;
	float
		imgWF,
		imgHF;
	void pollEvent(const sf::Event& e) override;
	ImgViewer& accessImgViewer() override;

	void startHexagonPreview();
	void startVoronoiPreview();
	sf::Rect<double> getBB() const;

	//enum class AllocationType
	//{
	//	EditMode,
	//	RobotBased
	//} allocationType = AllocationType::EditMode;
	//Simulator2D sim;


	ImgViewer(Application& app, std::unique_ptr<sf::Image>&& currentImage);
	void onImgChange();

	// updates filter and color map
	void recreateFilterTexture();

	void updateImg(const sf::Image& img);
};