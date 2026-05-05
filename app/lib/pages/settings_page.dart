import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/user_settings_provider.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage({super.key});

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  final _heightController = TextEditingController();
  final _weightController = TextEditingController();
  final _ageController = TextEditingController();
  final _stepGoalController = TextEditingController();
  final _calorieGoalController = TextEditingController();
  final _sleepGoalController = TextEditingController();

  String _selectedGender = '男';
  bool _useImperialUnits = false;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _loadSettings();
    });
  }

  void _loadSettings() {
    final provider = context.read<UserSettingsProvider>();
    _heightController.text = provider.height.toStringAsFixed(0);
    _weightController.text = provider.weight.toStringAsFixed(1);
    _ageController.text = provider.age.toString();
    _selectedGender = provider.gender;
    _stepGoalController.text = provider.stepGoal.toString();
    _calorieGoalController.text = provider.calorieGoal.toString();
    _sleepGoalController.text = provider.sleepGoal.toString();
    _useImperialUnits = provider.useImperialUnits;
    setState(() {});
  }

  @override
  void dispose() {
    _heightController.dispose();
    _weightController.dispose();
    _ageController.dispose();
    _stepGoalController.dispose();
    _calorieGoalController.dispose();
    _sleepGoalController.dispose();
    super.dispose();
  }

  Future<void> _saveSettings() async {
    final provider = context.read<UserSettingsProvider>();
    provider.height = double.tryParse(_heightController.text) ?? 170;
    provider.weight = double.tryParse(_weightController.text) ?? 70;
    provider.age = int.tryParse(_ageController.text) ?? 25;
    provider.gender = _selectedGender;
    provider.stepGoal = int.tryParse(_stepGoalController.text) ?? 8000;
    provider.calorieGoal = int.tryParse(_calorieGoalController.text) ?? 300;
    provider.sleepGoal = int.tryParse(_sleepGoalController.text) ?? 8;
    provider.useImperialUnits = _useImperialUnits;

    await provider.saveSettings();

    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('设置已保存')),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<UserSettingsProvider>();
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    return Scaffold(
      appBar: AppBar(
        title: const Text('设置'),
        centerTitle: true,
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          _buildSectionHeader(theme, colorScheme, '设备连接'),
          const SizedBox(height: 8),
          Card(
            child: ListTile(
              leading: CircleAvatar(
                backgroundColor: provider.isDeviceConnected
                    ? colorScheme.primaryContainer
                    : colorScheme.errorContainer,
                child: Icon(
                  provider.isDeviceConnected
                      ? Icons.bluetooth_connected
                      : Icons.bluetooth_disabled,
                  color: provider.isDeviceConnected
                      ? colorScheme.primary
                      : colorScheme.error,
                ),
              ),
              title: Text(
                provider.isDeviceConnected ? '已连接' : '未连接',
              ),
              subtitle: Text(
                provider.isDeviceConnected ? '点击查看设备详情' : '点击扫描设备',
              ),
              trailing: const Icon(Icons.chevron_right),
              onTap: () {
                Navigator.pushNamed(context, '/device_scan');
              },
            ),
          ),
          const SizedBox(height: 24),
          _buildSectionHeader(theme, colorScheme, '个人参数'),
          const SizedBox(height: 8),
          Card(
            child: Column(
              children: [
                _buildTextFieldTile(
                  context: context,
                  label: '身高',
                  controller: _heightController,
                  suffix: 'cm',
                  keyboardType: TextInputType.number,
                  colorScheme: colorScheme,
                ),
                const Divider(height: 1, indent: 16, endIndent: 16),
                _buildTextFieldTile(
                  context: context,
                  label: '体重',
                  controller: _weightController,
                  suffix: 'kg',
                  keyboardType: TextInputType.numberWithOptions(decimal: true),
                  colorScheme: colorScheme,
                ),
                const Divider(height: 1, indent: 16, endIndent: 16),
                _buildTextFieldTile(
                  context: context,
                  label: '年龄',
                  controller: _ageController,
                  suffix: '岁',
                  keyboardType: TextInputType.number,
                  colorScheme: colorScheme,
                ),
                const Divider(height: 1, indent: 16, endIndent: 16),
                _buildGenderTile(colorScheme),
              ],
            ),
          ),
          const SizedBox(height: 24),
          _buildSectionHeader(theme, colorScheme, '每日目标'),
          const SizedBox(height: 8),
          Card(
            child: Column(
              children: [
                _buildTextFieldTile(
                  context: context,
                  label: '步数目标',
                  controller: _stepGoalController,
                  suffix: '步',
                  keyboardType: TextInputType.number,
                  colorScheme: colorScheme,
                ),
                const Divider(height: 1, indent: 16, endIndent: 16),
                _buildTextFieldTile(
                  context: context,
                  label: '卡路里目标',
                  controller: _calorieGoalController,
                  suffix: 'kcal',
                  keyboardType: TextInputType.number,
                  colorScheme: colorScheme,
                ),
                const Divider(height: 1, indent: 16, endIndent: 16),
                _buildTextFieldTile(
                  context: context,
                  label: '睡眠时长目标',
                  controller: _sleepGoalController,
                  suffix: '小时',
                  keyboardType: TextInputType.numberWithOptions(decimal: true),
                  colorScheme: colorScheme,
                ),
              ],
            ),
          ),
          const SizedBox(height: 24),
          _buildSectionHeader(theme, colorScheme, '单位设置'),
          const SizedBox(height: 8),
          Card(
            child: SwitchListTile(
              title: const Text('使用英制单位'),
              subtitle: Text(
                _useImperialUnits ? '英尺/磅' : '厘米/公斤',
              ),
              value: _useImperialUnits,
              onChanged: (value) {
                setState(() => _useImperialUnits = value);
              },
              secondary: Icon(
                _useImperialUnits ? Icons.straighten : Icons.monitor_weight,
                color: colorScheme.primary,
              ),
            ),
          ),
          const SizedBox(height: 32),
          FilledButton.icon(
            onPressed: _saveSettings,
            icon: const Icon(Icons.save),
            label: const Text('保存设置'),
            style: FilledButton.styleFrom(
              minimumSize: const Size(double.infinity, 52),
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(16),
              ),
            ),
          ),
          const SizedBox(height: 24),
        ],
      ),
    );
  }

  Widget _buildSectionHeader(
    ThemeData theme,
    ColorScheme colorScheme,
    String title,
  ) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 4),
      child: Text(
        title,
        style: theme.textTheme.titleSmall?.copyWith(
          fontWeight: FontWeight.w600,
          color: colorScheme.primary,
        ),
      ),
    );
  }

  Widget _buildTextFieldTile({
    required BuildContext context,
    required String label,
    required TextEditingController controller,
    required String suffix,
    required TextInputType keyboardType,
    required ColorScheme colorScheme,
  }) {
    return ListTile(
      title: Text(label),
      trailing: SizedBox(
        width: 160,
        child: TextField(
          controller: controller,
          keyboardType: keyboardType,
          textAlign: TextAlign.right,
          decoration: InputDecoration(
            suffixText: suffix,
            suffixStyle: TextStyle(
              color: colorScheme.onSurfaceVariant,
              fontSize: 14,
            ),
            border: InputBorder.none,
            isDense: true,
            contentPadding: const EdgeInsets.symmetric(vertical: 8),
          ),
          style: const TextStyle(fontSize: 16),
        ),
      ),
    );
  }

  Widget _buildGenderTile(ColorScheme colorScheme) {
    return ListTile(
      title: const Text('性别'),
      trailing: DropdownButton<String>(
        value: _selectedGender,
        underline: const SizedBox(),
        items: const [
          DropdownMenuItem(value: '男', child: Text('男')),
          DropdownMenuItem(value: '女', child: Text('女')),
        ],
        onChanged: (value) {
          if (value != null) {
            setState(() => _selectedGender = value);
          }
        },
      ),
    );
  }
}
