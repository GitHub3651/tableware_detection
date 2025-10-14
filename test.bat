@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM 配置路径 - 请根据需要修改这两个路径
set IMAGE_PATH=D:\BaiduNetdiskDownload\project\tableware_detection\image_samples\1\2 (1).jpg

set IMAGE_FOLDER_PATH=D:\BaiduNetdiskDownload\project\tableware_detection\image_samples\1
echo ===============================================
echo        餐具检测程序测试脚本
echo ===============================================
echo.
echo 请选择测试模式:
echo 1. 单张图片测试
echo 2. 连续图片测试（遍历文件夹）
echo.
set /p choice=请输入选择 (1 或 2): 

if "%choice%"=="1" goto SINGLE_TEST
if "%choice%"=="2" goto BATCH_TEST
echo 无效选择，退出程序
pause
exit /b 1

:SINGLE_TEST
echo.
echo === 单张图片测试模式 ===
echo 图片路径: !IMAGE_PATH!
echo.

if not exist "build\Release\tableware_detection.exe" (
    echo 错误: 程序未编译，请先运行 build.bat
    pause
    exit /b 1
)

if not exist "!IMAGE_PATH!" (
    echo 错误: 找不到图片: !IMAGE_PATH!
    echo 请在脚本中修改IMAGE_PATH变量
    pause
    exit /b 1
)

echo 开始处理图片...
"build\Release\tableware_detection.exe" "!IMAGE_PATH!"
goto END

:BATCH_TEST
echo.
echo === 连续图片测试模式 ===
echo 文件夹路径: !IMAGE_FOLDER_PATH!
echo.

if not exist "build\Release\tableware_detection.exe" (
    echo 错误: 程序未编译，请先运行 build.bat
    pause
    exit /b 1
)

if not exist "!IMAGE_FOLDER_PATH!" (
    echo 错误: 找不到文件夹: !IMAGE_FOLDER_PATH!
    echo 请在脚本中修改IMAGE_FOLDER_PATH变量
    pause
    exit /b 1
)

echo 扫描图片文件...
set /a count=0
set /a success=0
set /a failed=0

echo.
echo 提示: 在连续处理过程中，您可以按 Ctrl+C 来终止处理
echo.

REM 遍历文件夹中的图片文件
for %%f in ("!IMAGE_FOLDER_PATH!\*.jpg" "!IMAGE_FOLDER_PATH!\*.jpeg" "!IMAGE_FOLDER_PATH!\*.png" "!IMAGE_FOLDER_PATH!\*.bmp" "!IMAGE_FOLDER_PATH!\*.tiff" "!IMAGE_FOLDER_PATH!\*.tif") do (
    if exist "%%f" (
        set /a count+=1
        echo.
        echo [!count!] 正在处理: %%~nxf
        echo 按 Ctrl+C 可以终止处理
        echo ----------------------------------------
        
        REM 调用程序处理每张图片
        "build\Release\tableware_detection.exe" "%%f"
        
        REM 检查程序执行结果
        if !errorlevel! equ 0 (
            set /a success+=1
            echo [成功] %%~nxf 处理完成
        ) else (
            set /a failed+=1
            echo [失败] %%~nxf 处理失败
        )
        echo ----------------------------------------
    )
)

echo.
echo ===============================================
echo              连续处理完成统计
echo ===============================================
echo 总共处理图片: !count! 张
echo 成功处理: !success! 张
echo 处理失败: !failed! 张
if !count! equ 0 (
    echo.
    echo 警告: 在指定文件夹中没有找到图片文件
    echo 支持的格式: .jpg .jpeg .png .bmp .tiff .tif
)
echo ===============================================

:END
echo.
echo 程序执行完成
pause