import serial
import requests
import time
import smtplib
import ssl
from credentials import *

COM_PORT = CREDENTIALS_COM_PORT
BAUD_RATE = 9600
SERIAL_TIMEOUT = 0.1
LOOP_TIMEOUT = 1
INIT_TIMEOUT = 3

EMAIL_ACCOUNT = CREDENTIALS_EMAIL_ACCOUNT
EMAIL_PASSWORD = CREDENTIALS_EMAIL_PASSWORD

MAIN_URL = CREDENTIALS_MAIN_URL
TEMPERATURE_POST_API = "/temperature/post"
DEVICE_LED_STATE_POST_API = "/device_led_state/post"
CLOUD_LED_STATE_GET_API = "/cloud_led_state/get"
MESSAGES_POST_API="/messages/post"
NEW_MESSAGE_GET_API="/new_message/get"
TIMESTAMP_GET_API="/timestamp/get"
FLOODS_POST_API="/floods/post"
DELETE_FLOOD_GET_API="/delete_flood/get"

DEVICE_COMMAND_TURN_LED_ON = "A"
DEVICE_COMMAND_TURN_LED_OFF = "S"
DEVICE_COMMAND_GET_TEMPERATURE = "T"
DEVICE_COMMAND_GET_LED_STATE  = "L"
DEVICE_COMMAND_READ_MESSAGES = "M"
DEVICE_COMMAND_WRITE_MESSAGE = "N"
DEVICE_COMMAND_READ_TIMESTAMP = "G"
DEVICE_COMMAND_WRITE_TIMESTAMP = "H"
DEVICE_COMMAND_READ_FLOOD_DETECTED = "F"
DEVICE_COMMAND_READ_FLOODS = 'Y'
DEVICE_COMMAND_DELETE_FLOODS = 'U'

class SerialConverter:
    def __init__(self):
        self._serialManager = serial.Serial(COM_PORT, BAUD_RATE, timeout=SERIAL_TIMEOUT)

    def _send_command(self, command):
        self._serialManager.write(command.encode("utf-8"))
        time.sleep(SERIAL_TIMEOUT)
        output = self._serialManager.readlines()
        return output

    def send_temperature_to_cloud(self):
        print("TEMPERATURE  -> CLOUD")
        temperature = self._send_command(DEVICE_COMMAND_GET_TEMPERATURE)[0].decode("utf-8").strip()
        if temperature:
            requests.post(MAIN_URL + TEMPERATURE_POST_API, data=temperature)
        else:
            print("Error: couldn't get the temperature from the device")

    def send_led_state_to_cloud(self):
        print("LED_STATE    -> CLOUD")
        led_state = self._send_command(DEVICE_COMMAND_GET_LED_STATE)[0].decode("utf-8").strip()
        if led_state:
            requests.post(MAIN_URL + DEVICE_LED_STATE_POST_API, data=led_state)
        else:
            print("Error: couldn't get the led state from the device")

    def set_led_state_on_device(self):
        print("LED_STATE    -> DEVICE")
        led_state = requests.get(MAIN_URL + CLOUD_LED_STATE_GET_API).content.decode("utf-8")
        if led_state:
            if "OFF" == led_state:
                self._send_command(DEVICE_COMMAND_TURN_LED_OFF)
            elif "ON" == led_state:
                self._send_command(DEVICE_COMMAND_TURN_LED_ON)
            else:
                print("Error: incorrect led state received from the cloud")
        else:
            print("Error: couldn't set the new led state to the device")

    def send_messages_to_cloud(self):
        print("MESSAGES     -> CLOUD")
        messages = self._send_command(DEVICE_COMMAND_READ_MESSAGES)
        message_table = []

        for message in messages:
            decoded_message = message.decode("utf-8").strip().split("|")
            is_valid = int(decoded_message[0])
            if 1 == is_valid:
                message_entry = {}
                message_entry["timestamp"] = int(decoded_message[1])
                message_entry["message"] = decoded_message[2]
                message_table.append(message_entry)

        if message_table:
            requests.post(MAIN_URL + MESSAGES_POST_API, json=message_table)
        else:
            print("Error: couldn't get the messages from the device")

    def check_new_message_for_device(self):
        new_message = requests.get(MAIN_URL + NEW_MESSAGE_GET_API).json()
        if True == new_message["valid"]:
            print("NEW MESSAGE  -> DEVICE")
            command = DEVICE_COMMAND_WRITE_MESSAGE + "|{0}|{1}!".format(new_message["timestamp"], new_message["message"])
            self._send_command(command)

    def send_timestamp_to_device(self):
        print("TIMESTAMP    -> DEVICE")
        timestamp = requests.get(MAIN_URL + TIMESTAMP_GET_API).content.decode("utf-8")
        command = DEVICE_COMMAND_WRITE_TIMESTAMP + "|{0}!".format(timestamp) 
        output = self._send_command(command)

    def check_flood_sensor_state(self):
        floodState = self._send_command(DEVICE_COMMAND_READ_FLOOD_DETECTED)[0].decode("utf-8").strip()
        if "FLOOD_DETECTED" == floodState:
            print("FLOOD        -> EMAIL")
            context = ssl.create_default_context()
            with smtplib.SMTP("smtp.gmail.com", 587) as server:
                server.starttls(context=context)
                server.login(EMAIL_ACCOUNT, EMAIL_PASSWORD)
                server.sendmail(EMAIL_ACCOUNT, EMAIL_ACCOUNT, "FLOOD DETECTED")

    def send_floods_to_cloud(self):
        print("FLOODS       -> CLOUD")
        floods = self._send_command(DEVICE_COMMAND_READ_FLOODS)
        floods_table = []

        for flood in floods:
            decoded_message = flood.decode("utf-8").strip().split("|")
            is_valid = int(decoded_message[0])
            if 1 == is_valid:
                floods_table.append(int(decoded_message[1]))

        if floods_table:
            requests.post(MAIN_URL + FLOODS_POST_API, json=floods_table)
        else:
            print("Error: couldn't get the floods from the device")

    def check_delete_flood_for_device(self):
        delete_flood = requests.get(MAIN_URL + DELETE_FLOOD_GET_API).json()
        if True == delete_flood["valid"]:
            print("DELETE FLOOD -> DEVICE")
            command = DEVICE_COMMAND_DELETE_FLOODS + "|{0}!".format(delete_flood["timestamp"])
            self._send_command(command)

    def close(self):
        self._serialManager.close()

def main():
    serialConverter = None
    try:
        serialConverter = SerialConverter()

        time.sleep(INIT_TIMEOUT)

        serialConverter.send_led_state_to_cloud()
        serialConverter.send_temperature_to_cloud()
        serialConverter.send_timestamp_to_device()

        serialConverter.send_messages_to_cloud()
        serialConverter.send_floods_to_cloud()

        while True:
            print("--------------------------------------")

            serialConverter.send_temperature_to_cloud()
            serialConverter.send_led_state_to_cloud()
            serialConverter.set_led_state_on_device()
            serialConverter.send_timestamp_to_device()

            serialConverter.send_messages_to_cloud()
            serialConverter.send_floods_to_cloud()

            serialConverter.check_new_message_for_device()
            serialConverter.check_delete_flood_for_device()
            serialConverter.check_flood_sensor_state()

            time.sleep(LOOP_TIMEOUT)

    except Exception as e:
        print(e)
        serialConverter.close()

if __name__ == "__main__":
    main()