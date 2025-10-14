#ifndef HSV_LUT_H
#define HSV_LUT_H

#include <opencv2/opencv.hpp>
#include <string>
#include <cstdint>

using namespace cv;
using namespace std;

/**
 * HSV查找表(LUT)加速处理类
 *
 * 功能：
 * 1. 建立HSV色彩空间的查找表，避免重复计算
 * 2. 支持LUT文件的保存和加载，提升启动速度
 * 3. 提供与传统方法兼容的图像处理接口
 * 4. 内存管理和性能统计
 */
class HSVLookupTable
{
private:
    // LUT数据存储 - 16MB内存 (256^3 bytes)
    static bool initialized;
    static unsigned char lutData[256][256][256];

    // 文件管理
    static const string LUT_FILE_PATH;
    static const string LUT_MAGIC_HEADER;
    static const uint32_t LUT_VERSION;

    // LUT文件头结构
    struct LUTFileHeader
    {
        char magic[8];          // "HSVLUT01"
        uint32_t version;       // 版本号
        uint32_t checksum;      // 校验和
        uint64_t timestamp;     // 创建时间
        uint32_t hsvParamsHash; // HSV参数哈希值（检测参数变化）
    };

    // 私有方法
    static void buildLUT();
    static bool loadLUTFromFile();
    static bool saveLUTToFile();
    static void validateLUT();
    static uint32_t calculateHSVParamsHash();
    static uint32_t calculateChecksum(const unsigned char *data, size_t size);
    static void showProgress(int current, int total, const string &operation);

public:
    // 主要接口
    static bool initialize();
    static Mat processImage(const Mat &bgrImage);
    static void cleanup();
    static bool isReady();

    // 信息和统计
    static void printMemoryStats();
    static string getStatusInfo();
    static double getMemoryUsageMB();

    // 调试和维护
    static void forceBuildLUT(); // 强制重建LUT
    static bool clearLUTFile();  // 清除LUT文件
    static void printLUTStats(); // 打印LUT使用统计
};

#endif // HSV_LUT_H