#pragma once
#include "ViewerBase.hpp"
#include "ImgViewer.hpp"

class Application;

class Sequencer : public ViewerBase
{
public:
	float r;

	ImgViewer& accessImgViewer() override;
	void handleAutoPlayToggled();

	void calculateGoals(bool resetPositions);


	bool canPressPrev();
	bool canPressNext();
	void updateNewImageAndTryStartNavigator(size_t off);
	void prev();
	void next();

	bool autoPlay = false;

	std::unique_ptr<ImgViewer> imgViewer;
	void pollEvent(const sf::Event& e) override;
	Sequencer(Application& app, float r);
	size_t n = 50; // number of robots
	std::vector<std::unique_ptr<sf::Image>> seqImgs;
	size_t imgIndex = 0;

	void processNewImage(bool resetPositions);
};