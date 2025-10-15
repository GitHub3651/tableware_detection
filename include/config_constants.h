#ifndef CONFIG_CONSTANTS_H
#define CONFIG_CONSTANTS_H

namespace Config
{
    // 形态学处理参数 - 只使用膨胀操作
    constexpr int MORPH_DILATE_KERNEL_SIZE = 4; // 膨胀核大小 (连接断裂区域)

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
        {10, 25, 50, 250, 0, 255}, // 木色筷子: 橙黄色调，中高饱和度，较高亮度
        {0, 180, 0, 90, 0, 60},    // 黑色勺子: 全色调，任意饱和度，低亮度
        // {8, 25, 30, 150, 80, 200}    // 深木色备用: 更严格的木色范围
        // 添加更多物体只需在这里加一行，数量会自动计算
    };

    constexpr int RANGE_COUNT = sizeof(HSV_RANGES) / sizeof(HSV_RANGES[0]); // 自动计算HSV范围数量
}

// 模板匹配配置
namespace TemplateMatchConfig
{
    // 模板文件夹路径
    const std::string TEMPLATE_FOLDER = "image_samples/2/muban";

    // 旋转角度配置（用于处理筷子等细长物体的轻微倾斜）
    const double ROTATION_MIN = -6.0; // 最小旋转角度（度）
    const double ROTATION_MAX = 6.0;  // 最大旋转角度（度）
    const double ROTATION_STEP = 3.0; // 角度步长（度）
    // 实际测试角度：-15, -10, -5, 0, 5, 10, 15（7个角度）

    // 每个模板的阈值（按文件名顺序：1.jpg, 2.jpg, ...）
    // 使用像素相似度匹配（TM_SQDIFF_NORMED），范围 [0, 1]，1.0=完全相同
    // 建议阈值：0.85-0.95
    const std::vector<double> THRESHOLDS = {0.85, 0.85};
}

#endif // CONFIG_CONSTANTS_H