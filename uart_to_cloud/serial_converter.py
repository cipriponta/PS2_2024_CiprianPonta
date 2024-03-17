import serial
import requests
import time

COM_PORT = "COM4"
BAUD_RATE = 9600
SERIAL_TIMEOUT = 0.1
LOOP_TIMEOUT = 1
INIT_TIMEOUT = 3

MAIN_URL = "http://127.0.0.1:5000"
TEMPERATURE_POST_API = "/temperature/post"
DEVICE_LED_STATE_POST_API = "/device_led_state/post"
CLOUD_LED_STATE_GET_API = "/cloud_led_state/get"
MESSAGES_POST_API="/messages/post"

DEVICE_COMMAND_TURN_LED_ON = "A"
DEVICE_COMMAND_TURN_LED_OFF = "S"
DEVICE_COMMAND_GET_TEMPERATURE = "T"
DEVICE_COMMAND_GET_LED_STATE  = "L"
DEVICE_COMMAND_READ_MESSAGES = "M"
DEVICE_COMMAND_WRITE_MESSAGE = "N"

class SerialConverter:
    def __init__(self):
        self._serialManager = serial.Serial(COM_PORT, BAUD_RATE, timeout=SERIAL_TIMEOUT)

    def _sendCommand(self, command):
        self._serialManager.write(command.encode("utf-8"))
        time.sleep(SERIAL_TIMEOUT)
        output = self._serialManager.readlines()
        return output

    def send_temperature_to_cloud(self):
        print("TEMPERATURE  -> CLOUD")
        temperature = self._sendCommand(DEVICE_COMMAND_GET_TEMPERATURE)[0].decode("utf-8").strip()
        if temperature:
            requests.post(MAIN_URL + TEMPERATURE_POST_API, data=temperature)
        else:
            print("Error: couldn't get the temperature from the device")

    def send_led_state_to_cloud(self):
        print("LED_STATE    -> CLOUD")
        led_state = self._sendCommand(DEVICE_COMMAND_GET_LED_STATE)[0].decode("utf-8").strip()
        if led_state:
            requests.post(MAIN_URL + DEVICE_LED_STATE_POST_API, data=led_state)
        else:
            print("Error: couldn't get the led state from the device")

    def set_led_state_on_device(self):
        print("LED_STATE    -> DEVICE")
        led_state = requests.get(MAIN_URL + CLOUD_LED_STATE_GET_API).content.decode("utf-8")
        if led_state:
            if "OFF" == led_state:
                self._sendCommand(DEVICE_COMMAND_TURN_LED_OFF)
            elif "ON" == led_state:
                self._sendCommand(DEVICE_COMMAND_TURN_LED_ON)
            else:
                print("Error: incorrect led state received from the cloud")
        else:
            print("Error: couldn't set the new led state to the device")

    def send_messages_to_cloud(self):
        print("MESSAGES     -> CLOUD")
        messages = self._sendCommand(DEVICE_COMMAND_READ_MESSAGES)
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

    def check_new_message_to_device(self):
        pass

    def close(self):
        self._serialManager.close()

def main():
    serialConverter = None
    try:
        serialConverter = SerialConverter()

        time.sleep(INIT_TIMEOUT)

        serialConverter.send_led_state_to_cloud()
        serialConverter.send_temperature_to_cloud()
        serialConverter.send_messages_to_cloud()

        while True:
            print("--------------------------------------")
            serialConverter.send_temperature_to_cloud()
            serialConverter.send_led_state_to_cloud()
            serialConverter.set_led_state_on_device()
            serialConverter.send_messages_to_cloud()
            print("--------------------------------------")
            time.sleep(LOOP_TIMEOUT)

    except Exception as e:
        print(e)
        serialConverter.close()

if __name__ == "__main__":
    main()