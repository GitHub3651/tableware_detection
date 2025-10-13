@echo off
chcp 65001 >nul
echo 开始编译餐具检测项目...

REM 创建build目录
if not exist "build" mkdir build
cd build

REM 使用CMake生成项目文件
echo 生成项目文件...
cmake .. -G "Visual Studio 16 2019" -A x64

REM 编译项目
echo 编译项目...
cmake --build . --config Release

echo 编译完成！

# 检查是否编译成功
if exist Release\tableware_detection.exe (
    echo 可执行文件位于: build\Release\tableware_detection.exe
    echo.
    echo 使用方法: tableware_detection.exe 图像文件路径
    echo 示例: tableware_detection.exe tableware.jpg
) else (
    echo 编译失败！请检查错误信息。
)

pause