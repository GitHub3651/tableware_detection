#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

// 图像缩放函数
Mat resizeImageByScale(const Mat &originalImage, double scale = 0.1);

// HSV二值分割函数
Mat createHueBinaryMask(const Mat &hsvImage);

// 形态学处理函数
Mat performMorphological(const Mat &binaryImage);

// 轮廓填充函数
Mat fillContours(const Mat &binaryImage);

// 连通域填充函数
Mat fillConnectedComponents(const Mat &binaryImage);

// 基于全图面积百分比的连通域过滤函数
Mat filterConnectedComponentsByPercent(const Mat &binaryImage, double minPercentage = 2.0);

// 找到白色像素最多的列并绘制
Mat findAndDrawMaxColumn(const Mat &binaryImage, const Mat &originalImage);

// 全局变量声明
extern bool g_isOK;

#endif // IMAGE_PROCESSING_H