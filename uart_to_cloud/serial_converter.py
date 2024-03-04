import serial
import requests
import time

COM_PORT = "/dev/ttyUSB0"
BAUD_RATE = 9600
TIMEOUT = 0.1
MAIN_URL = "http://127.0.0.1:5000"
TEMPERATURE_POST_API = "/temperature/post"
LED_STATE_POST_API = "/led_state/post"
LED_STATE_GET_API = "/led_state/get"

def temperature_post_api(temperature):
    requests.post(MAIN_URL + TEMPERATURE_POST_API, data=temperature)

def led_state_post_api(led_state):
    requests.post(MAIN_URL + LED_STATE_POST_API, data=led_state)

def led_state_get_api():
    led_state = requests.get(MAIN_URL + LED_STATE_GET_API).content.decode('utf-8')
    print(led_state)

def main():
    serialManager = None
    try:
        serialManager = serial.Serial(COM_PORT, BAUD_RATE, timeout=TIMEOUT)
        while True:
            cmd_input = input().encode('utf-8')
            serialManager.write(cmd_input)
            time.sleep(TIMEOUT)
            output = serialManager.readline()
            if output:
                print(output.decode('utf-8'), end="")
    except:
        serialManager.close()

if __name__ == "__main__":
    led_state_get_api()
    # main()