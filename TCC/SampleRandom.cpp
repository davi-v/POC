#include "Pch.hpp"
#include "SampleRandom.hpp"

void SampleRandom::samplesExtraDrawUI()
{
	if (ImGui::InputScalar("Seed", ImGuiDataType_U32, &samplesSeed))
		recalculateSamplesAndAllocation();
}

const char* SampleRandom::getTitle()
{
	return RANDOM_SAMPLING;
}

SampleRandom::SampleRandom(ViewerBase& viewerBase) :
	SampleBased(viewerBase),
	samplesSeed(0)
{
	recalculateSamplesAndAllocation();
}

void SampleRandom::recalculateSamplesAndAllocation()
{
	samples.clear();
	const auto& img = *viewerBase.currentImage;
	const auto [X, Y] = img.getSize();
	std::deque<std::pair<unsigned, unsigned>> candidates;
	for (unsigned x = 0; x != X; x++)
		for (unsigned y = 0; y != Y; y++)
			if (img.getPixel(x, y).a)
				candidates.emplace_back(x, y);

	const auto nCandidates = candidates.size();

	std::mt19937 mt(samplesSeed);
	std::uniform_int_distribution<size_t> dIndex(0, nCandidates - 1);
	std::uniform_real_distribution<float> dOff(0, 1);

	samples.resize(limitedNSamples);
	auto off = viewerBase.imageOffsetF;
	for (size_t i = 0; i != limitedNSamples; i++)
	{
		auto& p = samples[i];
		const auto& ip = candidates[dIndex(mt)];
		p.x = (float)ip.first + off.x + dOff(mt);
		p.y = (float)ip.second + off.y + dOff(mt);
	}

	recalculateGreedyMethod();
}
