import 'dart:math';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:fl_chart/fl_chart.dart';
import '../providers/health_data_provider.dart';

class DashboardPage extends StatefulWidget {
  const DashboardPage({super.key});

  @override
  State<DashboardPage> createState() => _DashboardPageState();
}

class _DashboardPageState extends State<DashboardPage>
    with TickerProviderStateMixin {
  late AnimationController _heartBeatController;
  late Animation<double> _heartBeatAnimation;

  @override
  void initState() {
    super.initState();
    _heartBeatController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 800),
    )..repeat(reverse: true);
    _heartBeatAnimation = Tween<double>(begin: 0.95, end: 1.05).animate(
      CurvedAnimation(parent: _heartBeatController, curve: Curves.easeInOut),
    );
  }

  @override
  void dispose() {
    _heartBeatController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    return Consumer<HealthDataProvider>(
      builder: (context, provider, child) {
        return Scaffold(
          body: Container(
            decoration: BoxDecoration(
              gradient: LinearGradient(
                begin: Alignment.topCenter,
                end: Alignment.bottomCenter,
                colors: [
                  colorScheme.primaryContainer.withValues(alpha: 0.4),
                  colorScheme.surface,
                  colorScheme.surface,
                ],
              ),
            ),
            child: SafeArea(
              child: SingleChildScrollView(
                padding: const EdgeInsets.symmetric(horizontal: 20),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    _buildTopBar(context, provider),
                    const SizedBox(height: 16),
                    _buildHeartRateSection(context, provider),
                    const SizedBox(height: 24),
                    _buildMetricsGrid(context, provider),
                    const SizedBox(height: 24),
                    _buildSportTypeSection(context, provider),
                    const SizedBox(height: 24),
                    _buildBatterySection(context, provider),
                    const SizedBox(height: 32),
                  ],
                ),
              ),
            ),
          ),
        );
      },
    );
  }

  Widget _buildTopBar(BuildContext context, HealthDataProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        Text(
          '仪表盘',
          style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                fontWeight: FontWeight.bold,
              ),
        ),
        Row(
          children: [
            Container(
              width: 12,
              height: 12,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: provider.isOnline ? Colors.green : Colors.grey,
                boxShadow: [
                  BoxShadow(
                    color: (provider.isOnline ? Colors.green : Colors.grey)
                        .withValues(alpha: 0.5),
                    blurRadius: 6,
                    spreadRadius: 2,
                  ),
                ],
              ),
            ),
            const SizedBox(width: 6),
            Text(
              provider.isOnline ? '在线' : '离线',
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                    color: colorScheme.onSurfaceVariant,
                  ),
            ),
          ],
        ),
      ],
    );
  }

  Widget _buildHeartRateSection(
      BuildContext context, HealthDataProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;
    return Center(
      child: Column(
        children: [
          AnimatedBuilder(
            animation: _heartBeatAnimation,
            builder: (context, child) {
              return Transform.scale(
                scale: _heartBeatAnimation.value,
                child: child,
              );
            },
            child: Container(
              width: 160,
              height: 160,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                gradient: LinearGradient(
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                  colors: [
                    colorScheme.primary.withValues(alpha: 0.15),
                    colorScheme.tertiary.withValues(alpha: 0.15),
                  ],
                ),
                border: Border.all(
                  color: colorScheme.primary.withValues(alpha: 0.2),
                  width: 3,
                ),
              ),
              child: Center(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Text(
                      '${provider.heartRate}',
                      style: Theme.of(context).textTheme.displayLarge?.copyWith(
                            fontWeight: FontWeight.bold,
                            color: colorScheme.primary,
                            height: 1.1,
                          ),
                    ),
                    Text(
                      'bpm',
                      style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                            color: colorScheme.onSurfaceVariant,
                          ),
                    ),
                  ],
                ),
              ),
            ),
          ),
          const SizedBox(height: 8),
          Text(
            '当前心率',
            style: Theme.of(context).textTheme.bodyLarge?.copyWith(
                  color: colorScheme.onSurfaceVariant,
                ),
          ),
        ],
      ),
    );
  }

  Widget _buildMetricsGrid(BuildContext context, HealthDataProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;
    return Row(
      children: [
        Expanded(
          child: _buildMetricCard(
            context,
            icon: Icons.bloodtype,
            label: '血氧',
            value: '${provider.bloodOxygen.toStringAsFixed(1)}%',
            color: Colors.red,
            child: SizedBox(
              height: 44,
              child: Center(
                child: SizedBox(
                  width: 40,
                  height: 40,
                  child: Stack(
                    alignment: Alignment.center,
                    children: [
                      CircularProgressIndicator(
                        value: provider.bloodOxygen / 100,
                        strokeWidth: 4,
                        backgroundColor:
                            colorScheme.surfaceContainerHighest,
                        valueColor: AlwaysStoppedAnimation(
                          Colors.red.withValues(alpha: 0.7),
                        ),
                      ),
                      Text(
                        '${provider.bloodOxygen.toInt()}',
                        style:
                            Theme.of(context).textTheme.labelSmall?.copyWith(
                                  fontWeight: FontWeight.bold,
                                ),
                      ),
                    ],
                  ),
                ),
              ),
            ),
          ),
        ),
        const SizedBox(width: 12),
        Expanded(
          child: _buildMetricCard(
            context,
            icon: Icons.directions_walk,
            label: '步数',
            value: '${provider.steps}',
            color: Colors.orange,
            child: Text(
              '步',
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                    color: colorScheme.onSurfaceVariant,
                  ),
            ),
          ),
        ),
        const SizedBox(width: 12),
        Expanded(
          child: _buildMetricCard(
            context,
            icon: Icons.local_fire_department,
            label: '卡路里',
            value: '${provider.calories.toStringAsFixed(0)}',
            color: Colors.deepOrange,
            child: Text(
              'kcal',
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                    color: colorScheme.onSurfaceVariant,
                  ),
            ),
          ),
        ),
      ],
    );
  }

  Widget _buildMetricCard(
    BuildContext context, {
    required IconData icon,
    required String label,
    required String value,
    required Color color,
    required Widget child,
  }) {
    final colorScheme = Theme.of(context).colorScheme;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            Icon(icon, color: color, size: 28),
            const SizedBox(height: 8),
            Text(
              value,
              style: Theme.of(context).textTheme.titleLarge?.copyWith(
                    fontWeight: FontWeight.bold,
                    color: colorScheme.onSurface,
                  ),
            ),
            const SizedBox(height: 4),
            child,
            const SizedBox(height: 4),
            Text(
              label,
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                    color: colorScheme.onSurfaceVariant,
                  ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSportTypeSection(
      BuildContext context, HealthDataProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;
    IconData sportIcon;
    String sportLabel;

    switch (provider.sportType) {
      case '走路':
        sportIcon = Icons.directions_walk;
        sportLabel = '走路';
        break;
      case '跑步':
        sportIcon = Icons.directions_run;
        sportLabel = '跑步';
        break;
      case '骑行':
        sportIcon = Icons.directions_bike;
        sportLabel = '骑行';
        break;
      default:
        sportIcon = Icons.person;
        sportLabel = '静止';
    }

    return Card(
      child: ListTile(
        leading: CircleAvatar(
          backgroundColor: colorScheme.primaryContainer,
          child: Icon(sportIcon, color: colorScheme.primary),
        ),
        title: Text('当前运动状态'),
        trailing: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(sportIcon, color: colorScheme.primary, size: 28),
            const SizedBox(width: 8),
            Text(
              sportLabel,
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.w600,
                    color: colorScheme.primary,
                  ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildBatterySection(
      BuildContext context, HealthDataProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;
    final battery = provider.batteryLevel;
    Color batteryColor;
    if (battery > 60) {
      batteryColor = Colors.green;
    } else if (battery > 20) {
      batteryColor = Colors.orange;
    } else {
      batteryColor = Colors.red;
    }

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.battery_std, color: batteryColor, size: 24),
                const SizedBox(width: 8),
                Text(
                  '电池电量',
                  style: Theme.of(context).textTheme.titleSmall,
                ),
                const Spacer(),
                Text(
                  '$battery%',
                  style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                        fontWeight: FontWeight.bold,
                        color: batteryColor,
                      ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            ClipRRect(
              borderRadius: BorderRadius.circular(8),
              child: LinearProgressIndicator(
                value: battery / 100,
                minHeight: 10,
                backgroundColor: colorScheme.surfaceContainerHighest,
                valueColor: AlwaysStoppedAnimation(batteryColor),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
