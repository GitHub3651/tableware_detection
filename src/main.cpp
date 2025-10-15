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

    // =====================================================
    // 算法开始计时（从读取图片之后开始）
    // =====================================================
    auto algorithmStart = chrono::steady_clock::now();

    // 先缩放原图
    Mat resizedImage = resizeImageByScale(originalImage, Config::RESIZE_SCALE);

    // 直接使用缩放后的图像进行处理
    Mat rgbImage = resizedImage.clone();

    // =====================================================
    // 图像处理流水线
    // =====================================================

    // 1. 创建HSV二值化结果
    Mat originalBinary = createHueBinaryMask(rgbImage);

    // 2. 形态学处理
    Mat morphProcessed = performMorphological(originalBinary);

    // 3. 轮廓填充处理
    Mat contourFilled = fillContours(morphProcessed);

    // 4. 连通域百分比过滤处理（基于全图面积百分比过滤）
    Mat finalResult = filterConnectedComponentsByPercent(contourFilled, Config::CONNECTED_COMPONENT_PERCENT);

    // =====================================================
    // 模板匹配判断 NG/OK
    // =====================================================

    cout << "\n========== 模板匹配判断 ==========" << endl;

    vector<TemplateMatchResult> matchResults;
    bool isOK = judgeByTemplateMatch(
        finalResult,
        TemplateMatchConfig::TEMPLATE_FOLDER,
        TemplateMatchConfig::THRESHOLDS,
        matchResults);

    // 算法处理完成（包含判断逻辑），记录结束时间
    auto algorithmEnd = chrono::steady_clock::now();
    auto totalEnd = chrono::steady_clock::now();

    // 计算耗时
    int algorithmMs = chrono::duration_cast<chrono::milliseconds>(algorithmEnd - algorithmStart).count();
    int totalMs = chrono::duration_cast<chrono::milliseconds>(totalEnd - totalStart).count();

    cout << "====================================" << endl;
    cout << "最终判定: " << (isOK ? "OK" : "NG") << endl;
    cout << "====================================" << endl;

    // 在控制台输出处理时间
    cout << "Algorithm time: " << algorithmMs << "ms" << endl;
    cout << "Total time: " << totalMs << "ms" << endl;

    // =====================================================
    // 结果显示
    // =====================================================

    // 显示6张图片：原图，缩放图，二值图，形态学处理，轮廓填充，连通域过滤
    vector<Mat> displayImages = {
        originalImage,  // 1. 原始BGR图像（未缩放）
        resizedImage,   // 2. 缩放后的图像
        originalBinary, // 3. HSV二值化图像
        morphProcessed, // 4. 形态学处理结果
        contourFilled,  // 5. 轮廓填充结果
        finalResult     // 6. 连通域百分比过滤结果
    };

    vector<string> displayTitles = {
        "1. Original Image",
        "2. Resized Image",
        "3. HSV Binary Mask",
        "4. Morphological",
        "5. Contour Filled",
        "6. Final Result"};

    // 创建subplot显示 (2行3列布局，显示6张处理步骤图)
    Mat subplotCanvas = createSubplotDisplay(displayImages, displayTitles, 2, 3);

    // 在画布左上角显示处理时间
    putText(subplotCanvas, "Algorithm: " + to_string(algorithmMs) + "ms", Point(10, 30),
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
    putText(subplotCanvas, "Total: " + to_string(totalMs) + "ms", Point(10, 60),
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);

    // 在画布右上角显示 OK/NG 判定结果
    string judgementText = isOK ? "OK" : "NG";
    Scalar judgementColor = isOK ? Scalar(0, 255, 0) : Scalar(0, 0, 255); // 绿色=OK, 红色=NG

    // 计算文本尺寸以便右对齐
    int fontFace = FONT_HERSHEY_SIMPLEX;
    double fontScale = 2.5;
    int thickness = 5;
    int baseline = 0;
    Size textSize = getTextSize(judgementText, fontFace, fontScale, thickness, &baseline);

    // 右上角位置（留出边距）
    Point textPos(subplotCanvas.cols - textSize.width - 30, textSize.height + 30);

    // 绘制文本
    putText(subplotCanvas, judgementText, textPos, fontFace, fontScale, judgementColor, thickness);

    // 显示主要结果窗口
    namedWindow("HSV Detection and Processing", WINDOW_AUTOSIZE);
    imshow("HSV Detection and Processing", subplotCanvas);

    // 等待用户按键
    int key = waitKey(0);

    // Close all windows
    destroyAllWindows();

    // 只有按下空格键(32)才进入HSV颜色分析
    if (key == 32) // 空格键的ASCII码是32
    {
        // =====================================================
        // 交互式颜色分析
        // =====================================================

        // 转换为HSV用于颜色分析
        Mat hsvImage;
        cvtColor(rgbImage, hsvImage, COLOR_BGR2HSV);

        // 显示交互式颜色分析窗口
        showColorAnalysis(hsvImage, rgbImage);
    }

    return 0;
}