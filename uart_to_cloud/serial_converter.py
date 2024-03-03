import serial
import requests
import time

COM_PORT = "/dev/ttyUSB0"
BAUD_RATE = 9600
TIMEOUT = 0.1

def main():
    serialManager = None
    try:
        serialManager = serial.Serial(COM_PORT, BAUD_RATE, timeout = TIMEOUT)
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
    main()