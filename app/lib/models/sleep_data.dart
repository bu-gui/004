class SleepData {
  final double totalSleepHours;
  final int quality;
  final double deepSleepHours;
  final double lightSleepHours;
  final String sleepTime;
  final String wakeTime;
  final List<double>? hrvData;

  SleepData({
    required this.totalSleepHours,
    required this.quality,
    required this.deepSleepHours,
    required this.lightSleepHours,
    required this.sleepTime,
    required this.wakeTime,
    this.hrvData,
  });

  factory SleepData.fromJson(Map<String, dynamic> json) {
    return SleepData(
      totalSleepHours: (json['totalSleepHours'] as num).toDouble(),
      quality: json['quality'] as int,
      deepSleepHours: (json['deepSleepHours'] as num).toDouble(),
      lightSleepHours: (json['lightSleepHours'] as num).toDouble(),
      sleepTime: json['sleepTime'] as String,
      wakeTime: json['wakeTime'] as String,
      hrvData: (json['hrvData'] as List<dynamic>?)
          ?.map((e) => (e as num).toDouble())
          .toList(),
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'totalSleepHours': totalSleepHours,
      'quality': quality,
      'deepSleepHours': deepSleepHours,
      'lightSleepHours': lightSleepHours,
      'sleepTime': sleepTime,
      'wakeTime': wakeTime,
      'hrvData': hrvData,
    };
  }
}
