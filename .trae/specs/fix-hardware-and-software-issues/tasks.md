# Tasks

## 固件端修复任务

### 批次 1：编译修复 + BLE + 通信（可并行）
- [x] Task 1: 修复所有头文件与源文件签名冲突（零优先级10项）
  - [x] 1.1: `mpu6050_read_all` 统一为 `(mpu6050_data_t *data)`
  - [x] 1.2: `calorie_init` 统一为 `(void)`，体重参数通过 Kconfig 或默认值
  - [x] 1.3: `heart_rate_feed_sample` 统一为 `(uint32_t ir_raw, uint32_t red_raw)`
  - [x] 1.4: `spo2_get_result` 统一为 `(spo2_result_t *result)` 指针返回
  - [x] 1.5: `step_counter_t` 结构体及函数签名统一（`esp_err_t` 返回，`distance_km`）
  - [x] 1.6: `sleep_analyzer` 头/源 `sleep_result_t` 结构体统一
  - [x] 1.7: `arrhythmia_screener` 函数名为 `arrhythmia_screener_feed_rr_interval(uint32_t)`，返回值为枚举
  - [x] 1.8: `ssd1306` 函数返回类型全部改为 `esp_err_t`
  - [x] 1.9: 删除 `ble.c` 内部 `ble_data_packet_t`，统一用头文件版本
  - [x] 1.10: `wifi_conn.c` 函数改为 `wifi_conn_init`、`wifi_conn_connect` 等

- [x] Task 2: 修复 BLE 模块致命错误 + 完善 GATT 服务
  - [x] 2.1: 删除内部重复的结构体定义
  - [x] 2.2: 替换 `ble_gattc_notify_custom` 为 `ble_gatts_notify_custom`
  - [x] 2.3: 修复 conn_handle 硬编码为0的问题（跟踪实际连接句柄）
  - [x] 2.4: 实现 CCCD 描述符
  - [x] 2.5: 在广播数据中添加服务 UUID
  - [x] 2.6: 修复 GATT service/characteristic 定义在栈上的问题（改为静态）
  - [x] 2.7: 实现 `ble_svc_access_cb` 回调支持读写操作
  - [x] 2.8: 添加初始化返回值检查

- [x] Task 3: 修复 DeepSeek API + WiFi 安全与功能问题
  - [x] 3.1: 移除硬编码 API Key，改为 Kconfig 配置宏
  - [x] 3.2: 移除硬编码 WiFi SSID/密码
  - [x] 3.3: Authorization header 添加 `"Bearer "` 前缀
  - [x] 3.4: 增大 HTTP 响应缓冲区（2048→4096）
  - [x] 3.5: 删除死代码 `upload_data()` 函数和 `health_report_t`
  - [x] 3.6: `wifi_conn_connect` 中 `xEventGroupWaitBits` 添加超时

### 批次 2：传感器驱动 + I2C + 电源管理（可并行）
- [x] Task 4: 修复 I2C 总线 + OLED + Flash 驱动问题
  - [x] 4.1: 添加 I2C 互斥锁和公开带锁 API
  - [x] 4.2: 添加 I2C 读写重试机制（失败重试3次）
  - [x] 4.3: 为 `i2c_bus_init` 添加返回值检查
  - [x] 4.4: `i2c_bus_scan` 改为 ESP_LOGx 并条件编译
  - [x] 4.5: 实现 `ssd1306_set_power` 关屏功能
  - [x] 4.6: 优化 `ssd1306_display` 通过整帧传输
  - [x] 4.7: OLED 添加字符范围安全检查
  - [x] 4.8: 为 `w25q32` 添加跨页写入、互斥锁、超时处理

- [x] Task 5: 修复 MAX30102 + MPU6050 驱动问题
  - [x] 5.1: MAX30102 采样率改为 50Hz（匹配读取频率）
  - [x] 5.2: 检查 FIFO 溢出标志
  - [x] 5.3: 初始化后添加 >100ms 稳定延时
  - [x] 5.4: MPU6050 校准函数调用和 DLPF 配置优化
  - [x] 5.5: 初始化函数添加返回值检查

### 批次 3：算法逻辑修复（需批次2完成后）
- [x] Task 6: 修复算法模块核心逻辑
  - [x] 6.1: 修复 `calorie.c` 热量累加逻辑（`base + hr_correction` 正确累加入 `total_calories`）
  - [x] 6.2: 修复 `motion_classifier` 过零率 `==9.8` 边界问题
  - [x] 6.3: 为 `motion_classifier.h` 补充 `motion_classifier_process` 声明
  - [x] 6.4: 修正 `sleep_analyzer` HRV 计算（使用 RMSSD）
  - [x] 6.5: 为 `arrhythmia_screener` 添加异步数据接收（从 heart_rate 获取 RR 间期）
  - [x] 6.6: 修复 `spo2.c` 分子分母为 0 保护 + `data_ready` 信号质量
  - [x] 6.7: 为 `fall_detector` 添加冷却时间机制

### 批次 4：main.c + 电源管理 + 执行器（可并行）
- [x] Task 7: 修复 main.c 数据流断裂问题
  - [x] 7.1: 添加 `calorie_update_steps/update_heart_rate/update_motion_type` 调用
  - [x] 7.2: 添加 `motion_classifier_process()` 和 `fall_detector_process()` 调用
  - [x] 7.3: 添加 `mpu6050_calibrate()` 调用
  - [x] 7.4: 传递 RR 间期到 `arrhythmia_screener_feed_rr_interval`
  - [x] 7.5: 添加 WiFi 启用逻辑（通过 BLE 指令或按钮触发）
  - [x] 7.6: 在 `display_task` 中增加自动熄屏逻辑
  - [x] 7.7: 增加看门狗喂食

- [x] Task 8: 修复电源管理 + 执行器问题
  - [x] 8.1: 实现 `power_mgmt_update_screen_power`
  - [x] 8.2: 深度睡眠前保存健康数据到 NVS/Flash
  - [x] 8.3: 低电量渐进降级策略（先关 WiFi，再降采样率，最后深睡）
  - [x] 8.4: `actuator_fall_alert` 改为非阻塞（定时器回调）
  - [x] 8.5: 调用 `actuator_vibrate_stop/buzzer_stop` 停止告警

### 批次 5：代码质量
- [x] Task 9: 代码质量提升
  - [x] 9.1: 将 `printf` 替换为 `ESP_LOGx`
  - [x] 9.2: 为 `sys_status` 添加互斥保护
  - [x] 9.3: 为所有初始化函数添加返回值检查

## APP 端修复任务

### 批次 A：紧急 BLE + 权限修复（可并行）
- [x] Task 10: 修复 BLE 通信协议匹配
  - [x] 10.1: 将 `ble_service.dart` 中 UUID 改为固件 128 位 UUID
  - [x] 10.2: 统一 `_packetToBytes` 与固件 `ble_data_packet_t` 字段顺序/类型
  - [x] 10.3: 统一 `_parseBytes` 与固件数据结构一致
  - [x] 10.4: 修复 `ble_provider.dart` 中 `_handleData` 解析与 `_parseBytes` 不一致
  - [x] 10.5: 修复扫描结果重复订阅问题
  - [x] 10.6: 特征值订阅改为按 UUID 精确匹配

- [x] Task 11: 修复权限与配置问题
  - [x] 11.1: Android `AndroidManifest.xml` 添加蓝牙和定位权限
  - [x] 11.2: iOS `Info.plist` 添加 `NSBluetoothAlwaysUsageDescription`

### 批次 B：API + Provider 修复（可并行）
- [x] Task 12: 修复 DeepSeek API 服务问题
  - [x] 12.1: Dio 实例复用（创建一次缓存）
  - [x] 12.2: 重试拦截器中复用缓存的 Dio 实例
  - [x] 12.3: 修复流式聊天类型处理（兼容 String 和 List<int>）
  - [x] 12.4: 添加 HTTP 错误状态码处理
  - [x] 12.5: 在 `settings_page.dart` 添加 API Key 输入项

- [x] Task 13: 修复 Provider 数据流问题
  - [x] 13.1: `deepseek_provider` 中 `sendChatMessage` 改为流式追加
  - [x] 13.2: `health_data_provider` 中假数据替换为真实数据库查询
  - [x] 13.3: 修复 `_updateDailySummary` 中 minHeartRate/maxHeartRate 计算
  - [x] 13.4: 限制 `_chatHistory` 长度（最多50条）
  - [x] 13.5: `user_settings_provider` 中 `isDeviceConnected` 修复
  - [x] 13.6: API Key 从 provider 同步到 service

### 批次 C：UI 修复
- [x] Task 14: 修复 UI 页面问题
  - [x] 14.1: `report_page.dart` 从 `report.summaryItems` 读取真实数据
  - [x] 14.2: `history_page.dart` 移除假数据
  - [x] 14.3: `settings_page.dart` 添加 Form 输入验证
  - [x] 14.4: `device_scan_page.dart` 按 RSSI 排序
  - [x] 14.5: `ai_assistant_page.dart` 流式更新优化

## 文档更新任务
- [x] Task 15: 更新问题文档状态标记
  - [x] 15.1: 在 `现阶段已发现硬件问题.md` 中标记已修复项
  - [x] 15.2: 在 `现阶段软件发现的问题.md` 中标记已修复项
  - [x] 15.3: 更新 `说明.md` 中的修复记录章节

## 编译验证
- [x] Task 16: 编译固件验证
  - [x] 16.1: 运行 PlatformIO 编译
  - [x] 16.2: 确认编译零错误
  - [x] 16.3: 确认固件大小未超限

## Task Dependencies
- Task 6 依赖 Task 1（签名修复）
- Task 7 依赖 Task 1, 6（编译通过 + 算法修复后 main.c 才能正确调用）
- Task 8 依赖 Task 4（I2C/Flash 驱动修复后，深度睡眠保存数据才能工作）
- Task 10 依赖 Task 2（APP 端 BLE UUID 必须与固件 BLE 统一）
- Task 12 依赖 Task 11（API Key 有输入入口后才能测试）
- Task 15 在全部修复完成后执行
