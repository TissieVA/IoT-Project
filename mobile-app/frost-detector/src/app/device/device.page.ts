import { Component, OnInit, NgZone } from '@angular/core';
import { ActivatedRoute, Router } from '@angular/router';
import { BLE } from '@awesome-cordova-plugins/ble/ngx';

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

  constructor(private router: Router,
    private activatedRoute: ActivatedRoute,
    private ble: BLE,
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
    var data = this.getHexTimeFromDateTime(this.dateTimeFromPicker);
    this.ble.writeWithoutResponse(this.peripheral, this.serviceUUID, this.rxCharacteristic, data.buffer);
  }

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
    return array.buffer;
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
