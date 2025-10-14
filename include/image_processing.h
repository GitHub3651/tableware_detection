#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

// 图像缩放函数
Mat resizeImageByScale(const Mat &originalImage, double scale = 0.1);

// 模糊处理函数
Mat applyBlurProcessing(const Mat &inputImage);

// 多HSV二值分割函数 (直接接受BGR图像)
Mat createHueBinaryMask(const Mat &bgrImage);

// LAB色彩空间二值分割函数
Mat createLABBinaryMask(const Mat &bgrImage);

// 形态学处理函数
Mat performMorphological(const Mat &binaryImage);

// 轮廓填充函数
Mat fillContours(const Mat &binaryImage);

// 连通域填充函数
Mat fillConnectedComponents(const Mat &binaryImage);

// 基于全图面积百分比的连通域过滤函数
Mat filterConnectedComponentsByPercent(const Mat &binaryImage, double minPercentage = 2.0);

// CLAHE对比度限制自适应直方图均衡
Mat enhanceContrast_CLAHE(const Mat &inputImage);

#endif // IMAGE_PROCESSING_H