import 'package:flutter/material.dart';
import 'package:smart_band/pages/main_page.dart';
import 'package:smart_band/pages/dashboard_page.dart';
import 'package:smart_band/pages/history_page.dart';
import 'package:smart_band/pages/sleep_page.dart';
import 'package:smart_band/pages/report_page.dart';
import 'package:smart_band/pages/plan_page.dart';
import 'package:smart_band/pages/ai_assistant_page.dart';
import 'package:smart_band/pages/settings_page.dart';
import 'package:smart_band/pages/device_scan_page.dart';

class AppRoutes {
  static const String home = '/';
  static const String dashboard = '/dashboard';
  static const String history = '/history';
  static const String sleep = '/sleep';
  static const String report = '/report';
  static const String plan = '/plan';
  static const String aiAssistant = '/ai_assistant';
  static const String settings = '/settings';
  static const String deviceScan = '/device_scan';

  static final Map<String, WidgetBuilder> routes = {
    home: (_) => const MainPage(),
    dashboard: (_) => const DashboardPage(),
    history: (_) => const HistoryPage(),
    sleep: (_) => const SleepPage(),
    report: (_) => const ReportPage(),
    plan: (_) => const PlanPage(),
    aiAssistant: (_) => const AiAssistantPage(),
    settings: (_) => const SettingsPage(),
    deviceScan: (_) => const DeviceScanPage(),
  };
}
