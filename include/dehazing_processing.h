/*
 * 去雾处理模块 - 处理透明材质和大气散射影响
 *
 * 功能：
 * 1. 暗通道先验去雾算法
 * 2. 透明材质增强处理
 * 3. 大气光估算与透射率计算
 * 4. 针对餐具袋的局部去雾优化
 */

#ifndef DEHAZING_PROCESSING_H
#define DEHAZING_PROCESSING_H

#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

namespace DehazingProcessing
{

    // ==================== 基础去雾算法 ====================

    /**
     * 暗通道先验去雾算法 (Dark Channel Prior)
     * 基于何恺明的经典算法，去除图像中的雾霾效应
     * @param inputImage 输入的有雾图像
     * @param windowSize 暗通道计算窗口大小，默认15
     * @param omega 天空区域保留参数，默认0.95
     * @param t0 透射率下界，默认0.1
     * @return 去雾后的清晰图像
     */
    Mat darkChannelPrior(const Mat &inputImage,
                         int windowSize = 15,
                         double omega = 0.95,
                         double t0 = 0.1);

    /**
     * 计算图像的暗通道
     * 暗通道是图像去雾的核心概念
     * @param image 输入图像
     * @param windowSize 最小值滤波窗口大小
     * @return 暗通道图像（单通道灰度图）
     */
    Mat calculateDarkChannel(const Mat &image, int windowSize = 15);

    /**
     * 估算大气光值
     * 在暗通道中选择最亮的0.1%像素对应的原图区域
     * @param image 原始图像
     * @param darkChannel 暗通道图像
     * @return 估算的大气光值 [B, G, R]
     */
    Scalar estimateAtmosphericLight(const Mat &image, const Mat &darkChannel);

    /**
     * 计算透射率图
     * 透射率表示光线穿过介质的能力
     * @param image 原始图像
     * @param atmosphericLight 大气光值
     * @param omega 保留系数，控制去雾强度
     * @param windowSize 暗通道窗口大小
     * @return 透射率图（0-1范围的浮点图）
     */
    Mat calculateTransmission(const Mat &image,
                              const Scalar &atmosphericLight,
                              double omega = 0.95,
                              int windowSize = 15);

    // ==================== 透明材质专用算法 ====================

    /**
     * 透明材质增强处理
     * 专门针对透明餐具袋等材质造成的图像质量下降
     * @param inputImage 输入图像
     * @param enhanceStrength 增强强度，1.0-3.0，默认2.0
     * @return 增强后的图像
     */
    Mat transparentMaterialEnhance(const Mat &inputImage,
                                   double enhanceStrength = 2.0);

    /**
     * 局部对比度自适应增强
     * 检测并增强低对比度区域（通常是透明材质覆盖区域）
     * @param inputImage 输入图像
     * @param blockSize 局部区域大小，默认32
     * @param threshold 低对比度阈值，默认20
     * @return 增强后的图像
     */
    Mat adaptiveContrastEnhance(const Mat &inputImage,
                                int blockSize = 32,
                                double threshold = 20.0);

    // ==================== 优化与后处理 ====================

    /**
     * 导向滤波优化透射率
     * 使用导向滤波平滑透射率图，避免halo效应
     * @param transmission 原始透射率图
     * @param guide 导向图像（通常是输入的灰度图）
     * @param radius 滤波半径，默认60
     * @param epsilon 正则化参数，默认0.0001
     * @return 优化后的透射率图
     */
    Mat guidedFilterTransmission(const Mat &transmission,
                                 const Mat &guide,
                                 int radius = 60,
                                 double epsilon = 0.0001);

    /**
     * 颜色校正与饱和度恢复
     * 去雾后进行颜色校正，恢复自然的饱和度
     * @param image 去雾后的图像
     * @param saturationBoost 饱和度提升系数，默认1.2
     * @return 颜色校正后的图像
     */
    Mat colorCorrection(const Mat &image, double saturationBoost = 1.2);

    // ==================== 综合处理方案 ====================

    /**
     * 餐具袋去雾综合处理
     * 针对餐具检测场景优化的去雾流程
     * @param inputImage 输入图像
     * @param method 处理方法 ("DARK_CHANNEL", "TRANSPARENT", "COMBINED")
     * @return 处理后的图像
     */
    Mat removeHazeForTableware(const Mat &inputImage,
                               const string &method = "COMBINED");

    /**
     * 批量测试不同去雾参数
     * 用于参数调优和效果对比
     * @param inputImage 输入图像
     * @return 包含不同参数处理结果的图像向量
     */
    vector<Mat> batchDehazingTest(const Mat &inputImage);

    // ==================== 辅助函数 ====================

    /**
     * 最小值滤波
     * 在指定窗口内计算最小值，用于暗通道计算
     * @param src 源图像
     * @param dst 目标图像
     * @param windowSize 窗口大小
     */
    void minFilter(const Mat &src, Mat &dst, int windowSize);

    /**
     * 图像质量评估
     * 评估去雾效果的客观指标
     * @param original 原始图像
     * @param processed 处理后图像
     * @return 质量分数（越高越好）
     */
    double evaluateImageQuality(const Mat &original, const Mat &processed);

} // namespace DehazingProcessing

#endif // DEHAZING_PROCESSING_H