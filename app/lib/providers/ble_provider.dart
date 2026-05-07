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

  // 固件特征值 UUID
  static const String _characteristicUuid =
      '11234567-89ab-cdef-fedc-ba9876543210';

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
      // 取消旧的订阅，确保只有一个订阅
      await _scanSubscription?.cancel();
      _scanSubscription = null;

      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 15));

      _scanSubscription = FlutterBluePlus.scanResults.listen(
        (results) {
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
        },
        onError: (e) {
          _error = '扫描出错: $e';
          _isScanning = false;
          notifyListeners();
        },
      );
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
          // 按固件特征值 UUID 精确匹配
          if (characteristic.uuid.toString().toLowerCase() ==
              _characteristicUuid) {
            await characteristic.setNotifyValue(true);
            _dataSubscription = characteristic.onValueReceived.listen((data) {
              _handleData(data);
            });
            return;
          }
        }
      }
      _error = '未找到固件特征值 (UUID: $_characteristicUuid)';
      notifyListeners();
    } catch (e) {
      _error = '订阅数据失败: $e';
      notifyListeners();
    }
  }

  void _handleData(List<int> data) {
    if (data.length < 19) return;
    try {
      final packet = BlePacket.fromBytes(data);
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
