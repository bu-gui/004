class PlanItem {
  final String time;
  final String activity;
  final int duration;
  final int targetHeartRate;
  final bool completed;

  PlanItem({
    required this.time,
    required this.activity,
    required this.duration,
    required this.targetHeartRate,
    required this.completed,
  });

  factory PlanItem.fromJson(Map<String, dynamic> json) {
    return PlanItem(
      time: json['time'] as String,
      activity: json['activity'] as String,
      duration: json['duration'] as int,
      targetHeartRate: json['targetHeartRate'] as int,
      completed: json['completed'] as bool,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'time': time,
      'activity': activity,
      'duration': duration,
      'targetHeartRate': targetHeartRate,
      'completed': completed,
    };
  }

  @override
  String toString() {
    return 'PlanItem(time: $time, activity: $activity, '
        'duration: $duration, targetHeartRate: $targetHeartRate, '
        'completed: $completed)';
  }
}

class TrainingPlan {
  final DateTime date;
  final String title;
  final List<PlanItem> items;

  TrainingPlan({
    required this.date,
    required this.title,
    required this.items,
  });

  factory TrainingPlan.fromMap(Map<String, dynamic> map) => TrainingPlan.fromJson(map);

  factory TrainingPlan.fromJson(Map<String, dynamic> json) {
    return TrainingPlan(
      date: DateTime.parse(json['date'] as String),
      title: json['title'] as String,
      items: (json['items'] as List)
          .map((e) => PlanItem.fromJson(e as Map<String, dynamic>))
          .toList(),
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'date': date.toIso8601String(),
      'title': title,
      'items': items.map((e) => e.toJson()).toList(),
    };
  }

  @override
  String toString() {
    return 'TrainingPlan(date: $date, title: $title, items: $items)';
  }
}
