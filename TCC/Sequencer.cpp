#include "Pch.hpp"
#include "Sequencer.hpp"

#include "Previewer.hpp"

#include "FileExplorer.hpp"

bool Sequencer::canPressPrev()
{
	return imgIndex;
}

bool Sequencer::canPressNext()
{
	return seqImgs.size() && imgIndex != seqImgs.size() - 1;
}

void Sequencer::updateNewImageAndTryStartNavigator(size_t off)
{
	imgIndex += off;
	onImgChange(seqImgs[imgIndex].get());
}

void Sequencer::prev()
{
	updateNewImageAndTryStartNavigator(-1);
}

void Sequencer::next()
{
	updateNewImageAndTryStartNavigator(1);
}

Sequencer::Sequencer(Application* app, SequenceImg&& seq) :
	ViewerBase(app, seq.front().get()),
	seqImgs(std::move(seq))
{
}

void Sequencer::pollEventExtra(const sf::Event& e)
{
	switch (e.type)
	{
	case sf::Event::KeyPressed:
	{
		switch (e.key.code)
		{
		case sf::Keyboard::Left:
		{
			if (canPressPrev())
				prev();
		}
		break;
		case sf::Keyboard::Right:
		{
			if (canPressNext())
				next();
		}
		break;
		}
	}
	break;
	}
}

void Sequencer::drawUIExtra()
{
	if (ImGui::BeginMenu("Sequence"))
	{
		if (ImGui::MenuItem("Prev", "Left", nullptr, canPressPrev()))
			prev();
		if (ImGui::MenuItem("Next", "Right", nullptr, canPressNext()))
			next();

		ImGui::EndMenu();
	}
	
}
