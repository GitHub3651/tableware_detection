/*
 * 图像处理模块 - 包含所有图像处理相关函数
 */

#include "image_processing.h"
#include "config_constants.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <filesystem>
#include <algorithm>

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

// 图像缩放函数 - 按指定比例缩放图像
Mat resizeImageByScale(const Mat &originalImage, double scale)
{
    if (originalImage.empty())
    {
        cerr << "Error: Empty input image for resizing" << endl;
        return Mat();
    }

    // 计算新的尺寸
    int newWidth = static_cast<int>(originalImage.cols * scale);
    int newHeight = static_cast<int>(originalImage.rows * scale);

    // 确保尺寸至少为1
    newWidth = max(1, newWidth);
    newHeight = max(1, newHeight);

    Mat resizedImage;
    resize(originalImage, resizedImage, Size(newWidth, newHeight), 0, 0, INTER_LINEAR);

    cout << "Image resized from " << originalImage.cols << "x" << originalImage.rows
         << " to " << resizedImage.cols << "x" << resizedImage.rows
         << " (scale: " << scale << ")" << endl;

    return resizedImage;
}

// 模糊处理函数 - 减少纹理干扰和噪声
Mat applyBlurProcessing(const Mat &inputImage)
{
    if (inputImage.empty())
    {
        cerr << "Error: Empty input image for blur processing" << endl;
        return Mat();
    }

    // 如果禁用模糊处理，直接返回原图
    if (!Config::ENABLE_BLUR)
    {
        cout << "Blur processing disabled" << endl;
        return inputImage.clone();
    }

    Mat blurredImage;

    // 使用配置参数进行高斯模糊
    Size kernelSize(Config::BLUR_KERNEL_SIZE, Config::BLUR_KERNEL_SIZE);
    GaussianBlur(inputImage, blurredImage, kernelSize, Config::BLUR_SIGMA);

    // 可选：添加中值滤波进一步去除椒盐噪声
    // medianBlur(blurredImage, blurredImage, 3);

    cout << "Applied Gaussian blur processing (kernel: " << Config::BLUR_KERNEL_SIZE
         << "x" << Config::BLUR_KERNEL_SIZE << ", sigma: " << Config::BLUR_SIGMA << ")" << endl;

    return blurredImage;
}

// 方案A：预编译优化的多HSV二值分割函数 (直接接受BGR图像)
Mat createHueBinaryMask(const Mat &bgrImage)
{
    if (bgrImage.empty())
    {
        cerr << "Error: Empty input image for HSV conversion" << endl;
        return Mat();
    }

    // 转换BGR到HSV
    Mat hsvImage;
    cvtColor(bgrImage, hsvImage, COLOR_BGR2HSV);

    Mat result = Mat::zeros(hsvImage.size(), CV_8UC1);

    // 编译器会完全展开这个循环，实现最佳性能
    for (int i = 0; i < Config::RANGE_COUNT; ++i)
    {
        Mat mask;
        inRange(hsvImage,
                Scalar(Config::HSV_RANGES[i][0], Config::HSV_RANGES[i][2], Config::HSV_RANGES[i][4]),
                Scalar(Config::HSV_RANGES[i][1], Config::HSV_RANGES[i][3], Config::HSV_RANGES[i][5]),
                mask);
        result |= mask;
    }

    return result;
}

// LAB色彩空间二值分割函数 - 专门检测白色和木色物体
Mat createLABBinaryMask(const Mat &bgrImage)
{
    if (bgrImage.empty())
    {
        cerr << "Error: Empty input image for LAB conversion" << endl;
        return Mat();
    }

    // 1. 转换到LAB色彩空间
    Mat labImage;
    cvtColor(bgrImage, labImage, COLOR_BGR2Lab);

    // 2. 分离LAB通道
    vector<Mat> labChannels;
    split(labImage, labChannels);

    Mat L = labChannels[0]; // 亮度通道 (0-255)
    Mat A = labChannels[1]; // 红绿通道 (0-255, 128为中性)
    Mat B = labChannels[2]; // 黄蓝通道 (0-255, 128为中性)

    // 3. 白色物体检测
    Mat whiteMask;
    Mat L_white, A_white, B_white;

    // 白色特征: 高亮度 + 接近中性的a/b值
    inRange(L, Scalar(200), Scalar(255), L_white); // 高亮度
    inRange(A, Scalar(122), Scalar(134), A_white); // 中性红绿 (128±6)
    inRange(B, Scalar(120), Scalar(136), B_white); // 轻微偏黄可接受 (128±8)

    // 白色 = 高亮度 AND 中性A AND 近中性B
    bitwise_and(L_white, A_white, whiteMask);
    bitwise_and(whiteMask, B_white, whiteMask);

    // 4. 木色物体检测
    Mat woodMask;
    Mat L_wood, A_wood, B_wood;

    // 木色特征: 中等亮度 + 偏红 + 偏黄
    inRange(L, Scalar(100), Scalar(180), L_wood); // 中等亮度
    inRange(A, Scalar(130), Scalar(145), A_wood); // 偏红 (>128)
    inRange(B, Scalar(132), Scalar(155), B_wood); // 偏黄 (>128)

    // 木色 = 中等亮度 AND 偏红 AND 偏黄
    bitwise_and(L_wood, A_wood, woodMask);
    bitwise_and(woodMask, B_wood, woodMask);

    // 5. 合并两种检测结果
    Mat finalMask;
    bitwise_or(whiteMask, woodMask, finalMask);

    // 6. 基本形态学处理去噪
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
    morphologyEx(finalMask, finalMask, MORPH_OPEN, kernel);  // 去除小噪声
    morphologyEx(finalMask, finalMask, MORPH_CLOSE, kernel); // 填充小空洞

    // 输出调试信息
    int whitePixels = countNonZero(whiteMask);
    int woodPixels = countNonZero(woodMask);
    int totalPixels = countNonZero(finalMask);

    cout << "LAB Detection Results:" << endl;
    cout << "- White pixels detected: " << whitePixels << endl;
    cout << "- Wood pixels detected: " << woodPixels << endl;
    cout << "- Total LAB pixels: " << totalPixels << endl;

    return finalMask;
}

// 形态学处理函数
Mat performMorphological(const Mat &binaryImage)
{
    Mat result;

    // 只进行膨胀操作 - 连接断裂区域
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(Config::MORPH_DILATE_KERNEL_SIZE, Config::MORPH_DILATE_KERNEL_SIZE));

    // 膨胀操作
    dilate(binaryImage, result, kernel);

    return result;
}

// 轮廓填充函数
Mat fillContours(const Mat &binaryImage)
{
    Mat result = binaryImage.clone();

    // 查找轮廓
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(result, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // 填充所有外部轮廓
    for (size_t i = 0; i < contours.size(); i++)
    {
        // 只填充面积大于阈值的轮廓，过滤掉小的噪点
        double area = contourArea(contours[i]);
        if (area > Config::MIN_CONTOUR_AREA)
        {
            fillPoly(result, contours[i], Scalar(255));
        }
    }

    return result;
}

// 连通域填充函数
Mat fillConnectedComponents(const Mat &binaryImage)
{
    Mat result = binaryImage.clone();
    Mat labels, stats, centroids;

    // 连通域分析
    int numComponents = connectedComponentsWithStats(binaryImage, labels, stats, centroids);

    // 遍历每个连通域
    for (int i = 1; i < numComponents; i++) // 跳过背景 (标签0)
    {
        int area = stats.at<int>(i, CC_STAT_AREA);

        // 只保留面积大于阈值的连通域
        if (area > Config::MIN_CONNECTED_AREA)
        {
            // 填充该连通域
            Mat mask = (labels == i);
            result.setTo(255, mask);
        }
        else
        {
            // 删除小的连通域
            Mat mask = (labels == i);
            result.setTo(0, mask);
        }
    }

    return result;
}

// 基于全图面积百分比的连通域过滤函数
Mat filterConnectedComponentsByPercent(const Mat &binaryImage, double minPercentage)
{
    Mat result = binaryImage.clone();
    Mat labels, stats, centroids;

    // 连通域分析
    int numComponents = connectedComponentsWithStats(binaryImage, labels, stats, centroids);

    // 计算全图面积和最小面积阈值
    int totalArea = binaryImage.rows * binaryImage.cols;
    int minArea = totalArea * (minPercentage / 100.0);

    // 遍历每个连通域
    for (int i = 1; i < numComponents; i++) // 跳过背景 (标签0)
    {
        int area = stats.at<int>(i, CC_STAT_AREA);

        // 只保留面积大于阈值的连通域
        if (area >= minArea)
        {
            // 填充该连通域
            Mat mask = (labels == i);
            result.setTo(255, mask);
        }
        else
        {
            // 删除小的连通域
            Mat mask = (labels == i);
            result.setTo(0, mask);
        }
    }

    return result;
}

// CLAHE对比度限制自适应直方图均衡
Mat enhanceContrast_CLAHE(const Mat &inputImage)
{
    if (inputImage.empty())
    {
        cerr << "Error: Empty input image for CLAHE" << endl;
        return Mat();
    }

    Mat result;

    if (inputImage.channels() == 3)
    {
        // 对彩色图像，在LAB空间处理
        Mat lab;
        cvtColor(inputImage, lab, COLOR_BGR2Lab);
        vector<Mat> channels;
        split(lab, channels);

        // 创建CLAHE对象，参数固定用于测试
        Ptr<CLAHE> clahe = createCLAHE();
        clahe->setClipLimit(3.0);            // 对比度限制
        clahe->setTilesGridSize(Size(8, 8)); // 网格大小8x8

        clahe->apply(channels[0], channels[0]); // 只处理L通道
        merge(channels, lab);
        cvtColor(lab, result, COLOR_Lab2BGR);
    }
    else
    {
        // 灰度图像直接处理
        Ptr<CLAHE> clahe = createCLAHE();
        clahe->setClipLimit(3.0);
        clahe->setTilesGridSize(Size(8, 8));
        clahe->apply(inputImage, result);
    }

    cout << "Applied CLAHE enhancement (clip: 3.0, tiles: 8x8)" << endl;
    return result;
}

// ==================== 模板匹配判断实现 ====================

/**
 * @brief 旋转图像（保持图像完整，不裁剪）
 * @param src 源图像
 * @param angle 旋转角度（度，正值为逆时针）
 * @return 旋转后的图像
 */
static Mat rotateImage(const Mat &src, double angle)
{
    // 计算旋转中心
    Point2f center(src.cols / 2.0, src.rows / 2.0);

    // 获取旋转矩阵
    Mat rotMat = getRotationMatrix2D(center, angle, 1.0);

    // 计算旋转后的边界框尺寸
    double abs_cos = abs(rotMat.at<double>(0, 0));
    double abs_sin = abs(rotMat.at<double>(0, 1));
    int new_w = int(src.rows * abs_sin + src.cols * abs_cos);
    int new_h = int(src.rows * abs_cos + src.cols * abs_sin);

    // 调整旋转矩阵以适应新尺寸
    rotMat.at<double>(0, 2) += (new_w / 2.0 - center.x);
    rotMat.at<double>(1, 2) += (new_h / 2.0 - center.y);

    // 执行旋转
    Mat rotated;
    warpAffine(src, rotated, rotMat, Size(new_w, new_h),
               INTER_LINEAR, BORDER_CONSTANT, Scalar(0));

    return rotated;
}

bool judgeByTemplateMatch(
    const Mat &resultImage,
    const string &templateFolder,
    const vector<double> &thresholds,
    vector<TemplateMatchResult> &results)
{
    results.clear();

    if (resultImage.empty())
    {
        cerr << "错误: 输入图像为空" << endl;
        return false;
    }

    // Step 1: 获取模板文件列表（读取文件夹中所有图片文件）
    vector<string> files;

    try
    {
        // 检查文件夹是否存在
        if (!fs::exists(templateFolder) || !fs::is_directory(templateFolder))
        {
            cerr << "错误: 模板文件夹不存在或不是目录: " << templateFolder << endl;
            return false;
        }

        // 遍历文件夹中的所有文件
        for (const auto &entry : fs::directory_iterator(templateFolder))
        {
            if (entry.is_regular_file())
            {
                string filename = entry.path().filename().string();
                string extension = entry.path().extension().string();

                // 只处理图片文件（.jpg, .jpeg, .png, .bmp）
                transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                if (extension == ".jpg" || extension == ".jpeg" ||
                    extension == ".png" || extension == ".bmp")
                {
                    files.push_back(filename);
                }
            }
        }

        // 按文件名排序
        sort(files.begin(), files.end());
    }
    catch (const fs::filesystem_error &e)
    {
        cerr << "错误: 读取模板文件夹失败: " << e.what() << endl;
        return false;
    }

    if (files.empty())
    {
        cerr << "错误: 模板文件夹中没有找到图片文件" << endl;
        return false;
    }

    // Step 2: 验证配置
    if (files.size() != thresholds.size())
    {
        cerr << "错误: 模板数量(" << files.size()
             << ") != 阈值数量(" << thresholds.size() << ")" << endl;
        return false;
    }

    cout << "找到 " << files.size() << " 个模板文件" << endl;

    // Step 3: 遍历每个模板进行多角度匹配
    bool allPassed = true;

    for (size_t i = 0; i < files.size(); i++)
    {
        string templatePath = templateFolder + "/" + files[i];
        Mat templateImg = imread(templatePath, IMREAD_GRAYSCALE);

        TemplateMatchResult result;
        result.filename = files[i];

        if (templateImg.empty())
        {
            cerr << "错误: 无法加载模板 " << files[i] << endl;
            result.score = 0.0;
            result.passed = false;
            allPassed = false;
            results.push_back(result);
            continue;
        }

        // 初始检查模板尺寸（原始角度）
        int templateTotalPixels = templateImg.cols * templateImg.rows;
        int templateWhitePixels = countNonZero(templateImg);
        int resultTotalPixels = resultImage.cols * resultImage.rows;
        int resultWhitePixels = countNonZero(resultImage);

        double templateDensity = (double)templateWhitePixels / templateTotalPixels * 100.0;
        double resultDensity = (double)resultWhitePixels / resultTotalPixels * 100.0;

        cout << "模板 " << files[i] << " 原始尺寸: "
             << templateImg.cols << "x" << templateImg.rows
             << " (" << templateTotalPixels << "像素)"
             << ", 白色像素: " << templateWhitePixels
             << " (密度: " << fixed << setprecision(1) << templateDensity << "%)" << endl;
        cout << "结果图尺寸: " << resultImage.cols << "x" << resultImage.rows
             << " (" << resultTotalPixels << "像素)"
             << ", 白色像素: " << resultWhitePixels
             << " (密度: " << resultDensity << "%)" << endl;

        if (templateImg.cols > resultImage.cols ||
            templateImg.rows > resultImage.rows)
        {
            cerr << "警告: 模板 " << files[i] << " 尺寸("
                 << templateImg.cols << "x" << templateImg.rows
                 << ") 大于结果图(" << resultImage.cols << "x" << resultImage.rows << ")" << endl;
        }

        // Step 4: 多角度旋转匹配
        double bestSimilarity = 0.0;
        double bestAngle = 0.0;
        int testedAngles = 0;

        // 构建角度测试序列：0, +step, -step, +2*step, -2*step, ...
        vector<double> angleSequence;
        angleSequence.push_back(0.0); // 先测试0度
        for (double offset = TemplateMatchConfig::ROTATION_STEP;
             offset <= TemplateMatchConfig::ROTATION_MAX;
             offset += TemplateMatchConfig::ROTATION_STEP)
        {
            angleSequence.push_back(offset);  // 正角度
            angleSequence.push_back(-offset); // 负角度
        }

        // 按照中心扩散顺序测试角度
        for (double angle : angleSequence)
        {
            // 旋转模板
            Mat rotatedTemplate = (abs(angle) < 0.01) ? templateImg.clone() : rotateImage(templateImg, angle);

            // 检查旋转后的模板尺寸
            if (rotatedTemplate.cols > resultImage.cols ||
                rotatedTemplate.rows > resultImage.rows)
            {
                // 旋转后尺寸过大，跳过此角度
                cout << "  角度" << angle << "°: 旋转后尺寸过大("
                     << rotatedTemplate.cols << "x" << rotatedTemplate.rows
                     << " > " << resultImage.cols << "x" << resultImage.rows
                     << ")，跳过此角度" << endl;
                continue;
            }

            // 执行模板匹配（使用归一化平方差）
            Mat matchResult;
            matchTemplate(resultImage, rotatedTemplate, matchResult, TM_SQDIFF_NORMED);

            // 找到最小差值
            double minVal;
            minMaxLoc(matchResult, &minVal, nullptr, nullptr, nullptr);

            // 转换为相似度（越大越好）
            double similarity = 1.0 - minVal;

            // 调试输出
            int templateWhitePixels = countNonZero(rotatedTemplate);
            int resultWhitePixels = countNonZero(resultImage);
            cout << "  角度" << angle << "°: minVal=" << fixed << setprecision(3) << minVal
                 << ", similarity=" << similarity
                 << ", 模板白色像素=" << templateWhitePixels
                 << ", 结果图白色像素=" << resultWhitePixels << endl;

            // 更新最佳得分
            if (similarity > bestSimilarity)
            {
                bestSimilarity = similarity;
                bestAngle = angle;
            }

            testedAngles++;

            // 早停：如果找到足够好的匹配，提前退出
            if (similarity >= thresholds[i])
            {
                // cout << "  找到满足阈值的匹配，提前退出" << endl;
                break;
            }
        }

        // 判断是否通过
        result.score = bestSimilarity;
        result.passed = (bestSimilarity >= thresholds[i]);

        if (!result.passed)
        {
            allPassed = false;
        }

        results.push_back(result);

        // 打印结果
        cout << "模板 " << files[i] << ": "
             << "最佳相似度=" << fixed << setprecision(3) << bestSimilarity
             << " (角度=" << bestAngle << "°, 测试角度数=" << testedAngles
             << ", 阈值=" << thresholds[i] << ") "
             << (result.passed ? "[通过]" : "[失败]") << endl;
    }

    return allPassed;
}

// ==================== 模板匹配判断实现 ====================
