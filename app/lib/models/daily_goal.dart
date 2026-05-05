class DailyGoal {
  final int steps;
  final double calories;
  final double sleepHours;

  DailyGoal({
    required this.steps,
    required this.calories,
    required this.sleepHours,
  });

  factory DailyGoal.fromMap(Map<String, dynamic> map) {
    return DailyGoal(
      steps: (map['steps'] as int?) ?? 10000,
      calories: (map['calories'] as num?)?.toDouble() ?? 2000,
      sleepHours: (map['sleep_hours'] as num?)?.toDouble() ?? 8.0,
    );
  }

  Map<String, dynamic> toMap() {
    return {'steps': steps, 'calories': calories, 'sleep_hours': sleepHours};
  }

  factory DailyGoal.fromJson(Map<String, dynamic> json) {
    return DailyGoal(
      steps: json['steps'] as int,
      calories: (json['calories'] as num).toDouble(),
      sleepHours: (json['sleepHours'] as num).toDouble(),
    );
  }

  Map<String, dynamic> toJson() {
    return {'steps': steps, 'calories': calories, 'sleepHours': sleepHours};
  }

  @override
  String toString() {
    return 'DailyGoal(steps: $steps, calories: $calories, '
        'sleepHours: $sleepHours)';
  }
}
