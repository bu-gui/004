class BleDevice {
  final String name;
  final int rssi;
  final String macAddress;
  final bool isConnected;

  BleDevice({
    required this.name,
    required this.rssi,
    required this.macAddress,
    required this.isConnected,
  });

  factory BleDevice.fromJson(Map<String, dynamic> json) {
    return BleDevice(
      name: json['name'] as String,
      rssi: json['rssi'] as int,
      macAddress: json['macAddress'] as String,
      isConnected: json['isConnected'] as bool,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'name': name,
      'rssi': rssi,
      'macAddress': macAddress,
      'isConnected': isConnected,
    };
  }
}
