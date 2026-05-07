import 'dart:async';
import 'dart:convert';
import 'package:dio/dio.dart';
import 'package:smart_band/models/daily_summary.dart';
import 'package:smart_band/models/daily_report.dart';
import 'package:smart_band/models/training_plan.dart';

class DeepSeekApiService {
  static final DeepSeekApiService _instance = DeepSeekApiService._internal();
  factory DeepSeekApiService() => _instance;
  DeepSeekApiService._internal();

  Dio? _dio;

  static const String _baseUrl = 'https://api.deepseek.com/v1/chat/completions';
  static const String _model = 'deepseek-chat';
  static const int _maxRetries = 3;

  String _apiKey = '';

  void setApiKey(String key) {
    _apiKey = key;
    // API Key 变更后重新创建 Dio 实例
    _dio = null;
  }

  Dio _createDio() {
    final dio = Dio(
      BaseOptions(
        baseUrl: '',
        connectTimeout: const Duration(seconds: 30),
        receiveTimeout: const Duration(seconds: 60),
        sendTimeout: const Duration(seconds: 30),
        headers: {
          'Content-Type': 'application/json',
          'Authorization': 'Bearer $_apiKey',
        },
      ),
    );
    dio.interceptors.add(_RetryInterceptor(dio, maxRetries: _maxRetries));
    return dio;
  }

  Dio get _client {
    _dio ??= _createDio();
    return _dio!;
  }

  Future<DailyReport> generateReport(DailySummary summary) async {
    final response = await _client.post(
      _baseUrl,
      data: {
        'model': _model,
        'messages': [
          {
            'role': 'system',
            'content':
                '你是一个专业的健康分析助手。根据用户的每日健康数据，生成一份详细的健康报告，'
                '包含整体状况分析、各项指标评价、运动建议和健康提醒。以JSON格式返回。',
          },
          {'role': 'user', 'content': _buildReportPrompt(summary)},
        ],
        'stream': false,
      },
    );
    String content = response.data['choices'][0]['message']['content'];
    return DailyReport.fromJson(jsonDecode(content));
  }

  Future<TrainingPlan> generatePlan(List<DailySummary> summaries) async {
    final response = await _client.post(
      _baseUrl,
      data: {
        'model': _model,
        'messages': [
          {
            'role': 'system',
            'content':
                '你是一个专业的运动训练教练。根据用户的近期健康数据，制定一份个性化的运动训练计划。'
                '包含训练目标、每日训练安排、运动类型建议和注意事项。以JSON格式返回。',
          },
          {'role': 'user', 'content': _buildPlanPrompt(summaries)},
        ],
        'stream': false,
      },
    );
    String content = response.data['choices'][0]['message']['content'];
    return TrainingPlan.fromJson(jsonDecode(content));
  }

  Stream<String> chat(String message) async* {
    final response = await _client.post(
      _baseUrl,
      data: {
        'model': _model,
        'messages': [
          {
            'role': 'system',
            'content':
                '你是一个智能健康助手，可以回答关于健康、运动、饮食等方面的问题。'
                '请用友好、专业的口吻回答用户的问题。',
          },
          {'role': 'user', 'content': message},
        ],
        'stream': true,
      },
      options: Options(responseType: ResponseType.stream),
    );

    final stream = response.data.stream;
    String buffer = '';
    await for (var chunk in stream) {
      buffer += utf8.decode(chunk as List<int>);
      while (buffer.contains('\n')) {
        int lineEnd = buffer.indexOf('\n');
        String line = buffer.substring(0, lineEnd).trim();
        buffer = buffer.substring(lineEnd + 1);
        if (line.startsWith('data: ')) {
          String data = line.substring(6);
          if (data == '[DONE]') return;
          try {
            var json = jsonDecode(data);
            String? content = json['choices']?[0]?['delta']?['content'];
            if (content != null && content.isNotEmpty) {
              yield content;
            }
          } catch (_) {}
        }
      }
    }
  }

  Future<DailyReport> getDailyReport(DailySummary summary) async {
    return generateReport(summary);
  }

  Future<TrainingPlan> getTrainingPlan(List<DailySummary> summaries) async {
    return generatePlan(summaries);
  }

  Stream<String> sendMessage(String message) async* {
    yield* chat(message);
  }

  String _buildReportPrompt(DailySummary summary) {
    return '请根据以下今日健康数据生成健康报告：\n'
        '日期：${summary.date}\n'
        '总步数：${summary.totalSteps}\n'
        '总卡路里：${summary.totalCalories}\n'
        '平均心率：${summary.avgHeartRate}\n'
        '最小心率：${summary.minHeartRate}\n'
        '最大心率：${summary.maxHeartRate}\n'
        '平均血氧：${summary.avgSpo2}\n'
        '运动时长：${summary.motionMinutes}分钟\n'
        '睡眠时长：${summary.sleepHours}小时\n'
        '睡眠质量：${summary.sleepQuality}\n'
        '跌倒次数：${summary.fallCount}';
  }

  String _buildPlanPrompt(List<DailySummary> summaries) {
    StringBuffer sb = StringBuffer('请根据以下近期健康数据制定训练计划：\n\n');
    for (var summary in summaries) {
      sb.writeln(
        '日期：${summary.date}，步数：${summary.totalSteps}，'
        '卡路里：${summary.totalCalories}，平均心率：${summary.avgHeartRate}，'
        '运动时长：${summary.motionMinutes}分钟',
      );
    }
    return sb.toString();
  }
}

class _RetryInterceptor extends Interceptor {
  final Dio _dio;
  final int maxRetries;

  _RetryInterceptor(this._dio, {required this.maxRetries});

  @override
  void onError(DioException err, ErrorInterceptorHandler handler) async {
    if (_isRetryable(err)) {
      int retryCount = (err.requestOptions.extra['retryCount'] as int?) ?? 0;
      if (retryCount < maxRetries) {
        retryCount++;
        err.requestOptions.extra['retryCount'] = retryCount;
        await Future.delayed(Duration(seconds: retryCount * 2));
        try {
          // 使用传入的 Dio 实例（保留了 BaseOptions 配置）
          final response = await _dio.fetch(err.requestOptions);
          handler.resolve(response);
          return;
        } catch (e) {
          handler.next(err);
          return;
        }
      }
    }
    handler.next(err);
  }

  bool _isRetryable(DioException err) {
    return err.type == DioExceptionType.connectionTimeout ||
        err.type == DioExceptionType.sendTimeout ||
        err.type == DioExceptionType.receiveTimeout ||
        err.type == DioExceptionType.connectionError;
  }
}
