#include "Pch.hpp"
#include "ImgViewer.hpp"

#include "Application.hpp"
#include "OpenCVSFML.hpp"
#include "Utilities.hpp"

#include "PreviewHex.hpp"
#include "PreviewVoronoi.hpp"

ImgViewer::ImgViewer(Application& app, std::unique_ptr<sf::Image>&& c) :
	ViewerBase(app),
	currentImage(std::move(c)),
	borderColor(sf::Color::Red)
{
	currentFilterSprite.setScale(PX_FILTER_SCALE, PX_FILTER_SCALE); // pixel side of filter preview
	onImgChange();
}

void ImgViewer::recreateFilterTexture()
{
	auto side = static_cast<unsigned>(ceil(radius * 2));
	//auto side = static_cast<unsigned>(ceil(r));

	sf::Image currentFilterImage;
	currentFilterImage.create(side, side, sf::Color::White);

	std::vector<float> filterValues(square(static_cast<size_t>(side)));
	if (side == 1)
		filterValues.front() = 1.f;
	else
	{
		currentFilterImage.create(side, side);
		auto mid = vec2f{ static_cast<float>(side) } *.5f;

		auto std = static_cast<float>(side) * .5f / 3.f;
		auto var = std * std;
		auto G = [&](float x) // gaussiana com média 0
		{
			static constexpr auto e = std::numbers::e_v<float>;
			return 1 / (std * sqrtf(2 * PI)) * powf(e, -.5f * square(x) / var);
		};

		auto M = G(0);

		for (unsigned x = 0; x != side; x++)
			for (unsigned y = 0; y != side; y++)
			{
				auto off = vec2f(x + .5f, y + .5f) - mid;


				float strength = G(length(off));
				filterValues[x * side + y] = strength;

				sf::Color color{
				static_cast<sf::Uint8>(strength / M * 255),
				static_cast<sf::Uint8>(strength / M * 255),
				static_cast<sf::Uint8>(strength / M * 255)
				};
				currentFilterImage.setPixel(x, y, color);
			}

		auto sum = std::accumulate(filterValues.begin(), filterValues.end(), 0.f);
		for (auto& f : filterValues)
			f /= sum;
	}
	currentFilterTexture.loadFromImage(currentFilterImage);
	currentFilterSprite.setTexture(currentFilterTexture, true);

	colorMapImage = *currentImage; // hard bug to find. Opencv does things with pointers
	auto mat = SFMLImageToOpenCVBGRA(colorMapImage);
	cv::Mat kernel(side, side, CV_32F, (void*)filterValues.data());
	cv::filter2D(mat, mat, -1, kernel);
	CVBGRAToSFMLTextureAndImage(mat, colorMapTexture, colorMapImage);
	colorMapSprite.setTexture(colorMapTexture, true);
}

void ImgViewer::onImgChange()
{
	currentImageTexture.loadFromImage(*currentImage);
	currentImageSprite.setTexture(currentImageTexture, true);

	const auto& img = accessImgViewer();
	const auto [imgW, imgH] = img.currentImage->getSize();
	imgWF = (float)imgW,
	imgHF = (float)imgH;
	auto halfX = imgWF * .5f;
	auto halfY = imgHF * .5f;
	const auto newImgCenter = app.getWindowSizeF() * .5f;
	auto centralizeSprite = [&](auto&& s)
	{
		s.setOrigin(halfX, halfY);
		s.setPosition(newImgCenter);
	};
	centralizeSprite(currentImageSprite);
	centralizeSprite(colorMapSprite);

	sf::Vector2f prevImgCenter(
		imgWF * .5f,
		imgHF * .5f
	);
	imageOffsetF = newImgCenter - prevImgCenter;

	recreateFilterTexture();

	if (previewer)
		previewer->onImgChangeImpl();
}

void ImgViewer::updateImg(const sf::Image& img)
{
	*currentImage = img;
	onImgChange();
}

sf::Rect<double> ImgViewer::getBB() const
{
	auto& off = imageOffsetF;
	auto [w, h] = currentImage->getSize();
	return { off.x, off.y, (double)w, (double)h };
}

void ImgViewer::pollEvent(const sf::Event& e)
{
	if (previewer)
		previewer->pollEvent(e);
}

ImgViewer& ImgViewer::accessImgViewer()
{
	return *this;
}

void ImgViewer::startVoronoiPreview()
{
	previewer = std::make_unique<PreviewVoronoi>(*this);
}

void ImgViewer::startHexagonPreview()
{
	previewer = std::make_unique<PreviewHex>(*this);
}