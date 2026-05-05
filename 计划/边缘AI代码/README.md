# 边缘AI模型训练代码

## 文件说明

| 文件 | 用途 | 难度 | 预计耗时 |
|------|------|:----:|:--------:|
| `train_motion_classifier.py` | 训练运动类型识别CNN模型 (TFLite) | ⭐⭐ | 2~3小时 |
| `train_fall_detector.py` | 训练跌倒检测MLP模型 (ESP-DL) | ⭐⭐ | 1~2小时 |

## 使用步骤

### 1. 安装Python环境

```bash
pip install tensorflow numpy pandas scikit-learn
```

### 2. 训练运动识别模型

```bash
python train_motion_classifier.py
```

自动完成: 下载数据 → 训练 → 导出 → 生成头文件

输出文件:
- `models/motion_model.tflite` - 未量化模型
- `models/motion_model_quant.tflite` - INT8量化模型
- `models/motion_model_data.h` - ESP32可直接用的C数组

### 3. 训练跌倒检测模型

```bash
python train_fall_detector.py
```

输出文件:
- `models/fall_model_espdl.hpp` - ESP-DL格式头文件

### 4. 部署到ESP32-S3

将生成的头文件复制到ESP32工程目录，参考《边缘AI_实现指南.md》中的C++代码集成。

## 数据说明

- 运动识别: 自动使用WISDM公开数据集
- 跌倒检测: 默认使用模拟数据，建议替换为MobiFall等真实数据集
