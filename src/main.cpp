/*
 * 餐具检测主程序
 *
 * 功能：
 * 1. 读取图像并进行预处理
 * 2. 调用图像处理模块进行检测
 * 3. 调用显示模块进行结果展示
 * 4. 保存处理结果
 *
 * 使用方法：
 * tableware_detection.exe <image_path>
 * 例如：tableware_detection.exe tableware.jpg
 */

#include "image_processing.h"
#include "display.h"
#include "config_constants.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <chrono>

using namespace cv;
using namespace std;

int main(int argc, char *argv[])
{
    // Check command line arguments
    if (argc != 2)
    {
        cout << "Usage: " << argv[0] << " <image_path>" << endl;
        cout << "Example: " << argv[0] << " tableware.jpg" << endl;
        system("pause");
        return -1;
    }

    string imagePath = argv[1];

    // 开始总计时
    auto totalStart = chrono::steady_clock::now();

    // 读取图像
    Mat originalImage = imread(imagePath, IMREAD_COLOR);

    // Check if image is loaded successfully
    if (originalImage.empty())
    {
        cerr << "Error: Cannot load image " << imagePath << endl;
        cerr << "Please check if the file path is correct" << endl;
        system("pause");
        return -1;
    }

    // 算法开始计时
    auto algorithmStart = chrono::steady_clock::now();

    // Resize image to specified scale for processing using image processing module
    Mat rgbImage = resizeImageByScale(originalImage, Config::RESIZE_SCALE);

    // Create HSV image storage variable
    Mat hsvImage;

    // Convert BGR to HSV (OpenCV reads images in BGR format by default)
    cvtColor(rgbImage, hsvImage, COLOR_BGR2HSV);

    // Split HSV channels for analysis
    vector<Mat> hsvChannels;
    split(hsvImage, hsvChannels);

    // =====================================================
    // 图像处理流水线
    // =====================================================

    // 1. 创建原始HSV的二值化结果
    Mat originalBinary = createHueBinaryMask(hsvImage);

    // 2. 形态学处理
    Mat morphProcessed = performMorphological(originalBinary);

    // 3. 轮廓填充处理
    Mat contourFilled = fillContours(morphProcessed);

    // 4. 连通域百分比过滤处理（基于全图面积百分比过滤）
    Mat finalResult = filterConnectedComponentsByPercent(contourFilled, Config::CONNECTED_COMPONENT_PERCENT);

    // 算法处理完成，记录结束时间
    auto algorithmEnd = chrono::steady_clock::now();
    auto totalEnd = chrono::steady_clock::now();

    // 计算耗时
    int algorithmMs = chrono::duration_cast<chrono::milliseconds>(algorithmEnd - algorithmStart).count();
    int totalMs = chrono::duration_cast<chrono::milliseconds>(totalEnd - totalStart).count();

    // 在控制台输出处理时间
    cout << "Algorithm time: " << algorithmMs << "ms" << endl;
    cout << "Total time: " << totalMs << "ms" << endl; // =====================================================
    // 结果显示
    // =====================================================

    // 显示6张图片：原图，缩放图，HSV图，二值图，形态学处理，连通域过滤
    vector<Mat> displayImages = {
        originalImage,  // 1. 原始BGR图像（未缩放）
        rgbImage,       // 2. 缩放后的BGR图像
        hsvImage,       // 3. HSV图像
        originalBinary, // 4. 二值化图像
        morphProcessed, // 5. 形态学处理结果
        finalResult     // 6. 连通域百分比过滤结果
    };

    vector<string> displayTitles = {
        "1. Original Image",
        "2. Resized Image",
        "3. HSV Image",
        "4. Binary Mask",
        "5. Morphological",
        "6. Final Result"};

    // 创建subplot显示 (3行3列布局，第8、9个位置留空)
    Mat subplotCanvas = createSubplotDisplay(displayImages, displayTitles, 3, 3);

    // 在画布上显示两个关键处理时间
    putText(subplotCanvas, "Total Time: " + to_string(totalMs) + "ms", Point(10, 30),
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
    putText(subplotCanvas, "Algorithm Only: " + to_string(algorithmMs) + "ms", Point(10, 60),
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 200, 200), 2);

    // 显示主要结果窗口
    namedWindow("HSV and Morphological Processing", WINDOW_AUTOSIZE);
    imshow("HSV and Morphological Processing", subplotCanvas);

    // Wait for user to view the subplot
    waitKey(0);

    // Close the subplot window
    destroyWindow("HSV and Morphological Processing");

    // =====================================================
    // 交互式颜色分析
    // =====================================================

    showColorAnalysis(hsvImage, rgbImage);

    // =====================================================
    // 保存结果
    // =====================================================

    // Save the result images
    string resultOutputPath = "morphological_processing_" + imagePath;
    bool resultSaveSuccess = imwrite(resultOutputPath, subplotCanvas);

    // Save individual processing results
    string binaryOutputPath = "binary_mask_" + imagePath;
    bool binarySaveSuccess = imwrite(binaryOutputPath, originalBinary);

    string morphOutputPath = "morphological_" + imagePath;
    bool morphSaveSuccess = imwrite(morphOutputPath, morphProcessed);

    string finalOutputPath = "final_result_" + imagePath;
    bool finalSaveSuccess = imwrite(finalOutputPath, finalResult);

    // 输出保存结果
    if (resultSaveSuccess && binarySaveSuccess && morphSaveSuccess && finalSaveSuccess)
    {
        cout << "\nAll result images saved successfully!" << endl;
        cout << "- Main result: " << resultOutputPath << endl;
        cout << "- Binary mask: " << binaryOutputPath << endl;
        cout << "- Morphological result: " << morphOutputPath << endl;
        cout << "- Final result: " << finalOutputPath << endl;
    }
    else
    {
        cout << "\nWarning: Some images may not be saved successfully." << endl;
    }

    system("pause");
    return 0;
}