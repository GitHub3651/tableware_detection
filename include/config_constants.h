#ifndef CONFIG_CONSTANTS_H
#define CONFIG_CONSTANTS_H

namespace Config
{
    // 形态学处理参数
    constexpr int MORPH_OPEN_KERNEL_SIZE = 5;   // 开运算核大小 (去噪)
    constexpr int MORPH_CLOSE_KERNEL_SIZE = 19; // 闭运算核大小 (填充)

    // 轮廓和连通域过滤参数
    constexpr double MIN_CONTOUR_AREA = 200.0;          // 最小轮廓面积阈值
    constexpr double MIN_CONNECTED_AREA = 200.0;        // 最小连通域面积阈值
    constexpr double CONNECTED_COMPONENT_PERCENT = 2.0; // 连通域面积百分比阈值（小于2%全图面积的小连体域将会被过滤）

    // 图像缩放参数
    constexpr double RESIZE_SCALE = 0.1; // 图像缩放比例 (缩放到原尺寸的10%)

    // 模糊处理参数
    constexpr int BLUR_KERNEL_SIZE = 3; // 高斯模糊核大小
    constexpr double BLUR_SIGMA = 1;    // 高斯模糊标准差
    constexpr bool ENABLE_BLUR = true;  // 是否启用模糊处理

    // 方案A：预编译优化的多HSV检测 (针对木筷子+黑勺子优化)
    constexpr int HSV_RANGES[][6] = {
        {10, 25, 50, 130, 100, 255}, // 木色筷子: 橙黄色调，中高饱和度，较高亮度
        {0, 180, 0, 80, 0, 60},      // 黑色勺子: 全色调，任意饱和度，低亮度
        // {8, 25, 30, 150, 80, 200}    // 深木色备用: 更严格的木色范围
        // 添加更多物体只需在这里加一行，数量会自动计算
    };

    constexpr int RANGE_COUNT = sizeof(HSV_RANGES) / sizeof(HSV_RANGES[0]); // 自动计算HSV范围数量
}

#endif // CONFIG_CONSTANTS_H