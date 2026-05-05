"""
智能运动健康手环 - 运动类型识别模型训练脚本
==========================================
使用WISDM公开数据集训练CNN模型
输出: TFLite模型 + C数组头文件(可直接嵌入ESP32工程)

使用步骤:
  1. pip install tensorflow numpy pandas scikit-learn
  2. 下载WISDM数据集到本目录
  3. python train_motion_classifier.py
  4. 将输出的 models/motion_model_data.h 复制到ESP32工程

数据集下载: https://www.cis.fordham.edu/wisdm/dataset.php
"""

import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split
import os
import urllib.request
import zipfile

# ============================================
# 1. 自动下载WISDM数据集
# ============================================
def download_wisdm():
    """如果本地没有数据集，自动下载"""
    if os.path.exists('WISDM_ar_v1.1_raw.txt'):
        print("本地数据集已存在")
        return
    
    print("下载WISDM数据集...")
    url = "https://www.cis.fordham.edu/wisdm/includes/datasets/latest/WISDM_ar_latest.zip"
    try:
        urllib.request.urlretrieve(url, 'wisdm.zip')
        with zipfile.ZipFile('wisdm.zip', 'r') as zip_ref:
            zip_ref.extractall('./')
        print("下载完成")
    except:
        print("自动下载失败，请手动下载WISDM数据集放到本目录")
        print("下载地址: https://www.cis.fordham.edu/wisdm/dataset.php")
        exit(1)


def load_data():
    """加载并预处理数据"""
    download_wisdm()
    
    # 读取数据
    data = []
    labels = []
    
    with open('WISDM_ar_v1.1_raw.txt', 'r') as f:
        for line in f:
            line = line.strip()
            if not line or ';' in line:
                continue
            parts = line.split(',')
            if len(parts) >= 6:
                try:
                    activity = parts[1].strip()
                    x = float(parts[3])
                    y = float(parts[4])
                    z = float(parts[5])
                    
                    # 映射到我们的类别
                    if activity in ['Walking', 'Jogging']:
                        data.append([x, y, z])
                        labels.append(1 if activity == 'Jogging' else 0)
                    elif activity in ['Sitting', 'Standing']:
                        data.append([x, y, z])
                        labels.append(2)  # 静止
                except:
                    continue
    
    return np.array(data), np.array(labels)


def create_windows(data, labels, window_size=50, step_size=25):
    """滑动窗口切分"""
    X, y = [], []
    for i in range(0, len(data) - window_size, step_size):
        X.append(data[i:i+window_size])
        # 窗口标签取众数
        label_window = labels[i:i+window_size]
        values, counts = np.unique(label_window, return_counts=True)
        y.append(values[np.argmax(counts)])
    return np.array(X), np.array(y)


def build_model(input_shape=(50, 3)):
    """轻量级CNN模型"""
    model = tf.keras.Sequential([
        tf.keras.layers.Conv1D(16, 3, activation='relu', input_shape=input_shape),
        tf.keras.layers.MaxPooling1D(2),
        tf.keras.layers.Conv1D(32, 3, activation='relu'),
        tf.keras.layers.MaxPooling1D(2),
        tf.keras.layers.Flatten(),
        tf.keras.layers.Dense(32, activation='relu'),
        tf.keras.layers.Dropout(0.2),
        tf.keras.layers.Dense(3, activation='softmax')  # 0:走路, 1:跑步, 2:静止
    ])
    
    model.compile(optimizer='adam',
                  loss='sparse_categorical_crossentropy',
                  metrics=['accuracy'])
    return model


def convert_to_c_array(tflite_model, output_path='models/motion_model_data.h'):
    """将TFLite模型转换为C数组头文件"""
    os.makedirs('models', exist_ok=True)
    
    c_array = '#pragma once\n\n'
    c_array += '#include <stdint.h>\n\n'
    c_array += 'const unsigned char motion_model_tflite[] = {\n'
    
    for i, byte in enumerate(tflite_model):
        c_array += f'0x{byte:02x}, '
        if (i + 1) % 12 == 0:
            c_array += '\n'
    
    c_array += '\n};\n\n'
    c_array += f'const int motion_model_tflite_len = {len(tflite_model)};\n'
    
    with open(output_path, 'w') as f:
        f.write(c_array)
    
    print(f'模型头文件已生成: {output_path}')
    print(f'模型大小: {len(tflite_model)/1024:.1f} KB')


def main():
    print("=" * 50)
    print("运动类型识别 - 模型训练")
    print("=" * 50)
    
    # 1. 加载数据
    print("\n[1/5] 加载数据...")
    data, labels = load_data()
    print(f"  原始数据: {len(data)} 条")
    
    # 2. 滑动窗口
    print("\n[2/5] 滑动窗口切分...")
    X, y = create_windows(data, labels, window_size=50, step_size=25)
    print(f"  窗口样本: {X.shape[0]} 个")
    print(f"  标签分布: 走路={np.sum(y==0)}, 跑步={np.sum(y==1)}, 静止={np.sum(y==2)}")
    
    # 3. 划分数据集
    print("\n[3/5] 划分训练/测试集...")
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y
    )
    
    # 4. 训练模型
    print("\n[4/5] 训练CNN模型...")
    model = build_model()
    model.fit(X_train, y_train,
              epochs=10,
              batch_size=32,
              validation_data=(X_test, y_test),
              verbose=1)
    
    # 评估
    test_loss, test_acc = model.evaluate(X_test, y_test, verbose=0)
    print(f"\n  测试集准确率: {test_acc:.2%}")
    
    # 5. 导出TFLite
    print("\n[5/5] 导出TFLite模型...")
    
    # 未量化版本
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()
    
    # INT8量化版本
    def representative_dataset():
        for i in range(min(100, len(X_train))):
            yield [X_train[i:i+1].astype(np.float32)]
    
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative_dataset
    converter.target_spec.supported_types = [tf.int8]
    tflite_quant = converter.convert()
    
    # 保存
    os.makedirs('models', exist_ok=True)
    with open('models/motion_model.tflite', 'wb') as f:
        f.write(tflite_model)
    with open('models/motion_model_quant.tflite', 'wb') as f:
        f.write(tflite_quant)
    
    print(f"  未量化: {len(tflite_model)/1024:.1f} KB")
    print(f"  INT8量化: {len(tflite_quant)/1024:.1f} KB")
    
    # 生成C数组
    print("\n生成C数组头文件...")
    convert_to_c_array(tflite_quant)
    
    print("\n" + "=" * 50)
    print("完成! 请将 models/motion_model_data.h 复制到ESP32工程中")
    print("=" * 50)


if __name__ == '__main__':
    main()
