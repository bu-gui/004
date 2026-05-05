import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:fl_chart/fl_chart.dart';
import '../providers/health_data_provider.dart';
import '../models/sleep_data.dart';

class SleepPage extends StatelessWidget {
  const SleepPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('睡眠记录')),
      body: Consumer<HealthDataProvider>(
        builder: (context, provider, child) {
          final sleepData = provider.getTodaySleep();
          if (sleepData == null) {
            return const Center(child: Text('暂无睡眠数据'));
          }
          return SingleChildScrollView(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _buildSleepDurationSection(context, sleepData),
                const SizedBox(height: 20),
                _buildSleepQualitySection(context, sleepData),
                const SizedBox(height: 20),
                _buildSleepTimeCards(context, sleepData),
                const SizedBox(height: 20),
                _buildSleepPieChart(context, sleepData),
                if ((sleepData.hrvData?.isNotEmpty ?? false)) ...[
                  const SizedBox(height: 20),
                  _buildHrvChart(context, sleepData),
                ],
                const SizedBox(height: 32),
              ],
            ),
          );
        },
      ),
    );
  }

  Widget _buildSleepDurationSection(BuildContext context, SleepData sleepData) {
    final colorScheme = Theme.of(context).colorScheme;
    final totalHours = sleepData.totalSleepHours;

    return Card(
      child: Container(
        width: double.infinity,
        padding: const EdgeInsets.symmetric(vertical: 32, horizontal: 24),
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(16),
          gradient: LinearGradient(
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
            colors: [
              colorScheme.primaryContainer.withValues(alpha: 0.3),
              colorScheme.secondaryContainer.withValues(alpha: 0.2),
            ],
          ),
        ),
        child: Column(
          children: [
            Text(
              '睡眠时长',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                color: colorScheme.onSurfaceVariant,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              totalHours.toStringAsFixed(1),
              style: Theme.of(context).textTheme.displayLarge?.copyWith(
                fontWeight: FontWeight.bold,
                color: colorScheme.primary,
                height: 1.1,
              ),
            ),
            Text(
              '小时',
              style: Theme.of(context).textTheme.bodyLarge?.copyWith(
                color: colorScheme.onSurfaceVariant,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSleepQualitySection(BuildContext context, SleepData sleepData) {
    final colorScheme = Theme.of(context).colorScheme;
    final quality = sleepData.quality;

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Row(
          children: [
            SizedBox(
              width: 100,
              height: 100,
              child: Stack(
                alignment: Alignment.center,
                children: [
                  SizedBox(
                    width: 100,
                    height: 100,
                    child: CircularProgressIndicator(
                      value: quality / 100,
                      strokeWidth: 8,
                      backgroundColor: colorScheme.surfaceContainerHighest,
                      valueColor: AlwaysStoppedAnimation(
                        quality >= 80
                            ? Colors.green
                            : quality >= 60
                            ? Colors.orange
                            : Colors.red,
                      ),
                    ),
                  ),
                  Text(
                    '${quality.toInt()}',
                    style: Theme.of(context).textTheme.headlineMedium?.copyWith(
                      fontWeight: FontWeight.bold,
                      color: quality >= 80
                          ? Colors.green
                          : quality >= 60
                          ? Colors.orange
                          : Colors.red,
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(width: 24),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    '睡眠质量评分',
                    style: Theme.of(context).textTheme.titleSmall?.copyWith(
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const SizedBox(height: 8),
                  Text(
                    quality >= 80
                        ? '优秀'
                        : quality >= 60
                        ? '良好'
                        : quality >= 40
                        ? '一般'
                        : '较差',
                    style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                      fontWeight: FontWeight.w600,
                      color: quality >= 80
                          ? Colors.green
                          : quality >= 60
                          ? Colors.orange
                          : Colors.red,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    quality >= 80
                        ? '睡眠质量非常好，继续保持'
                        : quality >= 60
                        ? '睡眠质量不错，还可以改善'
                        : '需要改善睡眠质量',
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                      color: colorScheme.onSurfaceVariant,
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSleepTimeCards(BuildContext context, SleepData sleepData) {
    final colorScheme = Theme.of(context).colorScheme;

    return Row(
      children: [
        Expanded(
          child: Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                children: [
                  Icon(Icons.bedtime, color: colorScheme.primary, size: 28),
                  const SizedBox(height: 8),
                  Text(
                    '入睡时间',
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                      color: colorScheme.onSurfaceVariant,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    sleepData.sleepTime,
                    style: Theme.of(context).textTheme.titleLarge?.copyWith(
                      fontWeight: FontWeight.bold,
                      color: colorScheme.onSurface,
                    ),
                  ),
                ],
              ),
            ),
          ),
        ),
        const SizedBox(width: 12),
        Expanded(
          child: Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                children: [
                  Icon(Icons.wb_sunny, color: Colors.orange, size: 28),
                  const SizedBox(height: 8),
                  Text(
                    '醒来时间',
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                      color: colorScheme.onSurfaceVariant,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    sleepData.wakeTime,
                    style: Theme.of(context).textTheme.titleLarge?.copyWith(
                      fontWeight: FontWeight.bold,
                      color: colorScheme.onSurface,
                    ),
                  ),
                ],
              ),
            ),
          ),
        ),
      ],
    );
  }

  Widget _buildSleepPieChart(BuildContext context, SleepData sleepData) {
    final colorScheme = Theme.of(context).colorScheme;
    final deepHours = sleepData.deepSleepHours;
    final lightHours = sleepData.lightSleepHours;

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              '睡眠结构',
              style: Theme.of(
                context,
              ).textTheme.titleSmall?.copyWith(fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 16),
            SizedBox(
              height: 200,
              child: Row(
                children: [
                  Expanded(
                    child: PieChart(
                      PieChartData(
                        sectionsSpace: 2,
                        centerSpaceRadius: 40,
                        sections: [
                          PieChartSectionData(
                            color: const Color(0xFF4A6CF7),
                            value: deepHours,
                            title:
                                '${(deepHours / (deepHours + lightHours) * 100).toStringAsFixed(0)}%',
                            radius: 50,
                            titleStyle: TextStyle(
                              fontSize: 12,
                              fontWeight: FontWeight.bold,
                              color: colorScheme.onPrimary,
                            ),
                          ),
                          PieChartSectionData(
                            color: const Color(0xFF9BB1FF),
                            value: lightHours,
                            title:
                                '${(lightHours / (deepHours + lightHours) * 100).toStringAsFixed(0)}%',
                            radius: 50,
                            titleStyle: TextStyle(
                              fontSize: 12,
                              fontWeight: FontWeight.bold,
                              color: colorScheme.onPrimary,
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                  const SizedBox(width: 16),
                  Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      _buildLegendItem(
                        context,
                        color: const Color(0xFF4A6CF7),
                        label: '深睡',
                        value: '${deepHours.toStringAsFixed(1)}h',
                      ),
                      const SizedBox(height: 12),
                      _buildLegendItem(
                        context,
                        color: const Color(0xFF9BB1FF),
                        label: '浅睡',
                        value: '${lightHours.toStringAsFixed(1)}h',
                      ),
                    ],
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildLegendItem(
    BuildContext context, {
    required Color color,
    required String label,
    required String value,
  }) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        Container(
          width: 12,
          height: 12,
          decoration: BoxDecoration(shape: BoxShape.circle, color: color),
        ),
        const SizedBox(width: 8),
        Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(label, style: Theme.of(context).textTheme.bodySmall),
            Text(
              value,
              style: Theme.of(
                context,
              ).textTheme.bodyMedium?.copyWith(fontWeight: FontWeight.bold),
            ),
          ],
        ),
      ],
    );
  }

  Widget _buildHrvChart(BuildContext context, SleepData sleepData) {
    final colorScheme = Theme.of(context).colorScheme;

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'HRV 趋势',
              style: Theme.of(
                context,
              ).textTheme.titleSmall?.copyWith(fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 12),
            SizedBox(
              height: 160,
              child: LineChart(
                LineChartData(
                  lineTouchData: LineTouchData(
                    touchTooltipData: LineTouchTooltipData(
                      getTooltipItems: (touchedSpots) {
                        return touchedSpots.map((spot) {
                          return LineTooltipItem(
                            '${spot.y.toInt()} ms',
                            TextStyle(
                              color: colorScheme.onPrimary,
                              fontWeight: FontWeight.bold,
                            ),
                          );
                        }).toList();
                      },
                    ),
                  ),
                  gridData: FlGridData(
                    show: true,
                    drawVerticalLine: false,
                    getDrawingHorizontalLine: (value) {
                      return FlLine(
                        color: colorScheme.outlineVariant.withValues(
                          alpha: 0.3,
                        ),
                        strokeWidth: 1,
                      );
                    },
                  ),
                  titlesData: FlTitlesData(
                    show: true,
                    bottomTitles: AxisTitles(
                      sideTitles: SideTitles(showTitles: false),
                    ),
                    leftTitles: AxisTitles(
                      sideTitles: SideTitles(
                        showTitles: true,
                        reservedSize: 36,
                        getTitlesWidget: (value, meta) {
                          return Text(
                            '${value.toInt()}',
                            style: TextStyle(
                              fontSize: 10,
                              color: colorScheme.onSurfaceVariant,
                            ),
                          );
                        },
                      ),
                    ),
                    topTitles: AxisTitles(
                      sideTitles: SideTitles(showTitles: false),
                    ),
                    rightTitles: AxisTitles(
                      sideTitles: SideTitles(showTitles: false),
                    ),
                  ),
                  borderData: FlBorderData(show: false),
                  lineBarsData: [
                    LineChartBarData(
                      spots:
                          sleepData.hrvData
                              ?.asMap()
                              .entries
                              .map((e) => FlSpot(e.key.toDouble(), e.value))
                              .toList() ??
                          [],
                      isCurved: true,
                      color: const Color(0xFF7C4DFF),
                      barWidth: 3,
                      isStrokeCapRound: true,
                      dotData: FlDotData(show: false),
                      belowBarData: BarAreaData(
                        show: true,
                        color: const Color(0xFF7C4DFF).withValues(alpha: 0.1),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
