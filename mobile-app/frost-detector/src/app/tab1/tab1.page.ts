import { Component } from '@angular/core';
import { FCM } from '@awesome-cordova-plugins/fcm/ngx';

@Component({
  selector: 'app-tab1',
  templateUrl: 'tab1.page.html',
  styleUrls: ['tab1.page.scss']
})
export class Tab1Page {

  constructor(private fcm: FCM) {}

  // getToken() {
  //   this.fcm.getToken().then(token => {
  //     // Register your new token in your back-end if you want
  //     // backend.registerToken(token);
  //   });
  // }

}
