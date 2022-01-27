# Frost Detector Mobile Application
Our mobile application is built in Ionic Angular using the Capacitor native runtime. This allows the application to be deployed for Android as well as IOS.
The Frost Detector mobile app is used to configure the time at which the Frost Detector device should do its daily reading. Furthermore, it is used to get notified of these daily readings and it checks if the windshield of the car equipped with a frost detector device is frozen or fogged up. A history of previous readings can also be seen in the readings history of the app.

## Running the app
The app can be installed through the [frost-detector-app.apk](https://github.com/TissieVA/IoT-Project/blob/master/mobile-app/apk/frost-detector-app.apk) or you can build it manually as explained in the following sections.
### Prerequisites
* Node.js
* A code editor
* Android Studio

### Install Ionic Tooling
* Pull the repository from Github
* Navigate to the '/mobile-app/frost-detector' directory and open the project in a code editor of choice.  
* Open a terminal and execute ```$ npm install -g @ionic/cli native-run cordova-res``` to install Ionic.

### Starting the app
* From here, you can run the application on localhost in your browser, although most functionalities won't work:
```
$ ionic serve
```
* To run the app on a mobile android phone:
```
$ ionic cap sync

$ ionic cap copy

$ ionic cap open android
```
* Connect an android phone to your computer and install its drivers
* The phone will show up in android studio and you can deploy the app by simply clicken the play button
* That's all!

### Configuring a Frost Detector
When a Frost Detector device is in close proximity of your phone running the app, you can look for it in the connection tab.
1. Awake the Frost Detector by pressing the bluetooth button on the Frost Detector
2. Turn on the bluetooth and location service of your phone
3. Go to the 'connect' tab of the app
4. Press the 'scan' button
![scan-overview](https://github.com/TissieVA/IoT-Project/blob/master/Project/documentation/images/scan-overview.png)
5. Press the 'connect' button. Once connected, you will receive a confirmation of a succesfull connection
6. You can set the alarm time with a slider:
![time-slider](https://github.com/TissieVA/IoT-Project/blob/master/Project/documentation/images/time-slider.png)
7. Finally, by pressing the 'update alarm' button, the alarm is set on the Frost Detector