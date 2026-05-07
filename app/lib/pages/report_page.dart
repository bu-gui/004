import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:intl/intl.dart';
import '../providers/deepseek_provider.dart';

class ReportPage extends StatefulWidget {
  const ReportPage({super.key});

  @override
  State<ReportPage> createState() => _ReportPageState();
}

class _ReportPageState extends State<ReportPage> {
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      context.read<DeepSeekProvider>().loadReport();
    });
  }

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<DeepSeekProvider>();
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    return Scaffold(
      appBar: AppBar(title: const Text('每日健康报告'), centerTitle: true),
      body: provider.isLoading
          ? _buildSkeletonLoader(theme)
          : provider.report == null
          ? _buildEmptyState(theme, colorScheme)
          : _buildReportContent(context, provider, theme, colorScheme),
    );
  }

  Widget _buildSkeletonLoader(ThemeData theme) {
    return ListView.builder(
      padding: const EdgeInsets.all(16),
      itemCount: 5,
      itemBuilder: (context, index) {
        return Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Container(
                  width: 120,
                  height: 16,
                  decoration: BoxDecoration(
                    color: theme.colorScheme.surfaceContainerHighest,
                    borderRadius: BorderRadius.circular(4),
                  ),
                ),
                const SizedBox(height: 12),
                Container(
                  width: double.infinity,
                  height: 48,
                  decoration: BoxDecoration(
                    color: theme.colorScheme.surfaceContainerHighest,
                    borderRadius: BorderRadius.circular(8),
                  ),
                ),
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _buildEmptyState(ThemeData theme, ColorScheme colorScheme) {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.insert_chart_outlined,
            size: 64,
            color: colorScheme.onSurfaceVariant.withValues(alpha: 0.4),
          ),
          const SizedBox(height: 16),
          Text(
            '暂无报告数据',
            style: theme.textTheme.titleMedium?.copyWith(
              color: colorScheme.onSurfaceVariant,
            ),
          ),
          const SizedBox(height: 8),
          FilledButton.tonalIcon(
            onPressed: () => context.read<DeepSeekProvider>().loadReport(),
            icon: const Icon(Icons.refresh),
            label: const Text('重新加载'),
          ),
        ],
      ),
    );
  }

  /// 从 report.summaryItems 中查找第一个 label 包含 [key] 的项，返回其 value
  /// 若未找到则返回 [fallback]
  String _getItemValue(String key, {String fallback = '--'}) {
    final provider = context.read<DeepSeekProvider>();
    final report = provider.report;
    if (report == null) return fallback;
    try {
      final item = report.summaryItems.firstWhere(
        (item) => item.label.contains(key),
      );
      return item.value;
    } catch (_) {
      return fallback;
    }
  }

  Widget _buildReportContent(
    BuildContext context,
    DeepSeekProvider provider,
    ThemeData theme,
    ColorScheme colorScheme,
  ) {
    final report = provider.report!;
    final dateStr = DateFormat('yyyy年MM月dd日').format(report.date);

    return RefreshIndicator(
      onRefresh: () => context.read<DeepSeekProvider>().loadReport(),
      child: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          Text(
            dateStr,
            style: theme.textTheme.titleLarge?.copyWith(
              fontWeight: FontWeight.bold,
            ),
            textAlign: TextAlign.center,
          ),
          const SizedBox(height: 16),
          _buildSummaryCard(
            theme: theme,
            colorScheme: colorScheme,
            icon: Icons.directions_walk,
            title: '运动总结',
            children: [
              _buildReportItem(
                label: '步数',
                value: '${_getItemValue('步数', fallback: '--')}',
                icon: Icons.directions_walk,
                colorScheme: colorScheme,
              ),
              _buildReportItem(
                label: '卡路里',
                value: '${_getItemValue('卡路里', fallback: '--')}',
                icon: Icons.local_fire_department,
                colorScheme: colorScheme,
              ),
            ],
          ),
          const SizedBox(height: 12),
          _buildSummaryCard(
            theme: theme,
            colorScheme: colorScheme,
            icon: Icons.favorite,
            title: '心率分析',
            children: [
              _buildReportItem(
                label: '静息心率',
                value: '${_getItemValue('静息', fallback: '-- bpm')}',
                icon: Icons.favorite_border,
                colorScheme: colorScheme,
              ),
              _buildReportItem(
                label: '平均心率',
                value: '${_getItemValue('平均', fallback: '-- bpm')}',
                icon: Icons.favorite,
                colorScheme: colorScheme,
              ),
              _buildReportItem(
                label: '最高心率',
                value: '${_getItemValue('最高', fallback: '-- bpm')}',
                icon: Icons.favorite,
                colorScheme: colorScheme,
              ),
            ],
          ),
          const SizedBox(height: 12),
          _buildSummaryCard(
            theme: theme,
            colorScheme: colorScheme,
            icon: Icons.bedtime,
            title: '睡眠评估',
            children: [
              _buildReportItem(
                label: '睡眠时长',
                value: '${_getItemValue('睡眠时长', fallback: '-- 小时')}',
                icon: Icons.schedule,
                colorScheme: colorScheme,
              ),
              _buildReportItem(
                label: '睡眠质量',
                value: '${_getItemValue('睡眠质量', fallback: '--')}',
                icon: Icons.star_half,
                colorScheme: colorScheme,
              ),
            ],
          ),
          const SizedBox(height: 12),
          _buildAdviceCard(
            theme: theme,
            colorScheme: colorScheme,
            advice: report.content,
          ),
        ],
      ),
    );
  }

  Widget _buildSummaryCard({
    required ThemeData theme,
    required ColorScheme colorScheme,
    required IconData icon,
    required String title,
    required List<Widget> children,
  }) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(icon, color: colorScheme.primary, size: 20),
                const SizedBox(width: 8),
                Text(
                  title,
                  style: theme.textTheme.titleSmall?.copyWith(
                    fontWeight: FontWeight.w600,
                    color: colorScheme.primary,
                  ),
                ),
              ],
            ),
            const Divider(height: 24),
            ...children,
          ],
        ),
      ),
    );
  }

  Widget _buildReportItem({
    required String label,
    required String value,
    required IconData icon,
    required ColorScheme colorScheme,
  }) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 6),
      child: Row(
        children: [
          Icon(icon, size: 18, color: colorScheme.onSurfaceVariant),
          const SizedBox(width: 12),
          Expanded(
            child: Text(
              label,
              style: TextStyle(color: colorScheme.onSurfaceVariant),
            ),
          ),
          Text(value, style: const TextStyle(fontWeight: FontWeight.w600)),
        ],
      ),
    );
  }

  Widget _buildAdviceCard({
    required ThemeData theme,
    required ColorScheme colorScheme,
    required String advice,
  }) {
    return Card(
      color: colorScheme.primaryContainer.withValues(alpha: 0.3),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.auto_awesome, color: colorScheme.primary, size: 20),
                const SizedBox(width: 8),
                Text(
                  'AI 建议',
                  style: theme.textTheme.titleSmall?.copyWith(
                    fontWeight: FontWeight.w600,
                    color: colorScheme.primary,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            Container(
              width: double.infinity,
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: colorScheme.surfaceContainerLow,
                borderRadius: BorderRadius.circular(12),
                border: Border(
                  left: BorderSide(color: colorScheme.primary, width: 3),
                ),
              ),
              child: Text(
                '"$advice"',
                style: theme.textTheme.bodyMedium?.copyWith(
                  fontStyle: FontStyle.italic,
                  height: 1.5,
                  color: colorScheme.onSurface,
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
