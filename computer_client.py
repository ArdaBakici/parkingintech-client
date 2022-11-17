import requests
import json
from serial import Serial
import serial
import time

# Constants || Varies from computer to computer check these variables before running the program
SERVER_ADRESS = 'https://www.parking-in.tech/'
#SERVER_ADRESS = 'http://localhost:5000/'
SUFFIX = 'data/update/'
dev_id = 1
lot_id = 1
dev_key = 'deneme123'
arduino_port = 'COM3' # in windows COM, in linux /dev/...
baud_rate = 9600
time_out = 5
period_time = 900 # 900
retry_time = 300 # 300
error_delay = 60

def getEmptySlots():
    # TODO Get data from arduino
    return [2,3,4]


def setup():
    pass


def loop():
    # Serial communication chart
    # s | Arduino: Ready to take data
    # o | Python: Send data
    # a | Arduino: data array start
    # p | Arduino: data array parse
    # e | Arduino: data array end
    # r | Python: Successfully got the data
    with serial.Serial(port=arduino_port, baudrate=baud_rate, timeout=time_out) as ser:
        waiting_data = False
        com_available = False
        last_updated = None
        while(not com_available):
            if readSerial(ser) == "s":
                print("Connection established with arduino")
                com_available = True
                # To initate first data request give a minus value
                last_updated = -(period_time*2)
            else:
                print("No response from arduino trying again...")

        while True:
            if readSerial(ser) == 'a':
                print("Got data from arduino.")
                data = readArrayFormatAndRespond(ser)
                last_updated = time.time()
                sendDataToServer(data)
                waiting_data = False
            if waiting_data:
                if (time.time() - last_updated) > retry_time:
                    print(f"No connection with arduino trying again in {retry_time} seconds...")
                    last_updated = time.time()
            elif (time.time() - last_updated) > period_time:
                if last_updated < 0:
                    print("First data is requested from arduino.")
                else:
                    print(f"No data from arduino about {period_time} seconds. Trying to established connection.")
                ser.write(b'o')
                waiting_data = True
                last_updated = time.time()


def readSerial(ser, read_size=1):
    return ser.read(size=read_size).decode('utf-8')

def readArrayFormatAndRespond(ser):
    ard_data = ser.read_until(expected=b'e') # read
    print(ard_data) # debug
    ser.write(b'r') # respond
    data_str = ard_data.decode('utf-8')
    data_str = remove_suffix(data_str, 'e')
    data_str = data_str.split('p')
    data = []
    for num_str in data_str:
        if num_str.isnumeric():
            data.append(int(num_str))
    return data


def sendDataToServer(empty_slots):
    res = requests.post(f"{SERVER_ADRESS}{SUFFIX}{dev_id}", json={"dev_key": dev_key, 
                                                                  "empty_slots": empty_slots,
                                                                  "lot_id": lot_id})
    if res.ok:
        print(f"SERVER: {res.json()}")

res = requests.post(f"https://akilli-otopark.herokuapp.com/data/update/1", json={"dev_key": 'deneme123', "empty_slots": [], "lot_id": 1})

def remove_suffix(input_string, suffix):
    if suffix and input_string.endswith(suffix):
        return input_string[:-len(suffix)]
    return input_string


if __name__ == '__main__':
    while True:
        print('Welcome!\nDo not forget that this program is a infinite loop and you need to manually close it')
        print('Starting connection with arduino...')
        loop()
        #try:
        #    loop()
        #except:
        #    print('Error occured. Please check if arduino is connected or remote server is running. Retrying in 1 minute...')
        #    time.sleep(error_delay)
