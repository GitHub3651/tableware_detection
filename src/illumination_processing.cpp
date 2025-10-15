/*
 * 光照处理模块实现 - 处理反光、光照不均等问题
 */

#include "illumination_processing.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>

using namespace cv;
using namespace std;

namespace IlluminationProcessing
{

    // ==================== Retinex算法实现 ====================

    Mat singleScaleRetinex(const Mat &inputImage, double sigma)
    {
        // 检查输入图像是否为空
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for SSR" << endl;
            return Mat();
        }

        // 第1步：将8位整数图像(0-255)转换为浮点数图像(0.0-1.0)
        // 这样便于进行数学运算，避免整数运算的精度损失
        Mat floatImg;
        inputImage.convertTo(floatImg, CV_32FC3, 1.0 / 255.0);

        // 第2步：分离BGR三个颜色通道，因为Retinex需要对每个通道单独处理
        // B通道、G通道、R通道分别代表蓝、绿、红三种颜色分量
        vector<Mat> channels;
        split(floatImg, channels);

        // 第3步：对每个颜色通道(B、G、R)分别进行Retinex处理
        for (int i = 0; i < channels.size(); i++)
        {
            Mat channel = channels[i]; // 当前处理的通道(B/G/R中的一个)

            // 第4步：添加极小值避免对数运算中的log(0)错误
            // 因为log(0)是未定义的，会导致程序崩溃
            channel += 1e-6;

            // 第5步：使用高斯滤波估算当前通道的照明分量L(x,y)
            // sigma参数控制滤波范围：sigma越大，估算的照明越平滑
            // 这里模拟人眼对大范围光照变化的感知
            Mat illumination;
            GaussianBlur(channel, illumination, Size(0, 0), sigma);
            illumination += 1e-6; // 同样避免log(0)问题

            // 第6步：应用Retinex核心公式 log(R) = log(I) - log(L)
            // 这是Retinex的数学基础：反射分量 = 原图像 - 照明分量
            // 在对数域进行减法等价于在原域进行除法：R = I / L
            Mat logChannel, logIllumination;
            log(channel, logChannel);           // 计算原图像的对数：log(I)
            log(illumination, logIllumination); // 计算照明的对数：log(L)

            // 执行减法得到反射分量：log(R) = log(I) - log(L)
            // 这样就去除了照明的影响，保留了物体本身的颜色特征
            channels[i] = logChannel - logIllumination;
        }

        // 第7步：将处理后的B、G、R三个通道重新合并成彩色图像
        Mat result;
        merge(channels, result);

        // 第8步：更激烈的后处理 - 强化去反光效果
        // 8.1 进一步扩大数值范围，让对比度更强烈
        result = max(result, -2.5); // 允许更暗的值：增强暗部对比度
        result = min(result, 2.5);  // 允许更亮的值：增强亮部对比度

        // 线性映射：将 [-2.5, 2.5] 范围映射到 [0, 255] 范围
        // 进一步扩大范围让对比度更激烈，反光消除效果更明显
        result = (result + 2.5) / 5.0 * 255.0;

        // 第9步：转换回8位整数格式(CV_8UC3)，这是OpenCV图像的标准格式
        result.convertTo(result, CV_8UC3);

        // 第10步：进一步减少原图混合，让Retinex效果更激烈
        // 10%使用原图(保持极少量自然度) + 90%使用处理结果(强化去反光效果)
        // 这样能更激烈地消除勺子表面的反光，让HSV检测更稳定
        Mat finalResult;
        addWeighted(inputImage, 0.10, result, 0.90, 0, finalResult);

        return finalResult; // 返回最终的去反光图像
    }

    Mat multiScaleRetinex(const Mat &inputImage,
                          const vector<double> &sigmas,
                          const vector<double> &weights)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for MSR" << endl;
            return Mat();
        }

        if (sigmas.size() != weights.size())
        {
            cerr << "Error: Sigmas and weights size mismatch" << endl;
            return Mat();
        }

        // 累积结果
        Mat result = Mat::zeros(inputImage.size(), CV_32FC3);

        // 对每个尺度进行SSR处理并加权
        for (size_t i = 0; i < sigmas.size(); i++)
        {
            Mat ssrResult = singleScaleRetinex(inputImage, sigmas[i]);

            Mat floatSSR;
            ssrResult.convertTo(floatSSR, CV_32FC3);

            result += weights[i] * floatSSR;
        }

        // 转换回8位图像
        Mat output;
        result.convertTo(output, CV_8UC3);

        return output;
    }

    Mat multiScaleRetinexCR(const Mat &inputImage,
                            const vector<double> &sigmas,
                            const vector<double> &weights,
                            double alpha, double beta)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for MSRCR" << endl;
            return Mat();
        }

        // 先进行MSR处理
        Mat msrResult = multiScaleRetinex(inputImage, sigmas, weights);

        // 转换为浮点型进行颜色恢复
        Mat floatMSR, floatInput;
        msrResult.convertTo(floatMSR, CV_32FC3, 1.0 / 255.0);
        inputImage.convertTo(floatInput, CV_32FC3, 1.0 / 255.0);

        // 计算颜色恢复系数
        Mat grayInput;
        cvtColor(floatInput, grayInput, cv::COLOR_BGR2GRAY);

        Mat colorRecovery;
        log(alpha * grayInput + 1e-6, colorRecovery);
        colorRecovery = beta * colorRecovery;

        // 将单通道扩展为三通道
        vector<Mat> cChannels(3);
        cChannels[0] = colorRecovery;
        cChannels[1] = colorRecovery;
        cChannels[2] = colorRecovery;

        Mat colorRecovery3C;
        merge(cChannels, colorRecovery3C);

        // 应用颜色恢复
        Mat result;
        multiply(floatMSR, colorRecovery3C, result);

        // 归一化并转换回8位
        normalize(result, result, 0, 255, NORM_MINMAX);
        result.convertTo(result, CV_8UC3);

        return result;
    }

    // ==================== 反光抑制算法实现 ====================

    Mat gaussianBackgroundSubtraction(const Mat &inputImage,
                                      int kernelSize,
                                      double gain)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for background subtraction" << endl;
            return Mat();
        }

        // 确保kernelSize为奇数
        if (kernelSize % 2 == 0)
            kernelSize++;

        // 高斯滤波获得背景
        Mat background;
        GaussianBlur(inputImage, background, Size(kernelSize, kernelSize), 0);

        // 转换为浮点型进行减法
        Mat floatInput, floatBackground;
        inputImage.convertTo(floatInput, CV_32FC3);
        background.convertTo(floatBackground, CV_32FC3);

        // 背景减法
        Mat result = floatInput - floatBackground;
        result = gain * result + 128; // 增益和偏移

        // 限制范围并转换回8位
        result = max(result, 0);
        result = min(result, 255);
        result.convertTo(result, CV_8UC3);

        return result;
    }

    Mat morphologicalTopHat(const Mat &inputImage, int kernelSize)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for top-hat" << endl;
            return Mat();
        }

        // 创建椭圆形结构元素
        Mat kernel = getStructuringElement(MORPH_ELLIPSE,
                                           Size(kernelSize, kernelSize));

        // 顶帽运算
        Mat topHat;
        morphologyEx(inputImage, topHat, MORPH_TOPHAT, kernel);

        // 从原图中减去顶帽结果
        Mat result;
        subtract(inputImage, topHat, result);

        return result;
    }

    Mat adaptiveReflectionRemoval(const Mat &inputImage, int threshold)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for reflection removal" << endl;
            return Mat();
        }

        Mat result = inputImage.clone();

        // 转换为灰度图检测反光区域
        Mat gray;
        cvtColor(inputImage, gray, cv::COLOR_BGR2GRAY);

        // 创建反光掩码
        Mat reflectionMask;
        cv::threshold(gray, reflectionMask, threshold, 255, THRESH_BINARY);

        // 对反光区域进行修复（使用周围像素的中值）
        Mat filtered;
        medianBlur(inputImage, filtered, 5);

        // 只在反光区域应用滤波结果
        filtered.copyTo(result, reflectionMask);

        return result;
    }

    // ==================== 光照均匀化算法实现 ====================

    Mat enhancedCLAHE(const Mat &inputImage,
                      double clipLimit,
                      Size tileGridSize)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for CLAHE" << endl;
            return Mat();
        }

        // 转换到LAB色彩空间
        Mat lab;
        cvtColor(inputImage, lab, cv::COLOR_BGR2Lab);

        // 分离L通道
        vector<Mat> labChannels;
        split(lab, labChannels);

        // 对L通道应用CLAHE
        Ptr<CLAHE> clahe = createCLAHE(clipLimit, tileGridSize);
        clahe->apply(labChannels[0], labChannels[0]);

        // 合并通道并转换回BGR
        Mat result;
        merge(labChannels, lab);
        cvtColor(lab, result, cv::COLOR_Lab2BGR);

        return result;
    }

    Mat homomorphicFiltering(const Mat &inputImage,
                             double gammaH,
                             double gammaL,
                             double cutoff)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for homomorphic filtering" << endl;
            return Mat();
        }

        // 转换为灰度和浮点型
        Mat gray, floatImg;
        cvtColor(inputImage, gray, cv::COLOR_BGR2GRAY);
        gray.convertTo(floatImg, CV_32F);

        // 取对数
        floatImg += 1.0; // 避免log(0)
        log(floatImg, floatImg);

        // 进行DFT
        Mat dftImg;
        dft(floatImg, dftImg, DFT_COMPLEX_OUTPUT);

        // 创建高通滤波器
        Mat filter = Mat::zeros(floatImg.size(), CV_32F);
        Point center(filter.cols / 2, filter.rows / 2);

        for (int i = 0; i < filter.rows; i++)
        {
            for (int j = 0; j < filter.cols; j++)
            {
                double d = sqrt(pow(i - center.y, 2) + pow(j - center.x, 2));
                double h = (gammaH - gammaL) * (1 - exp(-pow(d, 2) / (2 * pow(cutoff, 2)))) + gammaL;
                filter.at<float>(i, j) = h;
            }
        }

        // 应用滤波器
        vector<Mat> dftChannels;
        split(dftImg, dftChannels);
        multiply(dftChannels[0], filter, dftChannels[0]);
        multiply(dftChannels[1], filter, dftChannels[1]);
        merge(dftChannels, dftImg);

        // 逆DFT
        Mat result;
        dft(dftImg, result, DFT_INVERSE | DFT_REAL_OUTPUT);

        // 取指数
        exp(result, result);
        result -= 1.0;

        // 归一化并转换
        normalize(result, result, 0, 255, NORM_MINMAX);
        result.convertTo(result, CV_8U);

        // 转换回彩色图像
        Mat colorResult;
        cvtColor(result, colorResult, cv::COLOR_GRAY2BGR);

        return colorResult;
    }

    // ==================== 综合处理方案实现 ====================

    Mat removeReflectionForTableware(const Mat &inputImage,
                                     const string &method)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for tableware processing" << endl;
            return Mat();
        }

        cout << "Applying " << method << " for reflection removal..." << endl;

        if (method == "SSR")
        {
            return singleScaleRetinex(inputImage, 80.0);
        }
        else if (method == "MSR")
        {
            return multiScaleRetinex(inputImage);
        }
        else if (method == "MSRCR")
        {
            return multiScaleRetinexCR(inputImage);
        }
        else if (method == "GAUSS")
        {
            return gaussianBackgroundSubtraction(inputImage);
        }
        else if (method == "COMBINED")
        {
            // 激烈的组合处理：使用更小的sigma值，强化反光去除效果
            // sigma=30 能更好地处理局部反光，对勺子表面反光效果更好
            return singleScaleRetinex(inputImage, 30.0);
        }
        else
        {
            cerr << "Unknown method: " << method << endl;
            return inputImage.clone();
        }
    }

    vector<Mat> batchProcessForComparison(const Mat &inputImage)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for batch processing" << endl;
            return vector<Mat>();
        }

        vector<Mat> results;
        vector<string> methods = {"ORIGINAL", "SSR", "MSR", "MSRCR", "GAUSS", "COMBINED"};

        // 原图
        results.push_back(inputImage.clone());

        // 各种处理方法
        results.push_back(singleScaleRetinex(inputImage, 80.0));
        results.push_back(multiScaleRetinex(inputImage));
        results.push_back(multiScaleRetinexCR(inputImage));
        results.push_back(gaussianBackgroundSubtraction(inputImage));
        results.push_back(removeReflectionForTableware(inputImage, "COMBINED"));

        cout << "Batch processing completed. Generated " << results.size() << " results." << endl;

        return results;
    }

} // namespace IlluminationProcessing