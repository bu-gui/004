import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/ble_provider.dart';
import '../models/ble_device.dart';
import '../config/routes.dart';

class DeviceScanPage extends StatefulWidget {
  const DeviceScanPage({super.key});

  @override
  State<DeviceScanPage> createState() => _DeviceScanPageState();
}

class _DeviceScanPageState extends State<DeviceScanPage> {
  late BleProvider _bleProvider;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    _bleProvider = context.read<BleProvider>();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted && !_bleProvider.isScanning) {
        _bleProvider.startScan();
      }
    });
  }

  @override
  void dispose() {
    if (_bleProvider.isScanning) {
      _bleProvider.stopScan();
    }
    super.dispose();
  }

  void _showConnectDialog(BuildContext context, BleDevice device) {
    showDialog(
      context: context,
      builder: (dialogContext) {
        return AlertDialog(
          title: const Text('连接设备'),
          content: Text('是否连接 "${device.name}"？'),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(dialogContext).pop(),
              child: const Text('取消'),
            ),
            FilledButton(
              onPressed: () {
                Navigator.of(dialogContext).pop();
                _connectDevice(context, device);
              },
              child: const Text('连接'),
            ),
          ],
        );
      },
    );
  }

  Future<void> _connectDevice(BuildContext context, BleDevice device) async {
    final provider = context.read<BleProvider>();
    final scaffoldMessenger = ScaffoldMessenger.of(context);

    try {
      await provider.connect(device);
      if (mounted) {
        scaffoldMessenger.showSnackBar(
          SnackBar(
            content: Text('已成功连接 ${device.name}'),
            backgroundColor: Colors.green,
          ),
        );
        Navigator.of(context).pushReplacementNamed(AppRoutes.dashboard);
      }
    } catch (e) {
      if (mounted) {
        scaffoldMessenger.showSnackBar(
          SnackBar(content: Text('连接失败: $e'), backgroundColor: Colors.red),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final colorScheme = Theme.of(context).colorScheme;

    return Scaffold(
      appBar: AppBar(
        title: const Text('设备扫描'),
        actions: [
          Consumer<BleProvider>(
            builder: (context, provider, child) {
              return IconButton(
                icon: Icon(provider.isScanning ? Icons.stop : Icons.search),
                onPressed: () {
                  if (provider.isScanning) {
                    provider.stopScan();
                  } else {
                    provider.startScan();
                  }
                },
              );
            },
          ),
        ],
      ),
      body: Consumer<BleProvider>(
        builder: (context, provider, child) {
          return Column(
            children: [
              _buildStatusBar(context, provider),
              Expanded(
                child: provider.deviceList.isEmpty
                    ? _buildEmptyState(context, provider)
                    : _buildDeviceList(context, provider),
              ),
            ],
          );
        },
      ),
    );
  }

  Widget _buildStatusBar(BuildContext context, BleProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
      color: colorScheme.surfaceContainerLow,
      child: Row(
        children: [
          Container(
            width: 10,
            height: 10,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: provider.isScanning ? Colors.green : Colors.grey,
            ),
          ),
          const SizedBox(width: 10),
          Text(
            provider.isScanning ? '扫描中...' : '未扫描',
            style: Theme.of(context).textTheme.bodyMedium?.copyWith(
              color: provider.isScanning
                  ? Colors.green
                  : colorScheme.onSurfaceVariant,
            ),
          ),
          const Spacer(),
          Text(
            '发现 ${provider.deviceList.length} 个设备',
            style: Theme.of(context).textTheme.bodySmall?.copyWith(
              color: colorScheme.onSurfaceVariant,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildEmptyState(BuildContext context, BleProvider provider) {
    final colorScheme = Theme.of(context).colorScheme;

    // 如果有错误信息，优先显示错误
    if (provider.error != null) {
      return Center(
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 32),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(Icons.error_outline, size: 64, color: colorScheme.error),
              const SizedBox(height: 16),
              Text(
                provider.error!,
                textAlign: TextAlign.center,
                style: Theme.of(
                  context,
                ).textTheme.bodyLarge?.copyWith(color: colorScheme.error),
              ),
              const SizedBox(height: 8),
              if (provider.error!.contains('蓝牙权限'))
                Text(
                  '请前往 设置 > 应用 > 智能手环 > 权限，开启蓝牙和位置权限',
                  textAlign: TextAlign.center,
                  style: Theme.of(context).textTheme.bodySmall?.copyWith(
                    color: colorScheme.onSurfaceVariant,
                  ),
                ),
              if (!provider.error!.contains('蓝牙权限'))
                Text(
                  '确保手机蓝牙已开启，且设备已开机并处于广播状态',
                  textAlign: TextAlign.center,
                  style: Theme.of(context).textTheme.bodySmall?.copyWith(
                    color: colorScheme.onSurfaceVariant,
                  ),
                ),
              const SizedBox(height: 24),
              FilledButton.icon(
                icon: const Icon(Icons.refresh),
                label: const Text('重新扫描'),
                onPressed: () => provider.startScan(),
              ),
            ],
          ),
        ),
      );
    }

    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.bluetooth_searching,
            size: 80,
            color: colorScheme.onSurfaceVariant.withValues(alpha: 0.5),
          ),
          const SizedBox(height: 16),
          Text(
            provider.isScanning ? '正在搜索设备...' : '点击搜索按钮开始扫描',
            style: Theme.of(context).textTheme.bodyLarge?.copyWith(
              color: colorScheme.onSurfaceVariant,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            '确保设备已开启蓝牙',
            style: Theme.of(context).textTheme.bodySmall?.copyWith(
              color: colorScheme.onSurfaceVariant.withValues(alpha: 0.7),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildDeviceList(BuildContext context, BleProvider provider) {
    // 按 RSSI 信号强度降序排序（信号强的排前面）
    final sortedDevices = List<BleDevice>.from(provider.devices)
      ..sort((a, b) => b.rssi.compareTo(a.rssi));

    return ListView.builder(
      padding: const EdgeInsets.all(16),
      itemCount: sortedDevices.length,
      itemBuilder: (context, index) {
        final device = sortedDevices[index];
        return Card(
          key: ValueKey(device.macAddress),
          margin: const EdgeInsets.only(bottom: 12),
          child: ListTile(
            leading: Container(
              width: 48,
              height: 48,
              decoration: BoxDecoration(
                color: Theme.of(context).colorScheme.primaryContainer,
                borderRadius: BorderRadius.circular(12),
              ),
              child: Icon(
                Icons.watch,
                color: Theme.of(context).colorScheme.primary,
              ),
            ),
            title: Text(
              device.name.isNotEmpty ? device.name : '未知设备',
              style: const TextStyle(fontWeight: FontWeight.w600),
            ),
            subtitle: Text(device.macAddress),
            trailing: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Container(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 8,
                    vertical: 4,
                  ),
                  decoration: BoxDecoration(
                    color: Colors.green.withValues(alpha: 0.1),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Text(
                    '${device.rssi} dBm',
                    style: const TextStyle(
                      color: Colors.green,
                      fontSize: 12,
                      fontWeight: FontWeight.w500,
                    ),
                  ),
                ),
                const SizedBox(width: 8),
                IconButton(
                  icon: const Icon(Icons.link),
                  onPressed: () => _showConnectDialog(context, device),
                ),
              ],
            ),
          ),
        );
      },
    );
  }
}
