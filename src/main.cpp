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
#include "hsv_lut.h"
#include "display.h"
#include "config_constants.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <chrono>

using namespace cv;
using namespace std;

// 用户选择界面
int displayMethodSelection()
{
    cout << "\n========================================" << endl;
    cout << "         餐具检测 - 处理方法选择" << endl;
    cout << "========================================" << endl;
    cout << "1. 传统HSV检测" << endl;
    cout << "   • 适合：单张图片处理" << endl;
    cout << "   • 特点：稳定，无需初始化" << endl;
    cout << "   • 速度：中等" << endl;
    cout << "----------------------------------------" << endl;
    cout << "2. LUT加速检测" << endl;
    cout << "   • 适合：批量图片处理" << endl;
    cout << "   • 特点：首次慢，后续极快" << endl;
    cout << "   • 内存：占用16MB" << endl;
    cout << "========================================" << endl;

    int choice;
    while (true)
    {
        cout << "请选择处理方法 (1/2): ";
        cin >> choice;

        if (choice == 1 || choice == 2)
        {
            break;
        }
        else
        {
            cout << "! 请输入 1 或 2" << endl;
        }
    }

    return choice;
}

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

    // 显示方法选择界面
    int processingMethod = displayMethodSelection();

    // 如果选择LUT方法，进行初始化
    bool useLUT = (processingMethod == 2);
    if (useLUT)
    {
        if (!HSVLookupTable::initialize())
        {
            cerr << "! LUT初始化失败，切换到传统方法" << endl;
            useLUT = false;
        }
    }

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

    // =====================================================
    // 图像处理流水线
    // =====================================================

    // 1. 创建HSV二值化结果（根据选择使用不同方法）
    Mat originalBinary;
    string methodName;

    if (useLUT)
    {
        originalBinary = HSVLookupTable::processImage(rgbImage);
        methodName = "LUT加速";
    }
    else
    {
        originalBinary = createHueBinaryMask(rgbImage);
        methodName = "传统HSV";
    }

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

    // 在控制台输出处理时间和方法信息
    cout << "\n========== 处理完成 ==========" << endl;
    cout << "处理方法: " << methodName << endl;
    cout << "算法耗时: " << algorithmMs << "ms" << endl;
    cout << "总耗时: " << totalMs << "ms" << endl;

    if (useLUT)
    {
        cout << "LUT状态: " << HSVLookupTable::getStatusInfo() << endl;
    }
    cout << "============================" << endl; // =====================================================
    // 结果显示
    // =====================================================

    // 显示5张图片：原图，缩放图，二值图，形态学处理，连通域过滤
    vector<Mat> displayImages = {
        originalImage,  // 1. 原始BGR图像（未缩放）
        rgbImage,       // 2. 缩放后的BGR图像
        originalBinary, // 3. HSV二值化图像
        morphProcessed, // 4. 形态学处理结果
        finalResult     // 5. 连通域百分比过滤结果
    };

    vector<string> displayTitles = {
        "1. Original Image",
        "2. Resized Image",
        "3. " + methodName + " Binary Mask",
        "4. Morphological",
        "5. Final Result"};

    // 创建subplot显示 (2行3列布局，显示5张处理步骤图)
    Mat subplotCanvas = createSubplotDisplay(displayImages, displayTitles, 2, 3);

    // 在画布上显示处理信息
    putText(subplotCanvas, "Method: " + methodName, Point(10, 30),
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 255, 0), 2);
    putText(subplotCanvas, "Total: " + to_string(totalMs) + "ms", Point(10, 60),
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
    putText(subplotCanvas, "Algorithm: " + to_string(algorithmMs) + "ms", Point(10, 90),
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 200, 200), 2);

    // 显示主要结果窗口
    namedWindow("HSV Detection and Processing", WINDOW_AUTOSIZE);
    imshow("HSV Detection and Processing", subplotCanvas);

    // Wait for user to view the subplot
    waitKey(0);

    // 使用更安全的窗口关闭方法
    destroyAllWindows();

    // 清理LUT资源（如果使用了）
    if (useLUT)
    {
        HSVLookupTable::cleanup();
    }

    return 0;
}