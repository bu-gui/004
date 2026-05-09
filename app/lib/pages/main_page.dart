import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'dashboard_page.dart';
import 'history_page.dart';
import 'sleep_page.dart';
import 'report_page.dart';
import 'plan_page.dart';
import 'device_scan_page.dart';
import '../providers/ble_provider.dart';

class MainPage extends StatefulWidget {
  const MainPage({super.key});

  @override
  State<MainPage> createState() => _MainPageState();
}

class _MainPageState extends State<MainPage> {
  int _selectedIndex = 0;
  bool _wasConnected = false;

  static const List<Widget> _pages = [
    DashboardPage(),
    HistoryPage(),
    SleepPage(),
    ReportPage(),
    PlanPage(),
    DeviceScanPage(),
  ];

  static const List<String> _titles = ['仪表盘', '历史', '睡眠', '报告', '计划', '设备'];

  void _onItemTapped(int index) {
    setState(() {
      _selectedIndex = index;
    });
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    final bleProvider = context.watch<BleProvider>();
    if (bleProvider.isConnected && !_wasConnected && _selectedIndex == 5) {
      _wasConnected = true;
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (mounted) {
          _onItemTapped(0);
        }
      });
    } else if (!bleProvider.isConnected) {
      _wasConnected = false;
    }

    return Scaffold(
      appBar: AppBar(
        title: Text(_titles[_selectedIndex]),
        centerTitle: true,
        elevation: 0,
      ),
      body: _pages[_selectedIndex],
      bottomNavigationBar: BottomNavigationBar(
        items: const <BottomNavigationBarItem>[
          BottomNavigationBarItem(icon: Icon(Icons.dashboard), label: '仪表盘'),
          BottomNavigationBarItem(icon: Icon(Icons.history), label: '历史'),
          BottomNavigationBarItem(icon: Icon(Icons.bed), label: '睡眠'),
          BottomNavigationBarItem(icon: Icon(Icons.description), label: '报告'),
          BottomNavigationBarItem(icon: Icon(Icons.sports), label: '计划'),
          BottomNavigationBarItem(
            icon: Icon(Icons.bluetooth_searching),
            label: '设备',
          ),
        ],
        currentIndex: _selectedIndex,
        selectedItemColor: colorScheme.primary,
        unselectedItemColor: colorScheme.onSurface.withOpacity(0.6),
        type: BottomNavigationBarType.fixed,
        onTap: _onItemTapped,
      ),
    );
  }
}
