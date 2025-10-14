#ifndef ROI_SELECTOR_H
#define ROI_SELECTOR_H

#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

// ROI选择器类
class ROISelector
{
private:
    Mat originalImage;
    Mat displayImage;
    vector<Point> roiPoints;
    bool drawing;
    bool finished;
    string windowName;

    // 静态成员用于鼠标回调
    static ROISelector *instance;

public:
    ROISelector(const Mat &image, const string &winName = "ROI Selector");
    ~ROISelector();

    // 显示ROI选择窗口并获取用户绘制的ROI
    Rect selectROI();

    // 根据ROI裁剪图像
    Mat cropImageWithROI(const Mat &image, const Rect &roi);

    // 鼠标回调函数
    static void onMouse(int event, int x, int y, int flags, void *userdata);

private:
    // 绘制ROI区域
    void drawROI();

    // 重置ROI选择
    void resetROI();

    // 计算ROI边界矩形
    Rect calculateBoundingRect();
};

// 便捷函数：直接选择ROI并返回裁剪后的图像
Mat selectAndCropROI(const Mat &inputImage, const string &windowTitle = "Select ROI Area");

#endif // ROI_SELECTOR_H