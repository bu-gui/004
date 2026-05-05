import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:smart_band/models/daily_report.dart';
import 'package:smart_band/models/training_plan.dart';
import 'package:smart_band/models/daily_summary.dart';
import 'package:smart_band/services/database_service.dart';
import 'package:smart_band/services/deepseek_api_service.dart';

class ChatMessage {
  final String content;
  final bool isUser;

  ChatMessage({required this.content, required this.isUser});

  Map<String, dynamic> toJson() => {'content': content, 'isUser': isUser};

  factory ChatMessage.fromJson(Map<String, dynamic> json) => ChatMessage(
    content: json['content'] as String,
    isUser: json['isUser'] as bool,
  );
}

class DeepSeekProvider extends ChangeNotifier {
  final DeepSeekApiService _apiService;
  final DatabaseService _databaseService;

  DeepSeekProvider(this._apiService, this._databaseService);

  DailyReport? _currentReport;
  TrainingPlan? _currentPlan;
  bool _isLoading = false;
  bool _isOffline = false;
  List<ChatMessage> _chatHistory = [];

  DailyReport? get currentReport => _currentReport;
  TrainingPlan? get currentPlan => _currentPlan;
  bool get isLoading => _isLoading;
  bool get isOffline => _isOffline;
  List<ChatMessage> get chatHistory => List.unmodifiable(_chatHistory);

  Future<void> fetchDailyReport() async {
    _isLoading = true;
    _isOffline = false;
    notifyListeners();

    try {
      _currentReport = await _apiService.generateReport(
        await _getTodaySummary(),
      );
      if (_currentReport != null) {
        final dateStr = DateTime.now().toIso8601String().split('T')[0];
        await _databaseService.cacheReport(
          dateStr,
          jsonEncode(_currentReport!.toJson()),
        );
      }
      _isOffline = false;
    } catch (e) {
      final dateStr = DateTime.now().toIso8601String().split('T')[0];
      final cached = await _databaseService.getCachedReport(dateStr);
      if (cached != null) {
        _currentReport = DailyReport.fromJson(jsonDecode(cached));
      }
      _isOffline = true;
    }

    _isLoading = false;
    notifyListeners();
  }

  Future<void> fetchTrainingPlan() async {
    _isLoading = true;
    _isOffline = false;
    notifyListeners();

    try {
      _currentPlan = await _apiService.generatePlan([await _getTodaySummary()]);
      if (_currentPlan != null) {
        final dateStr = DateTime.now().toIso8601String().split('T')[0];
        await _databaseService.cachePlan(
          dateStr,
          jsonEncode(_currentPlan!.toJson()),
        );
      }
      _isOffline = false;
    } catch (e) {
      final dateStr = DateTime.now().toIso8601String().split('T')[0];
      final cachedPlan = await _databaseService.getCachedPlan(dateStr);
      if (cachedPlan != null) {
        _currentPlan = TrainingPlan.fromJson(jsonDecode(cachedPlan));
      }
      _isOffline = true;
    }

    _isLoading = false;
    notifyListeners();
  }

  List<ChatMessage> get chatMessages => _chatHistory;

  DailyReport? get report => _currentReport;

  List<dynamic> get weeklyPlans {
    if (_currentPlan != null) {
      return _currentPlan!.items;
    }
    return [];
  }

  Future<void> loadReport() => fetchDailyReport();

  Future<void> loadPlans() => fetchTrainingPlan();

  Future<void> loadChatHistory() async {
    try {
      final dateStr = DateTime.now().toIso8601String().split('T')[0];
      final cached = await _databaseService.getCachedReport(dateStr);
      if (cached != null) {
        _currentReport = DailyReport.fromJson(jsonDecode(cached));
      }
      notifyListeners();
    } catch (_) {
      notifyListeners();
    }
  }

  Future<void> sendChatMessage(String content) async {
    if (content.trim().isEmpty) return;

    final userMessage = ChatMessage(content: content, isUser: true);
    _chatHistory.add(userMessage);
    _isLoading = true;
    notifyListeners();

    try {
      final reply = await _apiService.chat(content).join();
      final assistantMessage = ChatMessage(content: reply, isUser: false);
      _chatHistory.add(assistantMessage);
      _isOffline = false;
    } catch (e) {
      final errorMessage = ChatMessage(
        content: '抱歉，当前网络不可用，请稍后再试。',
        isUser: false,
      );
      _chatHistory.add(errorMessage);
      _isOffline = true;
    }

    _isLoading = false;
    notifyListeners();
  }

  Future<void> loadFromCache() async {
    try {
      final dateStr = DateTime.now().toIso8601String().split('T')[0];
      final cached = await _databaseService.getCachedReport(dateStr);
      if (cached != null) {
        _currentReport = DailyReport.fromJson(jsonDecode(cached));
      }
      final cachedPlan = await _databaseService.getCachedPlan(dateStr);
      if (cachedPlan != null) {
        _currentPlan = TrainingPlan.fromJson(jsonDecode(cachedPlan));
      }
      _isOffline = true;
      notifyListeners();
    } catch (_) {
      notifyListeners();
    }
  }

  Future<DailySummary> _getTodaySummary() async {
    final now = DateTime.now();
    final dateStr = now.toIso8601String().split('T')[0];
    try {
      return await _databaseService.getTodaySummary() ??
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
    } catch (_) {
      return DailySummary(
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
    }
  }

  void clearChatHistory() {
    _chatHistory.clear();
    notifyListeners();
  }
}
