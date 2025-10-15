/*
 * HSV均值化处理模块 - 小窗口HSV值均值化
 */

#ifndef HSV_NORMALIZATION_H
#define HSV_NORMALIZATION_H

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

namespace HSVNormalization
{

    /**
     * 全图HSV均值化处理
     * 在指定窗口大小内对HSV进行均值化，抵消反光和阻挡影响
     * @param inputImage 输入BGR图像
     * @param windowSize 均值化窗口大小
     * @return 处理后的BGR图像
     */
    Mat globalHSVNormalization(const Mat &inputImage, int windowSize = 5);

} // namespace HSVNormalization

#endif // HSV_NORMALIZATION_H