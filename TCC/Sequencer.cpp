#include "Pch.hpp"
#include "Sequencer.hpp"

ImgViewer& Sequencer::accessImgViewer()
{
	return *imgViewer;
}

void Sequencer::handleAutoPlayToggled()
{
	//if (autoPlay)
	//	imgViewer->sim.recreateNavigatorAndPlay();
	//else
	//	imgViewer->sim.quitNavigator();
}

void Sequencer::calculateGoals(bool resetPositions)
{
	processNewImage(resetPositions);

	//imgViewer->calculateBestHexagonSideBasedOnNRobotsAndR();
	//imgViewer->updateNewGoalsAndCalculateEdges();
	//imgViewer->sim.editModeType = Simulator2D::EditModeType::Navigation;
}

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
	//imgIndex += off;
	//
	//if (autoPlay)
	//	imgViewer->sim.quitNavigator();
	//
	//calculateGoals(false);
	//
	//if (autoPlay)
	//	imgViewer->sim.recreateNavigatorAndPlay();
}

void Sequencer::prev()
{
	updateNewImageAndTryStartNavigator(-1);
}

void Sequencer::next()
{
	updateNewImageAndTryStartNavigator(1);
}

void Sequencer::pollEvent(const sf::Event& e)
{
	if (imgViewer)
		imgViewer->pollEvent(e);

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
		case sf::Keyboard::Enter:
		{
			if (NotHoveringIMGui())
			{
				TOGGLE(autoPlay);
				handleAutoPlayToggled();
			}
		}
		break;
		}
	}
	break;
	}
}

Sequencer::Sequencer(Application& app, float r) :
	ViewerBase(app),
	r(r)
{
	static constexpr const char* HARDCODED_PATHS[] = { // por enquanto ainda não fiz a interface para imagens quaisquer
		"exemplos/i1.png",
		"exemplos/i2.png",
		"exemplos/i3.png",
		"exemplos/i4.png",
		"exemplos/i5.png",
		"exemplos/i6.png",
	};
	for (auto& p : HARDCODED_PATHS)
	{
		std::unique_ptr<sf::Image> content;
		MakeUniquePtr(content);
		auto& img = *content;
		img.loadFromFile(p);
		seqImgs.emplace_back(std::move(content));
	}
	calculateGoals(true);
}

//void Sequencer::draw()
//{
//	//if (ImGui::BeginMenu("Sequence"))
//	//{
//	//	ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);
//	//	ImGui::InputScalar("Number of Robots", ImGuiDataType_U64, &n);
//	//	if (ShowRadiusOption(r, 1000.f))
//	//	{
//	//		imgViewer->recreateFilterTexture();
//	//		imgViewer->updatedRadiusCallback(r);
//	//	}
//	//
//	//	auto hasSomething = !seqImgs.empty();
//	//	if (ImGui::MenuItem("Reset Positions", NULL, false, hasSomething))
//	//	{
//	//		calculateGoals(true);
//	//		if (autoPlay)
//	//			imgViewer->sim.recreateNavigatorAndPlay();
//	//	}
//	//
//	//	if (ImGui::MenuItem("Prev", NULL, false, canPressPrev()))
//	//		prev();
//	//	if (ImGui::MenuItem("Next", NULL, false, canPressNext()))
//	//		next();
//	//	if (ImGui::MenuItem("Auto Play", NULL, &autoPlay))
//	//		handleAutoPlayToggled();
//	//
//	//	ImGui::PopItemFlag();
//	//	ImGui::EndMenu();
//	//}
//	//
//	//if (imgViewer)
//	//	imgViewer->draw();
//}

void Sequencer::processNewImage(bool resetPositions)
{
	//auto& img = *seqImgs[imgIndex];
	//
	//if (imgViewer)
	//{
	//	auto& v = *imgViewer;
	//	v.updateImg(img);
	//	//imgViewer->updateRadiusNRobotsImg(r, n, img);
	//}
	//else
	//{
	//	std::unique_ptr<sf::Image> imgPtr;
	//	MakeUniquePtr(imgPtr, img);
	//	MakeUniquePtr(imgViewer, app, std::move(imgPtr), r, n);
	//}
	//
	//if (resetPositions)
	//{
	//	auto& sim = imgViewer->sim;
	//	sim.clearAll();
	//
	//	auto H = static_cast<size_t>(floor(sqrt(n)));
	//	auto W = n / H;
	//	auto mod = n % H;
	//	float spacing = 3.f;
	//	auto off = r * spacing;
	//	auto& [sX, sY] = imgViewer->imageOffsetF;
	//
	//	for (size_t i = 0; i != W; i++)
	//		for (size_t j = 0; j != H; j++)
	//			sim.addAgent(sX + i * off, sY + j * off);
	//
	//	auto lastRowY = H * off;
	//	for (size_t i = 0; i != mod; i++)
	//		sim.addAgent(sX + i * off, sY + lastRowY);
	//}
}
