"""
PCB 缺陷图像数据增强：对指定类别文件夹中的 JPG 做小幅随机旋转。

整体流程：
  - 变量 file 表示缺陷类别名（当前为 Open_circuit），据此拼出「原图目录」与「旋转后输出目录」。
  - 遍历原图目录下所有 .jpg，对每张图在 [-10°, 10°] 内随机取一个整数角度。
  - 调用 rotate_bound_white_bg：在旋转时扩大画布以容纳完整图像，空白区域用固定灰背景色填充，
    避免裁切掉边角像素。
  - 将旋转结果写入「类别名_rotation」目录；同时在「类别名_angles.txt」中追加一行，
    记录「文件名、制表符、角度」，便于后续与标签或训练脚本对齐。

使用前请根据本机环境修改 path1/path2 中的根路径；若需在 Windows 下运行，路径分隔与
glob 模式也可能需要一并调整。
"""
import cv2
from math import *
import numpy as np
import random
import os
import glob as gb

def rotate_bound_white_bg(image, angle):
    # 读取图像高宽，并计算中心点坐标
    (h, w) = image.shape[:2]
    (cX, cY) = (w // 2, h // 2)

    # 获取 2D 旋转矩阵（角度取负以实现顺时针旋转），并取出 sin、cos（矩阵中的旋转分量）
    M = cv2.getRotationMatrix2D((cX, cY), -angle, 1.0)
    cos = np.abs(M[0, 0])
    sin = np.abs(M[0, 1])

    # 计算旋转后外接矩形的新宽高
    nW = int((h * sin) + (w * cos))
    nH = int((h * cos) + (w * sin))

    # 修正旋转矩阵的平移项，使图像围绕新画布中心旋转
    M[0, 2] += (nW / 2) - cX
    M[1, 2] += (nH / 2) - cY

    # 仿射变换完成旋转并返回；borderValue 为填充背景色 (B,G,R)
    # 可选：不指定 borderValue 时默认用黑边填充，可改用下面一行
    # return cv2.warpAffine(image, M, (nW, nH))
    return cv2.warpAffine(image, M, (nW, nH),borderValue=(143,148,151))

file="Open_circuit"
f=open(file+'_angles.txt','a')
out_name=[]
path1 = "/home/weapon/Desktop/PCB_DATASET/"+file
# 调试用：print(path1)
path2 = "/home/weapon/Desktop/PCB_DATASET/"+file+"_rotation"
# 调试用：print(path2)
paths = gb.glob(path1+"/*.jpg")
# 调试用：print(paths)
for path in paths:
    file_name=path.split('/')[-1].split('.')[-2]
    # 调试用：print(file_name)
    angle=random.randint(-10,10)
    img = cv2.imread(path)

    imgRotation = rotate_bound_white_bg(img, angle)

	# 调试用：print(angle)
    f.write(str(file_name) + '\t' +  str(angle) + '\n')
    cv2.imwrite(path2+"/"+file_name+".jpg",imgRotation)
	# 调试用：显示原图与旋转结果（需取消下面三行注释）
	#cv2.imshow("img",img)
	#cv2.imshow("imgRotation",imgRotation)
	#cv2.waitKey(0)
 	
