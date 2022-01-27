import { Component, OnInit, NgZone } from '@angular/core';
import { ActivatedRoute, Router } from '@angular/router';
import { BLE } from '@awesome-cordova-plugins/ble/ngx';
import { BluetoothLE } from '@awesome-cordova-plugins/bluetooth-le/ngx';

@Component({
  selector: 'app-device',
  templateUrl: './device.page.html',
  styleUrls: ['./device.page.scss'],
})
export class DevicePage implements OnInit {

  peripheral: any = {};
  statusMessage: string;
  isConnected: boolean = false;
  dateTimeFromPicker: any;
  serviceUUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e'; //Nordic UART Service
  rxCharacteristic = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; //Write data to the RX Characteristic to send it on to the UART interface
  txCharacteristic = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';
  battery = {
    service: '1800',
    level: '2A00'
};

  constructor(private router: Router,
    private activatedRoute: ActivatedRoute,
    private ble: BLE,
    private bluetoothle: BluetoothLE,
    private ngZone: NgZone) {
    let device = this.router.getCurrentNavigation().extras.state;
    this.setStatus('Connecting to ' + device.name || device.id);
    this.ble.connect(device.id).subscribe(
      peripheral => this.onConnected(peripheral),
      peripheral => this.onDeviceDisconnected(peripheral)
    );
  }

  ngOnInit() {
  }

  onConnected(peripheral) {
    this.ngZone.run(() => {
      this.setStatus('Connected');
      this.peripheral = peripheral;
      this.isConnected = true;
    });
  }

  onDeviceDisconnected(peripheral) {
    this.ngZone.run(() => {
      this.setStatus('Disconnected');
    });
  }

  updateAlarmOverBLE() {
    // Convert datetime in a 3 byte array with time values
    //var data = this.getHexTimeFromDateTime(this.dateTimeFromPicker);
    var data = this.stringToBytes('test');
    const encoder = new TextEncoder();
    this.ble.write(this.peripheral, this.battery.service, this.battery.level, encoder.encode('test')).then(
      succ => {alert("wrote back");},
      fail => {alert("did not write back");}
    );
  }

  readBLE(){
    // read data from a characteristic, do something with output data
    this.ble.read(this.peripheral, this.battery.service, this.battery.level).then(
      succ => {alert("READ SUCCESS");},
      fail => {alert("READ FAILURE");}
    );
  }

  readSucc(data){
    console.log("Hooray we have data"+JSON.stringify(data));
    alert("Successfully read data from device."+JSON.stringify(data));
  };
  readFail(failure){
    alert("Failed to read characteristic from device.");
  };

  getHexTimeFromDateTime(dateTime){
    var data = new Uint8Array(3);
    var d = dateTime.split('T')[1];
    var t = d.split('+')[0];
    data[0] = t.split(':')[0].toString(16); // hours
    data[1] = t.split(':')[1].toString(16); // minutes
    data[2] = (t.split(':')[2]).split('.')[0].toString(16); // seconds
    return data;
  }

  stringToBytes(string) {
    var array = new Uint8Array(string.length);
    for (var i = 0, l = string.length; i < l; i++) {
        array[i] = string.charCodeAt(i);
    }
    return array;
  }

  // Disconnect peripheral when leaving the page
  ionViewWillLeave() {
    console.log('ionViewWillLeave disconnecting Bluetooth');
    this.ble.disconnect(this.peripheral.id).then(
      () => console.log('Disconnected ' + JSON.stringify(this.peripheral)),
      () => console.log('ERROR disconnecting ' + JSON.stringify(this.peripheral))
    )
  }

  setStatus(message) {
    console.log(message);
    this.ngZone.run(() => {
      this.statusMessage = message;
    });
  }
}
