import 'dart:typed_data';

class HeartRateData {
  final int bpm;
  final double confidence;

  HeartRateData({required this.bpm, required this.confidence});

  factory HeartRateData.fromJson(Map<String, dynamic> json) {
    return HeartRateData(
      bpm: json['bpm'] as int,
      confidence: (json['confidence'] as num).toDouble(),
    );
  }

  Map<String, dynamic> toJson() {
    return {'bpm': bpm, 'confidence': confidence};
  }

  @override
  String toString() {
    return 'HeartRateData(bpm: $bpm, confidence: $confidence)';
  }
}

enum MotionType { static, walking, running, cycling }

class MotionData {
  final MotionType type;
  final double confidence;

  MotionData({required this.type, required this.confidence});

  factory MotionData.fromJson(Map<String, dynamic> json) {
    return MotionData(
      type: MotionType.values.firstWhere(
        (e) => e.name == json['type'],
        orElse: () => MotionType.static,
      ),
      confidence: (json['confidence'] as num).toDouble(),
    );
  }

  Map<String, dynamic> toJson() {
    return {'type': type.name, 'confidence': confidence};
  }

  @override
  String toString() {
    return 'MotionData(type: $type, confidence: $confidence)';
  }
}

class FallAlert {
  final bool detected;
  final DateTime timestamp;

  FallAlert({required this.detected, required this.timestamp});

  factory FallAlert.fromJson(Map<String, dynamic> json) {
    return FallAlert(
      detected: json['detected'] as bool,
      timestamp: DateTime.parse(json['timestamp'] as String),
    );
  }

  Map<String, dynamic> toJson() {
    return {'detected': detected, 'timestamp': timestamp.toIso8601String()};
  }

  @override
  String toString() {
    return 'FallAlert(detected: $detected, timestamp: $timestamp)';
  }
}

class HealthRecord {
  final int id;
  final String timestamp;
  final double heartRate;
  final int spo2;
  final int steps;
  final double calories;
  final int motionType;
  final int battery;

  HealthRecord({
    required this.id,
    required this.timestamp,
    required this.heartRate,
    required this.spo2,
    required this.steps,
    required this.calories,
    required this.motionType,
    required this.battery,
  });

  factory HealthRecord.fromMap(Map<String, dynamic> map) {
    return HealthRecord(
      id: map['id'] as int,
      timestamp: map['timestamp'] as String,
      heartRate: (map['heart_rate'] as num?)?.toDouble() ?? 0,
      spo2: (map['spo2'] as int?) ?? 0,
      steps: (map['steps'] as int?) ?? 0,
      calories: (map['calories'] as num?)?.toDouble() ?? 0,
      motionType: (map['motion_type'] as int?) ?? 0,
      battery: (map['battery'] as int?) ?? 0,
    );
  }

  Map<String, dynamic> toMap() {
    return {
      'timestamp': timestamp,
      'heart_rate': heartRate,
      'spo2': spo2,
      'steps': steps,
      'calories': calories,
      'motion_type': motionType,
      'battery': battery,
    };
  }

  @override
  String toString() {
    return 'HealthRecord(id: $id, timestamp: $timestamp, heartRate: $heartRate, '
        'spo2: $spo2, steps: $steps, calories: $calories, '
        'motionType: $motionType, battery: $battery)';
  }
}

class BlePacket {
  final double heartRate;
  final int spo2;
  final int steps;
  final double calories;
  final int motionType;
  final int fallDetected;
  final double battery;

  BlePacket({
    required this.heartRate,
    required this.spo2,
    required this.steps,
    required this.calories,
    required this.motionType,
    required this.fallDetected,
    required this.battery,
  });

  factory BlePacket.fromBytes(List<int> bytes) {
    final data = ByteData.view(Uint8List.fromList(bytes).buffer);
    return BlePacket(
      heartRate: data.getFloat32(0, Endian.little),
      spo2: data.getUint8(4),
      steps: data.getUint32(5, Endian.little),
      calories: data.getFloat32(9, Endian.little),
      motionType: data.getUint8(13),
      fallDetected: data.getUint8(14),
      battery: data.getFloat32(15, Endian.little),
    );
  }

  List<int> toBytes() {
    final data = ByteData(19);
    data.setFloat32(0, heartRate, Endian.little);
    data.setUint8(4, spo2);
    data.setUint32(5, steps, Endian.little);
    data.setFloat32(9, calories, Endian.little);
    data.setUint8(13, motionType);
    data.setUint8(14, fallDetected);
    data.setFloat32(15, battery, Endian.little);
    return data.buffer.asUint8List().toList();
  }

  factory BlePacket.fromJson(Map<String, dynamic> json) {
    return BlePacket(
      heartRate: (json['heartRate'] as num).toDouble(),
      spo2: json['spo2'] as int,
      steps: json['steps'] as int,
      calories: (json['calories'] as num).toDouble(),
      motionType: json['motionType'] as int,
      fallDetected: json['fallDetected'] as int,
      battery: (json['battery'] as num).toDouble(),
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'heartRate': heartRate,
      'spo2': spo2,
      'steps': steps,
      'calories': calories,
      'motionType': motionType,
      'fallDetected': fallDetected,
      'battery': battery,
    };
  }

  @override
  String toString() {
    return 'BlePacket(heartRate: $heartRate, spo2: $spo2, steps: $steps, '
        'calories: $calories, motionType: $motionType, '
        'fallDetected: $fallDetected, battery: $battery)';
  }
}
