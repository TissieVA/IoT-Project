import json
import base64
import struct

def decode_data_from_payload(data):
    json_data = json.loads(data)
    device_id = json_data['end_device_ids']['device_id']
    frm_payload = json_data['uplink_message']['frm_payload']
    received_at = json_data['received_at']
    hex_payload = base64.b64decode(frm_payload).hex()
    hex_payload_noheader = hex_payload[8:]
    return_obj = (device_id, get_temperature(hex_payload_noheader), get_humidity(hex_payload_noheader), get_battery_charge(hex_payload_noheader), received_at)
    return return_obj

def get_temperature(data):
    # Only keep temperature part of payload
    size = len(data)
    hex_temperature = data[:size - 12]
    # Convert hex to float little endian
    temperature = struct.unpack('<f', bytes.fromhex(hex_temperature))[0]
    return temperature

def get_humidity(data):
    # Only keep humidity part of payload
    data_notemp = data[8:]
    size = len(data_notemp)
    hex_humidity = data_notemp[:size - 4]
    # Convert hex to float little endian
    humidity = struct.unpack('<f', bytes.fromhex(hex_humidity))[0]
    return humidity

def get_battery_charge(data):
    # Only keep battery charge part of payload
    hex_battery_charge = data[16:]
    # Swap endianness and convert to uint16
    battery_charge = int(hex_battery_charge[2:4] + hex_battery_charge[0:2], 16)
    return battery_charge
