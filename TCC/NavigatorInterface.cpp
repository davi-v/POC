#include "Pch.hpp"
#include "NavigatorInterface.hpp"

#include "Utilities.hpp"

static constexpr float DEFAULT_TIME_STEP = 1.f / DEFAULT_TICKS_PER_SECOND;

NavigatorInterface::NavigatorInterface() :
	drawDestinationLines(false),
	running(false),
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
