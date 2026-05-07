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
  // 固件 128-bit UUID: 服务 01234567-89AB-CDEF-FEDC-BA9876543210
  // 特征值 11234567-89AB-CDEF-FEDC-BA9876543210 (单一特征，支持 Notify)
  static const String _serviceUuid = '01234567-89ab-cdef-fedc-ba9876543210';
  static const String _txUuid = '11234567-89ab-cdef-fedc-ba9876543210';
  static const String _rxUuid = '11234567-89ab-cdef-fedc-ba9876543210';

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
      if (data.length >= 19) {
        BlePacket packet = _parseBytes(data);
        _dataStreamController!.add(packet);
      }
    });
    _rxCharacteristic!.setNotifyValue(true);
    return _dataStreamController!.stream;
  }

  /// 将 BlePacket 编码为 19 字节的二进制数据，与固件 ble_data_packet_t 一致
  /// 偏移: 0-3 heartRate(float32 LE), 4 spo2(uint8), 5-8 steps(uint32 LE),
  ///       9-12 calories(float32 LE), 13 motionType(uint8),
  ///       14 fallDetected(uint8), 15-18 battery(float32 LE)
  List<int> _packetToBytes(BlePacket packet) {
    final data = ByteData(19);
    data.setFloat32(0, packet.heartRate, Endian.little);
    data.setUint8(4, packet.spo2);
    data.setUint32(5, packet.steps, Endian.little);
    data.setFloat32(9, packet.calories, Endian.little);
    data.setUint8(13, packet.motionType);
    data.setUint8(14, packet.fallDetected);
    data.setFloat32(15, packet.battery, Endian.little);
    return data.buffer.asUint8List();
  }

  /// 从 19 字节的二进制数据解析为 BlePacket
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
