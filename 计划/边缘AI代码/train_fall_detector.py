"""
智能运动健康手环 - 跌倒检测模型训练脚本
==========================================
训练MLP分类器用于跌倒检测
输出: ESP-DL格式的模型参数头文件

使用步骤:
  1. pip install numpy scikit-learn
  2. python train_fall_detector.py
  3. 将输出的 models/fall_model_espdl.hpp 复制到ESP32工程
"""

import numpy as np
from sklearn.neural_network import MLPClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score, confusion_matrix
import os
import json

# ============================================
# 特征提取函数
# ============================================
def extract_features(window_data):
    """
    从加速度窗口提取6维特征
    
    参数:
        window_data: (200, 3) numpy数组, [ax, ay, az] 单位 m/s²
    
    返回:
        (6,) 特征向量
    """
    ax, ay, az = window_data[:, 0], window_data[:, 1], window_data[:, 2]
    
    # 合成加速度幅值 (Signal Magnitude Vector)
    smv = np.sqrt(ax**2 + ay**2 + az**2)
    
    features = [
        np.max(smv),                    # 最大冲击力
        np.mean(smv),                    # 平均加速度
        np.std(smv),                     # 标准差(变化剧烈程度)
        np.max(np.abs(ax)),              # X轴最大绝对值
        np.abs(np.mean(az) - 9.8),       # Z轴与重力偏差(判断是否倒地)
        np.max(smv) - np.min(smv)        # 峰峰值(冲击幅度)
    ]
    return np.array(features)


# ============================================
# 生成演示数据（用于验证流程）
# ============================================
def generate_demo_data():
    """
    生成模拟的加速度数据
    
    正常活动: 小幅波动, 围绕重力加速度
    跌倒动作: 大幅冲击, 峰值可达10-15 m/s²
    
    生产环境请替换为真实采集数据!
    """
    np.random.seed(42)
    X, y = [], []
    
    # 正常活动 (200个样本)
    for _ in range(200):
        window = np.random.normal(0, 0.3, (200, 3))
        window[:, 2] += 9.8  # 重力加速度
        X.append(extract_features(window))
        y.append(0)
    
    # 跌倒动作 (100个样本)
    for _ in range(100):
        window = np.random.normal(0, 0.3, (200, 3))
        window[:, 2] += 9.8
        
        # 在窗口中部模拟冲击
        peak = np.random.uniform(8, 15)
        impact_start = np.random.randint(60, 100)
        
        for t in range(impact_start, min(impact_start + 30, 200)):
            factor = np.exp(-((t - impact_start - 15) / 8) ** 2)
            window[t, 2] += peak * factor
            window[t, 0] += peak * 0.3 * factor
            window[t, 1] += peak * 0.2 * factor
        
        X.append(extract_features(window))
        y.append(1)
    
    return np.array(X), np.array(y)


# ============================================
# 加载真实数据（替换上述演示数据）
# ============================================
def load_real_data():
    """
    从CSV文件加载真实采集的加速度数据
    
    CSV格式要求:
        timestamp, ax, ay, az, label
        0, 9.8, 0.1, 0.2, 0
        0.01, 9.7, 0.2, 0.3, 0
        ...
    
    label: 0=正常, 1=跌倒
    
    可以先使用公开数据集:
    - MobiFall: http://www.iro.umontreal.ca/~labimage/Dataset/
    - SisFall: https://github.com/mdrohmann/SisFall
    """
    csv_path = 'fall_data.csv'
    
    if not os.path.exists(csv_path):
        print(f"未找到真实数据文件 {csv_path}")
        print("使用演示数据进行验证...")
        return generate_demo_data()
    
    print(f"加载真实数据: {csv_path}")
    data = np.loadtxt(csv_path, delimiter=',', skiprows=1)
    
    # 滑动窗口切分
    window_size = 200
    windows = []
    labels = []
    
    for i in range(0, len(data) - window_size, 50):
        window = data[i:i+window_size]
        ax, ay, az = window[:, 1], window[:, 2], window[:, 3]
        labels_window = window[:, 4]
        
        # 窗口标签: 如果超过20%的样本标记为跌倒
        if np.mean(labels_window) > 0.2:
            windows.append(np.column_stack([ax, ay, az]))
            labels.append(1)
        else:
            windows.append(np.column_stack([ax, ay, az]))
            labels.append(0)
    
    X = np.array([extract_features(w) for w in windows])
    y = np.array(labels)
    
    print(f"加载完成: {len(X)} 个窗口, 跌倒占比 {y.mean():.1%}")
    return X, y


# ============================================
# 训练并导出ESP-DL模型
# ============================================
def train_and_export():
    print("=" * 50)
    print("跌倒检测 - 模型训练")
    print("=" * 50)
    
    # 1. 加载数据
    print("\n[1/4] 加载数据...")
    X, y = load_real_data()
    print(f"  样本数: {X.shape[0]}, 特征维度: {X.shape[1]}")
    print(f"  跌倒样本: {y.sum()}, 正常样本: {len(y)-y.sum()}")
    
    # 2. 划分数据集
    print("\n[2/4] 划分训练/测试集...")
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y
    )
    
    # 3. 训练MLP
    print("\n[3/4] 训练MLP分类器...")
    model = MLPClassifier(
        hidden_layer_sizes=(16, 8),    # 2层隐藏层, 轻量设计
        activation='relu',
        solver='adam',
        max_iter=500,
        random_state=42,
        verbose=True
    )
    model.fit(X_train, y_train)
    
    # 评估
    y_pred = model.predict(X_test)
    acc = accuracy_score(y_test, y_pred)
    cm = confusion_matrix(y_test, y_pred)
    
    print(f"\n  测试准确率: {acc:.2%}")
    print(f"  混淆矩阵:")
    print(f"             预测正常  预测跌倒")
    print(f"  实际正常    {cm[0,0]:4d}      {cm[0,1]:4d}")
    print(f"  实际跌倒    {cm[1,0]:4d}      {cm[1,1]:4d}")
    
    # 4. 导出ESP-DL头文件
    print("\n[4/4] 导出ESP-DL模型头文件...")
    os.makedirs('models', exist_ok=True)
    
    with open('models/fall_model_espdl.hpp', 'w') as f:
        f.write("// 跌倒检测MLP模型参数 (ESP-DL格式)\n")
        f.write("// 自动生成 - 请勿手动修改\n\n")
        f.write("#pragma once\n\n")
        f.write("#include <stdint.h>\n\n")
        
        # 写入每层参数
        for i, (W, b) in enumerate(zip(model.coefs_, model.intercepts_)):
            n_input, n_output = W.shape
            
            f.write(f"// Layer {i}: {n_input} -> {n_output}\n")
            
            # 权重矩阵
            f.write(f"const float layer{i}_weights[{n_input}][{n_output}] = {{\n")
            for row in W:
                f.write("    {")
                f.write(", ".join(f"{v:.6f}f" for v in row))
                f.write("},\n")
            f.write("};\n\n")
            
            # 偏置向量
            f.write(f"const float layer{i}_bias[{n_output}] = {{\n    ")
            f.write(", ".join(f"{v:.6f}f" for v in b))
            f.write("\n};\n\n")
        
        # 写入模型配置
        f.write("// 模型配置\n")
        f.write(f"const int FALL_N_FEATURES = {X.shape[1]};\n")
        f.write(f"const int FALL_N_CLASSES = 2;\n")
        f.write(f"const float FALL_THRESHOLD = 0.7f;  // 跌倒判定阈值\n")
    
    print("  已生成: models/fall_model_espdl.hpp")
    print(f"  模型参数: {sum(w.size for w in model.coefs_)} 个浮点数")
    
    print("\n" + "=" * 50)
    print("完成! 请将 models/fall_model_espdl.hpp 复制到ESP32工程")
    print("=" * 50)
    
    return model


if __name__ == '__main__':
    train_and_export()
