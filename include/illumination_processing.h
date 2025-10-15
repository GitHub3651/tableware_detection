/*
 * 光照处理模块 - 处理反光、光照不均等问题
 *
 * 功能：
 * 1. Retinex算法 - 光照不变性处理
 * 2. 反光抑制 - 高斯背景减法
 * 3. 光照均匀化 - 直方图均衡化增强
 * 4. 多尺度光照处理
 */

#ifndef ILLUMINATION_PROCESSING_H
#define ILLUMINATION_PROCESSING_H

#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

namespace IlluminationProcessing
{

    // ==================== Retinex算法系列 ====================

    /**
     * 单尺度Retinex算法 (SSR)
     * @param inputImage 输入RGB图像
     * @param sigma 高斯核标准差，控制处理尺度
     * @return 处理后的图像
     */
    Mat singleScaleRetinex(const Mat &inputImage, double sigma = 80.0);

    /**
     * 多尺度Retinex算法 (MSR)
     * @param inputImage 输入RGB图像
     * @param sigmas 多个sigma值的向量
     * @param weights 对应的权重向量
     * @return 处理后的图像
     */
    Mat multiScaleRetinex(const Mat &inputImage,
                          const vector<double> &sigmas = {15.0, 80.0, 250.0},
                          const vector<double> &weights = {1.0 / 3, 1.0 / 3, 1.0 / 3});

    /**
     * 带颜色恢复的多尺度Retinex算法 (MSRCR)
     * @param inputImage 输入RGB图像
     * @param sigmas 多个sigma值
     * @param weights sigma权重
     * @param alpha 颜色恢复系数alpha
     * @param beta 颜色恢复系数beta
     * @return 处理后的图像
     */
    Mat multiScaleRetinexCR(const Mat &inputImage,
                            const vector<double> &sigmas = {15.0, 80.0, 250.0},
                            const vector<double> &weights = {1.0 / 3, 1.0 / 3, 1.0 / 3},
                            double alpha = 125.0, double beta = 46.0);

    // ==================== 反光抑制算法 ====================

    /**
     * 高斯背景减法去反光
     * @param inputImage 输入图像
     * @param kernelSize 高斯核大小
     * @param gain 增益控制
     * @return 去反光后的图像
     */
    Mat gaussianBackgroundSubtraction(const Mat &inputImage,
                                      int kernelSize = 51,
                                      double gain = 1.2);

    /**
     * 形态学顶帽变换去反光
     * @param inputImage 输入图像
     * @param kernelSize 形态学核大小
     * @return 处理后的图像
     */
    Mat morphologicalTopHat(const Mat &inputImage, int kernelSize = 15);

    /**
     * 自适应反光检测与修复
     * @param inputImage 输入图像
     * @param threshold 反光检测阈值
     * @return 修复后的图像
     */
    Mat adaptiveReflectionRemoval(const Mat &inputImage, int threshold = 200);

    // ==================== 光照均匀化算法 ====================

    /**
     * 增强版CLAHE处理（针对反光优化）
     * @param inputImage 输入图像
     * @param clipLimit 对比度限制
     * @param tileGridSize 网格大小
     * @return 处理后的图像
     */
    Mat enhancedCLAHE(const Mat &inputImage,
                      double clipLimit = 2.0,
                      Size tileGridSize = Size(4, 4));

    /**
     * 同态滤波光照校正
     * @param inputImage 输入图像
     * @param gammaH 高频增益
     * @param gammaL 低频增益
     * @param cutoff 截止频率
     * @return 处理后的图像
     */
    Mat homomorphicFiltering(const Mat &inputImage,
                             double gammaH = 2.0,
                             double gammaL = 0.5,
                             double cutoff = 30.0);

    // ==================== 综合处理方案 ====================

    /**
     * 餐具反光综合处理方案
     * 结合多种算法的最优处理流程
     * @param inputImage 输入图像
     * @param method 处理方法选择 (SSR, MSR, MSRCR, GAUSS, COMBINED)
     * @return 处理后的图像
     */
    Mat removeReflectionForTableware(const Mat &inputImage,
                                     const string &method = "COMBINED");

    /**
     * 批量测试不同参数效果
     * 用于参数调优和效果对比
     * @param inputImage 输入图像
     * @return 包含不同处理结果的图像向量
     */
    vector<Mat> batchProcessForComparison(const Mat &inputImage);

} // namespace IlluminationProcessing

#endif // ILLUMINATION_PROCESSING_H