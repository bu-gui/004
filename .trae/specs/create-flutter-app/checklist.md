# Checklist

## 项目工程
- [x] Flutter工程创建成功，`flutter pub get` 无错误
- [x] Material Design 3 主题配置完成
- [x] 页面路由配置完成

## 数据模型
- [x] BLE数据模型定义完成（BlePacket, HealthRecord, HeartRateData）
- [x] 健康数据模型定义完成（DailySummary）
- [x] 报告模型定义完成（DailyReport, TrainingPlan）
- [x] 用户设置模型定义完成（UserProfile, DailyGoal）
- [x] 额外模型定义完成（SleepData, BleDevice）

## 本地数据库
- [x] DatabaseService 帮助类实现
- [x] 健康记录表 CRUD 实现
- [x] 每日摘要表 CRUD 实现
- [x] 报告缓存实现
- [x] 设置持久化实现

## BLE通信
- [x] 设备扫描功能实现
- [x] 设备连接功能实现
- [x] 数据订阅与解析功能实现
- [x] 连接状态管理实现

## 状态管理
- [x] BleProvider 实现
- [x] HealthDataProvider 实现
- [x] DeepSeekProvider 实现
- [x] UserSettingsProvider 实现

## 页面功能
- [x] 仪表盘页面：心率/血氧/步数/卡路里实时显示
- [x] 仪表盘页面：运动类型图标
- [x] 仪表盘页面：电池电量
- [x] 仪表盘页面：离线/在线状态
- [x] 历史记录页面：日/周/月视图切换
- [x] 历史记录页面：步数柱状图
- [x] 历史记录页面：心率折线图
- [x] 历史记录页面：运动类型分布
- [x] 睡眠页面：睡眠时长/质量评分
- [x] 睡眠页面：深睡/浅睡饼图
- [x] 报告页面：每日报告展示
- [x] 训练计划页面：计划列表
- [x] AI助手页面：对话界面
- [x] AI助手页面：API调用
- [x] 设置页面：个人参数设置
- [x] 设置页面：每日目标设置
- [x] 设备扫描页面：设备列表
- [x] 设备扫描页面：连接/断开操作

## 集成与测试
- [x] flutter analyze 通过（0 error, 仅warning/info）
- [x] 代码编译验证通过
