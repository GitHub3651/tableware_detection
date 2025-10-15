/*
 * 去雾处理模块实现 - 处理透明材质和大气散射影响
 */

#include "dehazing_processing.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>

using namespace cv;
using namespace std;

namespace DehazingProcessing
{

    // ==================== 基础去雾算法实现 ====================

    Mat darkChannelPrior(const Mat &inputImage,
                         int windowSize,
                         double omega,
                         double t0)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for dark channel prior" << endl;
            return Mat();
        }

        cout << "Applying Dark Channel Prior dehazing..." << endl;

        // 步骤1: 计算暗通道
        Mat darkChannel = calculateDarkChannel(inputImage, windowSize);

        // 步骤2: 估算大气光
        Scalar atmosphericLight = estimateAtmosphericLight(inputImage, darkChannel);
        cout << "Atmospheric light: [" << atmosphericLight[0] << ", "
             << atmosphericLight[1] << ", " << atmosphericLight[2] << "]" << endl;

        // 步骤3: 计算透射率
        Mat transmission = calculateTransmission(inputImage, atmosphericLight, omega, windowSize);

        // 步骤4: 导向滤波优化透射率
        Mat grayImage;
        cvtColor(inputImage, grayImage, cv::COLOR_BGR2GRAY);
        transmission = guidedFilterTransmission(transmission, grayImage);

        // 步骤5: 恢复清晰图像
        Mat result = Mat::zeros(inputImage.size(), CV_8UC3);

        for (int y = 0; y < inputImage.rows; y++)
        {
            for (int x = 0; x < inputImage.cols; x++)
            {
                Vec3b pixel = inputImage.at<Vec3b>(y, x);
                float t = max(transmission.at<float>(y, x), (float)t0); // 透射率下界限制

                // 大气散射模型逆运算: J(x) = (I(x) - A) / t + A
                for (int c = 0; c < 3; c++)
                {
                    double J = (pixel[c] - atmosphericLight[c]) / t + atmosphericLight[c];
                    result.at<Vec3b>(y, x)[c] = saturate_cast<uchar>(J);
                }
            }
        }

        // 步骤6: 颜色校正
        result = colorCorrection(result, 1.1);

        cout << "Dark channel prior dehazing completed" << endl;
        return result;
    }

    Mat calculateDarkChannel(const Mat &image, int windowSize)
    {
        // 确保窗口大小为奇数
        if (windowSize % 2 == 0)
            windowSize++;

        // 分离BGR通道
        vector<Mat> channels;
        split(image, channels);

        // 计算每个像素的最小通道值
        Mat minChannels = Mat::zeros(image.size(), CV_8UC1);
        for (int y = 0; y < image.rows; y++)
        {
            for (int x = 0; x < image.cols; x++)
            {
                uchar minVal = min({channels[0].at<uchar>(y, x),
                                    channels[1].at<uchar>(y, x),
                                    channels[2].at<uchar>(y, x)});
                minChannels.at<uchar>(y, x) = minVal;
            }
        }

        // 在局部窗口内计算最小值（暗通道）
        Mat darkChannel;
        minFilter(minChannels, darkChannel, windowSize);

        return darkChannel;
    }

    Scalar estimateAtmosphericLight(const Mat &image, const Mat &darkChannel)
    {
        // 找到暗通道中最亮的0.1%像素
        int totalPixels = darkChannel.rows * darkChannel.cols;
        int topPixels = max(1, (int)(totalPixels * 0.001)); // 0.1%

        // 创建像素强度和位置的对应关系
        vector<pair<uchar, Point>> pixelIntensities;
        for (int y = 0; y < darkChannel.rows; y++)
        {
            for (int x = 0; x < darkChannel.cols; x++)
            {
                uchar intensity = darkChannel.at<uchar>(y, x);
                pixelIntensities.push_back(make_pair(intensity, Point(x, y)));
            }
        }

        // 按强度降序排列
        sort(pixelIntensities.begin(), pixelIntensities.end(),
             [](const pair<uchar, Point> &a, const pair<uchar, Point> &b)
             {
                 return a.first > b.first;
             });

        // 在这些最亮像素对应的原图位置中找最亮的
        Vec3f atmosphericLight(0, 0, 0);
        float maxIntensity = 0;

        for (int i = 0; i < topPixels; i++)
        {
            Point pos = pixelIntensities[i].second;
            Vec3b pixel = image.at<Vec3b>(pos.y, pos.x);
            float intensity = pixel[0] + pixel[1] + pixel[2]; // BGR总和

            if (intensity > maxIntensity)
            {
                maxIntensity = intensity;
                atmosphericLight = Vec3f(pixel[0], pixel[1], pixel[2]);
            }
        }

        return Scalar(atmosphericLight[0], atmosphericLight[1], atmosphericLight[2]);
    }

    Mat calculateTransmission(const Mat &image,
                              const Scalar &atmosphericLight,
                              double omega,
                              int windowSize)
    {
        // 归一化原图到大气光
        Mat normalizedImage;
        image.convertTo(normalizedImage, CV_32FC3);

        for (int c = 0; c < 3; c++)
        {
            normalizedImage /= atmosphericLight[c];
        }

        // 计算归一化图像的暗通道
        Mat darkChannel = calculateDarkChannel(normalizedImage, windowSize);

        // 透射率 = 1 - omega * 暗通道
        Mat transmission = Mat::ones(darkChannel.size(), CV_32FC1);
        darkChannel.convertTo(darkChannel, CV_32FC1, 1.0 / 255.0);

        for (int y = 0; y < transmission.rows; y++)
        {
            for (int x = 0; x < transmission.cols; x++)
            {
                transmission.at<float>(y, x) = 1.0 - omega * darkChannel.at<float>(y, x);
            }
        }

        return transmission;
    }

    // ==================== 透明材质专用算法实现 ====================

    Mat transparentMaterialEnhance(const Mat &inputImage, double enhanceStrength)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for transparent material enhance" << endl;
            return Mat();
        }

        cout << "Applying transparent material enhancement..." << endl;

        // 步骤1: 检测低对比度区域（透明材质区域）
        Mat grayImage;
        cvtColor(inputImage, grayImage, cv::COLOR_BGR2GRAY);

        Mat localStd; // 局部标准差，表示对比度
        Scalar mean, stddev;
        meanStdDev(grayImage, mean, stddev);

        // 步骤2: 自适应对比度增强
        Mat enhancedImage = adaptiveContrastEnhance(inputImage, 16, 15.0);

        // 步骤3: 边缘锐化
        Mat kernel = (Mat_<float>(3, 3) << 0, -1, 0,
                      -1, 5, -1,
                      0, -1, 0);
        Mat sharpenedImage;
        filter2D(enhancedImage, sharpenedImage, -1, kernel);

        // 步骤4: 混合原图和增强结果
        Mat result;
        double alpha = min(enhanceStrength * 0.6, 0.9);
        addWeighted(inputImage, 1.0 - alpha, sharpenedImage, alpha, 0, result);

        cout << "Transparent material enhancement completed" << endl;
        return result;
    }

    Mat adaptiveContrastEnhance(const Mat &inputImage, int blockSize, double threshold)
    {
        Mat result = inputImage.clone();

        for (int y = 0; y < inputImage.rows; y += blockSize)
        {
            for (int x = 0; x < inputImage.cols; x += blockSize)
            {
                // 定义当前块的区域
                int endY = min(y + blockSize, inputImage.rows);
                int endX = min(x + blockSize, inputImage.cols);
                Rect blockRect(x, y, endX - x, endY - y);

                // 提取当前块
                Mat block = inputImage(blockRect);
                Mat grayBlock;
                cvtColor(block, grayBlock, cv::COLOR_BGR2GRAY);

                // 计算对比度（标准差）
                Scalar mean, stddev;
                meanStdDev(grayBlock, mean, stddev);

                // 如果对比度低于阈值，进行增强
                if (stddev[0] < threshold)
                {
                    Mat enhancedBlock;
                    // 使用CLAHE增强局部对比度
                    Ptr<CLAHE> clahe = createCLAHE(3.0, Size(4, 4));

                    // 转换到LAB空间，只处理L通道
                    Mat labBlock;
                    cvtColor(block, labBlock, cv::COLOR_BGR2Lab);
                    vector<Mat> labChannels;
                    split(labBlock, labChannels);

                    clahe->apply(labChannels[0], labChannels[0]);
                    merge(labChannels, labBlock);
                    cvtColor(labBlock, enhancedBlock, cv::COLOR_Lab2BGR);

                    // 更新结果图像的对应区域
                    enhancedBlock.copyTo(result(blockRect));
                }
            }
        }

        return result;
    }

    // ==================== 优化与后处理实现 ====================

    Mat guidedFilterTransmission(const Mat &transmission,
                                 const Mat &guide,
                                 int radius,
                                 double epsilon)
    {
        // 简化的导向滤波实现
        Mat result;

        // 转换为浮点型
        Mat floatTransmission, floatGuide;
        transmission.convertTo(floatTransmission, CV_32FC1);
        guide.convertTo(floatGuide, CV_32FC1, 1.0 / 255.0);

        // 使用高斯滤波作为简化的导向滤波
        GaussianBlur(floatTransmission, result, Size(2 * radius + 1, 2 * radius + 1), radius / 3.0);

        return result;
    }

    Mat colorCorrection(const Mat &image, double saturationBoost)
    {
        Mat result;

        // 转换到HSV空间进行饱和度调整
        Mat hsvImage;
        cvtColor(image, hsvImage, cv::COLOR_BGR2HSV);

        vector<Mat> hsvChannels;
        split(hsvImage, hsvChannels);

        // 增强饱和度通道
        hsvChannels[1] *= saturationBoost;
        hsvChannels[1] = min(hsvChannels[1], 255.0);

        merge(hsvChannels, hsvImage);
        cvtColor(hsvImage, result, cv::COLOR_HSV2BGR);

        return result;
    }

    // ==================== 综合处理方案实现 ====================

    Mat removeHazeForTableware(const Mat &inputImage, const string &method)
    {
        if (inputImage.empty())
        {
            cerr << "Error: Empty input image for tableware dehazing" << endl;
            return Mat();
        }

        cout << "Applying " << method << " dehazing for tableware..." << endl;

        if (method == "DARK_CHANNEL")
        {
            return darkChannelPrior(inputImage, 15, 0.9, 0.15);
        }
        else if (method == "TRANSPARENT")
        {
            return transparentMaterialEnhance(inputImage, 1.8);
        }
        else if (method == "COMBINED")
        {
            // 组合方案：先去雾再增强
            Mat step1 = darkChannelPrior(inputImage, 9, 0.85, 0.2); // 温和的去雾
            Mat step2 = transparentMaterialEnhance(step1, 1.5);     // 透明材质增强
            return step2;
        }
        else
        {
            cerr << "Unknown dehazing method: " << method << endl;
            return inputImage.clone();
        }
    }

    vector<Mat> batchDehazingTest(const Mat &inputImage)
    {
        if (inputImage.empty())
        {
            return vector<Mat>();
        }

        vector<Mat> results;

        // 原图
        results.push_back(inputImage.clone());

        // 不同参数的暗通道去雾
        results.push_back(darkChannelPrior(inputImage, 9, 0.85, 0.2));  // 温和
        results.push_back(darkChannelPrior(inputImage, 15, 0.9, 0.15)); // 标准
        results.push_back(darkChannelPrior(inputImage, 21, 0.95, 0.1)); // 强力

        // 透明材质增强
        results.push_back(transparentMaterialEnhance(inputImage, 1.5));
        results.push_back(transparentMaterialEnhance(inputImage, 2.5));

        cout << "Batch dehazing test completed. Generated " << results.size() << " results." << endl;

        return results;
    }

    // ==================== 辅助函数实现 ====================

    void minFilter(const Mat &src, Mat &dst, int windowSize)
    {
        dst = Mat::zeros(src.size(), src.type());
        int halfWindow = windowSize / 2;

        for (int y = 0; y < src.rows; y++)
        {
            for (int x = 0; x < src.cols; x++)
            {
                uchar minVal = 255;

                // 在窗口内寻找最小值
                for (int dy = -halfWindow; dy <= halfWindow; dy++)
                {
                    for (int dx = -halfWindow; dx <= halfWindow; dx++)
                    {
                        int ny = y + dy;
                        int nx = x + dx;

                        // 边界处理
                        ny = max(0, min(ny, src.rows - 1));
                        nx = max(0, min(nx, src.cols - 1));

                        minVal = min(minVal, src.at<uchar>(ny, nx));
                    }
                }

                dst.at<uchar>(y, x) = minVal;
            }
        }
    }

    double evaluateImageQuality(const Mat &original, const Mat &processed)
    {
        // 简单的质量评估：计算边缘强度和对比度改善
        Mat grayOriginal, grayProcessed;
        cvtColor(original, grayOriginal, cv::COLOR_BGR2GRAY);
        cvtColor(processed, grayProcessed, cv::COLOR_BGR2GRAY);

        // 计算边缘强度
        Mat edgesOriginal, edgesProcessed;
        Canny(grayOriginal, edgesOriginal, 50, 150);
        Canny(grayProcessed, edgesProcessed, 50, 150);

        double edgeRatioOriginal = countNonZero(edgesOriginal) / (double)(original.rows * original.cols);
        double edgeRatioProcessed = countNonZero(edgesProcessed) / (double)(processed.rows * processed.cols);

        // 边缘增强度作为质量指标
        double qualityScore = edgeRatioProcessed / (edgeRatioOriginal + 1e-6);

        return qualityScore;
    }

} // namespace DehazingProcessing