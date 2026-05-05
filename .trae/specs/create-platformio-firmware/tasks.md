# Tasks

## Task 1: 创建 PlatformIO 工程框架
- [x] 创建 platformio.ini（esp32-s3-devkitc-1, ESP-IDF框架）
- [x] 创建 partitions.csv（factory 2MB + storage 1MB）
- [x] 创建 sdkconfig.defaults（BLE/Wi-Fi/PSRAM配置）
- [x] 创建 CMakeLists.txt（根目录）
- [x] 创建 src/CMakeLists.txt（idf_component_register 19个源文件）

## Task 2: 实现18个头文件（src/include/）
- [x] i2c_bus.h max30102.h mpu6050.h ssd1306.h w25q32.h
- [x] heart_rate.h spo2.h step_counter.h calorie.h
- [x] fall_detector.h motion_classifier.h arrhythmia_screener.h sleep_analyzer.h
- [x] ble.h wifi_conn.h deepseek_api.h actuator.h power_mgmt.h

## Task 3: 实现I2C总线驱动（i2c_bus.c）
- [x] i2c_bus_init：SDA=GPIO18, SCL=GPIO19, 100kHz
- [x] i2c_bus_write/read：寄存器读写
- [x] i2c_bus_scan：1-127地址扫描

## Task 4: 实现传感器驱动
- [x] max30102.c：初始化+FIFO读取(IR/RED双通道)
- [x] mpu6050.c：6轴读取+温度+零偏校准
- [x] ssd1306.c：128x64 framebuffer+6x8字库+绘图API
- [x] w25q32.c：SPI初始化+读写+扇区擦除

## Task 5: 实现传感器算法层
- [x] heart_rate.c：DC滤波+IIR带通+自适应阈值+BPM计算
- [x] spo2.c：AC/DC分离+R值查表
- [x] step_counter.c：合成幅值+动态阈值+防抖
- [x] calorie.c：MET查表(静止1.0/步行3.5/跑步8.0/骑行6.0)
- [x] arrhythmia_screener.c：SDNN/RMSSD/pNN50+房颤/早搏规则
- [x] sleep_analyzer.c：体动检测+HRV+睡眠阶段判断

## Task 6: 实现边缘AI层
- [x] fall_detector.c：200样本@100Hz→6维特征→MLP(16→8→2)→3次确认
- [x] motion_classifier.c：50样本@50Hz→方差/范围/过零率→4类识别

## Task 7: 实现通信层
- [x] ble.c：NimBLE协议栈+GATT Service+16字节Notification
- [x] wifi_conn.c：STA模式+事件处理+自动重连
- [x] deepseek_api.c：cJSON+HTTP POST+DeepSeek API调用

## Task 8: 实现执行器与电源管理
- [x] actuator.c：PWM马达(GPIO4)+PWM蜂鸣器(GPIO5)
- [x] power_mgmt.c：5种功耗模式+电压检测+Deep Sleep

## Task 9: 实现主程序 main.c
- [x] system_status_t结构体+sensor/display/comm/alert/power 5个FreeRTOS任务
- [x] sensor_task：20ms周期，I2C互斥，MPU6050+MAX30102采集→各算法模块
- [x] display_task：100ms周期，3页OLED轮播
- [x] comm_task：1s周期，BLE推送+Wi-Fi DeepSeek上传
- [x] alert_task：队列监听+跌倒告警+执行器响应
- [x] power_task：5s周期，空闲/低电量检测+模式切换
- [x] app_main：NVS初始化→模块init→5个任务创建

## Task 10: 验证与测试
- [x] C代码编译通过（0 error）
- [x] 链接成功（firmware.elf 10.9MB）
- [x] 生成可烧录镜像（firmware.bin 821KB）
- [x] CMakeLists包含19个源文件

# Task Dependencies
- Task 1 → Task 2 → Tasks 3-8(并行) → Task 9 → Task 10
