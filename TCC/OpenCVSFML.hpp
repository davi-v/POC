#pragma once

// opencv native is BGR
// opencv operates with pointers to the sf::Image
cv::Mat SFMLImageToOpenCVNative(sf::Image& image);
cv::Mat SFMLImageToOpenCVBGRA(sf::Image& image);

void CVBGRToCVRGBA(cv::Mat& m);
void CVBGRAToCVRGBA(cv::Mat& m);
void SFMLImageToSFMLTexture(const sf::Image& img, sf::Texture& t);
void CVRGBAToSFMLTextureAndImage(const cv::Mat& m, sf::Texture& texture, sf::Image& image);
void CVRGBAToSFMLImage(const cv::Mat& m, sf::Image& image);
void CVRGBAToSFMLTexture(const cv::Mat& m, sf::Texture& texture);
void CVBGRToSFMLTexture(cv::Mat& m, sf::Texture& texture);
void CVBGRToSFMLTextureAndImage(cv::Mat& m, sf::Texture& texture, sf::Image& image);
void CVBGRToSFMLImage(cv::Mat& m, sf::Image& image);
void CVBGRAToSFMLImage(cv::Mat& m, sf::Image& image);
void CVBGRAToSFMLTextureAndImage(cv::Mat& m, sf::Texture& texture, sf::Image& image);