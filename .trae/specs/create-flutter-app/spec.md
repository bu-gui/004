# 智能运动健康手环 Flutter APP Spec

## Why
为智能运动健康手环项目开发跨平台移动端应用（iOS + Android），实现BLE数据同步、健康数据可视化展示、DeepSeek大模型报告查看等功能，形成"手环+APP+云端"完整产品闭环。

## What Changes
- 新建 `d:\004\app\` Flutter工程目录
- 创建完整的Flutter APP项目结构
- 实现BLE通信模块（设备扫描、连接、数据接收）
- 实现实时数据仪表盘页面（心率/血氧/步数/卡路里）
- 实现运动历史记录页面（日/周/月视图 + fl_chart图表）
- 实现睡眠记录详情页面
- 实现DeepSeek每日报告与AI训练计划页面
- 实现AI助手对话页面
- 实现设备管理与设置页面
- 实现本地数据缓存（sqflite离线存储）
- 集成Material Design 3主题

## Impact
- Affected specs: Flutter应用完整开发
- Affected code: `d:\004\app\` 下所有Dart文件

---

## ADDED Requirements

### Requirement 1: 项目工程搭建
The Flutter APP工程 SHALL 使用Flutter 3.x + Dart 3.x，使用Material Design 3主题风格。

#### Scenario: 标准工程创建
- **GIVEN** 开发环境已安装Flutter SDK
- **WHEN** 执行 `flutter create` 创建工程
- **THEN** 工程目录结构完整，编译通过

### Requirement 2: BLE通信模块
APP SHALL 基于 flutter_blue_plus 实现与ESP32-S3手环的BLE通信。

#### Scenario: 设备扫描与连接
- **GIVEN** APP已启动且蓝牙权限已授予
- **WHEN** 用户点击"搜索设备"
- **THEN** APP开始BLE扫描，列表显示附近名为"SmartBand"的设备
- **AND** 用户点击设备名称后可发起连接

#### Scenario: 数据接收
- **GIVEN** APP已连接手环设备
- **WHEN** 手环通过BLE发送心率/血氧/步数等数据
- **THEN** APP实时解析并更新UI展示

### Requirement 3: 实时数据仪表盘
APP SHALL 在主页面实时展示手环监测的核心健康数据。

#### Scenario: 数据展示
- **WHEN** 手环通过BLE推送数据
- **THEN** 主页面显示当前心率(数值+动画)、血氧饱和度、步数、卡路里消耗
- **AND** 显示当前运动类型（静止/走路/跑步/骑行）图标
- **AND** 显示手环电池电量百分比

#### Scenario: 断网状态
- **WHEN** 手机处于断网状态
- **THEN** 仪表盘页面仍然能正常显示BLE推送的实时数据
- **AND** 显示"离线模式"状态提示

### Requirement 4: 运动历史记录
APP SHALL 提供运动历史记录查看功能，支持日/周/月视图切换。

#### Scenario: 历史数据查看
- **WHEN** 用户切换到历史记录页面
- **THEN** 默认显示今日数据总览
- **AND** 用户可切换日/周/月视图查看步数趋势图(柱状图)
- **AND** 用户可查看心率趋势折线图
- **AND** 图表数据来自本地sqflite缓存

### Requirement 5: 睡眠记录
APP SHALL 展示睡眠分析数据。

#### Scenario: 睡眠详情
- **WHEN** 用户打开睡眠页面
- **THEN** 显示昨晚睡眠时长
- **AND** 显示睡眠质量评分（0-100）
- **AND** 显示入睡/醒来时间
- **AND** 显示深睡/浅睡比例饼图

### Requirement 6: DeepSeek每日报告与AI训练计划
APP SHALL 展示DeepSeek大模型生成的每日健康报告与个性化训练计划。

#### Scenario: 报告查看
- **WHEN** 用户打开报告页面（需联网）
- **THEN** 显示最新一期的每日健康报告（文字+图表）
- **AND** 报告包含：运动总结、心率分析、睡眠评估、AI建议

#### Scenario: 训练计划
- **WHEN** 用户打开训练计划页面（需联网）
- **THEN** 展示AI生成的今日训练计划
- **AND** 用户可查看本周训练计划列表

#### Scenario: 离线查看
- **WHEN** 设备处于断网状态
- **THEN** APP可查看之前缓存的历史报告与训练计划

### Requirement 7: AI助手
APP SHALL 提供基于DeepSeek API的AI健康助手对话功能。

#### Scenario: 对话咨询
- **WHEN** 用户在AI助手页面输入健康/运动相关问题
- **THEN** APP调用DeepSeek API获取回答并展示
- **AND** 对话记录保存在本地

### Requirement 8: 设备管理
APP SHALL 提供设备连接管理与个人参数设置功能。

#### Scenario: 设备设置
- **WHEN** 用户进入设置页面
- **THEN** 显示当前手环连接状态
- **AND** 用户可断开/重新连接设备

#### Scenario: 个人参数设置
- **WHEN** 用户在设置页面修改个人参数
- **THEN** 可设置身高、体重、年龄、性别
- **AND** 可设置每日目标（步数、卡路里、睡眠时长）
- **AND** 参数保存在本地sqflite中

### Requirement 9: 本地数据缓存
APP SHALL 使用sqflite数据库实现本地数据持久化。

#### Scenario: 数据持久化
- **WHEN** APP通过BLE接收到新数据
- **THEN** 数据自动保存到本地sqflite数据库
- **AND** 用户可在离线状态下浏览历史记录

---

## MODIFIED Requirements

无

---

## REMOVED Requirements

无
