from flask import Flask, jsonify, request
from flask_cors import CORS
import json
import sensordata_controller_v2 as sensordata_controller

app = Flask(__name__)
CORS(app, resources={r"/*": {"origins": "*"}})

@app.route("/all-sensor-data", methods=["GET"])
def get_sensor_data():
    all_sensors_data = sensordata_controller.get_sensor_values()
    all_json_sensors_data = []
    for d in all_sensors_data:
        sensor_data_dict = {
            "id": d[0],
            "topic": d[1],
            "device_id": d[2],
            "temperature": d[3],
            "humidity": d[4],
            "battery_charge": d[5],
            "created_at": d[6]
            }
        all_json_sensors_data.append(sensor_data_dict)
    return jsonify(all_json_sensors_data)


@app.route("/sensor-data/<id>", methods=["GET"])
def get_sensor_data_by_id(id):
    sensor_data = sensordata_controller.get_by_id(id)
    response = jsonify(
        id=sensor_data[0],
        topic=sensor_data[1],
        device_id=sensor_data[2],
        temperature=sensor_data[3],
        humidity=sensor_data[4],
        battery_charge=sensor_data[5],
        created_at=sensor_data[6]
        )
    return response

@app.route("/users/add", methods=["POST"])
def insert_user():
    user_details = request.get_json()
    print(user_details)
    name = user_details["name"]
    registration_token = user_details["registration_token"]
    result = sensordata_controller.verify_token(registration_token)
    return jsonify(result)


if __name__ == "__main__":
    app.run(host='0.0.0.0', port=8080, debug=False)
