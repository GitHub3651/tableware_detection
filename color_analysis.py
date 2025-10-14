#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
HSV颜色分析工具
用于分析餐具图片的颜色分布，帮助确定合适的HSV阈值
"""

import cv2
import numpy as np
import sys
import os
from pathlib import Path

def analyze_hsv_colors(image, image_path):
    """分析图片中的HSV颜色分布"""
    
    # 转换到HSV色彩空间
    hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)
    
    print(f"\n=== 分析图片: {image_path} ===")
    print(f"图片尺寸: {image.shape[1]}x{image.shape[0]}")
    
    # 计算HSV直方图
    h_hist = cv2.calcHist([hsv], [0], None, [180], [0, 180])
    s_hist = cv2.calcHist([hsv], [1], None, [256], [0, 256])
    v_hist = cv2.calcHist([hsv], [2], None, [256], [0, 256])
    
    # 找到主要颜色峰值
    h_peak = np.argmax(h_hist)
    s_peak = np.argmax(s_hist)
    v_peak = np.argmax(v_hist)
    
    print(f"\n主要颜色峰值:")
    print(f"H (色调) 峰值: {h_peak} (范围: 0-179)")
    print(f"S (饱和度) 峰值: {s_peak} (范围: 0-255)")
    print(f"V (明度) 峰值: {v_peak} (范围: 0-255)")
    
    # 分析不同HSV范围的像素分布
    print(f"\n像素分布分析:")
    
    color_ranges = [
        ("黑色/深灰", (0, 0, 0), (180, 255, 60)),
        ("白色/浅灰", (0, 0, 180), (180, 30, 255)),
        ("红色(低)", (0, 50, 50), (10, 255, 255)),
        ("红色(高)", (170, 50, 50), (180, 255, 255)),
        ("橙色", (10, 50, 50), (25, 255, 255)),
        ("黄色", (25, 50, 50), (35, 255, 255)),
        ("绿色", (35, 50, 50), (85, 255, 255)),
        ("青色", (85, 50, 50), (125, 255, 255)),
        ("蓝色", (125, 50, 50), (155, 255, 255)),
        ("紫色", (155, 50, 50), (170, 255, 255)),
        ("木色/棕色", (10, 30, 80), (30, 180, 200)),
        ("深木色", (8, 30, 50), (25, 150, 120)),
    ]
    
    total_pixels = image.shape[0] * image.shape[1]
    
    significant_colors = []
    
    for name, lower, upper in color_ranges:
        mask = cv2.inRange(hsv, np.array(lower), np.array(upper))
        pixel_count = cv2.countNonZero(mask)
        percentage = (pixel_count / total_pixels) * 100
        
        if percentage > 1.0:  # 只显示占比超过1%的颜色
            print(f"{name}: {percentage:.1f}% ({pixel_count} pixels)")
            print(f"  HSV范围: H({lower[0]}-{upper[0]}) S({lower[1]}-{upper[1]}) V({lower[2]}-{upper[2]})")
            significant_colors.append((name, percentage, lower, upper))
    
    # 中心区域采样分析
    print(f"\n中心区域采样 (40x40像素):")
    center_y, center_x = image.shape[0] // 2, image.shape[1] // 2
    sample_size = 20
    
    y1 = max(0, center_y - sample_size)
    y2 = min(image.shape[0], center_y + sample_size)
    x1 = max(0, center_x - sample_size)
    x2 = min(image.shape[1], center_x + sample_size)
    
    center_region = hsv[y1:y2, x1:x2]
    
    if center_region.size > 0:
        # 计算中心区域的平均HSV值
        mean_hsv = np.mean(center_region.reshape(-1, 3), axis=0)
        avg_h, avg_s, avg_v = int(mean_hsv[0]), int(mean_hsv[1]), int(mean_hsv[2])
        
        print(f"中心区域平均HSV: H={avg_h}, S={avg_s}, V={avg_v}")
        
        # 基于中心区域建议HSV阈值范围
        h_tolerance = 15
        s_tolerance = 50  
        v_tolerance = 50
        
        h_min = max(0, avg_h - h_tolerance)
        h_max = min(179, avg_h + h_tolerance)
        s_min = max(0, avg_s - s_tolerance)
        s_max = min(255, avg_s + s_tolerance)
        v_min = max(0, avg_v - v_tolerance)
        v_max = min(255, avg_v + v_tolerance)
        
        print(f"建议HSV阈值 (基于中心区域):")
        print(f"  H: {h_min}-{h_max}")
        print(f"  S: {s_min}-{s_max}")
        print(f"  V: {v_min}-{v_max}")
        print(f"  配置格式: {{{h_min},{h_max},{s_min},{s_max},{v_min},{v_max}}}")
    
    # 分析主要颜色区域
    print(f"\n主要颜色区域分析:")
    if significant_colors:
        # 按占比排序
        significant_colors.sort(key=lambda x: x[1], reverse=True)
        
        print("建议的HSV检测范围：")
        for i, (name, percentage, lower, upper) in enumerate(significant_colors[:3]):
            print(f"{i+1}. {name} ({percentage:.1f}%): {{{lower[0]},{upper[0]},{lower[1]},{upper[1]},{lower[2]},{upper[2]}}}")
    
    return hsv, significant_colors

def create_visualization(image, hsv, significant_colors, image_path):
    """创建颜色分析可视化"""
    
    # 创建结果显示窗口
    result_height = 600
    result_width = 800
    
    # 原图缩放
    scale = min(result_width / image.shape[1], result_height / image.shape[0]) * 0.4
    new_width = int(image.shape[1] * scale)
    new_height = int(image.shape[0] * scale)
    
    resized_image = cv2.resize(image, (new_width, new_height))
    
    # 创建结果图像
    result = np.zeros((result_height, result_width, 3), dtype=np.uint8)
    
    # 放置原图
    y_offset = 10
    x_offset = 10
    result[y_offset:y_offset+new_height, x_offset:x_offset+new_width] = resized_image
    
    # 显示主要颜色的掩码
    mask_width = 150
    mask_height = 100
    
    for i, (name, percentage, lower, upper) in enumerate(significant_colors[:3]):
        mask = cv2.inRange(hsv, np.array(lower), np.array(upper))
        mask_colored = cv2.applyColorMap(mask, cv2.COLORMAP_JET)
        mask_resized = cv2.resize(mask_colored, (mask_width, mask_height))
        
        y_pos = y_offset + (mask_height + 10) * i
        x_pos = x_offset + new_width + 20
        
        if y_pos + mask_height <= result_height and x_pos + mask_width <= result_width:
            result[y_pos:y_pos+mask_height, x_pos:x_pos+mask_width] = mask_resized
            
            # 添加文字标签
            cv2.putText(result, f"{name} {percentage:.1f}%", 
                       (x_pos, y_pos - 5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    return result

def main():
    # 直接在代码中指定要分析的图片路径
    image_paths = [
        "image_samples/1/2 (1).jpg",
        "image_samples/1/2 (2).jpg",
        "image_samples/1/2 (3).jpg",
        # 可以添加更多图片路径
    ]
    
    print("=== HSV颜色分析工具 ===")
    
    for i, image_path in enumerate(image_paths, 1):
        if not os.path.exists(image_path):
            print(f"错误: 文件不存在 {image_path}")
            continue
            
        image = cv2.imread(image_path)
        if image is None:
            print(f"错误: 无法加载图片 {image_path}")
            continue
        
        # 分析颜色
        hsv, significant_colors = analyze_hsv_colors(image, image_path)
        
        # 创建可视化
        result = create_visualization(image, hsv, significant_colors, image_path)
        
        # 显示结果
        window_name = f"Color Analysis - {Path(image_path).name}"
        cv2.imshow(window_name, result)
        
        if i < len(image_paths):
            print(f"\n按任意键继续下一张图片...")
            cv2.waitKey(0)
        else:
            print(f"\n按任意键退出...")
            cv2.waitKey(0)
    
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()