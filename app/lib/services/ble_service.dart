import 'dart:async';
import 'dart:typed_data';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:smart_band/models/health_data.dart';

class BleService {
  static final BleService _instance = BleService._internal();
  factory BleService() => _instance;
  BleService._internal();

  BluetoothDevice? _connectedDevice;
  BluetoothCharacteristic? _txCharacteristic;
  BluetoothCharacteristic? _rxCharacteristic;
  StreamSubscription<List<int>>? _notificationSubscription;
  StreamController<BlePacket>? _dataStreamController;

  static const String _targetDeviceName = 'SmartBand';
  static const String _serviceUuid = '0000ffe0-0000-1000-8000-00805f9b34fb';
  static const String _txUuid = '0000ffe1-0000-1000-8000-00805f9b34fb';
  static const String _rxUuid = '0000ffe2-0000-1000-8000-00805f9b34fb';

  bool get isConnected => _connectedDevice != null;

  Stream<ScanResult> scan() {
    FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));
    return FlutterBluePlus.scanResults
        .where(
          (results) => results.any(
            (result) => result.device.platformName == _targetDeviceName,
          ),
        )
        .expand((results) => results)
        .where((result) => result.device.platformName == _targetDeviceName);
  }

  Future<void> connect(BluetoothDevice device) async {
    await device.connect();
    _connectedDevice = device;
    final services = await device.discoverServices();
    for (var service in services) {
      if (service.uuid.toString().toLowerCase() == _serviceUuid.toLowerCase()) {
        for (var characteristic in service.characteristics) {
          if (characteristic.uuid.toString().toLowerCase() ==
              _txUuid.toLowerCase()) {
            _txCharacteristic = characteristic;
          } else if (characteristic.uuid.toString().toLowerCase() ==
              _rxUuid.toLowerCase()) {
            _rxCharacteristic = characteristic;
          }
        }
        break;
      }
    }
    _dataStreamController = StreamController<BlePacket>.broadcast();
  }

  Future<void> disconnect() async {
    await _notificationSubscription?.cancel();
    await _dataStreamController?.close();
    await _connectedDevice?.disconnect();
    _connectedDevice = null;
    _txCharacteristic = null;
    _rxCharacteristic = null;
    _notificationSubscription = null;
    _dataStreamController = null;
  }

  Future<void> sendData(BlePacket packet) async {
    if (_txCharacteristic == null) return;
    List<int> bytes = _packetToBytes(packet);
    await _txCharacteristic!.write(bytes);
  }

  Stream<BlePacket> startListening() {
    if (_rxCharacteristic == null || _dataStreamController == null) {
      return const Stream.empty();
    }
    _notificationSubscription = _rxCharacteristic!.onValueReceived.listen((
      data,
    ) {
      if (data.length >= 16) {
        BlePacket packet = _parseBytes(data);
        _dataStreamController!.add(packet);
      }
    });
    _rxCharacteristic!.setNotifyValue(true);
    return _dataStreamController!.stream;
  }

  List<int> _packetToBytes(BlePacket packet) {
    List<int> bytes = [];
    ByteData heartRateData = ByteData(4);
    heartRateData.setFloat32(0, packet.heartRate, Endian.little);
    bytes.addAll(heartRateData.buffer.asUint8List());
    bytes.add(packet.spo2);
    ByteData stepsData = ByteData(4);
    stepsData.setUint32(0, packet.steps, Endian.little);
    bytes.addAll(stepsData.buffer.asUint8List());
    ByteData caloriesData = ByteData(4);
    caloriesData.setFloat32(0, packet.calories, Endian.little);
    bytes.addAll(caloriesData.buffer.asUint8List());
    bytes.add(packet.motionType);
    bytes.add(packet.fallDetected);
    bytes.add(packet.battery.toInt());
    return bytes;
  }

  BlePacket _parseBytes(List<int> data) {
    ByteData byteData = ByteData.sublistView(Uint8List.fromList(data));
    double heartRate = byteData.getFloat32(0, Endian.little);
    int spo2 = byteData.getUint8(4);
    int steps = byteData.getUint32(5, Endian.little);
    double calories = byteData.getFloat32(9, Endian.little);
    int motionType = byteData.getUint8(13);
    int fallDetected = byteData.getUint8(14);
    int battery = byteData.getUint8(15);
    return BlePacket(
      heartRate: heartRate,
      spo2: spo2,
      steps: steps,
      calories: calories,
      motionType: motionType,
      fallDetected: fallDetected,
      battery: battery.toDouble(),
    );
  }
}
