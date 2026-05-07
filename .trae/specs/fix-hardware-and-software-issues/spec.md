# 修复硬件与软件已知问题 Spec

## Why
当前智能手环项目（包括固件端 ESP32-S3 和 APP 端 Flutter）存在大量已识别但未修复的问题，包括：头文件与源文件签名冲突导致编译失败、BLE 协议两端不匹配导致通信完全不可用、算法逻辑错误、安全性漏洞（API Key/WiFi 凭据硬编码）、数据流断裂（模块之间未正确调用）等。这些问题导致项目无法编译运行，核心功能不可用。

## What Changes

### 固件端（ESP32-S3）

#### 🔥 第零优先级 — 让项目能编译
- [x] **零-1**: 统一 `mpu6050_read_all` 头文件声明 `(mpu6050_data_t *data)` 与源文件实现签名
- [x] **零-2**: 统一 `calorie_init` 头文件声明 `(void)` 与源文件定义 `(float weight)`，改为默认体重参数
- [x] **零-3**: 统一 `heart_rate_feed_sample` 头文件声明 `(uint32_t ir_raw, uint32_t red_raw)` 与源文件只接收1个参数
- [x] **零-4**: 统一 `spo2_get_result` 头文件声明 `esp_err_t spo2_get_result(spo2_result_t *result)` 与源文件 `float spo2_get_result(void)`
- [x] **零-5**: 统一 `step_counter_t` / `step_result_t` 结构体及函数返回值类型（`esp_err_t` vs `void`）
- [x] **零-6**: 统一 `sleep_analyzer` 头文件与源文件 `sleep_result_t` 结构体
- [x] **零-7**: 统一 `arrhythmia_screener` 函数名与参数类型（`arrhythmia_screener_feed_rr_interval(uint32_t)` vs `feed_rr_interval(float)`）
- [x] **零-8**: 统一 `ssd1306` 函数返回类型（`esp_err_t` vs `void`）
- [x] **零-9**: 删除 `ble.c` 内部重复 `ble_data_packet_t` 定义，统一使用头文件版本
- [x] **零-10**: 统一 `wifi_conn` 函数命名（`wifi_conn_init` vs `wifi_init`）

#### 🔥 第一优先级 — 让项目能基本运行
- [x] **一-1**: 修复 BLE 致命错误（删除内部重复定义、替换 `ble_gattc_notify_custom`、实现最小 GATT 服务、修复 conn_handle 硬编码）
- [x] **一-2**: 移除深硬编码 API Key，改为通过 Kconfig 配置
- [x] **一-3**: 移除硬编码 WiFi SSID/密码，改为通过 Kconfig 或 NVS 配置
- [x] **一-4**: 修复 DeepSeek API Authorization header（添加 `"Bearer "` 前缀）
- [x] **一-5**: 添加 I2C 互斥锁保护（`ssd1306.c`、`i2c_bus.c` 使用全局互斥锁）
- [x] **一-6**: 修复 `calorie.c` 热量累加逻辑（将 `base + hr_correction` 正确加入 `total_calories`）
- [x] **一-7**: 在 `main.c` 中调用 `calorie_update_steps/update_heart_rate/update_motion_type`
- [x] **一-8**: 在 `main.c` 的 `sensor_task` 中添加 `motion_classifier_process()` 和 `fall_detector_process()` 调用
- [x] **一-9**: 降低 MAX30102 采样率至 50Hz 或改为批量读取，解决 FIFO 溢出
- [x] **一-10**: 配置 I2C 引脚为 GPIO 18/19 并添加条件编译支持

#### ⚡ 第二优先级 — 让核心算法有意义
- [x] **二-1**: 在 `main.c` 中添加 `mpu6050_calibrate()` 调用
- [x] **二-2**: 实现 RR 间期从 `heart_rate` 到 `arrhythmia_screener` 的传递
- [x] **二-3**: 为 `motion_classifier.h` 补充 `motion_classifier_process` 函数声明
- [x] **二-4**: 修正 `motion_classifier` 过零率逻辑（处理 `==9.8` 边界情况）
- [x] **二-5**: 修正 `sleep_analyzer` 的 HRV 计算（使用 RMSSD 而非标准差）
- [x] **二-6**: 为跌倒检测添加冷却时间机制
- [x] **二-7**: 为 `spo2_get_result` 添加 `data_ready` 信号质量检测和分子分母为0保护

#### 🛠️ 第三优先级 — 完善通信与外设
- [x] **三-1**: 实现 BLE CCCD 描述符和通知
- [x] **三-2**: 在 `wifi_conn_init` 中添加连接超时，`wifi_connect` 中为 `xEventGroupWaitBits` 添加超时
- [x] **三-3**: 实现 `ssd1306_set_power` 和 `power_mgmt_update_screen_power`
- [x] **三-4**: 在 `display_task` 中增加自动熄屏逻辑
- [x] **三-5**: 将 `actuator_fall_alert` 改为非阻塞实现（使用定时器回调）
- [x] **三-6**: 深度睡眠前将当前健康数据保存到 Flash（NVS）
- [x] **三-7**: 添加 I2C 重试机制（读取/写入失败时重试3次）

#### ✨ 第四优先级 — 代码质量与健壮性
- [x] **四-1**: 统一日志输出为 `ESP_LOGx`
- [x] **四-2**: 为所有共享资源添加互斥保护（`sys_status`、Flash、I2C 总线）
- [x] **四-3**: 添加看门狗喂食任务
- [x] **四-4**: 为所有传感器初始化函数检查返回值

### APP 端（Flutter）

#### 🔴 紧急问题
- [x] **A-1**: 修复 BLE UUID 与固件匹配（使用固件的 128 位 UUID）
- [x] **A-2**: 统一数据包解析格式（按固件 `ble_data_packet_t`：heart_rate float32, spo2 uint8, steps uint32, calories float32, motion_type uint8, fall_detected uint8, battery float32）
- [x] **A-3**: 添加 API Key 设置界面入口和 `setApiKey` 调用
- [x] **A-4**: 添加 Android 蓝牙权限（`BLUETOOTH_SCAN`、`BLUETOOTH_CONNECT`、`BLUETOOTH_ADVERTISE`）
- [x] **A-5**: 添加 Android 定位权限（`ACCESS_FINE_LOCATION`、`ACCESS_COARSE_LOCATION`）
- [x] **A-6**: 添加 iOS 蓝牙权限描述（`NSBluetoothAlwaysUsageDescription`）

#### 🟡 中等/🟢 轻微问题
- [x] **B-1**: 修复 `ble_service.dart` 中 Dio 实例未复用问题
- [x] **B-2**: 修复 `DeepSeekApiService` 重试拦截器新建 Dio 实例丢失配置问题
- [x] **B-3**: 修复 `ble_provider.dart` 扫描结果重复订阅
- [x] **B-4**: 修复 `ble_provider.dart` 中 `_handleData` 数据解析与 `_parseBytes` 不一致
- [x] **B-5**: 修复 `deepseek_provider.dart` 中 `sendChatMessage` 阻塞 UI 线程
- [x] **B-6**: 修复 `health_data_provider.dart` 中假数据问题
- [x] **B-7**: 修复 `deepseek_provider.dart` 中 `fetchTrainingPlan` 只传当天数据
- [x] **B-8**: 修复 `report_page.dart` 中指标显示 `--`
- [x] **B-9**: 修复 `user_settings_provider.dart` 中 `isDeviceConnected` 硬编码为 `false`
- [x] **B-10**: 修复 `database_service.dart` 中 `health_records` 表添加 `timestamp` 索引

### 文档更新
- [x] 更新 `说明.md` 中的修复记录
- [x] 更新问题文档中已修复项的状态标记

## Impact
- 固件端：所有驱动模块、算法模块、通信模块、`main.c`
- APP 端：所有服务层、状态管理层、UI 页面层
- 文档：`说明.md`、`现阶段已发现硬件问题.md`、`现阶段软件发现的问题.md`
