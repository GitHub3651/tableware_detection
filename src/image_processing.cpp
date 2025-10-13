/*
 * 图像处理模块 - 包含所有图像处理相关函数
 */

#include "image_processing.h"
#include "config_constants.h"
#include <iostream>

using namespace cv;
using namespace std;

// 全局变量定义
bool g_isOK = false;

// HSV二值分割函数 - 基于H和S通道条件
Mat createHueBinaryMask(const Mat &hsvImage)
{
    // 分离HSV通道
    vector<Mat> hsvChannels;
    split(hsvImage, hsvChannels);
    Mat hChannel = hsvChannels[0]; // H通道 (Hue) 0-179
    Mat sChannel = hsvChannels[1]; // S通道 (Saturation) 0-255

    Mat binaryMask = Mat::zeros(hsvImage.size(), CV_8UC1);

    // 新的二值化条件：s>150且h值在8-20之间的置为1（白色255），其他置为0（黑色）
    for (int i = 0; i < hChannel.rows; i++)
    {
        for (int j = 0; j < hChannel.cols; j++)
        {
            uchar hValue = hChannel.at<uchar>(i, j);
            uchar sValue = sChannel.at<uchar>(i, j);

            // 如果满足条件：S > 阈值 且 H 在指定范围内
            if (sValue > Config::HSV_S_MIN && hValue >= Config::HSV_H_MIN && hValue <= Config::HSV_H_MAX)
            {
                binaryMask.at<uchar>(i, j) = 255; // 满足条件的区域置为白色
            }
            else
            {
                binaryMask.at<uchar>(i, j) = 0; // 其他区域置为黑色
            }
        }
    }

    return binaryMask;
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

// 找到白色像素最多的列并绘制
Mat findAndDrawMaxColumn(const Mat &binaryImage, const Mat &originalImage)
{
    Mat result = originalImage.clone();

    // 统计每列的白色像素数量
    vector<int> columnCounts(binaryImage.cols, 0);
    for (int col = 0; col < binaryImage.cols; col++)
    {
        for (int row = 0; row < binaryImage.rows; row++)
        {
            if (binaryImage.at<uchar>(row, col) == 255)
            {
                columnCounts[col]++;
            }
        }
    }

    // 找到白色像素最多的列
    int maxCol = 0;
    int maxCount = columnCounts[0];
    for (int col = 1; col < binaryImage.cols; col++)
    {
        if (columnCounts[col] > maxCount)
        {
            maxCount = columnCounts[col];
            maxCol = col;
        }
    }

    // 在原图上画出这一列（绿色线）
    line(result, Point(maxCol, 0), Point(maxCol, binaryImage.rows - 1), Scalar(0, 255, 0), 3);

    // OK/NG判定逻辑
    int totalPixelsInColumn = binaryImage.rows;                 // 列像素总量
    double pixelRatio = (double)maxCount / totalPixelsInColumn; // 像素比例

    int centerCol = binaryImage.cols / 2;                                                 // 图像中心列坐标
    int colDistance = maxCol - centerCol;                                                 // 最大列与中心的距离（保留符号：负值表示偏左，正值表示偏右）
    double minAllowedDistance = Config::POSITION_OFFSET_MIN_THRESHOLD * binaryImage.cols; // 允许的最小偏移
    double maxAllowedDistance = Config::POSITION_OFFSET_MAX_THRESHOLD * binaryImage.cols; // 允许的最大偏移

    // 判定条件：
    // 1. 最大像素列的1数量 >= 像素密度阈值 * 列像素总量
    // 2. 最小偏移阈值 <= 最大像素列坐标 - 中间坐标 <= 最大偏移阈值 (偏移必须在设定范围内)
    bool condition1 = pixelRatio >= Config::PIXEL_RATIO_THRESHOLD;
    bool condition2 = (colDistance >= minAllowedDistance) && (colDistance <= maxAllowedDistance); // 偏移条件：在设定范围内
    bool isOK = condition1 && condition2;

    // 保存结果到全局变量
    g_isOK = isOK;

    // 计算偏移百分比
    double offsetPercentage = (double)colDistance / binaryImage.cols * 100;

    // Print detailed information to console
    cout << "Max white pixel column: " << maxCol << " (Count: " << maxCount << " pixels)" << endl;
    cout << "Pixel ratio: " << (pixelRatio * 100) << "% (Required: >= " << (Config::PIXEL_RATIO_THRESHOLD * 100) << "%)" << endl;
    cout << "Center offset: " << offsetPercentage << "% (Range: " << (Config::POSITION_OFFSET_MIN_THRESHOLD * 100) << "% ~ " << (Config::POSITION_OFFSET_MAX_THRESHOLD * 100) << "%)" << endl;
    cout << "Condition 1 (Pixel ratio >= " << (Config::PIXEL_RATIO_THRESHOLD * 100) << "%): " << (condition1 ? "PASS" : "FAIL") << endl;
    cout << "Condition 2 (Offset in range " << (Config::POSITION_OFFSET_MIN_THRESHOLD * 100) << "% ~ " << (Config::POSITION_OFFSET_MAX_THRESHOLD * 100) << "%): " << (condition2 ? "PASS" : "FAIL") << endl;
    cout << "Final Result: " << (isOK ? "OK" : "NG") << endl;

    return result;
}