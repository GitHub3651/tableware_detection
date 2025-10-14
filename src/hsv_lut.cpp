#include "hsv_lut.h"
#include "config_constants.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <filesystem>

using namespace cv;
using namespace std;
using namespace std::chrono;

// 静态成员变量定义
bool HSVLookupTable::initialized = false;
unsigned char HSVLookupTable::lutData[256][256][256];
const string HSVLookupTable::LUT_FILE_PATH = "hsv_lut_cache.bin";
const string HSVLookupTable::LUT_MAGIC_HEADER = "HSVLUT01";
const uint32_t HSVLookupTable::LUT_VERSION = 1;

/**
 * 初始化LUT系统
 * 尝试从文件加载，如果失败则重新构建
 */
bool HSVLookupTable::initialize()
{
    if (initialized)
    {
        return true;
    }

    cout << "========================================" << endl;
    cout << "  HSV查找表 (LUT) 初始化" << endl;
    cout << "========================================" << endl;

    // 尝试从文件加载
    if (loadLUTFromFile())
    {
        cout << "✓ 成功从文件加载LUT缓存" << endl;
        initialized = true;
        return true;
    }

    // 文件加载失败，重新构建
    cout << "! LUT文件不存在或已损坏，开始重新构建..." << endl;
    buildLUT();

    // 保存到文件
    if (saveLUTToFile())
    {
        cout << "✓ LUT已保存到文件: " << LUT_FILE_PATH << endl;
    }
    else
    {
        cout << "! 警告: LUT文件保存失败，下次启动将重新构建" << endl;
    }

    initialized = true;
    cout << "✓ HSV LUT初始化完成！" << endl;
    cout << "========================================" << endl;
    return true;
}

/**
 * 构建HSV查找表
 * 遍历所有可能的BGR组合，预计算HSV判断结果
 */
void HSVLookupTable::buildLUT()
{
    cout << "正在构建HSV查找表..." << endl;
    auto startTime = steady_clock::now();

    // 遍历所有可能的BGR值
    for (int b = 0; b < 256; b++)
    {
        showProgress(b, 256, "构建LUT");

        for (int g = 0; g < 256; g++)
        {
            for (int r = 0; r < 256; r++)
            {
                // 将BGR转换为HSV
                Mat bgrPixel = (Mat_<Vec3b>(1, 1) << Vec3b(b, g, r));
                Mat hsvPixel;
                cvtColor(bgrPixel, hsvPixel, COLOR_BGR2HSV);

                Vec3b hsv = hsvPixel.at<Vec3b>(0, 0);
                int h = hsv[0], s = hsv[1], v = hsv[2];

                // 检查是否在目标HSV范围内
                bool inTargetRange = false;
                for (int i = 0; i < Config::RANGE_COUNT; ++i)
                {
                    if (h >= Config::HSV_RANGES[i][0] && h <= Config::HSV_RANGES[i][1] &&
                        s >= Config::HSV_RANGES[i][2] && s <= Config::HSV_RANGES[i][3] &&
                        v >= Config::HSV_RANGES[i][4] && v <= Config::HSV_RANGES[i][5])
                    {
                        inTargetRange = true;
                        break;
                    }
                }

                // 存储结果到LUT
                lutData[b][g][r] = inTargetRange ? 255 : 0;
            }
        }
    }

    auto endTime = steady_clock::now();
    auto duration = duration_cast<milliseconds>(endTime - startTime);

    cout << "\n✓ LUT构建完成！耗时: " << duration.count() << "ms" << endl;
}

/**
 * 从文件加载LUT
 */
bool HSVLookupTable::loadLUTFromFile()
{
    if (!filesystem::exists(LUT_FILE_PATH))
    {
        return false;
    }

    ifstream file(LUT_FILE_PATH, ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    // 读取文件头
    LUTFileHeader header;
    file.read(reinterpret_cast<char *>(&header), sizeof(header));

    if (file.gcount() != sizeof(header))
    {
        file.close();
        return false;
    }

    // 验证文件头
    if (string(header.magic, 8) != LUT_MAGIC_HEADER ||
        header.version != LUT_VERSION)
    {
        file.close();
        return false;
    }

    // 检查HSV参数是否改变
    if (header.hsvParamsHash != calculateHSVParamsHash())
    {
        cout << "! HSV参数已更改，需要重新构建LUT" << endl;
        file.close();
        return false;
    }

    // 读取LUT数据
    file.read(reinterpret_cast<char *>(lutData), sizeof(lutData));
    if (file.gcount() != sizeof(lutData))
    {
        file.close();
        return false;
    }

    // 验证校验和
    uint32_t calculatedChecksum = calculateChecksum(reinterpret_cast<const unsigned char *>(lutData), sizeof(lutData));
    if (calculatedChecksum != header.checksum)
    {
        cout << "! LUT文件校验和错误，数据可能已损坏" << endl;
        file.close();
        return false;
    }

    file.close();
    return true;
}

/**
 * 保存LUT到文件
 */
bool HSVLookupTable::saveLUTToFile()
{
    ofstream file(LUT_FILE_PATH, ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    // 准备文件头
    LUTFileHeader header;
    strncpy(header.magic, LUT_MAGIC_HEADER.c_str(), 8);
    header.version = LUT_VERSION;
    header.checksum = calculateChecksum(reinterpret_cast<const unsigned char *>(lutData), sizeof(lutData));
    header.timestamp = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    header.hsvParamsHash = calculateHSVParamsHash();

    // 写入文件头和数据
    file.write(reinterpret_cast<const char *>(&header), sizeof(header));
    file.write(reinterpret_cast<const char *>(lutData), sizeof(lutData));

    file.close();
    return file.good();
}

/**
 * 使用LUT处理图像
 */
Mat HSVLookupTable::processImage(const Mat &bgrImage)
{
    if (!initialized)
    {
        cerr << "错误: HSV LUT未初始化" << endl;
        return Mat();
    }

    if (bgrImage.empty())
    {
        cerr << "错误: 输入图像为空" << endl;
        return Mat();
    }

    Mat result = Mat::zeros(bgrImage.size(), CV_8UC1);

    // 使用LUT快速处理每个像素
    for (int y = 0; y < bgrImage.rows; y++)
    {
        const Vec3b *bgrRow = bgrImage.ptr<Vec3b>(y);
        uchar *resultRow = result.ptr<uchar>(y);

        for (int x = 0; x < bgrImage.cols; x++)
        {
            Vec3b bgr = bgrRow[x];
            // 直接查表，无需计算！
            resultRow[x] = lutData[bgr[0]][bgr[1]][bgr[2]];
        }
    }

    return result;
}

/**
 * 计算HSV参数的哈希值（用于检测参数变化）
 */
uint32_t HSVLookupTable::calculateHSVParamsHash()
{
    uint32_t hash = 0;
    for (int i = 0; i < Config::RANGE_COUNT; ++i)
    {
        for (int j = 0; j < 6; ++j)
        {
            hash = hash * 31 + Config::HSV_RANGES[i][j];
        }
    }
    return hash;
}

/**
 * 计算数据校验和
 */
uint32_t HSVLookupTable::calculateChecksum(const unsigned char *data, size_t size)
{
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++)
    {
        checksum = checksum * 31 + data[i];
    }
    return checksum;
}

/**
 * 显示进度
 */
void HSVLookupTable::showProgress(int current, int total, const string &operation)
{
    if (current % 32 == 0 || current == total - 1)
    {
        int percentage = (current * 100) / total;
        cout << "\r" << operation << " 进度: " << setw(3) << percentage << "%" << flush;
    }
}

/**
 * 获取状态信息
 */
string HSVLookupTable::getStatusInfo()
{
    if (!initialized)
    {
        return "未初始化";
    }

    string info = "已就绪 | 内存: " + to_string(static_cast<int>(getMemoryUsageMB())) + "MB";

    if (filesystem::exists(LUT_FILE_PATH))
    {
        info += " | 缓存文件: 存在";
    }
    else
    {
        info += " | 缓存文件: 不存在";
    }

    return info;
}

/**
 * 获取内存使用量（MB）
 */
double HSVLookupTable::getMemoryUsageMB()
{
    return sizeof(lutData) / (1024.0 * 1024.0);
}

/**
 * 打印内存统计信息
 */
void HSVLookupTable::printMemoryStats()
{
    cout << "========== HSV LUT 内存统计 ==========" << endl;
    cout << "LUT大小: " << sizeof(lutData) << " 字节" << endl;
    cout << "内存使用: " << fixed << setprecision(1) << getMemoryUsageMB() << " MB" << endl;
    cout << "状态: " << getStatusInfo() << endl;
    cout << "=====================================" << endl;
}

/**
 * 清理资源
 */
void HSVLookupTable::cleanup()
{
    initialized = false;
    cout << "HSV LUT 已清理" << endl;
}

/**
 * 检查LUT是否就绪
 */
bool HSVLookupTable::isReady()
{
    return initialized;
}

/**
 * 强制重建LUT
 */
void HSVLookupTable::forceBuildLUT()
{
    cout << "强制重建HSV LUT..." << endl;
    buildLUT();
    saveLUTToFile();
    initialized = true;
}

/**
 * 清除LUT文件
 */
bool HSVLookupTable::clearLUTFile()
{
    if (filesystem::exists(LUT_FILE_PATH))
    {
        return filesystem::remove(LUT_FILE_PATH);
    }
    return true;
}

/**
 * 打印LUT统计信息
 */
void HSVLookupTable::printLUTStats()
{
    if (!initialized)
    {
        cout << "LUT未初始化，无法显示统计信息" << endl;
        return;
    }

    // 统计LUT中目标像素的数量
    int targetPixels = 0;
    int totalPixels = 256 * 256 * 256;

    for (int b = 0; b < 256; b++)
    {
        for (int g = 0; g < 256; g++)
        {
            for (int r = 0; r < 256; r++)
            {
                if (lutData[b][g][r] == 255)
                {
                    targetPixels++;
                }
            }
        }
    }

    double targetPercentage = (targetPixels * 100.0) / totalPixels;

    cout << "========== HSV LUT 统计信息 ==========" << endl;
    cout << "总像素空间: " << totalPixels << endl;
    cout << "目标像素: " << targetPixels << " (" << fixed << setprecision(2) << targetPercentage << "%)" << endl;
    cout << "背景像素: " << (totalPixels - targetPixels) << " (" << fixed << setprecision(2) << (100.0 - targetPercentage) << "%)" << endl;
    cout << "=====================================" << endl;
}