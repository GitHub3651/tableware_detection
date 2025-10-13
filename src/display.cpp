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
Mat g_displayImage;

// 鼠标回调函数 - 显示HSV值
void onMouse(int event, int x, int y, int flags, void *userdata)
{
    if (x >= 0 && y >= 0 && x < g_hsvImage.cols && y < g_hsvImage.rows)
    {
        // 获取HSV值
        Vec3b hsvPixel = g_hsvImage.at<Vec3b>(y, x);
        int h = hsvPixel[0]; // H通道 (0-179)
        int s = hsvPixel[1]; // S通道 (0-255)
        int v = hsvPixel[2]; // V通道 (0-255)

        // 获取原始BGR值
        Vec3b bgrPixel = g_originalImage.at<Vec3b>(y, x);
        int b = bgrPixel[0];
        int g = bgrPixel[1];
        int r = bgrPixel[2];

        // 创建显示图像的副本
        g_displayImage = g_originalImage.clone();

        // 在图像上绘制十字标记
        line(g_displayImage, Point(x - 10, y), Point(x + 10, y), Scalar(0, 255, 0), 2);
        line(g_displayImage, Point(x, y - 10), Point(x, y + 10), Scalar(0, 255, 0), 2);

        // 计算灰度值
        int gray = (int)(0.299 * r + 0.587 * g + 0.114 * b);

        // 创建扩展画布：图像 + 底部信息区域
        int infoHeight = 60;
        Mat canvas = Mat::zeros(g_displayImage.rows + infoHeight, g_displayImage.cols, CV_8UC3);

        // 复制图像到画布上部
        g_displayImage.copyTo(canvas(Rect(0, 0, g_displayImage.cols, g_displayImage.rows)));

        // 信息区域背景
        rectangle(canvas, Point(0, g_displayImage.rows), Point(g_displayImage.cols, g_displayImage.rows + infoHeight), Scalar(40, 40, 40), -1);

        // 分行显示文字
        putText(canvas, "Pos:(" + to_string(x) + "," + to_string(y) + ")", Point(10, g_displayImage.rows + 15), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 255, 255), 1);
        putText(canvas, "HSV:(" + to_string(h) + "," + to_string(s) + "," + to_string(v) + ")", Point(10, g_displayImage.rows + 30), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 255, 255), 1);
        putText(canvas, "RGB:(" + to_string(r) + "," + to_string(g) + "," + to_string(b) + ") Gray:" + to_string(gray), Point(10, g_displayImage.rows + 45), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 255, 255), 1);

        // 更新显示
        imshow("HSV Color Analysis - Move mouse to see values", canvas);
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
    // 设置全局变量
    g_hsvImage = hsvImage.clone();
    g_originalImage = originalImage.clone();
    g_displayImage = originalImage.clone();

    // 创建颜色分析窗口
    namedWindow("HSV Color Analysis - Move mouse to see values", WINDOW_AUTOSIZE);

    // 设置窗口位置到屏幕中心区域，避免窗口消失在左上角
    moveWindow("HSV Color Analysis - Move mouse to see values", 300, 200);

    // 设置鼠标回调函数
    setMouseCallback("HSV Color Analysis - Move mouse to see values", onMouse, nullptr);

    // 创建初始扩展画布
    int infoHeight = 60;
    Mat initialCanvas = Mat::zeros(g_originalImage.rows + infoHeight, g_originalImage.cols, CV_8UC3);
    g_originalImage.copyTo(initialCanvas(Rect(0, 0, g_originalImage.cols, g_originalImage.rows)));
    rectangle(initialCanvas, Point(0, g_originalImage.rows), Point(g_originalImage.cols, g_originalImage.rows + infoHeight), Scalar(40, 40, 40), -1);
    putText(initialCanvas, "Move mouse over image to see HSV values", Point(10, g_originalImage.rows + 30), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(200, 200, 200), 1);

    // 初始显示
    imshow("HSV Color Analysis - Move mouse to see values", initialCanvas);

    // 等待用户交互
    waitKey(0);

    // 关闭颜色分析窗口
    destroyWindow("HSV Color Analysis - Move mouse to see values");
}