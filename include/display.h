#ifndef DISPLAY_H
#define DISPLAY_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

using namespace cv;
using namespace std;

// 鼠标回调函数
void onMouse(int event, int x, int y, int flags, void *userdata);

// 创建subplot效果的函数
Mat createSubplotDisplay(const vector<Mat> &images,
                         const vector<string> &titles,
                         int rows, int cols);

// 显示交互式颜色分析窗口
void showColorAnalysis(const Mat &hsvImage, const Mat &originalImage);

// 全局变量声明
extern Mat g_hsvImage;
extern Mat g_originalImage;
extern Mat g_displayImage;

#endif // DISPLAY_H