import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:fl_chart/fl_chart.dart';
import 'package:intl/intl.dart';
import '../providers/health_data_provider.dart';

class HistoryPage extends StatefulWidget {
  const HistoryPage({super.key});

  @override
  State<HistoryPage> createState() => _HistoryPageState();
}

class _HistoryPageState extends State<HistoryPage>
    with SingleTickerProviderStateMixin {
  late TabController _tabController;
  bool _isListViewMode = false;

  static final FlBorderData _kNoBorder = FlBorderData(show: false);
  static const AxisTitles _kHiddenTopTitles = AxisTitles(
    sideTitles: SideTitles(showTitles: false),
  );
  static const AxisTitles _kHiddenRightTitles = AxisTitles(
    sideTitles: SideTitles(showTitles: false),
  );
  static const FlDotData _kNoDots = FlDotData(show: false);

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 3, vsync: this);
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('运动历史'),
        actions: [
          IconButton(
            icon: Icon(_isListViewMode ? Icons.bar_chart : Icons.list),
            onPressed: () {
              setState(() {
                _isListViewMode = !_isListViewMode;
              });
            },
            tooltip: _isListViewMode ? '图表模式' : '列表模式',
          ),
        ],
        bottom: TabBar(
          controller: _tabController,
          tabs: const [
            Tab(text: '日'),
            Tab(text: '周'),
            Tab(text: '月'),
          ],
        ),
      ),
      body: Consumer<HealthDataProvider>(
        builder: (context, provider, child) {
          if (_isListViewMode) {
            return _buildListViewMode(context, provider);
          }
          return TabBarView(
            controller: _tabController,
            children: [
              _buildDayView(context, provider),
              _buildWeekView(context, provider),
              _buildMonthView(context, provider),
            ],
          );
        },
      ),
    );
  }

  BarChartData _buildBarChartData({
    required ColorScheme colorScheme,
    required double maxY,
    required double horizontalInterval,
    required List<BarChartGroupData> barGroups,
    required AxisTitles bottomTitles,
  }) {
    return BarChartData(
      alignment: BarChartAlignment.spaceAround,
      maxY: maxY,
      barTouchData: BarTouchData(
        touchTooltipData: BarTouchTooltipData(
          getTooltipItem: (group, groupIndex, rod, rodIndex) {
            return BarTooltipItem(
              '${rod.toY.toInt()} 步',
              TextStyle(
                color: colorScheme.onPrimary,
                fontWeight: FontWeight.bold,
              ),
            );
          },
        ),
      ),
      titlesData: FlTitlesData(
        show: true,
        bottomTitles: bottomTitles,
        leftTitles: AxisTitles(
          sideTitles: SideTitles(
            showTitles: true,
            reservedSize: 40,
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
        topTitles: _kHiddenTopTitles,
        rightTitles: _kHiddenRightTitles,
      ),
      gridData: FlGridData(
        show: true,
        drawVerticalLine: false,
        horizontalInterval: horizontalInterval,
        getDrawingHorizontalLine: (value) {
          return FlLine(
            color: colorScheme.outlineVariant.withValues(alpha: 0.3),
            strokeWidth: 1,
          );
        },
      ),
      borderData: _kNoBorder,
      barGroups: barGroups,
    );
  }

  Widget _buildDayView(BuildContext context, HealthDataProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;
    final hourlySteps = provider.getHourlySteps(0);

    final barGroups = hourlySteps.asMap().entries.map((entry) {
      return BarChartGroupData(
        x: entry.key,
        barRods: [
          BarChartRodData(
            toY: entry.value.toDouble(),
            color: colorScheme.primary,
            width: 10,
            borderRadius: const BorderRadius.only(
              topLeft: Radius.circular(4),
              topRight: Radius.circular(4),
            ),
          ),
        ],
      );
    }).toList();

    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '今日步数',
            style: Theme.of(context).textTheme.titleMedium?.copyWith(
                  fontWeight: FontWeight.bold,
                ),
          ),
          const SizedBox(height: 8),
          SizedBox(
            height: 220,
            child: BarChart(
              _buildBarChartData(
                colorScheme: colorScheme,
                maxY: _getMaxY(hourlySteps.map((e) => e.toInt()).toList()),
                horizontalInterval:
                    _getInterval(hourlySteps.map((e) => e.toInt()).toList()),
                barGroups: barGroups,
                bottomTitles: AxisTitles(
                  sideTitles: SideTitles(
                    showTitles: true,
                    getTitlesWidget: (value, meta) {
                      if (value.toInt() % 4 == 0) {
                        return Padding(
                          padding: const EdgeInsets.only(top: 4),
                          child: Text(
                            '${value.toInt()}时',
                            style: TextStyle(
                              fontSize: 10,
                              color: colorScheme.onSurfaceVariant,
                            ),
                          ),
                        );
                      }
                      return const SizedBox.shrink();
                    },
                    reservedSize: 22,
                  ),
                ),
              ),
            ),
          ),
          const SizedBox(height: 32),
          _buildHeartRateChart(context, provider),
          const SizedBox(height: 24),
          _buildSportDistribution(context, provider),
        ],
      ),
    );
  }

  Widget _buildWeekView(BuildContext context, HealthDataProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;
    final weekData = provider.getDailyStepsThisWeek();
    final now = DateTime.now();
    final weekStart = now.subtract(Duration(days: now.weekday - 1));

    final barGroups = weekData.asMap().entries.map((entry) {
      return BarChartGroupData(
        x: entry.key,
        barRods: [
          BarChartRodData(
            toY: entry.value.toDouble(),
            color: colorScheme.tertiary,
            width: 18,
            borderRadius: const BorderRadius.only(
              topLeft: Radius.circular(4),
              topRight: Radius.circular(4),
            ),
          ),
        ],
      );
    }).toList();

    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '本周步数',
            style: Theme.of(context).textTheme.titleMedium?.copyWith(
                  fontWeight: FontWeight.bold,
                ),
          ),
          const SizedBox(height: 8),
          SizedBox(
            height: 220,
            child: BarChart(
              _buildBarChartData(
                colorScheme: colorScheme,
                maxY: _getMaxY(weekData.map((e) => e.toInt()).toList()),
                horizontalInterval:
                    _getInterval(weekData.map((e) => e.toInt()).toList()),
                barGroups: barGroups,
                bottomTitles: AxisTitles(
                  sideTitles: SideTitles(
                    showTitles: true,
                    getTitlesWidget: (value, meta) {
                      final day =
                          weekStart.add(Duration(days: value.toInt()));
                      return Padding(
                        padding: const EdgeInsets.only(top: 4),
                        child: Text(
                          DateFormat('M/d').format(day),
                          style: TextStyle(
                            fontSize: 10,
                            color: colorScheme.onSurfaceVariant,
                          ),
                        ),
                      );
                    },
                    reservedSize: 22,
                  ),
                ),
              ),
            ),
          ),
          const SizedBox(height: 32),
          _buildHeartRateChart(context, provider),
          const SizedBox(height: 24),
          _buildSportDistribution(context, provider),
        ],
      ),
    );
  }

  Widget _buildMonthView(BuildContext context, HealthDataProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;
    final monthData = provider.getDailyStepsThisMonth();

    final barGroups = monthData.asMap().entries.map((entry) {
      return BarChartGroupData(
        x: entry.key,
        barRods: [
          BarChartRodData(
            toY: entry.value.toDouble(),
            color: colorScheme.secondary,
            width: 6,
            borderRadius: const BorderRadius.only(
              topLeft: Radius.circular(3),
              topRight: Radius.circular(3),
            ),
          ),
        ],
      );
    }).toList();

    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '本月步数',
            style: Theme.of(context).textTheme.titleMedium?.copyWith(
                  fontWeight: FontWeight.bold,
                ),
          ),
          const SizedBox(height: 8),
          SizedBox(
            height: 220,
            child: BarChart(
              _buildBarChartData(
                colorScheme: colorScheme,
                maxY: _getMaxY(monthData.map((e) => e.toInt()).toList()),
                horizontalInterval:
                    _getInterval(monthData.map((e) => e.toInt()).toList()),
                barGroups: barGroups,
                bottomTitles: AxisTitles(
                  sideTitles: SideTitles(
                    showTitles: true,
                    interval: 5,
                    getTitlesWidget: (value, meta) {
                      return Padding(
                        padding: const EdgeInsets.only(top: 4),
                        child: Text(
                          '${value.toInt() + 1}日',
                          style: TextStyle(
                            fontSize: 9,
                            color: colorScheme.onSurfaceVariant,
                          ),
                        ),
                      );
                    },
                    reservedSize: 22,
                  ),
                ),
              ),
            ),
          ),
          const SizedBox(height: 32),
          _buildHeartRateChart(context, provider),
          const SizedBox(height: 24),
          _buildSportDistribution(context, provider),
        ],
      ),
    );
  }

  Widget _buildHeartRateChart(
      BuildContext context, HealthDataProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;
    final spots = provider.getHeartRateTrend().asMap().entries.map((e) {
      return FlSpot(e.key.toDouble(), e.value);
    }).toList();

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              '心率趋势',
              style: Theme.of(context).textTheme.titleSmall?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
            const SizedBox(height: 12),
            SizedBox(
              height: 160,
              child: spots.isEmpty
                  ? Center(
                      child: Text(
                        '暂无数据',
                        style: TextStyle(color: colorScheme.onSurfaceVariant),
                      ),
                    )
                  : LineChart(
                      LineChartData(
                        lineTouchData: LineTouchData(
                          touchTooltipData: LineTouchTooltipData(
                            getTooltipItems: (touchedSpots) {
                              return touchedSpots.map((spot) {
                                return LineTooltipItem(
                                  '${spot.y.toInt()} bpm',
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
                              color: colorScheme.outlineVariant
                                  .withValues(alpha: 0.3),
                              strokeWidth: 1,
                            );
                          },
                        ),
                        titlesData: FlTitlesData(
                          show: true,
                          bottomTitles: AxisTitles(
                            sideTitles: SideTitles(
                              showTitles: true,
                              reservedSize: 22,
                              getTitlesWidget: (value, meta) {
                                return Text(
                                  '${value.toInt()}:00',
                                  style: TextStyle(
                                    fontSize: 9,
                                    color: colorScheme.onSurfaceVariant,
                                  ),
                                );
                              },
                            ),
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
                          topTitles: _kHiddenTopTitles,
                          rightTitles: _kHiddenRightTitles,
                        ),
                        borderData: _kNoBorder,
                        minY: 40,
                        maxY: 200,
                        lineBarsData: [
                          LineChartBarData(
                            spots: spots,
                            isCurved: true,
                            color: colorScheme.primary,
                            barWidth: 3,
                            isStrokeCapRound: true,
                            dotData: _kNoDots,
                            belowBarData: BarAreaData(
                              show: true,
                              color:
                                  colorScheme.primary.withValues(alpha: 0.1),
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

  Widget _buildSportDistribution(
      BuildContext context, HealthDataProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;
    final distribution = provider.getSportTypeDistribution();
    final colors = [
      Colors.blue,
      Colors.orange,
      Colors.green,
      Colors.purple,
    ];
    final labels = distribution.keys.toList();

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              '运动类型分布',
              style: Theme.of(context).textTheme.titleSmall?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
            const SizedBox(height: 12),
            Wrap(
              spacing: 16,
              runSpacing: 8,
              children: labels.asMap().entries.map((entry) {
                final index = entry.key;
                final label = entry.value;
                final count = distribution[label] ?? 0;
                final color = colors[index % colors.length];
                return Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Container(
                      width: 10,
                      height: 10,
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        color: color,
                      ),
                    ),
                    const SizedBox(width: 6),
                    Text(
                      '$label: $count',
                      style: Theme.of(context).textTheme.bodySmall,
                    ),
                  ],
                );
              }).toList(),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildListViewMode(
      BuildContext context, HealthDataProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;
    final records = provider.getHistoryRecords();

    if (records.isEmpty) {
      return Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(
              Icons.history,
              size: 64,
              color: colorScheme.onSurfaceVariant.withValues(alpha: 0.4),
            ),
            const Text('暂无历史记录'),
          ],
        ),
      );
    }

    return ListView.separated(
      padding: const EdgeInsets.all(16),
      itemCount: records.length,
      separatorBuilder: (_, __) => const Divider(height: 1),
      itemBuilder: (context, index) {
        final record = records[index];
        final sportType = record['sportType'] as String;
        IconData typeIcon;
        switch (sportType) {
          case '走路':
            typeIcon = Icons.directions_walk;
            break;
          case '跑步':
            typeIcon = Icons.directions_run;
            break;
          case '骑行':
            typeIcon = Icons.directions_bike;
            break;
          default:
            typeIcon = Icons.person;
        }
        return ListTile(
          leading: CircleAvatar(
            backgroundColor: colorScheme.primaryContainer,
            child: Icon(typeIcon, color: colorScheme.primary, size: 20),
          ),
          title: Text(
            DateFormat('yyyy-MM-dd HH:mm')
                .format(record['dateTime'] as DateTime),
            style: Theme.of(context).textTheme.bodyMedium,
          ),
          subtitle: Text(
            '步数: ${record['steps']}  心率: ${record['heartRate']}bpm',
            style: Theme.of(context).textTheme.bodySmall,
          ),
          trailing: Text(
            '${(record['calories'] as num).toStringAsFixed(0)} kcal',
            style: Theme.of(context).textTheme.bodySmall?.copyWith(
                  color: colorScheme.primary,
                  fontWeight: FontWeight.w600,
                ),
          ),
        );
      },
    );
  }

  double _getMaxY(List<int> data) {
    final max = data.reduce((a, b) => a > b ? a : b);
    if (max == 0) return 100;
    return (max * 1.2).ceilToDouble();
  }

  double _getInterval(List<int> data) {
    final max = data.reduce((a, b) => a > b ? a : b);
    if (max <= 100) return 20;
    if (max <= 500) return 100;
    if (max <= 2000) return 500;
    return 1000;
  }
}
