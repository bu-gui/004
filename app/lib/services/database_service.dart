import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';
import 'package:smart_band/models/health_data.dart';
import 'package:smart_band/models/daily_summary.dart';
import 'package:smart_band/models/user_profile.dart';
import 'package:smart_band/models/daily_goal.dart';

class DatabaseService {
  static final DatabaseService _instance = DatabaseService._internal();
  factory DatabaseService() => _instance;
  DatabaseService._internal();

  static Database? _database;

  Future<Database> get database async {
    if (_database != null) return _database!;
    _database = await _initDatabase();
    return _database!;
  }

  Future<Database> _initDatabase() async {
    String path = join(await getDatabasesPath(), 'smart_band.db');
    return await openDatabase(
      path,
      version: 2,
      onCreate: _onCreate,
      onUpgrade: _onUpgrade,
    );
  }

  Future<void> _onCreate(Database db, int version) async {
    await db.execute('''
      CREATE TABLE health_records(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp TEXT,
        heart_rate REAL,
        spo2 INTEGER,
        steps INTEGER,
        calories REAL,
        motion_type INTEGER,
        battery INTEGER
      )
    ''');
    await db.execute('''
      CREATE TABLE daily_summaries(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        date TEXT UNIQUE,
        total_steps INTEGER,
        total_calories REAL,
        avg_heart_rate REAL,
        min_heart_rate REAL,
        max_heart_rate REAL,
        avg_spo2 REAL,
        motion_minutes INTEGER,
        sleep_hours REAL,
        sleep_quality INTEGER,
        fall_count INTEGER
      )
    ''');
    await db.execute('''
      CREATE TABLE reports_cache(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        date TEXT UNIQUE,
        report_json TEXT,
        plan_json TEXT,
        cached_at TEXT
      )
    ''');
    await db.execute('''
      CREATE TABLE user_profile(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT,
        age INTEGER,
        height REAL,
        weight REAL,
        gender INTEGER
      )
    ''');
    await db.execute('''
      CREATE TABLE daily_goal(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        steps INTEGER,
        calories REAL,
        sleep_hours REAL,
        motion_minutes INTEGER
      )
    ''');
    await db.execute('''
      CREATE TABLE chat_cache(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        key TEXT UNIQUE,
        value TEXT
      )
    ''');
  }

  Future<void> _onUpgrade(Database db, int oldVersion, int newVersion) async {
    if (oldVersion < 2) {
      await db.execute('''
        CREATE TABLE IF NOT EXISTS chat_cache(
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          key TEXT UNIQUE,
          value TEXT
        )
      ''');
    }
  }

  Future<int> insertHealthRecord(HealthRecord record) async {
    final db = await database;
    return await db.insert('health_records', record.toMap());
  }

  Future<List<HealthRecord>> getHealthRecords({
    String? startDate,
    String? endDate,
  }) async {
    final db = await database;
    List<Map<String, dynamic>> maps;
    if (startDate != null && endDate != null) {
      maps = await db.query(
        'health_records',
        where: 'timestamp >= ? AND timestamp <= ?',
        whereArgs: [startDate, endDate],
        orderBy: 'timestamp ASC',
      );
    } else if (startDate != null) {
      maps = await db.query(
        'health_records',
        where: 'timestamp >= ?',
        whereArgs: [startDate],
        orderBy: 'timestamp ASC',
      );
    } else if (endDate != null) {
      maps = await db.query(
        'health_records',
        where: 'timestamp <= ?',
        whereArgs: [endDate],
        orderBy: 'timestamp ASC',
      );
    } else {
      maps = await db.query('health_records', orderBy: 'timestamp DESC');
    }
    return maps.map((map) => HealthRecord.fromMap(map)).toList();
  }

  Future<DailySummary?> getTodaySummary() async {
    final db = await database;
    final today = DateTime.now().toIso8601String().substring(0, 10);
    List<Map<String, dynamic>> maps = await db.query(
      'daily_summaries',
      where: 'date = ?',
      whereArgs: [today],
    );
    if (maps.isEmpty) return null;
    return DailySummary.fromMap(maps.first);
  }

  Future<DailySummary?> getDailySummary(String date) async {
    final db = await database;
    List<Map<String, dynamic>> maps = await db.query(
      'daily_summaries',
      where: 'date = ?',
      whereArgs: [date],
    );
    if (maps.isEmpty) return null;
    return DailySummary.fromMap(maps.first);
  }

  Future<void> insertOrUpdateSummary(DailySummary summary) async {
    final db = await database;
    await db.insert(
      'daily_summaries',
      summary.toMap(),
      conflictAlgorithm: ConflictAlgorithm.replace,
    );
  }

  Future<void> saveDailySummary(DailySummary summary) async {
    await insertOrUpdateSummary(summary);
  }

  Future<void> cacheReport(String date, String reportJson) async {
    final db = await database;
    final existing = await db.query(
      'reports_cache',
      where: 'date = ?',
      whereArgs: [date],
    );
    if (existing.isNotEmpty) {
      await db.update(
        'reports_cache',
        {
          'report_json': reportJson,
          'cached_at': DateTime.now().toIso8601String(),
        },
        where: 'date = ?',
        whereArgs: [date],
      );
    } else {
      await db.insert('reports_cache', {
        'date': date,
        'report_json': reportJson,
        'plan_json': '',
        'cached_at': DateTime.now().toIso8601String(),
      });
    }
  }

  Future<String?> getCachedReport(String date) async {
    final db = await database;
    List<Map<String, dynamic>> maps = await db.query(
      'reports_cache',
      where: 'date = ?',
      whereArgs: [date],
    );
    if (maps.isEmpty) return null;
    return maps.first['report_json'] as String?;
  }

  Future<void> cachePlan(String date, String planJson) async {
    final db = await database;
    final existing = await db.query(
      'reports_cache',
      where: 'date = ?',
      whereArgs: [date],
    );
    if (existing.isNotEmpty) {
      await db.update(
        'reports_cache',
        {'plan_json': planJson, 'cached_at': DateTime.now().toIso8601String()},
        where: 'date = ?',
        whereArgs: [date],
      );
    } else {
      await db.insert('reports_cache', {
        'date': date,
        'report_json': '',
        'plan_json': planJson,
        'cached_at': DateTime.now().toIso8601String(),
      });
    }
  }

  Future<String?> getCachedPlan(String date) async {
    final db = await database;
    List<Map<String, dynamic>> maps = await db.query(
      'reports_cache',
      where: 'date = ?',
      whereArgs: [date],
    );
    if (maps.isEmpty) return null;
    return maps.first['plan_json'] as String?;
  }

  Future<void> saveChatMessages(String json) async {
    final db = await database;
    await db.insert('chat_cache', {
      'key': 'chat_messages',
      'value': json,
    }, conflictAlgorithm: ConflictAlgorithm.replace);
  }

  Future<String?> getChatMessages() async {
    final db = await database;
    List<Map<String, dynamic>> maps = await db.query(
      'chat_cache',
      where: 'key = ?',
      whereArgs: ['chat_messages'],
    );
    if (maps.isEmpty) return null;
    return maps.first['value'] as String?;
  }

  Future<String?> getCachedDailyReport(String date) async {
    return getCachedReport(date);
  }

  Future<String?> getCachedTrainingPlan(String date) async {
    return getCachedPlan(date);
  }

  Future<void> cacheTrainingPlan(String date, String planJson) async {
    await cachePlan(date, planJson);
  }

  Future<void> cacheDailyReport(String date, String reportJson) async {
    await cacheReport(date, reportJson);
  }

  Future<void> saveUserProfile(UserProfile profile) async {
    final db = await database;
    await db.insert(
      'user_profile',
      profile.toMap(),
      conflictAlgorithm: ConflictAlgorithm.replace,
    );
  }

  Future<UserProfile?> loadUserProfile() async {
    final db = await database;
    List<Map<String, dynamic>> maps = await db.query('user_profile', limit: 1);
    if (maps.isEmpty) return null;
    return UserProfile.fromMap(maps.first);
  }

  Future<void> saveDailyGoal(DailyGoal goal) async {
    final db = await database;
    await db.insert(
      'daily_goal',
      goal.toMap(),
      conflictAlgorithm: ConflictAlgorithm.replace,
    );
  }

  Future<DailyGoal?> loadDailyGoal() async {
    final db = await database;
    List<Map<String, dynamic>> maps = await db.query('daily_goal', limit: 1);
    if (maps.isEmpty) return null;
    return DailyGoal.fromMap(maps.first);
  }
}
