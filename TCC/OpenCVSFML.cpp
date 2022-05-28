#include "Pch.hpp"
#include "OpenCVSFML.hpp"

static auto CVConversion(cv::Mat& m, cv::ColorConversionCodes code)
{
	cv::cvtColor(m, m, code);
}

static auto SFMLImageToOpenCVCommon(sf::Image& image, cv::ColorConversionCodes code)
{
	auto s = image.getSize();
	cv::Mat mat(cv::Size(s.x, s.y), CV_8UC4, (void*)image.getPixelsPtr(), cv::Mat::AUTO_STEP);
	cv::cvtColor(mat, mat, code);
	return mat;
}

cv::Mat SFMLImageToOpenCVNative(sf::Image& image)
{
	return SFMLImageToOpenCVCommon(image, cv::COLOR_RGBA2BGR);
}

cv::Mat SFMLImageToOpenCVBGRA(sf::Image& image)
{
	return SFMLImageToOpenCVCommon(image, cv::COLOR_RGBA2BGRA);
}

void SFMLImageToSFMLTexture(const sf::Image& img, sf::Texture& t)
{
	if (!t.loadFromImage(img))
		LOG("Weird error: SFML texture failed to load from it's own image\n");
}

void CVRGBAToSFMLTextureAndImage(const cv::Mat& m, sf::Texture& texture, sf::Image& image)
{
	CVRGBAToSFMLImage(m, image);
	SFMLImageToSFMLTexture(image, texture);
}

void CVRGBAToSFMLImage(const cv::Mat& m, sf::Image& image)
{
	image.create(m.cols, m.rows, m.ptr());
}

void CVRGBAToSFMLTexture(const cv::Mat& m, sf::Texture& texture)
{
	sf::Image image;
	CVRGBAToSFMLTextureAndImage(m, texture, image);
}

void CVBGRToSFMLTexture(cv::Mat& m, sf::Texture& texture)
{
	sf::Image image;
	CVBGRToSFMLTextureAndImage(m, texture, image);
}

void CVBGRToCVRGBA(cv::Mat& m)
{
	CVConversion(m, cv::COLOR_BGR2RGBA);
}

void CVBGRAToCVRGBA(cv::Mat& m)
{
	CVConversion(m, cv::COLOR_BGRA2RGBA);
}

void CVBGRToSFMLTextureAndImage(cv::Mat& m, sf::Texture& texture, sf::Image& image)
{
	CVBGRToCVRGBA(m);
	CVRGBAToSFMLTextureAndImage(m, texture, image);
}

void CVBGRToSFMLImage(cv::Mat& m, sf::Image& image)
{
	CVBGRToCVRGBA(m);
	CVRGBAToSFMLImage(m, image);
}

void CVBGRAToSFMLImage(cv::Mat& m, sf::Image& image)
{
	CVBGRAToCVRGBA(m);
	CVRGBAToSFMLImage(m, image);
}

void CVBGRAToSFMLTextureAndImage(cv::Mat& m, sf::Texture& texture, sf::Image& image)
{
	CVBGRAToCVRGBA(m);
	CVRGBAToSFMLTextureAndImage(m, texture, image);
}
