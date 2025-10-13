/*
 * 图像处理模块 - 包含所有图像处理相关函数
 */

#include "image_processing.h"
#include "config_constants.h"
#include <iostream>

using namespace cv;
using namespace std;

// 图像缩放函数 - 按指定比例缩放图像
Mat resizeImageByScale(const Mat &originalImage, double scale)
{
    if (originalImage.empty())
    {
        cerr << "Error: Empty input image for resizing" << endl;
        return Mat();
    }

    // 计算新的尺寸
    int newWidth = static_cast<int>(originalImage.cols * scale);
    int newHeight = static_cast<int>(originalImage.rows * scale);

    // 确保尺寸至少为1
    newWidth = max(1, newWidth);
    newHeight = max(1, newHeight);

    Mat resizedImage;
    resize(originalImage, resizedImage, Size(newWidth, newHeight), 0, 0, INTER_LINEAR);

    cout << "Image resized from " << originalImage.cols << "x" << originalImage.rows
         << " to " << resizedImage.cols << "x" << resizedImage.rows
         << " (scale: " << scale << ")" << endl;

    return resizedImage;
}

// 方案A：预编译优化的多HSV二值分割函数
Mat createHueBinaryMask(const Mat &hsvImage)
{
    Mat result = Mat::zeros(hsvImage.size(), CV_8UC1);

    // 编译器会完全展开这个循环，实现最佳性能
    for (int i = 0; i < Config::RANGE_COUNT; ++i)
    {
        Mat mask;
        inRange(hsvImage,
                Scalar(Config::HSV_RANGES[i][0], Config::HSV_RANGES[i][2], Config::HSV_RANGES[i][4]),
                Scalar(Config::HSV_RANGES[i][1], Config::HSV_RANGES[i][3], Config::HSV_RANGES[i][5]),
                mask);
        result |= mask;
    }

    return result;
}

// 形态学处理函数
Mat performMorphological(const Mat &binaryImage)
{
    Mat result;

    // 创建椭圆形结构元素
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(Config::MORPH_OPEN_KERNEL_SIZE, Config::MORPH_OPEN_KERNEL_SIZE));

    // 先进行开运算去除噪点
    morphologyEx(binaryImage, result, MORPH_OPEN, kernel);

    // 再进行闭运算填充空洞
    kernel = getStructuringElement(MORPH_ELLIPSE, Size(Config::MORPH_CLOSE_KERNEL_SIZE, Config::MORPH_CLOSE_KERNEL_SIZE));
    morphologyEx(result, result, MORPH_CLOSE, kernel);

    return result;
}

// 轮廓填充函数
Mat fillContours(const Mat &binaryImage)
{
    Mat result = binaryImage.clone();

    // 查找轮廓
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(result, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // 填充所有外部轮廓
    for (size_t i = 0; i < contours.size(); i++)
    {
        // 只填充面积大于阈值的轮廓，过滤掉小的噪点
        double area = contourArea(contours[i]);
        if (area > Config::MIN_CONTOUR_AREA)
        {
            fillPoly(result, contours[i], Scalar(255));
        }
    }

    return result;
}

// 连通域填充函数
Mat fillConnectedComponents(const Mat &binaryImage)
{
    Mat result = binaryImage.clone();
    Mat labels, stats, centroids;

    // 连通域分析
    int numComponents = connectedComponentsWithStats(binaryImage, labels, stats, centroids);

    // 遍历每个连通域
    for (int i = 1; i < numComponents; i++) // 跳过背景 (标签0)
    {
        int area = stats.at<int>(i, CC_STAT_AREA);

        // 只保留面积大于阈值的连通域
        if (area > Config::MIN_CONNECTED_AREA)
        {
            // 填充该连通域
            Mat mask = (labels == i);
            result.setTo(255, mask);
        }
        else
        {
            // 删除小的连通域
            Mat mask = (labels == i);
            result.setTo(0, mask);
        }
    }

    return result;
}

// 基于全图面积百分比的连通域过滤函数
Mat filterConnectedComponentsByPercent(const Mat &binaryImage, double minPercentage)
{
    Mat result = binaryImage.clone();
    Mat labels, stats, centroids;

    // 连通域分析
    int numComponents = connectedComponentsWithStats(binaryImage, labels, stats, centroids);

    // 计算全图面积和最小面积阈值
    int totalArea = binaryImage.rows * binaryImage.cols;
    int minArea = totalArea * (minPercentage / 100.0);

    // 遍历每个连通域
    for (int i = 1; i < numComponents; i++) // 跳过背景 (标签0)
    {
        int area = stats.at<int>(i, CC_STAT_AREA);

        // 只保留面积大于阈值的连通域
        if (area >= minArea)
        {
            // 填充该连通域
            Mat mask = (labels == i);
            result.setTo(255, mask);
        }
        else
        {
            // 删除小的连通域
            Mat mask = (labels == i);
            result.setTo(0, mask);
        }
    }

    return result;
}
