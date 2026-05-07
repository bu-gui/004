# Checklist

## 固件端编译修复验证
- [ ] 零-1: `mpu6050_read_all` 签名统一为 `(mpu6050_data_t *data)` — `[mpu6050.c](file:///d:/004/firmware/src/mpu6050.c)` 和 `[mpu6050.h](file:///d:/004/firmware/include/mpu6050.h)` 签名一致
- [ ] 零-2: `calorie_init` 签名统一为 `(void)` — `[calorie.c](file:///d:/004/firmware/src/calorie.c)` 移除参数，`[calorie.h](file:///d:/004/firmware/include/calorie.h)` 声明不变
- [ ] 零-3: `heart_rate_feed_sample` 签名统一为 `(uint32_t ir_raw, uint32_t red_raw)` — `[heart_rate.c](file:///d:/004/firmware/src/heart_rate.c)` 和 `[heart_rate.h](file:///d:/004/firmware/include/heart_rate.h)` 签名一致
- [ ] 零-4: `spo2_get_result` 统一为 `esp_err_t spo2_get_result(spo2_result_t *result)` — `[spo2.c](file:///d:/004/firmware/src/spo2.c)` 和 `[spo2.h](file:///d:/004/firmware/include/spo2.h)` 签名一致
- [ ] 零-5: `step_counter_t` 结构体及函数签名统一 — `[step_counter.c](file:///d:/004/firmware/src/step_counter.c)` 和 `[step_counter.h](file:///d:/004/firmware/include/step_counter.h)` 接口一致，`distance_km` 统一单位
- [ ] 零-6: `sleep_result_t` 头源统一 — `[sleep_analyzer.c](file:///d:/004/firmware/src/sleep_analyzer.c)` 和 `[sleep_analyzer.h](file:///d:/004/firmware/include/sleep_analyzer.h)` 结构体一致
- [ ] 零-7: `arrhythmia_screener` 函数名统一 — `[arrhythmia_screener.c](file:///d:/004/firmware/src/arrhythmia_screener.c)` 函数名改为 `arrhythmia_screener_feed_rr_interval(uint32_t)`，返回值符合 `[arrhythmia_screener.h](file:///d:/004/firmware/include/arrhythmia_screener.h)` 枚举
- [ ] 零-8: `ssd1306` 返回类型统一为 `esp_err_t` — `[ssd1306.c](file:///d:/004/firmware/src/ssd1306.c)` 和 `[ssd1306.h](file:///d:/004/firmware/include/ssd1306.h)` 一致
- [ ] 零-9: `ble.c` 内部 `ble_data_packet_t` 已删除 — `[ble.c](file:///d:/004/firmware/src/ble.c)` 使用 `[ble.h](file:///d:/004/firmware/include/ble.h)` 定义
- [ ] 零-10: `wifi_conn.c` 函数名统一为 `wifi_conn_` 前缀 — `[wifi_conn.c](file:///d:/004/firmware/src/wifi_conn.c)` 和 `[wifi_conn.h](file:///d:/004/firmware/include/wifi_conn.h)` 一致

## BLE 修复验证
- [ ] BLE1: `ble_gattc_notify_custom` 替换为 `ble_gatts_notify_custom`
- [ ] BLE2: conn_handle 不再硬编码，使用实际连接句柄
- [ ] BLE3: CCCD 描述符已实现
- [ ] BLE4: 广播数据包含服务 UUID
- [ ] BLE5: GATT service/characteristic 为静态存储
- [ ] BLE6: `ble_svc_access_cb` 支持读写操作

## 通信安全验证
- [ ] DS1: API Key 不再硬编码在源码中，改为 Kconfig 配置
- [ ] WIFI5: SSID/密码不再硬编码在头文件中
- [ ] DS5: Authorization header 包含 `"Bearer "` 前缀

## I2C/驱动修复验证
- [ ] I2C1/OLED1: I2C 总线有互斥锁保护
- [ ] I2C5: I2C 读写有重试机制（失败重试3次）
- [ ] I2C4: `i2c_bus_init` 检查返回值
- [ ] OLED3: `ssd1306_set_power` 已实现
- [ ] OLED2: `ssd1306_display` 优化为整帧传输
- [ ] OLED6: 字符范围安全检查已添加
- [ ] FLASH1: `w25q32` 支持跨页写入

## 算法修复验证
- [ ] CAL1: `calorie_get_total` 中 `base + hr_correction` 正确累加入 `total_calories`
- [ ] MOT1: 过零率逻辑修正（处理 `==9.8` 边界）
- [ ] MOT5: `motion_classifier.h` 中有 `motion_classifier_process` 声明
- [ ] SLEEP1: HRV 计算使用 RMSSD
- [ ] ARR1: RR 间期数据正确传递到 `arrhythmia_screener`
- [ ] SPO2: 分子分母为0保护 + `data_ready` 信号质量
- [ ] FALL3: 跌倒检测有冷却时间机制

## main.c 数据流验证
- [ ] MAIN1: 调用 `calorie_update_steps/update_heart_rate/update_motion_type`
- [ ] MAIN9: 调用 `motion_classifier_process()`
- [ ] MAIN10: 调用 `fall_detector_process()`
- [ ] MAIN4: 调用 `mpu6050_calibrate()`
- [ ] MAIN2: RR 间期传递到 `arrhythmia_screener_feed_rr_interval`

## 电源管理验证
- [ ] PWR5: `power_mgmt_update_screen_power` 已实现
- [ ] PWR8: 低电量渐进降级策略（先关WiFi→降采样→深睡）
- [ ] PWR9: 深度休眠前保存数据到 Flash

## 执行器验证
- [ ] ACT2: `actuator_fall_alert` 改为非阻塞（定时器回调）
- [ ] ACT3: `actuator_vibrate_stop/buzzer_stop` 被调用

## APP BLE 修复验证
- [ ] APP-BLE1: `ble_service.dart` UUID 与固件 `custom_svc_uuid` / `custom_chr_uuid` 匹配
- [ ] APP-BLE2: `_packetToBytes` 字段顺序/类型与 `ble_data_packet_t` 一致
- [ ] APP-BLE3: `_parseBytes` 解析与固件数据结构一致
- [ ] APP-BLE4: `ble_provider.dart` 中 `_handleData` 与 `_parseBytes` 一致
- [ ] APP-BLE5: 扫描结果不再重复订阅
- [ ] APP-BLE6: 特征值订阅改为按 UUID 精确匹配

## APP 权限验证
- [ ] APP-PERM1: `AndroidManifest.xml` 包含 `BLUETOOTH_SCAN`/`BLUETOOTH_CONNECT`/`BLUETOOTH_ADVERTISE`
- [ ] APP-PERM2: `AndroidManifest.xml` 包含 `ACCESS_FINE_LOCATION`/`ACCESS_COARSE_LOCATION`
- [ ] APP-PERM3: `Info.plist` 包含 `NSBluetoothAlwaysUsageDescription`

## APP API/Provider 验证
- [ ] APP-API1: `ble_service.dart` Dio 实例已复用
- [ ] APP-API2: 重试拦截器使用缓存 Dio 实例
- [ ] APP-API3: `settings_page.dart` 有 API Key 输入项
- [ ] APP-PROV1: `sendChatMessage` 改为流式追加不阻塞 UI
- [ ] APP-PROV2: `health_data_provider` 假数据替换为数据库查询
- [ ] APP-PROV3: `_updateDailySummary` minHeartRate/maxHeartRate 计算正确
- [ ] APP-PROV4: `_chatHistory` 限制最多50条
- [ ] APP-PROV5: `isDeviceConnected` 不再硬编码为 false

## APP UI 验证
- [ ] APP-UI1: `report_page.dart` 显示真实数据
- [ ] APP-UI2: `settings_page.dart` 有 Form 输入验证
- [ ] APP-UI3: `device_scan_page.dart` 按 RSSI 排序

## 编译验证
- [ ] 固件编译零错误
- [ ] 固件大小未超过 2MB 分区限制
