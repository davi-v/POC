#pragma once
#include "SampleBased.hpp"

static constexpr auto RANDOM_SAMPLING = "Random Sampling";

class SampleRandom : public SampleBased
{
	void samplesExtraDrawUI() override;
	void initSamplesAndAllocation() override;
	void recalculateSamplesOnly() override;

	unsigned
		samplesSeed;

	const char* getTitle() override;

public:
	SampleRandom(ViewerBase& viewerBase);
};