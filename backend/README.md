# Backend
## API
A simple RESTfull API written with the micro web framework 'Flask'. The API is basic and is only used for prototype purposes. 
Since we currently make no distinction between different Frost Detector devices, all data from all connected Frost Detectors is stored in one central database table which can be requested through a single GET request. In future work, a distinction can be made based on the device EUI which is also stored in the database.

The API exposes 3 endpoints:
1. ```@app.route("/all-sensor-data", methods=["GET"])```
Returnes all sensor data from all Frost Detectors stored in the database.
2. ```@app.route("/sensor-data/<id>", methods=["GET"])```
Returns a specific reading from the database base.
3. ```@app.route("/users/add", methods=["POST"])```
Verifies a user to the system. Currently used to register a device to the notification service. This is automatically done every time the mobile application is launched.

## Frost Detector Service
A service that maps incomming MQTT sensor data from 'The Thing Network' to an SQLite database. The service also notifies the mobile application of a new reading. A new reading is received once every day based on the wake up time, configured in the Frost Detector device. The database stores 2 types of objects:
* A reading:
``` 
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    topic TEXT NOT NULL,
    device_id TEXT NOT NULL,
    temperature INTEGER NOT NULL,
    humidity INTEGER NOT NULL,
    battery_charge INTEGER NOT NULL,
    created_at TEXT NOT NULL
```
* And a user:
``` 
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    registration_token TEXT NOT NULL,
    created_at TEXT NOT NULL
```
Notifications are sent through the Firebase Cloud Messaging (FCM) protocol. 

## How to use
### Prequisites
* It is reccomended to use a python virtual environment (venv) to run the backend.
* pyfcm
* paho-mqtt
* flask and flask-cors
### Run
* When using a virtual environment, make sure to activate it
* You can start the API and the FDService each by running their main.py 