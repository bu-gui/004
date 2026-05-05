import 'package:flutter/foundation.dart';
import 'package:smart_band/models/user_profile.dart';
import 'package:smart_band/models/daily_goal.dart';
import 'package:smart_band/services/database_service.dart';

class UserSettingsProvider extends ChangeNotifier {
  final DatabaseService _databaseService;

  UserSettingsProvider(this._databaseService);

  UserProfile _profile = UserProfile(
    heightCm: 170.0,
    weightKg: 65.0,
    age: 30,
    gender: 'male',
  );
  DailyGoal _goal = DailyGoal(steps: 10000, calories: 2000.0, sleepHours: 8.0);
  bool _useImperial = false;

  UserProfile get profile => _profile;
  DailyGoal get goal => _goal;

  double get height => _profile.heightCm;
  double get weight => _profile.weightKg;
  int get age => _profile.age;
  String get gender => _profile.gender;
  int get stepGoal => _goal.steps;
  int get calorieGoal => _goal.calories.toInt();
  int get sleepGoal => _goal.sleepHours.toInt();
  bool get useImperialUnits => _useImperial;
  bool get isDeviceConnected => false;

  set height(double v) {
    _profile = UserProfile(
      heightCm: v,
      weightKg: _profile.weightKg,
      age: _profile.age,
      gender: _profile.gender,
    );
  }

  set weight(double v) {
    _profile = UserProfile(
      heightCm: _profile.heightCm,
      weightKg: v,
      age: _profile.age,
      gender: _profile.gender,
    );
  }

  set age(int v) {
    _profile = UserProfile(
      heightCm: _profile.heightCm,
      weightKg: _profile.weightKg,
      age: v,
      gender: _profile.gender,
    );
  }

  set gender(String v) {
    _profile = UserProfile(
      heightCm: _profile.heightCm,
      weightKg: _profile.weightKg,
      age: _profile.age,
      gender: v,
    );
  }

  set stepGoal(int v) {
    _goal = DailyGoal(
      steps: v,
      calories: _goal.calories,
      sleepHours: _goal.sleepHours,
    );
  }

  set calorieGoal(int v) {
    _goal = DailyGoal(
      steps: _goal.steps,
      calories: v.toDouble(),
      sleepHours: _goal.sleepHours,
    );
  }

  set sleepGoal(int v) {
    _goal = DailyGoal(
      steps: _goal.steps,
      calories: _goal.calories,
      sleepHours: v.toDouble(),
    );
  }

  set useImperialUnits(bool v) {
    _useImperial = v;
  }

  Future<void> loadSettings() async {
    try {
      final savedProfile = await _databaseService.loadUserProfile();
      if (savedProfile != null) {
        _profile = savedProfile;
      }

      final savedGoal = await _databaseService.loadDailyGoal();
      if (savedGoal != null) {
        _goal = savedGoal;
      }

      notifyListeners();
    } catch (_) {
      notifyListeners();
    }
  }

  Future<void> saveProfile(UserProfile profile) async {
    _profile = profile;
    try {
      await _databaseService.saveUserProfile(profile);
      notifyListeners();
    } catch (_) {
      notifyListeners();
    }
  }

  Future<void> saveGoal(DailyGoal goal) async {
    _goal = goal;
    try {
      await _databaseService.saveDailyGoal(goal);
      notifyListeners();
    } catch (_) {
      notifyListeners();
    }
  }

  Future<void> saveSettings() async {
    try {
      await _databaseService.saveUserProfile(_profile);
      await _databaseService.saveDailyGoal(_goal);
      notifyListeners();
    } catch (_) {
      notifyListeners();
    }
  }
}
