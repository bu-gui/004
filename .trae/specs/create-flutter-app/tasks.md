# Tasks

## Task 1: 创建Flutter工程基础框架
- [x] 使用 `flutter create` 创建 `d:\004\app\` 工程
- [x] 配置 pubspec.yaml，添加以下依赖：
  - flutter_blue_plus（BLE通信）
  - fl_chart（图表）
  - sqflite + path（本地数据库）
  - dio（HTTP请求）
  - provider（状态管理）
  - intl（国际化/日期格式化）
- [x] 搭建 Material Design 3 主题配置（colors.dart, theme.dart）
- [x] 创建页面路由结构（home, history, sleep, report, plan, ai_assistant, settings）

## Task 2: 实现数据模型层（Models）
- [x] 创建 BLE 数据模型（HeartRateData, MotionData, FallAlert, BlePacket）
- [x] 创建 健康数据模型（HealthRecord, DailySummary）
- [x] 创建 报告模型（DailyReport, TrainingPlan）
- [x] 创建 用户设置模型（UserProfile, DailyGoal）
- [x] 创建 SleepData, BleDevice 模型

## Task 3: 实现本地数据库层（Database）
- [x] 创建 sqflite 数据库帮助类（DatabaseService）
- [x] 创建健康记录表（health_records）
- [x] 创建每日摘要表（daily_summaries）
- [x] 创建报告缓存表（reports_cache）
- [x] 创建设置表（user_settings）

## Task 4: 实现BLE通信服务
- [x] 创建 BLE 扫描/连接/断开服务（BleService）
- [x] 实现设备发现与过滤（仅显示SmartBand）
- [x] 实现数据订阅与解析（心率/血氧/步数/运动类型/跌倒）
- [x] 实现连接状态管理（已连接/断开/重连）

## Task 5: 实现状态管理层（Providers）
- [x] 创建 BleProvider（BLE连接状态、数据流）
- [x] 创建 HealthDataProvider（健康数据聚合与存储）
- [x] 创建 DeepSeekProvider（API调用、报告缓存）
- [x] 创建 UserSettingsProvider（用户配置管理）

## Task 6: 实现主页面 - 实时数据仪表盘
- [x] 创建仪表盘页面（DashboardPage）
- [x] 实现心率数字显示（大号数字+动态动画）
- [x] 实现血氧/步数/卡路里卡片
- [x] 实现运动类型图标显示
- [x] 实现电池电量指示
- [x] 实现离线/在线状态切换

## Task 7: 实现运动历史记录页面
- [x] 创建历史记录页面（HistoryPage）
- [x] 实现日/周/月视图切换Tab
- [x] 实现步数柱状图（fl_chart）
- [x] 实现心率折线图（fl_chart）
- [x] 实现运动类型分布
- [x] 实现列表模式查看详细记录

## Task 8: 实现睡眠记录页面
- [x] 创建睡眠页面（SleepPage）
- [x] 实现睡眠时长显示
- [x] 实现睡眠质量评分显示
- [x] 实现入睡/醒来时间显示
- [x] 实现深睡/浅睡饼图

## Task 9: 实现DeepSeek报告与训练计划页面
- [x] 创建每日报告页面（ReportPage）
- [x] 创建训练计划页面（PlanPage）
- [x] 实现报告文字+图表展示
- [x] 实现训练计划列表展示
- [x] 实现离线缓存查看

## Task 10: 实现AI助手对话页面
- [x] 创建AI助手页面（AiAssistantPage）
- [x] 实现对话消息列表UI
- [x] 实现用户输入与发送
- [x] 实现DeepSeek API调用响应
- [x] 实现对话历史本地保存

## Task 11: 实现设置页面
- [x] 创建设置页面（SettingsPage）
- [x] 实现设备连接管理（连接/断开)
- [x] 实现个人参数设置（身高/体重/年龄/性别）
- [x] 实现每日目标设置（步数/卡路里/睡眠）
- [x] 实现单位切换（公制/英制）

## Task 12: 实现BLE设备扫描与连接页面
- [x] 创建设备扫描页面（DeviceScanPage）
- [x] 实现设备列表展示
- [x] 实现连接/断开操作
- [x] 实现连接状态反馈

## Task 13: 集成测试与联调
- [x] flutter analyze 通过（0 error）
- [x] 编译验证通过

# Task Dependencies
- Task 1 是前置任务
- Task 2, 3, 4 可并行进行
- Task 5 依赖 Task 2, 3, 4
- Task 6-12 可并行进行（均依赖 Task 5）
- Task 13 依赖所有 Task 6-12
