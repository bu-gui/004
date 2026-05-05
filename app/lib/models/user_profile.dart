class UserProfile {
  final double heightCm;
  final double weightKg;
  final int age;
  final String gender;

  UserProfile({
    required this.heightCm,
    required this.weightKg,
    required this.age,
    required this.gender,
  });

  factory UserProfile.fromMap(Map<String, dynamic> map) {
    return UserProfile(
      heightCm: (map['height'] as num?)?.toDouble() ?? 170.0,
      weightKg: (map['weight'] as num?)?.toDouble() ?? 65.0,
      age: (map['age'] as int?) ?? 30,
      gender: (map['gender'] == 0 || map['gender'] == 'male')
          ? 'male'
          : 'female',
    );
  }

  Map<String, dynamic> toMap() {
    return {
      'height': heightCm,
      'weight': weightKg,
      'age': age,
      'gender': gender == 'male' ? 0 : 1,
    };
  }

  factory UserProfile.fromJson(Map<String, dynamic> json) {
    return UserProfile(
      heightCm: (json['heightCm'] as num).toDouble(),
      weightKg: (json['weightKg'] as num).toDouble(),
      age: json['age'] as int,
      gender: json['gender'] as String,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'heightCm': heightCm,
      'weightKg': weightKg,
      'age': age,
      'gender': gender,
    };
  }

  @override
  String toString() {
    return 'UserProfile(heightCm: $heightCm, weightKg: $weightKg, '
        'age: $age, gender: $gender)';
  }
}
