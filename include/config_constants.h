#ifndef CONFIG_CONSTANTS_H
#define CONFIG_CONSTANTS_H

namespace Config
{
    // HSV检测参数 - 用于二值化分割
    constexpr int HSV_H_MIN = 8;   // 色相最小值 (最小H值)
    constexpr int HSV_H_MAX = 20;  // 色相最大值 (最大H值)
    constexpr int HSV_S_MIN = 150; // 饱和度最小值 (确保颜色纯度)

    // 形态学处理参数
    constexpr int MORPH_OPEN_KERNEL_SIZE = 5;   // 开运算核大小 (去噪)
    constexpr int MORPH_CLOSE_KERNEL_SIZE = 19; // 闭运算核大小 (填充)

    // 轮廓和连通域过滤参数
    constexpr double MIN_CONTOUR_AREA = 200.0;          // 最小轮廓面积阈值
    constexpr double MIN_CONNECTED_AREA = 200.0;        // 最小连通域面积阈值
    constexpr double CONNECTED_COMPONENT_PERCENT = 2.0; // 连通域面积百分比阈值（小于2%全图面积的小连体域将会被过滤）

    // 质量判定参数
    constexpr double PIXEL_RATIO_THRESHOLD = 0.4;          // （最多1的一列占整个宽的比例） (40%)
    constexpr double POSITION_OFFSET_MIN_THRESHOLD = 0.0;  // 最小位置偏移阈值 (0%)
    constexpr double POSITION_OFFSET_MAX_THRESHOLD = 0.15; // 最大位置偏移阈值 (15%)
}

#endif // CONFIG_CONSTANTS_H