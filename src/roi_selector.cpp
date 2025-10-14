/*
 * ROI选择器模块 - 交互式ROI区域选择
 */

#include "roi_selector.h"
#include "display.h"
#include <iostream>

using namespace cv;
using namespace std;

// 静态成员初始化
ROISelector *ROISelector::instance = nullptr;

// 创建带信息区域的扩展画布 (参考display.cpp的实现)
Mat createROIExtendedCanvas(const Mat &image, const string &line1, const string &line2, const string &line3)
{
    const int infoHeight = 80; // 增加高度以容纳更多信息
    Mat canvas = Mat::zeros(image.rows + infoHeight, image.cols, CV_8UC3);

    // 复制原图到画布上部
    image.copyTo(canvas(Rect(0, 0, image.cols, image.rows)));

    // 创建深灰色信息区域
    rectangle(canvas, Point(0, image.rows), Point(image.cols, image.rows + infoHeight),
              Scalar(50, 50, 50), -1);

    // 添加分隔线
    line(canvas, Point(0, image.rows), Point(image.cols, image.rows),
         Scalar(200, 200, 200), 2);

    // 显示三行信息文字
    putText(canvas, line1, Point(10, image.rows + 20),
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);
    putText(canvas, line2, Point(10, image.rows + 40),
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);
    putText(canvas, line3, Point(10, image.rows + 60),
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);

    return canvas;
}

ROISelector::ROISelector(const Mat &image, const string &winName)
    : originalImage(image.clone()), drawing(false), finished(false), windowName(winName)
{
    if (originalImage.empty())
    {
        cerr << "Error: Empty input image for ROI selection" << endl;
        return;
    }

    // 设置静态实例指针
    instance = this;

    // 创建显示图像副本
    displayImage = originalImage.clone();

    cout << "ROI Selector initialized. Window: " << windowName << endl;
}

ROISelector::~ROISelector()
{
    instance = nullptr;
}

Rect ROISelector::selectROI()
{
    if (originalImage.empty())
    {
        cerr << "Error: No image loaded for ROI selection" << endl;
        return Rect();
    }

    // 创建窗口
    namedWindow(windowName, WINDOW_AUTOSIZE);
    moveWindow(windowName, 100, 100);

    // 设置鼠标回调
    setMouseCallback(windowName, onMouse, this);

    // 显示使用说明
    cout << "\n=== ROI Selection Instructions ===" << endl;
    cout << "1. Click and drag to draw ROI rectangle" << endl;
    cout << "2. Press 'r' to reset ROI" << endl;
    cout << "3. Press 'Enter' or 'Space' to confirm ROI" << endl;
    cout << "4. Press 'Esc' to cancel" << endl;
    cout << "======================================" << endl;

    // 重置状态
    resetROI();

    // 主循环
    while (true)
    {
        // 显示图像，如果有ROI则使用扩展画布显示信息
        if (roiPoints.size() >= 2 && !drawing)
        {
            // 显示最终ROI信息
            Rect roi = calculateBoundingRect();
            string roiInfo = "ROI Size: " + to_string(roi.width) + "x" + to_string(roi.height);
            string posInfo = "Position: (" + to_string(roi.x) + "," + to_string(roi.y) + ")";
            string instructions = "Press Enter to confirm, R to reset, Esc to cancel";
            Mat extendedCanvas = createROIExtendedCanvas(displayImage, roiInfo, posInfo, instructions);
            imshow(windowName, extendedCanvas);
        }
        else
        {
            // 没有ROI时显示基本指令
            string instructions = "Click and drag to select ROI area";
            Mat extendedCanvas = createROIExtendedCanvas(displayImage, "ROI Selection", instructions, "Press Esc to cancel");
            imshow(windowName, extendedCanvas);
        }

        int key = waitKey(30) & 0xFF;

        if (key == 27)
        { // Esc键 - 取消
            cout << "ROI selection cancelled" << endl;
            destroyWindow(windowName);
            return Rect();
        }
        else if (key == 'r' || key == 'R')
        { // R键 - 重置
            resetROI();
            cout << "ROI reset" << endl;
        }
        else if (key == 13 || key == 32)
        { // Enter或空格键 - 确认
            if (roiPoints.size() >= 2)
            {
                Rect roi = calculateBoundingRect();
                cout << "ROI selected: (" << roi.x << ", " << roi.y
                     << ", " << roi.width << ", " << roi.height << ")" << endl;
                destroyWindow(windowName);
                return roi;
            }
            else
            {
                cout << "Please draw a valid ROI first" << endl;
            }
        }
    }

    return Rect();
}

Mat ROISelector::cropImageWithROI(const Mat &image, const Rect &roi)
{
    if (image.empty() || roi.area() <= 0)
    {
        cerr << "Error: Invalid image or ROI for cropping" << endl;
        return Mat();
    }

    // 确保ROI在图像范围内
    Rect validROI = roi & Rect(0, 0, image.cols, image.rows);

    if (validROI.area() <= 0)
    {
        cerr << "Error: ROI is outside image bounds" << endl;
        return Mat();
    }

    Mat croppedImage = image(validROI).clone();
    cout << "Image cropped with ROI: " << validROI.width << "x" << validROI.height << endl;

    return croppedImage;
}

void ROISelector::onMouse(int event, int x, int y, int flags, void *userdata)
{
    ROISelector *selector = static_cast<ROISelector *>(userdata);

    if (!selector)
        return;

    switch (event)
    {
    case EVENT_LBUTTONDOWN:
        selector->drawing = true;
        selector->roiPoints.clear();
        selector->roiPoints.push_back(Point(x, y));
        break;

    case EVENT_MOUSEMOVE:
        if (selector->drawing && selector->roiPoints.size() > 0)
        {
            // 更新显示图像
            selector->displayImage = selector->originalImage.clone();

            // 绘制当前矩形
            Point startPoint = selector->roiPoints[0];
            rectangle(selector->displayImage, startPoint, Point(x, y),
                      Scalar(0, 255, 0), 2);

            // 添加半透明填充
            Mat overlay = selector->displayImage.clone();
            rectangle(overlay, startPoint, Point(x, y),
                      Scalar(0, 255, 0), -1);
            addWeighted(selector->displayImage, 0.7, overlay, 0.3, 0, selector->displayImage);

            // 使用扩展画布显示鼠标移动信息
            string startInfo = "Start: (" + to_string(startPoint.x) + "," + to_string(startPoint.y) + ")";
            string currentInfo = "Current: (" + to_string(x) + "," + to_string(y) + ")";
            Mat extendedCanvas = createROIExtendedCanvas(selector->displayImage, "ROI Selection", startInfo, currentInfo);
            imshow(selector->windowName, extendedCanvas);
        }
        break;

    case EVENT_LBUTTONUP:
        if (selector->drawing)
        {
            selector->drawing = false;
            if (selector->roiPoints.size() > 0)
            {
                selector->roiPoints.push_back(Point(x, y));
                selector->drawROI();
            }
        }
        break;
    }
}

void ROISelector::drawROI()
{
    if (roiPoints.size() < 2)
        return;

    displayImage = originalImage.clone();

    // 计算矩形
    Rect roi = calculateBoundingRect();

    // 绘制ROI矩形
    rectangle(displayImage, roi, Scalar(0, 255, 0), 3);

    // 添加半透明填充
    Mat overlay = displayImage.clone();
    rectangle(overlay, roi, Scalar(0, 255, 0), -1);
    addWeighted(displayImage, 0.7, overlay, 0.3, 0, displayImage);

    // ROI信息将在扩展画布中显示，这里不再添加putText
}

void ROISelector::resetROI()
{
    roiPoints.clear();
    drawing = false;
    finished = false;
    displayImage = originalImage.clone();

    // 移除putText，初始提示将在扩展画布中显示
}

Rect ROISelector::calculateBoundingRect()
{
    if (roiPoints.size() < 2)
        return Rect();

    Point topLeft(min(roiPoints[0].x, roiPoints[1].x),
                  min(roiPoints[0].y, roiPoints[1].y));
    Point bottomRight(max(roiPoints[0].x, roiPoints[1].x),
                      max(roiPoints[0].y, roiPoints[1].y));

    return Rect(topLeft, bottomRight);
}

// 便捷函数实现
Mat selectAndCropROI(const Mat &inputImage, const string &windowTitle)
{
    if (inputImage.empty())
    {
        cerr << "Error: Empty input image for ROI selection" << endl;
        return Mat();
    }

    // 创建ROI选择器
    ROISelector selector(inputImage, windowTitle);

    // 选择ROI
    Rect roi = selector.selectROI();

    if (roi.area() > 0)
    {
        // 裁剪并返回图像
        return selector.cropImageWithROI(inputImage, roi);
    }

    // 如果没有选择ROI，返回原图
    cout << "No ROI selected, using original image" << endl;
    return inputImage.clone();
}