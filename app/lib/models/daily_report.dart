class ReportItem {
  final String label;
  final String value;
  final int iconCodePoint;

  ReportItem({
    required this.label,
    required this.value,
    required this.iconCodePoint,
  });

  factory ReportItem.fromJson(Map<String, dynamic> json) {
    return ReportItem(
      label: json['label'] as String,
      value: json['value'] as String,
      iconCodePoint: json['icon'] as int,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'label': label,
      'value': value,
      'icon': iconCodePoint,
    };
  }

  @override
  String toString() {
    return 'ReportItem(label: $label, value: $value, iconCodePoint: $iconCodePoint)';
  }
}

class DailyReport {
  final DateTime date;
  final String content;
  final List<ReportItem> summaryItems;

  DailyReport({
    required this.date,
    required this.content,
    required this.summaryItems,
  });

  factory DailyReport.fromMap(Map<String, dynamic> map) => DailyReport.fromJson(map);

  factory DailyReport.fromJson(Map<String, dynamic> json) {
    return DailyReport(
      date: DateTime.parse(json['date'] as String),
      content: json['content'] as String,
      summaryItems: (json['summaryItems'] as List)
          .map((e) => ReportItem.fromJson(e as Map<String, dynamic>))
          .toList(),
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'date': date.toIso8601String(),
      'content': content,
      'summaryItems': summaryItems.map((e) => e.toJson()).toList(),
    };
  }

  @override
  String toString() {
    return 'DailyReport(date: $date, content: $content, summaryItems: $summaryItems)';
  }
}
