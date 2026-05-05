import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:intl/intl.dart';
import '../providers/deepseek_provider.dart';

class PlanPage extends StatefulWidget {
  const PlanPage({super.key});

  @override
  State<PlanPage> createState() => _PlanPageState();
}

class _PlanPageState extends State<PlanPage> with SingleTickerProviderStateMixin {
  late TabController _tabController;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 7, vsync: this);
    WidgetsBinding.instance.addPostFrameCallback((_) {
      context.read<DeepSeekProvider>().loadPlans();
    });
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<DeepSeekProvider>();
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    return Scaffold(
      appBar: AppBar(
        title: const Text('今日训练计划'),
        centerTitle: true,
      ),
      body: provider.isLoading
          ? _buildSkeletonLoader(theme)
          : provider.weeklyPlans == null || provider.weeklyPlans!.isEmpty
              ? _buildEmptyState(theme, colorScheme, provider)
              : _buildPlanContent(provider, theme, colorScheme),
    );
  }

  Widget _buildSkeletonLoader(ThemeData theme) {
    return ListView.builder(
      padding: const EdgeInsets.all(16),
      itemCount: 4,
      itemBuilder: (context, index) {
        return Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Row(
              children: [
                Container(
                  width: 48,
                  height: 48,
                  decoration: BoxDecoration(
                    color: theme.colorScheme.surfaceContainerHighest,
                    borderRadius: BorderRadius.circular(8),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Container(
                        width: 80,
                        height: 14,
                        decoration: BoxDecoration(
                          color: theme.colorScheme.surfaceContainerHighest,
                          borderRadius: BorderRadius.circular(4),
                        ),
                      ),
                      const SizedBox(height: 8),
                      Container(
                        width: 160,
                        height: 14,
                        decoration: BoxDecoration(
                          color: theme.colorScheme.surfaceContainerHighest,
                          borderRadius: BorderRadius.circular(4),
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _buildEmptyState(
    ThemeData theme,
    ColorScheme colorScheme,
    DeepSeekProvider provider,
  ) {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.event_note_outlined,
            size: 64,
            color: colorScheme.onSurfaceVariant.withValues(alpha: 0.4),
          ),
          const SizedBox(height: 16),
          Text(
            '暂无训练计划',
            style: theme.textTheme.titleMedium?.copyWith(
              color: colorScheme.onSurfaceVariant,
            ),
          ),
          const SizedBox(height: 8),
          FilledButton.tonalIcon(
            onPressed: () => context.read<DeepSeekProvider>().loadPlans(),
            icon: const Icon(Icons.refresh),
            label: const Text('重新加载'),
          ),
        ],
      ),
    );
  }

  Widget _buildPlanContent(
    DeepSeekProvider provider,
    ThemeData theme,
    ColorScheme colorScheme,
  ) {
    final today = DateTime.now();
    final dayNames = ['一', '二', '三', '四', '五', '六', '日'];

    return Column(
      children: [
        Material(
          color: colorScheme.surface,
          child: TabBar(
            controller: _tabController,
            isScrollable: false,
            labelColor: colorScheme.onPrimaryContainer,
            unselectedLabelColor: colorScheme.onSurfaceVariant,
            indicatorColor: colorScheme.primary,
            tabs: List.generate(7, (index) {
              final date = today.add(Duration(days: index));
              return Tab(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      '周${dayNames[date.weekday - 1]}',
                      style: const TextStyle(fontSize: 12),
                    ),
                    const SizedBox(height: 2),
                    Text(
                      DateFormat('MM/dd').format(date),
                      style: const TextStyle(fontSize: 11),
                    ),
                  ],
                ),
              );
            }),
          ),
        ),
        Expanded(
          child: TabBarView(
            controller: _tabController,
            children: List.generate(7, (index) {
              final date = today.add(Duration(days: index));
              final dateStr = DateFormat('yyyy-MM-dd').format(date);
              final plans = provider.weeklyPlans!
                  .where((p) => p.date == dateStr)
                  .toList();

              if (plans.isEmpty) {
                return Center(
                  child: Text(
                    '当日暂无计划',
                    style: TextStyle(color: colorScheme.onSurfaceVariant),
                  ),
                );
              }

              return RefreshIndicator(
                onRefresh: () => context.read<DeepSeekProvider>().loadPlans(),
                child: ListView.builder(
                  padding: const EdgeInsets.all(16),
                  itemCount: plans.length,
                  itemBuilder: (context, i) {
                    final plan = plans[i];
                    return Card(
                      child: ListTile(
                        leading: CircleAvatar(
                          backgroundColor: plan.isCompleted
                              ? colorScheme.primaryContainer
                              : colorScheme.surfaceContainerHighest,
                          child: plan.isCompleted
                              ? Icon(
                                  Icons.check,
                                  color: colorScheme.primary,
                                )
                              : Text(
                                  plan.time.substring(0, 2),
                                  style: TextStyle(
                                    fontWeight: FontWeight.w600,
                                    color: colorScheme.onSurface,
                                  ),
                                ),
                        ),
                        title: Text(
                          plan.activityType,
                          style: const TextStyle(fontWeight: FontWeight.w600),
                        ),
                        subtitle: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            const SizedBox(height: 4),
                            Row(
                              children: [
                                Icon(
                                  Icons.schedule,
                                  size: 14,
                                  color: colorScheme.onSurfaceVariant,
                                ),
                                const SizedBox(width: 4),
                                Text(
                                  '${plan.time}  ·  ${plan.duration}分钟',
                                  style: TextStyle(
                                    fontSize: 13,
                                    color: colorScheme.onSurfaceVariant,
                                  ),
                                ),
                              ],
                            ),
                            const SizedBox(height: 2),
                            Row(
                              children: [
                                Icon(
                                  Icons.favorite,
                                  size: 14,
                                  color: colorScheme.onSurfaceVariant,
                                ),
                                const SizedBox(width: 4),
                                Text(
                                  '目标心率: ${plan.targetHeartRateZone}',
                                  style: TextStyle(
                                    fontSize: 13,
                                    color: colorScheme.onSurfaceVariant,
                                  ),
                                ),
                              ],
                            ),
                          ],
                        ),
                        trailing: plan.isCompleted
                            ? Icon(
                                Icons.check_circle,
                                color: colorScheme.primary,
                              )
                            : null,
                      ),
                    );
                  },
                ),
              );
            }),
          ),
        ),
      ],
    );
  }
}
