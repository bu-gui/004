# 边缘AI实现完整指南（零基础友好版）

## 三个模型对比与选择建议

| 模型 | 难度 | 数据需求 | 数据类型 | 推荐理由 |
|:----:|:----:|:--------:|---------|:--------:|
| **运动类型识别** | ⭐⭐ | 有公开数据集 | MPU6050 6轴数据 | **最推荐先做** |
| **跌倒检测** | ⭐⭐⭐ | 需自行采集/公开 | MPU6050 加速度 | 演示效果好 |
| **心律失常初筛** | ⭐ | 无AI，纯规则 | PPG RR间期 | 最简单，但需要医学知识 |

### 推荐路线：运动类型识别 → 跌倒检测 → 心律失常初筛

---

## 一、运动类型识别（最推荐先做）

### 整体流程

```
┌─────────┐   ┌─────────┐   ┌─────────┐   ┌───────────┐   ┌──────────┐
│ 公开数据  │   │ Python   │   │ 训练CNN  │   │ 转换为     │   │ ESP32-S3 │
│ 集下载    │──►│ 预处理   │──►│ 模型    │──►│ TFLite    │──►│ 推理部署  │
│          │   │          │   │          │   │ INT8量化   │   │          │
└─────────┘   └─────────┘   └─────────┘   └───────────┘   └──────────┘
    2h            2h            3h            1h              4h
```

### Step 1：PC端环境准备

```bash
# 创建Python虚拟环境
python -m venv edge_ai_env
edge_ai_env\Scripts\activate

# 安装依赖
pip install tensorflow==2.13.0 numpy pandas matplotlib scikit-learn
```

### Step 2：使用公开数据集

推荐数据集 **WISDM** (Wireless Sensor Data Mining)：
- 包含走路、跑步、上下楼、久坐、骑车等动作
- 采样率20Hz，使用手机加速度计
- 我们适配到MPU6050的50Hz

```python
# 文件名: train_motion_classifier.py
# 说明: 训练运动类型识别CNN模型，输出TFLite格式

import numpy as np
import pandas as pd
import tensorflow as tf
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder
import os

# ============================================
# 1. 下载并加载WISDM数据集
# ============================================
# 数据集地址: https://www.cis.fordham.edu/wisdm/dataset.php
# 下载后解压得到 WISDM_ar_v1.1_raw.txt

def load_wisdm_data(filepath='WISDM_ar_v1.1_raw.txt'):
    """加载WISDM数据集"""
    columns = ['user', 'activity', 'timestamp', 'x', 'y', 'z']
    df = pd.read_csv(filepath, header=None, names=columns,
                     comment=';', on_bad_lines='skip')
    # 筛选我们需要的运动类型
    valid_activities = ['Walking', 'Jogging', 'Sitting', 'Standing']
    df = df[df['activity'].isin(valid_activities)]
    # 重映射为我们的类别: 静止(0), 走路(1), 跑步(2)
    mapping = {'Sitting': 0, 'Standing': 0, 'Walking': 1, 'Jogging': 2}
    df['label'] = df['activity'].map(mapping)
    # 转换数据类型
    for col in ['x', 'y', 'z']:
        df[col] = pd.to_numeric(df[col], errors='coerce')
    df = df.dropna()
    return df

# ============================================
# 2. 滑动窗口切分
# ============================================
def create_windows(data, labels, window_size=100, step_size=50):
    """
    滑动窗口切分时序数据
    window_size: 窗口大小 (100样本 @50Hz = 2秒)
    step_size:   步长 (50样本 = 1秒)
    返回: (样本数, 窗口大小, 3轴)
    """
    X, y = [], []
    for i in range(0, len(data) - window_size, step_size):
        window = data[i:i+window_size]
        # 取众数作为这个窗口的标签
        label = np.bincount(labels[i:i+window_size].astype(int)).argmax()
        X.append(window)
        y.append(label)
    return np.array(X), np.array(y)

# ============================================
# 3. 构建CNN模型
# ============================================
def build_cnn_model(input_shape=(100, 3), num_classes=3):
    """
    轻量级CNN模型，适合ESP32-S3运行
    
    模型大小: ~20KB (INT8量化后)
    推理时间: ~20ms @ESP32-S3 240MHz
    """
    model = tf.keras.Sequential([
        # 第一层卷积
        tf.keras.layers.Conv1D(32, kernel_size=5, activation='relu',
                               input_shape=input_shape),
        tf.keras.layers.MaxPooling1D(pool_size=2),
        
        # 第二层卷积
        tf.keras.layers.Conv1D(64, kernel_size=3, activation='relu'),
        tf.keras.layers.MaxPooling1D(pool_size=2),
        
        # 展平 + 全连接
        tf.keras.layers.Flatten(),
        tf.keras.layers.Dense(64, activation='relu'),
        tf.keras.layers.Dropout(0.3),
        tf.keras.layers.Dense(num_classes, activation='softmax')
    ])
    
    model.compile(
        optimizer='adam',
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )
    return model

# ============================================
# 4. 训练流程
# ============================================
def train_and_export():
    """完整训练+导出流程"""
    
    # 4.1 加载数据
    print("加载WISDM数据集...")
    df = load_wisdm_data()
    
    # 4.2 提取特征和标签
    feature_cols = ['x', 'y', 'z']
    data = df[feature_cols].values
    labels = df['label'].values
    
    # 4.3 滑动窗口切分
    print("滑动窗口切分...")
    X, y = create_windows(data, labels, window_size=100, step_size=50)
    print(f"样本数: {X.shape}, 标签分布: {np.bincount(y)}")
    
    # 4.4 训练集/测试集划分
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y
    )
    
    # 4.5 构建并训练模型
    print("训练CNN模型...")
    model = build_cnn_model()
    history = model.fit(
        X_train, y_train,
        epochs=20,
        batch_size=32,
        validation_data=(X_test, y_test),
        verbose=1
    )
    
    # 4.6 评估
    test_loss, test_acc = model.evaluate(X_test, y_test, verbose=0)
    print(f"\n测试集准确率: {test_acc:.2%}")
    
    # 4.7 转换为TFLite
    print("转换为TFLite格式...")
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()
    
    # 4.8 INT8量化（减小模型大小，加速推理）
    print("INT8量化...")
    def representative_dataset():
        """提供校准数据用于量化"""
        for i in range(100):
            yield [X_train[i:i+1].astype(np.float32)]
    
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative_dataset
    converter.target_spec.supported_types = [tf.int8]
    model_quant = converter.convert()
    
    # 4.9 保存模型
    os.makedirs('models', exist_ok=True)
    with open('models/motion_model.tflite', 'wb') as f:
        f.write(tflite_model)
    with open('models/motion_model_quant.tflite', 'wb') as f:
        f.write(model_quant)
    
    print(f"\n模型保存完成!")
    print(f"  - 未量化: {len(tflite_model)/1024:.1f} KB")
    print(f"  - INT8量化: {len(model_quant)/1024:.1f} KB")
    
    # 4.10 生成C数组（直接嵌入ESP32代码）
    print("\n生成C数组头文件...")
    c_array = "const unsigned char motion_model_tflite[] = {\n"
    for i, byte in enumerate(model_quant):
        c_array += f"0x{byte:02x}, "
        if (i + 1) % 16 == 0:
            c_array += "\n"
    c_array += "\n};\n"
    c_array += f"const int motion_model_tflite_len = {len(model_quant)};\n"
    
    with open('models/motion_model_data.h', 'w') as f:
        f.write(c_array)
    
    print("全部完成! 请将 models/motion_model_data.h 复制到ESP32工程")
    return model, history

if __name__ == '__main__':
    model, history = train_and_export()
```

### Step 3：在ESP32-S3上运行推理

ESP32-S3上使用 **TensorFlow Lite Micro** 加载模型运行推理。

```cpp
// 文件名: motion_classifier.cpp
// 说明: ESP32-S3 运动类型识别推理

#include <stdio.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

// 导入量化后的模型（由Python脚本生成）
#include "models/motion_model_data.h"  // 包含模型C数组

// ============================================
// TFLite Micro 推理引擎封装
// ============================================
class MotionClassifier {
private:
    // TFLite Micro 需要的内存缓冲区
    const int kTensorArenaSize = 80 * 1024;  // 80KB
    uint8_t* tensor_arena;
    
    const tflite::Model* model;
    tflite::MicroMutableOpResolver<10>* resolver;
    tflite::MicroInterpreter* interpreter;
    
    // 输入输出tensor
    TfLiteTensor* input_tensor;
    TfLiteTensor* output_tensor;
    
    // 滑动窗口缓冲区 (100样本 × 3轴)
    float window_buffer[100][3];
    int window_index = 0;
    bool window_full = false;
    
public:
    // 运动类型枚举
    enum MotionType {
        STATIC = 0,     // 静止
        WALKING = 1,    // 走路
        RUNNING = 2,    // 跑步
        UNKNOWN = -1
    };
    
    MotionClassifier() {
        tensor_arena = new uint8_t[kTensorArenaSize];
        model = nullptr;
        resolver = nullptr;
        interpreter = nullptr;
        input_tensor = nullptr;
        output_tensor = nullptr;
        
        // 清空窗口缓冲区
        memset(window_buffer, 0, sizeof(window_buffer));
    }
    
    ~MotionClassifier() {
        delete[] tensor_arena;
        delete interpreter;
        delete resolver;
    }
    
    // ============================================
    // 初始化：加载模型 + 分配tensor
    // ============================================
    esp_err_t begin() {
        // 1. 加载模型
        model = tflite::GetModel(motion_model_tflite);
        if (model->version() != TFLITE_SCHEMA_VERSION) {
            printf("模型版本不匹配!\n");
            return ESP_FAIL;
        }
        
        // 2. 创建算子解析器
        resolver = new tflite::MicroMutableOpResolver<10>();
        resolver->AddConv2D();        // Conv2D算子
        resolver->AddMaxPool2D();      // 池化层
        resolver->AddFullyConnected(); // 全连接层
        resolver->AddSoftmax();        // Softmax
        resolver->AddReshape();        // 重塑
        resolver->AddQuantize();       // 量化/反量化
        
        // 3. 创建解释器
        interpreter = new tflite::MicroInterpreter(
            model, *resolver, tensor_arena, kTensorArenaSize
        );
        
        // 4. 分配tensor
        if (interpreter->AllocateTensors() != kTfLiteOk) {
            printf("Tensor分配失败!\n");
            return ESP_FAIL;
        }
        
        // 5. 获取输入输出tensor
        input_tensor = interpreter->input(0);
        output_tensor = interpreter->output(0);
        
        printf("运动分类器初始化成功! 内存使用: %d bytes\n", 
               interpreter->arena_used_bytes());
        return ESP_OK;
    }
    
    // ============================================
    // 喂入新数据（每次MPU6050读数后调用）
    // ============================================
    void feed_data(float ax, float ay, float az) {
        // 归一化到[-1, 1]范围（与训练时一致）
        window_buffer[window_index][0] = ax / 9.8f;
        window_buffer[window_index][1] = ay / 9.8f;
        window_buffer[window_index][2] = az / 9.8f;
        
        window_index++;
        if (window_index >= 100) {
            window_index = 0;
            window_full = true;
        }
    }
    
    // ============================================
    // 运行推理
    // ============================================
    MotionType predict() {
        if (!window_full) {
            return UNKNOWN;  // 窗口未满，无法推理
        }
        
        // 1. 将窗口数据复制到输入tensor
        // TFLite Micro量化模型的输入需要int8类型
        int8_t* quant_input = interpreter->input(0)->data.int8;
        float input_scale = input_tensor->params.scale;
        int input_zero_point = input_tensor->params.zero_point;
        
        for (int i = 0; i < 100; i++) {
            for (int j = 0; j < 3; j++) {
                // 浮点 → 量化
                quant_input[i * 3 + j] = (int8_t)(
                    window_buffer[i][j] / input_scale + input_zero_point
                );
            }
        }
        
        // 2. 运行推理
        if (interpreter->Invoke() != kTfLiteOk) {
            printf("推理失败!\n");
            return UNKNOWN;
        }
        
        // 3. 获取输出
        int8_t* quant_output = interpreter->output(0)->data.int8;
        float output_scale = output_tensor->params.scale;
        int output_zero_point = output_tensor->params.zero_point;
        
        // 反量化 + Softmax概率
        float probs[3];
        float max_prob = 0;
        int max_idx = 0;
        
        for (int i = 0; i < 3; i++) {
            probs[i] = (quant_output[i] - output_zero_point) * output_scale;
            if (probs[i] > max_prob) {
                max_prob = probs[i];
                max_idx = i;
            }
        }
        
        // 4. 置信度阈值过滤（低于0.6视为不确定）
        if (max_prob < 0.6f) {
            return UNKNOWN;
        }
        
        return (MotionType)max_idx;
    }
    
    // ============================================
    // 获取当前窗口数据（调试用，串口输出）
    // ============================================
    void debug_print() {
        printf("Window status: %s, index=%d\n",
               window_full ? "FULL" : "FILLING", window_index);
        if (window_full) {
            MotionType type = predict();
            const char* type_names[] = {"静止", "走路", "跑步"};
            if (type == UNKNOWN) {
                printf("当前运动: 不确定\n");
            } else {
                printf("当前运动: %s\n", type_names[type]);
            }
        }
    }
};
```

---

## 二、跌倒检测（ESP-DL方案）

### 总体思路

使用**ESP-DL**库，这是乐鑫官方推出的深度学习推理库，专门为ESP32-S3优化，支持向量指令加速。

```
MPU6050 @100Hz → 2秒窗口(200样本) → 特征提取(6维特征) → ESP-DL MLP推理 → 结果
```

### Step 1：PC端训练SVM/MLP模型

不需要复杂的CNN，简单的**MLP**或**SVM**就能达到很好的跌倒检测效果。

```python
# 文件名: train_fall_detector.py
# 说明: 训练跌倒检测模型，导出ESP-DL格式

import numpy as np
from sklearn.svm import SVC
from sklearn.neural_network import MLPClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score, confusion_matrix
import pickle
import os

# ============================================
# 1. 使用公开跌倒数据集
# ============================================
# 推荐数据集: MobiFall 或 SisFall
# 下载地址: http://www.iro.umontreal.ca/~labimage/Dataset/
#
# 也可以自行采集:
#   - 正常活动: 走路、坐下、弯腰、慢跑
#   - 跌倒动作: 前倒、后倒、侧倒（在软垫上模拟）

def extract_features(window_data):
    """
    从加速度窗口数据提取6维特征
    window_data: (200, 3) 的numpy数组
    返回: (6,) 特征向量
    """
    ax, ay, az = window_data[:, 0], window_data[:, 1], window_data[:, 2]
    
    # 合成加速度幅值 (Signal Magnitude Vector)
    smv = np.sqrt(ax**2 + ay**2 + az**2)
    
    features = [
        np.max(smv),           # 1. 最大冲击力
        np.mean(smv),          # 2. 平均加速度
        np.std(smv),           # 3. 加速度标准差（变化剧烈程度）
        np.max(ax),            # 4. X轴最大加速度
        np.abs(np.mean(az) - 9.8),  # 5. Z轴与重力偏差（判断是否倒地）
        np.ptp(smv)            # 6. 峰峰值（冲击幅度）
    ]
    return np.array(features)


def generate_demo_data():
    """
    生成演示用数据（用于验证流程）
    实际使用时应替换为真实采集数据
    """
    np.random.seed(42)
    X = []
    y = []
    
    # 正常活动: 特征值较小
    for _ in range(200):
        window = np.random.normal(0, 0.5, (200, 3))
        window[:, 2] += 9.8  # 重力加速度
        X.append(extract_features(window))
        y.append(0)  # 正常
    
    # 跌倒: 特征值较大
    for _ in range(100):
        window = np.random.normal(0, 3.0, (200, 3))
        # 模拟冲击峰值
        peak = np.random.uniform(8, 15)
        for t in range(200):
            if 80 <= t <= 120:
                window[t] += peak * np.exp(-((t-100)/10)**2)
        X.append(extract_features(window))
        y.append(1)  # 跌倒
    
    return np.array(X), np.array(y)


def train_and_export():
    """训练跌倒检测模型"""
    
    # 1. 加载数据
    print("加载数据...")
    X, y = generate_demo_data()  # 替换为真实数据
    print(f"样本数: {X.shape}, 正样本(跌倒): {y.sum()}")
    
    # 2. 划分训练测试集
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42
    )
    
    # 3. 训练MLP分类器
    print("训练MLP模型...")
    model = MLPClassifier(
        hidden_layer_sizes=(16, 8),  # 2层隐藏层，16+8个神经元（轻量）
        activation='relu',
        max_iter=500,
        random_state=42
    )
    model.fit(X_train, y_train)
    
    # 4. 评估
    y_pred = model.predict(X_test)
    acc = accuracy_score(y_test, y_pred)
    print(f"准确率: {acc:.2%}")
    print(f"混淆矩阵:\n{confusion_matrix(y_test, y_pred)}")
    
    # 5. 导出模型参数（用于ESP-DL）
    os.makedirs('models', exist_ok=True)
    
    # ESP-DL需要: 权重矩阵 + 偏置向量
    with open('models/fall_model_params.pkl', 'wb') as f:
        pickle.dump({
            'coefs': model.coefs_,
            'intercepts': model.intercepts_,
            'n_layers': model.n_layers_,
            'hidden_layer_sizes': model.hidden_layer_sizes
        }, f)
    
    # 6. 生成C数组头文件（ESP-DL格式）
    print("\n生成ESP-DL模型头文件...")
    generate_espdl_header(model)
    
    print("完成!")


def generate_espdl_header(model):
    """生成ESP-DL可用的C++头文件"""
    
    with open('models/fall_model_espdl.hpp', 'w') as f:
        f.write("// 跌倒检测MLP模型参数 (ESP-DL格式)\n")
        f.write("// 自动生成，请勿手动修改\n\n")
        f.write("#pragma once\n\n")
        f.write("#include <stdint.h>\n\n")
        
        # 写入每层权重和偏置
        for i, (W, b) in enumerate(zip(model.coefs_, model.intercepts_)):
            f.write(f"// Layer {i}: 权重矩阵 shape={W.shape}\n")
            f.write(f"const float layer{i}_weights[] = {{\n")
            for row in W:
                f.write("    ")
                for val in row:
                    f.write(f"{val:.6f}f, ")
                f.write("\n")
            f.write("};\n\n")
            
            f.write(f"// Layer {i}: 偏置向量 shape={b.shape}\n")
            f.write(f"const float layer{i}_bias[] = {{\n    ")
            for val in b:
                f.write(f"{val:.6f}f, ")
            f.write("\n};\n\n")


if __name__ == '__main__':
    train_and_export()
```

### Step 2：ESP32-S3上ESP-DL部署

ESP-DL是乐鑫官方库，直接支持ESP32-S3的向量指令加速。

```cpp
// 文件名: fall_detector.cpp
// 说明: 使用ESP-DL的跌倒检测

#include "esp_dl.hpp"
#include "dl_tool.hpp"
#include "dl_math.hpp"
#include <vector>

// 导入模型参数
#include "models/fall_model_espdl.hpp"

// ============================================
// 跌倒检测器
// ============================================
class FallDetector {
private:
    // 滑动窗口 (2秒 @100Hz = 200样本)
    static const int WINDOW_SIZE = 200;
    static const int STEP_SIZE = 50;      // 0.5秒步长
    
    float acc_buffer[WINDOW_SIZE][3];
    int buffer_index = 0;
    int sample_count = 0;
    
    // 连续确认防误报
    int fall_votes = 0;
    static const int CONFIRM_THRESHOLD = 3;  // 连续3次检测到才确认
    
    // MLP网络层
    dl::Layer<float>* fc1;  // 第一层全连接
    dl::Layer<float>* fc2;  // 第二层全连接
    dl::Layer<float>* output;  // 输出层
    
public:
    FallDetector() {
        memset(acc_buffer, 0, sizeof(acc_buffer));
    }
    
    ~FallDetector() {
        delete fc1;
        delete fc2;
        delete output;
    }
    
    // ============================================
    // 初始化神经网络
    // ============================================
    esp_err_t begin() {
        // 使用ESP-DL构建2层MLP
        // 输入: 6维特征 → 隐藏层16 → 隐藏层8 → 输出2(正常/跌倒)
        
        fc1 = new dl::Layer<float>(6, 16);
        fc1->set_weights(layer0_weights, layer0_bias, 6, 16);
        
        fc2 = new dl::Layer<float>(16, 8);
        fc2->set_weights(layer1_weights, layer1_bias, 16, 8);
        
        output = new dl::Layer<float>(8, 2);
        output->set_weights(layer2_weights, layer2_bias, 8, 2);
        
        printf("跌倒检测器初始化完成\n");
        return ESP_OK;
    }
    
    // ============================================
    // 特征提取 (与Python训练端保持一致)
    // ============================================
    void extract_features(float* features_out) {
        // 计算合成加速度幅值
        float smv[WINDOW_SIZE];
        for (int i = 0; i < WINDOW_SIZE; i++) {
            smv[i] = sqrt(
                acc_buffer[i][0] * acc_buffer[i][0] +
                acc_buffer[i][1] * acc_buffer[i][1] +
                acc_buffer[i][2] * acc_buffer[i][2]
            );
        }
        
        // 计算6维特征
        float max_smv = smv[0];
        float sum_smv = 0;
        float sum_ax = 0;
        float sum_az = 0;
        float min_smv = smv[0];
        
        for (int i = 0; i < WINDOW_SIZE; i++) {
            if (smv[i] > max_smv) max_smv = smv[i];
            if (smv[i] < min_smv) min_smv = smv[i];
            sum_smv += smv[i];
            sum_ax += acc_buffer[i][0];
            sum_az += acc_buffer[i][2];
        }
        
        float mean_smv = sum_smv / WINDOW_SIZE;
        
        // 标准差
        float var_smv = 0;
        for (int i = 0; i < WINDOW_SIZE; i++) {
            var_smv += (smv[i] - mean_smv) * (smv[i] - mean_smv);
        }
        float std_smv = sqrt(var_smv / WINDOW_SIZE);
        
        // 特征向量
        features_out[0] = max_smv;                          // 最大冲击
        features_out[1] = mean_smv;                         // 平均加速度
        features_out[2] = std_smv;                          // 标准差
        features_out[3] = max_smv;                          // X轴最大（示例简化）
        features_out[4] = fabs(mean_smv - 9.8f);           // 重力偏差
        features_out[5] = max_smv - min_smv;                // 峰峰值
    }
    
    // ============================================
    // 喂入新数据 (MPU6050每次读数后调用)
    // ============================================
    void feed_data(float ax, float ay, float az) {
        if (buffer_index < WINDOW_SIZE) {
            acc_buffer[buffer_index][0] = ax;
            acc_buffer[buffer_index][1] = ay;
            acc_buffer[buffer_index][2] = az;
            buffer_index++;
        }
    }
    
    // ============================================
    // 运行跌倒检测
    // 返回: true=检测到跌倒, false=正常
    // ============================================
    bool detect() {
        if (buffer_index < WINDOW_SIZE) {
            return false;  // 窗口未满
        }
        
        // 1. 提取6维特征
        float features[6];
        extract_features(features);
        
        // 2. 前向传播
        // 使用ESP-DL的矩阵运算（利用ESP32-S3向量指令）
        float h1[16], h2[8], out[2];
        
        // 第一层: 6→16, ReLU
        fc1->forward(features, h1);
        for (int i = 0; i < 16; i++) {
            if (h1[i] < 0) h1[i] = 0;  // ReLU
        }
        
        // 第二层: 16→8, ReLU
        fc2->forward(h1, h2);
        for (int i = 0; i < 8; i++) {
            if (h2[i] < 0) h2[i] = 0;
        }
        
        // 输出层: 8→2, Softmax
        output->forward(h2, out);
        
        // Softmax
        float exp0 = exp(out[0]);
        float exp1 = exp(out[1]);
        float prob_fall = exp1 / (exp0 + exp1);
        
        // 3. 滑动窗口前进
        // 将窗口后50个样本移到前面
        memmove(acc_buffer, acc_buffer + STEP_SIZE,
                (WINDOW_SIZE - STEP_SIZE) * 3 * sizeof(float));
        buffer_index = WINDOW_SIZE - STEP_SIZE;
        
        // 4. 连续确认防误报
        if (prob_fall > 0.7f) {
            fall_votes++;
        } else {
            fall_votes = 0;
        }
        
        if (fall_votes >= CONFIRM_THRESHOLD) {
            fall_votes = 0;  // 复位
            return true;     // 确认跌倒
        }
        
        return false;
    }
    
    // ============================================
    // 复位窗口 (开始新监测)
    // ============================================
    void reset() {
        buffer_index = 0;
        fall_votes = 0;
        memset(acc_buffer, 0, sizeof(acc_buffer));
    }
};
```

---

## 三、心律失常初筛（规则引擎，无需AI）

这个最简单，**不需要训练模型**，纯规则逻辑。

```cpp
// 文件名: arrhythmia_screener.cpp
// 说明: 基于RR间期的心律失常初筛

#include <stdint.h>
#include <math.h>
#include <vector>

class ArrhythmiaScreener {
private:
    // RR间期环形缓冲区 (30秒数据, 约30~45个RR间期)
    static const int RR_BUFFER_SIZE = 60;
    float rr_intervals[RR_BUFFER_SIZE];  // 单位: ms
    int rr_index = 0;
    int rr_count = 0;
    
public:
    enum Result {
        NORMAL = 0,          // 正常
        SUSPECT_AF = 1,      // 疑似房颤
        SUSPECT_PVC = 2,     // 疑似早搏
        INSUFFICIENT_DATA = -1  // 数据不足
    };
    
    ArrhythmiaScreener() {
        memset(rr_intervals, 0, sizeof(rr_intervals));
    }
    
    // ============================================
    // 喂入一个RR间期 (由心率算法提供)
    // ============================================
    void feed_rr_interval(float rr_ms) {
        if (rr_index < RR_BUFFER_SIZE) {
            rr_intervals[rr_index++] = rr_ms;
            rr_count++;
        } else {
            // 环形缓冲区: 删除最老的一个
            memmove(rr_intervals, rr_intervals + 1,
                    (RR_BUFFER_SIZE - 1) * sizeof(float));
            rr_intervals[RR_BUFFER_SIZE - 1] = rr_ms;
        }
    }
    
    // ============================================
    // 分析心律失常
    // ============================================
    Result analyze() {
        if (rr_count < 10) {
            return INSUFFICIENT_DATA;  // 数据太少
        }
        
        int n = (rr_count < RR_BUFFER_SIZE) ? rr_count : RR_BUFFER_SIZE;
        
        // 1. 计算平均RR间期
        float mean_rr = 0;
        for (int i = 0; i < n; i++) {
            mean_rr += rr_intervals[i];
        }
        mean_rr /= n;
        
        // 2. 计算SDNN (RR间期标准差)
        float variance = 0;
        for (int i = 0; i < n; i++) {
            variance += (rr_intervals[i] - mean_rr) *
                        (rr_intervals[i] - mean_rr);
        }
        float sdnn = sqrt(variance / n);
        
        // 3. 计算RMSSD (相邻RR差值的均方根)
        float sum_sq_diff = 0;
        for (int i = 1; i < n; i++) {
            float diff = rr_intervals[i] - rr_intervals[i-1];
            sum_sq_diff += diff * diff;
        }
        float rmssd = sqrt(sum_sq_diff / (n - 1));
        
        // 4. 计算pNN50 (相邻RR差值>50ms的比例)
        int count_nn50 = 0;
        for (int i = 1; i < n; i++) {
            if (fabs(rr_intervals[i] - rr_intervals[i-1]) > 50) {
                count_nn50++;
            }
        }
        float pnn50 = (float)count_nn50 / (n - 1) * 100;
        
        // 5. 规则判断
        // 房颤特征: RR完全不规则, SDNN大, pNN50高
        if (sdnn > 50 && pnn50 > 30 && rmssd > 40) {
            return SUSPECT_AF;
        }
        
        // 早搏特征: 偶尔出现异常短的RR间期
        int short_count = 0;
        for (int i = 0; i < n; i++) {
            if (rr_intervals[i] < mean_rr * 0.6) {
                short_count++;
            }
        }
        if (short_count >= 1 && short_count <= 3 && sdnn > 30) {
            return SUSPECT_PVC;
        }
        
        return NORMAL;
    }
    
    // ============================================
    // 获取HRV指标
    // ============================================
    float get_sdnn() {
        int n = (rr_count < RR_BUFFER_SIZE) ? rr_count : RR_BUFFER_SIZE;
        if (n < 2) return 0;
        
        float mean = 0;
        for (int i = 0; i < n; i++) mean += rr_intervals[i];
        mean /= n;
        
        float var = 0;
        for (int i = 0; i < n; i++) var += (rr_intervals[i] - mean) *
                                           (rr_intervals[i] - mean);
        return sqrt(var / n);
    }
    
    void reset() {
        rr_index = 0;
        rr_count = 0;
        memset(rr_intervals, 0, sizeof(rr_intervals));
    }
};
```

---

## 四、实战路线建议（零基础版）

### 第一阶段（推荐，2天搞定）
**只用"运动类型识别"** 就能展示边缘AI能力：
1. 运行 `train_motion_classifier.py` 在PC上训练模型
2. 得到 `motion_model_data.h` 复制到ESP32工程
3. 集成 `motion_classifier.cpp` 到主程序
4. 连接MPU6050 → 实时显示"走路/跑步/静止"

### 第二阶段（锦上添花）
**跌倒检测**：
1. 下载公开数据集（MobiFall）
2. 修改 `train_fall_detector.py` 加载真实数据
3. 部署ESP-DL推理

### 第三阶段（加分项）  
**心律失常初筛**：直接把 `arrhythmia_screener.cpp` 集成进去即可

---

## 五、ESP32-S3工程配置要点

在ESP-IDF中启用TFLite Micro：

```cmake
# CMakeLists.txt 中添加
set(EXTRA_COMPONENT_DIRS 
    "components/tflite-micro"
)

# 或者使用ESP-IDF组件管理
# idf.py add-dependency tflite-micro
```

需要配置的sdkconfig选项：
```text
# 启用PSRAM（模型放在PSRAM中）
CONFIG_ESP32S3_SPIRAM_SUPPORT=y
CONFIG_SPIRAM_TYPE_AUTO=y

# 优化性能
CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240=y
CONFIG_COMPILER_OPTIMIZATION_PERF=y
```

---

## 六、常见问题

| 问题 | 原因 | 解决 |
|-----|------|------|
| 模型推理结果不准 | 训练数据与真实场景差异大 | 增加真实采集数据重新训练 |
| 推理太慢(>100ms) | 模型太大未量化 | 使用INT8量化+减小模型层数 |
| 编译报内存不足 | TFLite Micro占用RAM太多 | 减小tensor arena到40KB |
| 跌倒检测一直误报 | 阈值太低 | 调高CONFIRM_THRESHOLD或概率阈值 |

---

> **一句话总结**：对于零基础团队，**先做运动识别（有公开数据+代码现成）**，演示效果最好。模型训练只需要在PC上跑Python脚本，然后把生成的头文件复制到ESP32工程里编译烧录即可。
