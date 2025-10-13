/*
 * 显示模块 - 包含所有图像显示和用户交互相关函数
 */

#include "display.h"
#include "image_processing.h"
#include "config_constants.h"
#include <iostream>

using namespace cv;
using namespace std;

// 全局变量定义
Mat g_hsvImage;
Mat g_originalImage;

// 创建扩展画布的辅助函数
Mat createExtendedCanvas(const Mat &image, const string &text1, const string &text2, const string &text3)
{
    const int infoHeight = 60;
    Mat canvas = Mat::zeros(image.rows + infoHeight, image.cols, CV_8UC3);
    image.copyTo(canvas(Rect(0, 0, image.cols, image.rows)));
    rectangle(canvas, Point(0, image.rows), Point(image.cols, image.rows + infoHeight), Scalar(40, 40, 40), -1);
    putText(canvas, text1, Point(10, image.rows + 15), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 255, 255), 1);
    putText(canvas, text2, Point(10, image.rows + 30), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 255, 255), 1);
    putText(canvas, text3, Point(10, image.rows + 45), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 255, 255), 1);
    return canvas;
}

// 鼠标回调函数 - 显示HSV值
void onMouse(int event, int x, int y, int flags, void *userdata)
{
    if (x >= 0 && y >= 0 && x < g_hsvImage.cols && y < g_hsvImage.rows)
    {
        // 获取像素值
        Vec3b hsvPixel = g_hsvImage.at<Vec3b>(y, x);
        Vec3b bgrPixel = g_originalImage.at<Vec3b>(y, x);

        // 创建带十字标记的图像
        Mat displayImg = g_originalImage.clone();
        line(displayImg, Point(x - 10, y), Point(x + 10, y), Scalar(0, 255, 0), 2);
        line(displayImg, Point(x, y - 10), Point(x, y + 10), Scalar(0, 255, 0), 2);

        // 生成信息文字
        string pos = "Pos:(" + to_string(x) + "," + to_string(y) + ")";
        string hsv = "HSV:(" + to_string(hsvPixel[0]) + "," + to_string(hsvPixel[1]) + "," + to_string(hsvPixel[2]) + ")";
        string rgb = "RGB:(" + to_string(bgrPixel[2]) + "," + to_string(bgrPixel[1]) + "," + to_string(bgrPixel[0]) +
                     ") Gray:" + to_string((int)(0.299 * bgrPixel[2] + 0.587 * bgrPixel[1] + 0.114 * bgrPixel[0]));

        // 显示扩展画布
        imshow("HSV Color Analysis - Move mouse to see values", createExtendedCanvas(displayImg, pos, hsv, rgb));
    }
}

// 创建subplot效果的函数（保持图像宽高比）
Mat createSubplotDisplay(const vector<Mat> &images,
                         const vector<string> &titles,
                         int rows, int cols)
{
    // 计算每个子图区域的最大大小
    int maxSubWidth = 300;
    int maxSubHeight = 250;
    int margin = 20;
    int titleHeight = 30;

    // 创建大画布
    Mat canvas = Mat::zeros(rows * (maxSubHeight + margin + titleHeight) + margin,
                            cols * (maxSubWidth + margin) + margin,
                            CV_8UC3);
    canvas.setTo(Scalar(50, 50, 50)); // 深灰色背景

    for (int i = 0; i < images.size() && i < titles.size(); i++)
    {
        int row = i / cols;
        int col = i % cols;

        // 计算子图区域的中心位置
        int centerX = col * (maxSubWidth + margin) + margin + maxSubWidth / 2;
        int centerY = row * (maxSubHeight + margin + titleHeight) + margin + titleHeight + maxSubHeight / 2;

        // 计算保持宽高比的缩放尺寸
        Mat currentImg = images[i];
        double aspectRatio = (double)currentImg.cols / currentImg.rows;

        int newWidth, newHeight;
        if (aspectRatio > (double)maxSubWidth / maxSubHeight)
        {
            // 图像较宽，以宽度为准
            newWidth = maxSubWidth;
            newHeight = (int)(maxSubWidth / aspectRatio);
        }
        else
        {
            // 图像较高，以高度为准
            newHeight = maxSubHeight;
            newWidth = (int)(maxSubHeight * aspectRatio);
        }

        // 缩放图像
        Mat resized;
        resize(currentImg, resized, Size(newWidth, newHeight));

        // 如果是灰度图，转换为彩色
        if (resized.channels() == 1)
        {
            cvtColor(resized, resized, COLOR_GRAY2BGR);
        }

        // 计算实际绘制位置（居中）
        int drawX = centerX - newWidth / 2;
        int drawY = centerY - newHeight / 2;

        // 确保不超出画布边界
        drawX = max(0, min(drawX, canvas.cols - newWidth));
        drawY = max(0, min(drawY, canvas.rows - newHeight));

        // 复制到画布
        resized.copyTo(canvas(Rect(drawX, drawY, newWidth, newHeight)));

        // 添加标题（在子图区域的上方）
        int titleX = col * (maxSubWidth + margin) + margin;
        int titleY = row * (maxSubHeight + margin + titleHeight) + margin + 20;
        putText(canvas, titles[i], Point(titleX, titleY),
                FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);

        // 添加图像尺寸信息
        string sizeInfo = to_string(currentImg.cols) + "x" + to_string(currentImg.rows);
        putText(canvas, sizeInfo, Point(titleX, titleY + 15),
                FONT_HERSHEY_SIMPLEX, 0.4, Scalar(200, 200, 200), 1);
    }

    return canvas;
}

// 显示交互式颜色分析窗口
void showColorAnalysis(const Mat &hsvImage, const Mat &originalImage)
{
    const string windowName = "HSV Color Analysis - Move mouse to see values";

    // 设置全局变量
    g_hsvImage = hsvImage.clone();
    g_originalImage = originalImage.clone();

    // 创建窗口并设置位置
    namedWindow(windowName, WINDOW_AUTOSIZE);
    moveWindow(windowName, 300, 200);
    setMouseCallback(windowName, onMouse, nullptr);

    // 显示初始画布
    imshow(windowName, createExtendedCanvas(originalImage, "", "Move mouse over image to see HSV values", ""));

    // 等待用户交互
    waitKey(0);
    destroyWindow(windowName);
}