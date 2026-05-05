import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import 'app.dart';
import 'providers/theme_provider.dart';
import 'providers/health_data_provider.dart';
import 'providers/ble_provider.dart';
import 'providers/deepseek_provider.dart';
import 'providers/user_settings_provider.dart';
import 'services/database_service.dart';
import 'services/deepseek_api_service.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  final databaseService = DatabaseService();
  final deepseekApiService = DeepSeekApiService();

  runApp(
    MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => ThemeProvider()),
        ChangeNotifierProvider(create: (_) => BleProvider()),
        ChangeNotifierProvider(
          create: (_) => HealthDataProvider(databaseService),
        ),
        ChangeNotifierProvider(
          create: (_) => UserSettingsProvider(databaseService),
        ),
        ChangeNotifierProvider(
          create: (_) => DeepSeekProvider(deepseekApiService, databaseService),
        ),
      ],
      child: const SmartBandApp(),
    ),
  );
}
