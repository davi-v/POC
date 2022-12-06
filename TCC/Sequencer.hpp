#pragma once
#include "ViewerBase.hpp"

class Application;

typedef std::deque<std::unique_ptr<sf::Image>> SequenceImg;

class Sequencer : public ViewerBase
{
public:

	
	bool canPressPrev();
	bool canPressNext();
	void updateNewImageAndTryStartNavigator(size_t off);
	void prev();
	void next();


	Sequencer(Application* app, SequenceImg&& seq);
	size_t n = 50; // number of robots
	SequenceImg seqImgs;
	size_t imgIndex = 0;

	void pollEventExtra(const sf::Event& e) override;
	void drawUIExtra() override;
};