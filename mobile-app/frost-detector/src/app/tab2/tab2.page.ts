import { Component, OnInit } from '@angular/core';
import { HttpClient } from '@angular/common/http';

@Component({
  selector: 'app-tab2',
  templateUrl: 'tab2.page.html',
  styleUrls: ['tab2.page.scss']
})
export class Tab2Page implements OnInit{

  constructor(private http: HttpClient) {}

  all_sensor_data: any = [];

  ngOnInit(){
    this.runHttp();
  }
  
  doRefresh(event) {
    this.runHttp();
    setTimeout(() => {
      event.target.complete();
    }, 2000);
  }

  runHttp() {
    this.http.get('http://143.129.80.197:8080/all-sensor-data')
      .subscribe(data => {
        console.log(data);
        this.all_sensor_data = data;
      });
  }
  
}
