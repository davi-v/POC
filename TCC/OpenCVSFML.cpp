#include "Pch.hpp"
#include "OpenCVSFML.hpp"

cv::Mat SFMLImageToOpenCVNative(const sf::Image& image)
{
	auto s = image.getSize();
	cv::Mat mat(cv::Size(s.x, s.y), CV_8UC4, (void*)image.getPixelsPtr(), cv::Mat::AUTO_STEP);
	cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGR);
	return mat;
}

void UpdateTextureFromCVRGBA(sf::Texture& texture, const cv::Mat& mRGBA, sf::Image& image)
{
	image.create(mRGBA.cols, mRGBA.rows, mRGBA.ptr());
	if (!texture.loadFromImage(image))
		LOG("SFML texture failed to load from image\n");
}

void UpdateTextureFromCVRGBA(sf::Texture& texture, const cv::Mat& mRGBA)
{
	sf::Image image;
	UpdateTextureFromCVRGBA(texture, mRGBA, image);
}