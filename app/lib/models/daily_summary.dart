class DailySummary {
  final int id;
  final String date;
  final int totalSteps;
  final double totalCalories;
  final double avgHeartRate;
  final double minHeartRate;
  final double maxHeartRate;
  final int avgSpo2;
  final int motionMinutes;
  final double sleepHours;
  final int sleepQuality;
  final int fallCount;

  DailySummary({
    required this.id,
    required this.date,
    required this.totalSteps,
    required this.totalCalories,
    required this.avgHeartRate,
    required this.minHeartRate,
    required this.maxHeartRate,
    required this.avgSpo2,
    required this.motionMinutes,
    required this.sleepHours,
    required this.sleepQuality,
    required this.fallCount,
  });

  factory DailySummary.fromMap(Map<String, dynamic> map) {
    return DailySummary(
      id: map['id'] as int,
      date: map['date'] as String,
      totalSteps: (map['total_steps'] as int?) ?? 0,
      totalCalories: (map['total_calories'] as num?)?.toDouble() ?? 0,
      avgHeartRate: (map['avg_heart_rate'] as num?)?.toDouble() ?? 0,
      minHeartRate: (map['min_heart_rate'] as num?)?.toDouble() ?? 0,
      maxHeartRate: (map['max_heart_rate'] as num?)?.toDouble() ?? 0,
      avgSpo2: (map['avg_spo2'] as num?)?.toInt() ?? 0,
      motionMinutes: (map['motion_minutes'] as int?) ?? 0,
      sleepHours: (map['sleep_hours'] as num?)?.toDouble() ?? 0,
      sleepQuality: (map['sleep_quality'] as int?) ?? 0,
      fallCount: (map['fall_count'] as int?) ?? 0,
    );
  }

  Map<String, dynamic> toMap() {
    return {
      'id': id,
      'date': date,
      'total_steps': totalSteps,
      'total_calories': totalCalories,
      'avg_heart_rate': avgHeartRate,
      'min_heart_rate': minHeartRate,
      'max_heart_rate': maxHeartRate,
      'avg_spo2': avgSpo2,
      'motion_minutes': motionMinutes,
      'sleep_hours': sleepHours,
      'sleep_quality': sleepQuality,
      'fall_count': fallCount,
    };
  }

  factory DailySummary.fromJson(Map<String, dynamic> json) {
    return DailySummary(
      id: json['id'] as int,
      date: json['date'] as String,
      totalSteps: json['totalSteps'] as int,
      totalCalories: (json['totalCalories'] as num).toDouble(),
      avgHeartRate: (json['avgHeartRate'] as num).toDouble(),
      minHeartRate: (json['minHeartRate'] as num).toDouble(),
      maxHeartRate: (json['maxHeartRate'] as num).toDouble(),
      avgSpo2: json['avgSpo2'] as int,
      motionMinutes: json['motionMinutes'] as int,
      sleepHours: (json['sleepHours'] as num).toDouble(),
      sleepQuality: json['sleepQuality'] as int,
      fallCount: json['fallCount'] as int,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'id': id,
      'date': date,
      'totalSteps': totalSteps,
      'totalCalories': totalCalories,
      'avgHeartRate': avgHeartRate,
      'minHeartRate': minHeartRate,
      'maxHeartRate': maxHeartRate,
      'avgSpo2': avgSpo2,
      'motionMinutes': motionMinutes,
      'sleepHours': sleepHours,
      'sleepQuality': sleepQuality,
      'fallCount': fallCount,
    };
  }

  @override
  String toString() {
    return 'DailySummary(id: $id, date: $date, totalSteps: $totalSteps, '
        'totalCalories: $totalCalories, avgHeartRate: $avgHeartRate, '
        'minHeartRate: $minHeartRate, maxHeartRate: $maxHeartRate, '
        'avgSpo2: $avgSpo2, motionMinutes: $motionMinutes, '
        'sleepHours: $sleepHours, sleepQuality: $sleepQuality, '
        'fallCount: $fallCount)';
  }
}
