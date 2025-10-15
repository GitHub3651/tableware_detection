/*
 * HSV均值化处理模块实现 - 小窗口HSV值均值化
 */

#include "hsv_normalization.h"

using namespace cv;
using namespace std;

namespace HSVNormalization
{

    Mat globalHSVNormalization(const Mat &inputImage, int windowSize)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for HSV normalization" << endl;
            return Mat();
        }

        // 确保窗口大小为奇数
        if (windowSize % 2 == 0)
        {
            windowSize++;
        }

        cout << "Applying HSV normalization with window size: " << windowSize << endl;

        // 转换到HSV色彩空间
        Mat hsvImage;
        cvtColor(inputImage, hsvImage, COLOR_BGR2HSV);

        // 对HSV图像进行均值滤波
        Mat result;
        blur(hsvImage, result, Size(windowSize, windowSize));

        // 转换回BGR色彩空间
        Mat outputImage;
        cvtColor(result, outputImage, COLOR_HSV2BGR);

        cout << "HSV normalization completed" << endl;
        return outputImage;
    }

} // namespace HSVNormalization