import 'dart:math';
import 'package:flutter/foundation.dart';
import 'package:smart_band/models/health_data.dart';
import 'package:smart_band/models/daily_summary.dart';
import 'package:smart_band/models/sleep_data.dart';
import 'package:smart_band/models/ble_device.dart';
import 'package:smart_band/services/database_service.dart';

class HealthDataProvider extends ChangeNotifier {
  final DatabaseService _databaseService;

  HealthDataProvider(this._databaseService);

  double _currentHeartRate = 0;
  int _currentSpo2 = 0;
  int _currentSteps = 0;
  double _currentCalories = 0;
  int _currentMotion = 0;
  int _batteryLevel = 0;
  bool _fallAlert = false;
  bool _isOffline = true;
  DailySummary _todaySummary = DailySummary(
    id: 0,
    date: DateTime.now().toIso8601String().split('T')[0],
    totalSteps: 0,
    totalCalories: 0,
    avgHeartRate: 0,
    minHeartRate: 0,
    maxHeartRate: 0,
    avgSpo2: 0,
    motionMinutes: 0,
    sleepHours: 0,
    sleepQuality: 0,
    fallCount: 0,
  );
  List<HealthRecord> _healthRecords = [];

  double get currentHeartRate => _currentHeartRate;
  int get currentSpo2 => _currentSpo2;
  int get currentSteps => _currentSteps;
  double get currentCalories => _currentCalories;
  int get currentMotion => _currentMotion;
  int get batteryLevel => _batteryLevel;
  bool get fallAlert => _fallAlert;
  double get heartRate => _currentHeartRate;
  int get bloodOxygen => _currentSpo2;
  int get steps => _currentSteps;
  double get calories => _currentCalories;
  String get sportType {
    switch (_currentMotion) {
      case 1:
        return '走路';
      case 2:
        return '跑步';
      case 3:
        return '骑行';
      default:
        return '静止';
    }
  }

  bool get isOnline => !_isOffline;
  DailySummary get todaySummary => _todaySummary;
  List<HealthRecord> get healthRecords => List.unmodifiable(_healthRecords);

  Future<void> updateFromBle(BlePacket packet) async {
    _currentHeartRate = packet.heartRate;
    _currentSpo2 = packet.spo2;
    _currentSteps = packet.steps;
    _currentCalories = packet.calories;
    _currentMotion = packet.motionType;
    _batteryLevel = packet.battery.toInt();
    _fallAlert = packet.fallDetected > 0;
    _isOffline = false;

    final record = HealthRecord(
      id: 0,
      timestamp: DateTime.now().toIso8601String(),
      heartRate: packet.heartRate,
      spo2: packet.spo2,
      steps: packet.steps,
      calories: packet.calories,
      motionType: packet.motionType,
      battery: packet.battery.toInt(),
    );

    try {
      await _databaseService.insertHealthRecord(record);
      await _updateDailySummary(record);
    } catch (_) {}

    notifyListeners();
  }

  Future<void> loadTodayData() async {
    try {
      final now = DateTime.now();
      final startOfDay = DateTime(now.year, now.month, now.day);
      final endOfDay = startOfDay.add(const Duration(days: 1));
      final dateStr = startOfDay.toIso8601String().split('T')[0];

      _healthRecords = await _databaseService.getHealthRecords(
        startDate: startOfDay.toIso8601String(),
        endDate: endOfDay.toIso8601String(),
      );

      _todaySummary =
          await _databaseService.getTodaySummary() ??
          DailySummary(
            id: 0,
            date: dateStr,
            totalSteps: 0,
            totalCalories: 0,
            avgHeartRate: 0,
            minHeartRate: 0,
            maxHeartRate: 0,
            avgSpo2: 0,
            motionMinutes: 0,
            sleepHours: 0,
            sleepQuality: 0,
            fallCount: 0,
          );

      if (_healthRecords.isNotEmpty) {
        final latest = _healthRecords.last;
        _currentHeartRate = latest.heartRate;
        _currentSpo2 = latest.spo2;
        _currentSteps = latest.steps;
        _currentCalories = latest.calories;
        _currentMotion = latest.motionType;
        _batteryLevel = latest.battery;
      }

      notifyListeners();
    } catch (_) {
      notifyListeners();
    }
  }

  Future<void> loadHistory(DateTime startDate, DateTime endDate) async {
    try {
      _healthRecords = await _databaseService.getHealthRecords(
        startDate: startDate.toIso8601String(),
        endDate: endDate.toIso8601String(),
      );
      notifyListeners();
    } catch (_) {
      notifyListeners();
    }
  }

  Future<void> refreshDailySummary() async {
    try {
      final now = DateTime.now();
      final dateStr = now.toIso8601String().split('T')[0];
      _todaySummary =
          await _databaseService.getTodaySummary() ??
          DailySummary(
            id: 0,
            date: dateStr,
            totalSteps: 0,
            totalCalories: 0,
            avgHeartRate: 0,
            minHeartRate: 0,
            maxHeartRate: 0,
            avgSpo2: 0,
            motionMinutes: 0,
            sleepHours: 0,
            sleepQuality: 0,
            fallCount: 0,
          );
      notifyListeners();
    } catch (_) {
      notifyListeners();
    }
  }

  Future<void> _updateDailySummary(HealthRecord record) async {
    final now = DateTime.now();
    final startOfDay = DateTime(now.year, now.month, now.day);
    final endOfDay = startOfDay.add(const Duration(days: 1));

    final records = await _databaseService.getHealthRecords(
      startDate: startOfDay.toIso8601String(),
      endDate: endOfDay.toIso8601String(),
    );

    if (records.isEmpty) return;

    double avgHeartRate = 0;
    double avgSpo2 = 0;
    int maxSteps = 0;
    double totalCalories = 0;
    int maxMotion = 0;

    for (var r in records) {
      avgHeartRate += r.heartRate;
      avgSpo2 += r.spo2;
      if (r.steps > maxSteps) maxSteps = r.steps;
      totalCalories += r.calories;
      if (r.motionType > maxMotion) maxMotion = r.motionType;
    }

    avgHeartRate /= records.length;
    avgSpo2 /= records.length;

    _todaySummary = DailySummary(
      id: 0,
      date: startOfDay.toIso8601String().split('T')[0],
      totalSteps: maxSteps,
      totalCalories: totalCalories,
      avgHeartRate: avgHeartRate,
      minHeartRate: 0,
      maxHeartRate: avgHeartRate,
      avgSpo2: avgSpo2.round(),
      motionMinutes: maxMotion,
      sleepHours: 0,
      sleepQuality: 0,
      fallCount: 0,
    );

    await _databaseService.insertOrUpdateSummary(_todaySummary);
  }

  Future<void> saveDailySummary(DailySummary summary) async {
    try {
      await _databaseService.insertOrUpdateSummary(summary);
      _todaySummary = summary;
      notifyListeners();
    } catch (_) {
      notifyListeners();
    }
  }

  List<double> getHourlySteps([int day = 0]) {
    final random = Random();
    return List.generate(24, (_) => random.nextDouble() * 1000);
  }

  List<double> getDailyStepsThisWeek() {
    final random = Random();
    return List.generate(7, (_) => random.nextDouble() * 10000);
  }

  List<double> getDailyStepsThisMonth() {
    final random = Random();
    return List.generate(30, (_) => random.nextDouble() * 10000);
  }

  List<double> getHeartRateTrend() {
    final random = Random();
    return List.generate(24, (_) => 60 + random.nextDouble() * 60);
  }

  Map<String, double> getSportTypeDistribution() {
    return {'静止': 10.0, '走路': 50.0, '跑步': 30.0, '骑行': 10.0};
  }

  List<Map<String, dynamic>> getHistoryRecords() {
    return _healthRecords
        .map(
          (r) => {
            'sportType': _motionTypeToString(r.motionType),
            'dateTime': DateTime.parse(r.timestamp),
            'steps': r.steps,
            'heartRate': r.heartRate,
            'calories': r.calories,
          },
        )
        .toList();
  }

  SleepData? getTodaySleep() {
    final now = DateTime.now();
    return SleepData(
      totalSleepHours: 7.5,
      deepSleepHours: 2.5,
      lightSleepHours: 5.0,
      quality: 75,
      sleepTime: now.subtract(const Duration(hours: 8)).toIso8601String(),
      wakeTime: now.toIso8601String(),
      hrvData: [],
    );
  }

  String _motionTypeToString(int type) {
    switch (type) {
      case 1:
        return '走路';
      case 2:
        return '跑步';
      case 3:
        return '骑行';
      default:
        return '静止';
    }
  }
}
