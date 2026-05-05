# 智能运动健康手环 - PlatformIO 固件工程 Spec

## Why
基于 ESP32-S3 主控，使用 PlatformIO + ESP-IDF 框架创建完整的智能手环固件工程，实现传感器数据采集、边缘AI推理、BLE/Wi-Fi通信、OLED显示、执行器响应和低功耗管理。

## What Changes
- 新建 `d:\004\firmware\` PlatformIO 工程（已存在基础结构）
- 创建完整的 ESP-IDF CMake 构建系统
- 实现硬件驱动层：I2C总线、MAX30102、MPU6050、SSD1306 OLED、W25Q32 Flash
- 实现传感器算法层：心率检测、血氧、步数、卡路里、HRV、睡眠分析
- 实现边缘AI推理：跌倒检测（规则+MLP）、运动类型识别（规则引擎+CNN预留）
- 实现通信层：BLE GATT Service、Wi-Fi HTTP Client、DeepSeek API
- 实现执行器控制：PWM振动马达、PWM蜂鸣器
- 实现电源管理：多级低功耗模式、Deep Sleep调度
- 实现 FreeRTOS 多任务主程序（5个任务）

## Impact
- Affected code: `d:\004\firmware\` 下所有文件
- 移除旧的 `mpu6050.c` 并重新实现完整驱动

---

## ADDED Requirements

### Requirement 1: 项目工程搭建
PlatformIO 工程 SHALL 使用 ESP-IDF 框架，支持 ESP32-S3 芯片。

#### Scenario: 工程创建
- **GIVEN** PlatformIO 已安装
- **WHEN** 用户打开 `d:\004\firmware\`
- **THEN** PlatformIO 自动识别工程配置，编译通过

#### Scenario: 硬件配置
- **GIVEN** ESP32-S3-DevKitC-1 开发板
- **WHEN** 用户执行编译
- **THEN** 启用 PSRAM、16MB Flash、BLE、Wi-Fi 配置

### Requirement 2: I2C总线驱动
系统 SHALL 提供统一的 I2C 总线驱动，管理 MAX30102(0x57)、MPU6050(0x68)、SSD1306(0x3C) 三个设备。

#### Scenario: 驱动功能
- **WHEN** 系统初始化 I2C 总线
- **THEN** 扫描总线上的所有设备
- **AND** 支持单个/多个字节寄存器的读写操作

### Requirement 3: MAX30102 心率血氧传感器驱动
驱动 SHALL 支持 MAX30102 的初始化、FIFO数据读取、LED电流配置。

#### Scenario: 数据读取
- **WHEN** 驱动读取 FIFO 寄存器
- **THEN** 返回红外(IR)和红光(Red)的原始 ADC 值
- **AND** 数据有效标志位指示数据质量

### Requirement 4: MPU6050 六轴传感器驱动
驱动 SHALL 支持 MPU6050 的初始化、加速度/角速度读取、零偏校准。

#### Scenario: 数据读取
- **WHEN** 驱动读取传感器
- **THEN** 返回三轴加速度(m/s²)和三轴角速度(°/s)
- **AND** 返回芯片温度

#### Scenario: 校准
- **WHEN** 用户调用校准函数
- **THEN** 采集100个样本求平均偏移量
- **AND** 后续读数自动减去偏移量

### Requirement 5: SSD1306 OLED 显示驱动
驱动 SHALL 支持 SSD1306 128×64 OLED 的初始化和基本绘图操作。

#### Scenario: 显示功能
- **WHEN** 驱动初始化完成
- **THEN** 支持清屏、像素绘制、字符/字符串显示、矩形绘制
- **AND** 支持整帧刷新（双缓冲）

### Requirement 6: W25Q32 SPI Flash 存储
驱动 SHALL 支持 W25Q32 的 SPI 读写、扇区擦除。

#### Scenario: 数据存储
- **WHEN** 应用层写入数据
- **THEN** 驱动按页写入(256字节)，支持跨页写入
- **AND** 支持4KB扇区擦除和全片擦除

### Requirement 7: 心率检测算法
系统 SHALL 基于 MAX30102 PPG 信号实现心率检测。

#### Scenario: 心率计算
- **WHEN** 驱动喂入 PPG 样本(100Hz)
- **THEN** 算法进行带通滤波(0.5-5Hz)
- **AND** 通过自适应阈值峰值检测计算心率
- **AND** 5秒内输出稳定心率值(30-220 bpm)

### Requirement 8: 血氧检测算法
系统 SHALL 基于红光/红外双通道比值法实现血氧估算。

#### Scenario: 血氧计算
- **WHEN** 驱动喂入 IR/RED 样本
- **THEN** 算法分离 AC/DC 分量
- **AND** 计算 R 值并通过查表得到 SpO2

### Requirement 9: 步数检测算法
系统 SHALL 基于 MPU6050 加速度数据实现步数计数。

#### Scenario: 步数计算
- **WHEN** 驱动喂入加速度数据
- **THEN** 算法计算合成加速度幅值
- **AND** 通过动态阈值峰值检测识别步数
- **AND** 最小步间隔 200ms 防抖动

### Requirement 10: 卡路里估算
系统 SHALL 基于步数、运动类型、心率综合估算卡路里消耗。

#### Scenario: 卡路里计算
- **WHEN** 步数和心率更新
- **THEN** 算法基于 MET 值表计算卡路里
- **AND** 运动静止时按 BMR 计算基础消耗

### Requirement 11: 跌倒检测（边缘AI）
系统 SHALL 基于 MPU6050 加速度数据实现实时跌倒检测。

#### Scenario: 跌倒检测
- **WHEN** 系统持续采集加速度数据(100Hz)
- **THEN** 滑动窗口(200样本=2秒)提取6维特征
- **AND** 通过内置 MLP 神经网络推理计算跌倒概率
- **AND** 连续3次确认防误报后才判定跌倒

#### Scenario: 告警响应
- **WHEN** 跌倒被确认
- **THEN** 触发振动马达+蜂鸣器告警
- **AND** 状态通知主程序通过 BLE 发送

### Requirement 12: 运动类型识别
系统 SHALL 基于 MPU6050 6轴数据实现运动类型识别。

#### Scenario: 运动分类
- **WHEN** 系统持续采集加速度数据(50Hz)
- **THEN** 滑动窗口(50样本=1秒)提取时域特征
- **AND** 通过规则引擎分类为静止/走路/跑步/骑行

### Requirement 13: BLE 通信
系统 SHALL 使用 NimBLE 实现 BLE GATT 服务，实时推送数据到 APP。

#### Scenario: 广播与连接
- **WHEN** 系统启动
- **THEN** BLE 以设备名 "SmartBand" 广播
- **AND** 支持手机 APP 扫描连接

#### Scenario: 数据推送
- **WHEN** 系统监测到新数据
- **THEN** 通过 Notification 推送心率/血氧/步数/运动类型/跌倒状态到 APP

### Requirement 14: Wi-Fi + DeepSeek API
系统 SHALL 支持 Wi-Fi 连接和 DeepSeek API 调用。

#### Scenario: 数据上传
- **WHEN** Wi-Fi 已连接且到定时上传时间
- **THEN** 系统上传当日运动摘要到 DeepSeek API
- **AND** 解析返回的 JSON 报告

### Requirement 15: 执行器控制
系统 SHALL 通过 PWM 控制振动马达和蜂鸣器。

#### Scenario: 执行器响应
- **WHEN** 收到告警指令
- **THEN** 振动马达按模式振动(短/中/长/脉冲)
- **AND** 蜂鸣器按模式发声(短/长/双响/警报)

### Requirement 16: 电源管理
系统 SHALL 实现多级低功耗模式。

#### Scenario: 模式切换
- **WHEN** 系统正常工作时为 ACTIVE 模式
- **THEN** 空闲超时自动降为 DAILY 模式
- **AND** 长时间无交互进入 LIGHT_SLEEP
- **AND** 低电量时进入 DEEP_SLEEP

### Requirement 17: FreeRTOS 多任务主程序
系统 SHALL 使用 FreeRTOS 的 5 个任务协同工作。

#### Scenario: 任务调度
- **WHEN** 系统启动
- **THEN** 创建 5 个任务：sensor(最高)、display、comm、alert、power(最低)
- **AND** 任务间通过队列和信号量同步数据
- **AND** I2C 总线通过互斥量保护

### Requirement 18: 心律失常初筛（规则引擎）
系统 SHALL 基于 RR 间期规则引擎实现心律失常初筛。

#### Scenario: 心律分析
- **WHEN** 收集到足量 RR 间期数据
- **THEN** 计算 SDNN/RMSSD/pNN50 指标
- **AND** 根据规则判断正常/疑似房颤/疑似早搏

### Requirement 19: 睡眠分析
系统 SHALL 基于体动+HRV 实现睡眠分析。

#### Scenario: 睡眠判断
- **WHEN** 连续10分钟体动少+心率下降
- **THEN** 判定为入睡
- **AND** 根据体动频率和 HRV 判断深睡/浅睡
- **AND** 输出睡眠时长、质量评分、入睡/醒来时间

---

## MODIFIED Requirements

无

---

## REMOVED Requirements

无
