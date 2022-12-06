#include "Pch.hpp"
#include "NavigatorInterface.hpp"

#include "Utilities.hpp"

static constexpr float DEFAULT_TIME_STEP = 1.f / DEFAULT_TICKS_PER_SECOND;

NavigatorInterface::NavigatorInterface() :
	trajectoryColor(sf::Color::Yellow),
	drawDestinationLines(false),
	running(false),
	tickRate(DEFAULT_TICKS_PER_SECOND),
	timeStep(DEFAULT_TIME_STEP)
{
}

void NavigatorInterface::startNavigating()
{
	running = true;
	navigatorAccTs = 0;
	navigatorLastTickTimestamp = globalClock.getElapsedTime().asSeconds();
}

void NavigatorInterface::loopUpdate()
{
	if (running)
	{
		// tick navigator logic. We are using the same thread as the rendering

		auto now = globalClock.getElapsedTime().asSeconds();
		navigatorAccTs += now - navigatorLastTickTimestamp;
		navigatorLastTickTimestamp = now;
		while (navigatorAccTs >= timeStep)
		{
			tick();
			navigatorAccTs -= timeStep;
		}
	}
}

bool NavigatorInterface::drawUI()
{
	bool opened = true;
	if (ImGui::Begin("Navigator", &opened))
	{
		if (ImGui::DragInt("Tick Rate", &tickRate, 1.0f, 1, 256, "%d", ImGuiSliderFlags_AlwaysClamp))
			updateTimeStep(1.f / tickRate);
		ImGui::SameLine(); HelpMarker("How many ticks per second to run the simulation");

		if (running)
		{
			if (ImGui::MenuItem("Pause"))
				running = false;
		}
		else
		{
			if (ImGui::MenuItem("Play"))
				startNavigating();
		}

		ImGui::Checkbox("Draw Destination Lines", &drawDestinationLines);
		
		drawUIExtra();

	}
	ImGui::End();
	return opened;
}
