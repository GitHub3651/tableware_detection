cmake_minimum_required(VERSION 3.10)
project(TablewareDetection)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编码为UTF-8以支持中文
if(WIN32)
    add_compile_options(/utf-8)
endif()

# 设置OpenCV路径 (指向build目录中的OpenCV配置)
set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/opencv/build")

# 查找OpenCV包
find_package(OpenCV REQUIRED)

# 包含头文件目录
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(include)

# 创建可执行文件 - 餐具检测程序
add_executable(tableware_detection 
    src/main.cpp
    src/image_processing.cpp
    src/display.cpp
    # src/hsv_lut.cpp  # LUT功能暂时不使用，但保留文件
)

# 链接OpenCV库
target_link_libraries(tableware_detection ${OpenCV_LIBS})

# 添加post-build命令，自动复制OpenCV DLL文件
if(WIN32)
    # 定义OpenCV DLL源路径
    set(OPENCV_DLL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/opencv/build/x64/vc16/bin")
    
    # 复制OpenCV DLL到输出目录
    add_custom_command(TARGET tableware_detection POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENCV_DLL_DIR}/opencv_world481.dll"
            $<TARGET_FILE_DIR:tableware_detection>
        COMMENT "复制 opencv_world481.dll 到输出目录"
    )
    
    add_custom_command(TARGET tableware_detection POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENCV_DLL_DIR}/opencv_videoio_ffmpeg481_64.dll"
            $<TARGET_FILE_DIR:tableware_detection>
        COMMENT "复制 opencv_videoio_ffmpeg481_64.dll 到输出目录"
    )
endif()

# 输出信息
message(STATUS "OpenCV版本: ${OpenCV_VERSION}")
message(STATUS "OpenCV包含目录: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV库: ${OpenCV_LIBS}")
message(STATUS "OpenCV DLL将自动复制到输出目录")