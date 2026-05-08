import 'dart:async';
import 'dart:typed_data';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:smart_band/models/health_data.dart';

class BleService {
  static final BleService _instance = BleService._internal();
  factory BleService() => _instance;
  BleService._internal();

  BluetoothDevice? _connectedDevice;
  BluetoothCharacteristic? _characteristic;
  StreamSubscription<List<int>>? _notificationSubscription;
  StreamController<BlePacket>? _dataStreamController;

  static const String _targetDeviceName = 'SmartBand';
  static const String _serviceUuid = '0000ffe0-0000-1000-8000-00805f9b34fb';
  static const String _characteristicUuid =
      '0000ffe1-0000-1000-8000-00805f9b34fb';

  bool get isConnected => _connectedDevice != null;

  Stream<ScanResult> scan() {
    FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));
    return FlutterBluePlus.scanResults.expand((results) => results);
  }

  Future<void> connect(BluetoothDevice device) async {
    await device.connect();
    _connectedDevice = device;
    final services = await device.discoverServices();
    for (var service in services) {
      if (service.uuid.toString().toLowerCase() == _serviceUuid.toLowerCase()) {
        for (var characteristic in service.characteristics) {
          if (characteristic.uuid.toString().toLowerCase() ==
              _characteristicUuid.toLowerCase()) {
            _characteristic = characteristic;
            break;
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
    _characteristic = null;
    _notificationSubscription = null;
    _dataStreamController = null;
  }

  Stream<BlePacket> startListening() {
    if (_characteristic == null || _dataStreamController == null) {
      return const Stream.empty();
    }
    _notificationSubscription = _characteristic!.onValueReceived.listen((data) {
      if (data.length >= 19) {
        BlePacket packet = _parseBytes(data);
        _dataStreamController!.add(packet);
      }
    });
    _characteristic!.setNotifyValue(true);
    return _dataStreamController!.stream;
  }

  BlePacket _parseBytes(List<int> data) {
    final byteData = ByteData.view(Uint8List.fromList(data).buffer);
    return BlePacket(
      heartRate: byteData.getFloat32(0, Endian.little),
      spo2: byteData.getUint8(4),
      steps: byteData.getUint32(5, Endian.little),
      calories: byteData.getFloat32(9, Endian.little),
      motionType: byteData.getUint8(13),
      fallDetected: byteData.getUint8(14),
      battery: byteData.getFloat32(15, Endian.little),
    );
  }
}
