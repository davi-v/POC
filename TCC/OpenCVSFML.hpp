#pragma once

cv::Mat SFMLImageToOpenCVNative(const sf::Image& image);

void UpdateTextureFromCVRGBA(sf::Texture& texture, const cv::Mat& mRGBA, sf::Image& image);

void UpdateTextureFromCVRGBA(sf::Texture& texture, const cv::Mat& mRGBA);