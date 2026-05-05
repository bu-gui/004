import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:smart_band/models/health_data.dart';
import 'package:smart_band/models/ble_device.dart';

class BleProvider extends ChangeNotifier {
  bool _isConnected = false;
  bool _isScanning = false;
  String _deviceName = '';
  List<ScanResult> _deviceList = [];
  List<BleDevice> _devices = [];
  BluetoothDevice? _btDevice;
  BleDevice? _connectedBleDevice;
  String? _error;

  final StreamController<BlePacket> _packetController =
      StreamController<BlePacket>.broadcast();

  StreamSubscription? _scanSubscription;
  StreamSubscription<BluetoothConnectionState>? _connectionSubscription;
  StreamSubscription<List<int>>? _dataSubscription;

  bool get isConnected => _isConnected;
  bool get isScanning => _isScanning;
  String get deviceName => _deviceName;
  List<ScanResult> get deviceList => List.unmodifiable(_deviceList);
  List<BleDevice> get devices => _devices;
  BleDevice? get connectedDevice => _connectedBleDevice;
  String? get error => _error;
  Stream<BlePacket> get packetStream => _packetController.stream;

  Future<void> startScan() async {
    _error = null;
    _deviceList.clear();
    _isScanning = true;
    notifyListeners();

    try {
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 15));

      _scanSubscription = FlutterBluePlus.scanResults.listen((results) {
        _deviceList = results
            .where(
              (r) =>
                  r.device.remoteId != null && r.device.platformName.isNotEmpty,
            )
            .toList();
        _devices = _deviceList
            .map(
              (r) => BleDevice(
                name: r.device.platformName,
                macAddress: r.device.remoteId.toString(),
                rssi: r.rssi,
                isConnected: _btDevice?.remoteId == r.device.remoteId,
              ),
            )
            .toList();
        notifyListeners();
      });

      FlutterBluePlus.scanResults
          .listen((results) {
            if (results.isNotEmpty) {
              _deviceList = results
                  .where(
                    (r) =>
                        r.device.remoteId != null &&
                        r.device.platformName.isNotEmpty,
                  )
                  .toList();
              _devices = _deviceList
                  .map(
                    (r) => BleDevice(
                      name: r.device.platformName,
                      macAddress: r.device.remoteId.toString(),
                      rssi: r.rssi,
                      isConnected: _btDevice?.remoteId == r.device.remoteId,
                    ),
                  )
                  .toList();
              notifyListeners();
            }
          })
          .onError((e) {
            _error = '扫描出错: $e';
            _isScanning = false;
            notifyListeners();
          });
    } catch (e) {
      _error = '启动扫描失败: $e';
      _isScanning = false;
      notifyListeners();
    }
  }

  Future<void> stopScan() async {
    _isScanning = false;
    await _scanSubscription?.cancel();
    _scanSubscription = null;
    try {
      await FlutterBluePlus.stopScan();
    } catch (_) {}
    notifyListeners();
  }

  Future<void> connect(BleDevice device) async {
    _error = null;
    notifyListeners();

    try {
      final target =
          _btDevice ??
          _deviceList
              .firstWhere(
                (r) => r.device.remoteId.toString() == device.macAddress,
                orElse: () => throw Exception('设备未找到'),
              )
              .device;
      await target.connect();
      _btDevice = target;
      _connectedBleDevice = device;
      _deviceName = device.name;
      _isConnected = true;

      _listenToConnection(target);
      await _discoverAndSubscribe(target);

      notifyListeners();
    } catch (e) {
      _error = '连接失败: $e';
      _isConnected = false;
      notifyListeners();
    }
  }

  Future<void> disconnect() async {
    if (_btDevice == null) return;

    await _dataSubscription?.cancel();
    _dataSubscription = null;
    await _connectionSubscription?.cancel();
    _connectionSubscription = null;

    try {
      await _btDevice!.disconnect();
    } catch (_) {}

    _btDevice = null;
    _connectedBleDevice = null;
    _isConnected = false;
    _deviceName = '';
    notifyListeners();
  }

  void _listenToConnection(BluetoothDevice device) {
    _connectionSubscription = device.connectionState.listen((state) {
      if (state == BluetoothConnectionState.disconnected) {
        _isConnected = false;
        _connectedBleDevice = null;
        _btDevice = null;
        _deviceName = '';
        _dataSubscription?.cancel();
        _dataSubscription = null;
        notifyListeners();
      }
    });
  }

  Future<void> _discoverAndSubscribe(BluetoothDevice device) async {
    try {
      List<BluetoothService> services = await device.discoverServices();
      for (var service in services) {
        for (var characteristic in service.characteristics) {
          if (characteristic.properties.notify) {
            await characteristic.setNotifyValue(true);
            _dataSubscription = characteristic.onValueReceived.listen((data) {
              _handleData(data);
            });
          }
        }
      }
    } catch (e) {
      _error = '订阅数据失败: $e';
      notifyListeners();
    }
  }

  void _handleData(List<int> data) {
    if (data.length < 5) return;
    try {
      final packet = BlePacket(
        heartRate: data[0].toDouble(),
        spo2: data[1],
        steps: (data[2] << 8) | data[3],
        calories: (data[2] << 8 | data[3]).toDouble(),
        motionType: data.length > 4 ? data[4] : 0,
        battery: data.length > 5 ? data[5].toDouble() : 0.0,
        fallDetected: data.length > 6 ? data[6] : 0,
      );
      _packetController.add(packet);
      notifyListeners();
    } catch (_) {}
  }

  @override
  void dispose() {
    _scanSubscription?.cancel();
    _connectionSubscription?.cancel();
    _dataSubscription?.cancel();
    _packetController.close();
    super.dispose();
  }
}
