import { Component, OnInit } from '@angular/core';
import {
  BleClient,
  dataViewToText,
  numbersToDataView,
  textToDataView,
  ScanResult,
} from '@capacitor-community/bluetooth-le';

@Component({
  selector: 'app-tab3',
  templateUrl: 'tab3.page.html',
  styleUrls: ['tab3.page.scss']
})
export class Tab3Page {
  dateTimeFromPicker: any;
  bluetoothScanResults: ScanResult[] = [];
  bluetoothIsScanning = false;
  bluetoothConnectedDevice?: ScanResult;
  readonly serviceUUID = `6e400001-b5a3-f393-e0a9-e50e24dcca9e`.toUpperCase(); //Nordic UART Service
  readonly rxCharacteristic = `6e400002-b5a3-f393-e0a9-e50e24dcca9e`.toUpperCase(); //Write data to the RX Characteristic to send it on to the UART interface
  readonly txCharacteristic = `6e400003-b5a3-f393-e0a9-e50e24dcca9e`.toUpperCase();

  constructor() {}

  ngOnInit(): void {}

  async scanForBluetoothDevices() {
    try {
      await BleClient.initialize();

      this.bluetoothScanResults = [];
      this.bluetoothIsScanning = true;

      // if you pass empty array to services it will show all nearby bluetooth devices
      await BleClient.requestLEScan(
        { services: [this.serviceUUID] },
        this.onBluetoothDeviceFound.bind(this)
      );

      const stopScanAfterMilliSeconds = 3500;
      setTimeout(async () => {
        await BleClient.stopLEScan();
        this.bluetoothIsScanning = false;
        console.log('stopped scanning');
      }, stopScanAfterMilliSeconds);
    } catch (error) {
      this.bluetoothIsScanning = false;
      console.error('scanForBluetoothDevices', error);
    }
  }

  onBluetoothDeviceFound(result) {
    console.log('received new scan result', result);
    this.bluetoothScanResults.push(result);
  }

  async connectToBluetoothDevice(scanResult: ScanResult) {
    const device = scanResult.device;

    try {
      await BleClient.connect(
        device.deviceId,
        this.onBluetooDeviceDisconnected.bind(this)
      );

      this.bluetoothConnectedDevice = scanResult;

      const deviceName = device.name ?? device.deviceId;
      alert(`connected to device ${deviceName}`);
    } catch (error) {
      console.error('connectToDevice', error);
      alert(JSON.stringify(error));
    }
  }

  async disconnectFromBluetoothDevice(scanResult: ScanResult) {
    const device = scanResult.device;
    try {
      await BleClient.disconnect(scanResult.device.deviceId);
      const deviceName = device.name ?? device.deviceId;
      alert(`disconnected from device ${deviceName}`);
    } catch (error) {
      console.error('disconnectFromDevice', error);
    }
  }

  onBluetooDeviceDisconnected(disconnectedDeviceId: string) {
    console.log(`Diconnected ${disconnectedDeviceId}`);
    this.bluetoothConnectedDevice = undefined;
  }

  async sendBluetoothWriteCommand() {
    if (!this.bluetoothConnectedDevice) {
      alert('Bluetooth device not connected');
      return;
    }

    try {
      let data = 0;
      await BleClient.write(
        this.bluetoothConnectedDevice.device.deviceId,
        this.serviceUUID,
        this.rxCharacteristic,
        numbersToDataView(this.getHexTimeFromDateTime(this.dateTimeFromPicker))
      );
      alert('Alarm updated');
    } catch (error) {
      console.log(`error: ${JSON.stringify(error)}`);
      alert(JSON.stringify(error));
    }
  }

  async sendBluetoothReadCommand() {
    const device = this.bluetoothConnectedDevice.device;
    if (!this.bluetoothConnectedDevice) {
      alert('Bluetooth device not connected');
      return;
    }
    else{
      try {
        const data = dataViewToText(
          await BleClient.read(
            device.deviceId,
            this.serviceUUID,
            this.txCharacteristic
          )
        );
        alert(`Read: ${data}`);
        console.log({ data });
  
        return { data };
      } catch (error) {
        console.error('Read error: ', JSON.stringify(error));
        alert(`${JSON.stringify(error)}`);
      }
    }
  }

  getHexTimeFromDateTime(dateTime){
    var data = [];
    var d = dateTime.split('T')[1];
    var t = d.split('+')[0];
    data[0] = t.split(':')[0].toString(16); // hours
    data[1] = t.split(':')[1].toString(16); // minutes
    console.log(data);
    return data;
  }

}
