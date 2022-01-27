import paho.mqtt.client as mqtt
import sqlite3
import json
import data_decoder as fd_decoder
from time import time
from pyfcm import FCMNotification


MQTT_HOST = 'eu1.cloud.thethings.network'
MQTT_PORT = 1883
MQTT_CLIENT_ID = 'mqtt-explorer-a526d6cb'
MQTT_USER = 'uantwerpen-iot-test@ttn'
MQTT_PASSWORD = 'NNSXS.PGWWIDSWK6HUOT3SF3RWGT3ZTZLGZVRDYJ7CHIQ.4MMZNDNTMJBSDL3PDRWK6YHDYKT65YHZ2T3DE4CF73Q5FMTGHXUQ'
TOPIC_EXT = 'v3/uantwerpen-iot-test@ttn/devices/eui-08504b5767365820/up' # Topic for exterior sensor values of frost detector
TOPIC_INT = 'v3/uantwerpen-iot-test@ttn/devices/eui-0e50434d67394920/up' # Topic for interior sensor values of frost detector
FIREBASE_API_KEY = "your_firebase_API_key_here"

# Location of the database file
DATABASE_FILE = 'frostdetector.db'


def on_connect(mqtt_client, user_data, flags, conn_result):
    mqtt_client.subscribe(TOPIC_EXT)
    mqtt_client.subscribe(TOPIC_INT)


def on_message(mqtt_client, user_data, message):
    payload = message.payload.decode('utf-8')
    print('MQTT Data Received...')
    print('MQTT Topic: ' + message.topic)
    #print('Data: ' + payload)
    decoded_data = fd_decoder.decode_data_from_payload(payload)
    print(decoded_data)
    db_conn = user_data['db_conn']
    sql = 'INSERT INTO sensors_data_v2 (topic, device_id, temperature, humidity, battery_charge, created_at) VALUES (?, ?, ?, ?, ?, ?)'
    cursor = db_conn.cursor()
    cursor.execute(sql, (message.topic, decoded_data[0], decoded_data[1], decoded_data[2], decoded_data[3], decoded_data[4]))
    db_conn.commit()
    cursor.close()
    # Send a notification to all registered users
    push_notify_all_users(db_conn, str(round(decoded_data[1], 2)) + '°C, ' + str(round(decoded_data[2], 2)) + '%')
    # Todo: in future work a user framework should be added 
    # This ensures a single frost detector only notifies users associated with this specific frostdetector 

def create_sensordata_table(db_conn):
    sql = """
    CREATE TABLE IF NOT EXISTS sensors_data_v2 (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        topic TEXT NOT NULL,
        device_id TEXT NOT NULL,
        temperature INTEGER NOT NULL,
        humidity INTEGER NOT NULL,
        battery_charge INTEGER NOT NULL,
        created_at TEXT NOT NULL
    )
    """
    cursor = db_conn.cursor()
    cursor.execute(sql)
    cursor.close()

def create_user_table(db_conn):
    sql = """
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        registration_token TEXT NOT NULL,
        created_at TEXT NOT NULL
    )
    """
    cursor = db_conn.cursor()
    cursor.execute(sql)
    cursor.close()

def push_notify_all_users(db_conn, message):
    push_service = FCMNotification(api_key=FIREBASE_API_KEY)
    # Send to multiple devices by passing a list of ids.
    registration_ids = get_all_registration_tokens(db_conn)
    message_title = "Message from Frost Detector"
    message_body = message
    result = push_service.notify_multiple_devices(registration_ids=registration_ids, message_title=message_title, message_body=message_body)

def get_all_registration_tokens(db_conn):
    db_conn.row_factory = lambda cursor, row: row[0]
    cursor = db_conn.cursor()
    statement = "SELECT registration_token FROM users"
    cursor.execute(statement)
    return cursor.fetchall()

def main():
    db_conn = sqlite3.connect(DATABASE_FILE)

    create_sensordata_table(db_conn)
    create_user_table(db_conn)

    mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.user_data_set({'db_conn': db_conn})

    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    print('Sensor service back online')
    push_notify_all_users(db_conn, 'Sensor service back online')

    mqtt_client.connect(MQTT_HOST, MQTT_PORT)
    mqtt_client.loop_forever()


main()
