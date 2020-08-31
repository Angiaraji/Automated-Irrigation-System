# import the modules needed
import time
import requests
import json
import serial
import RPi.GPIO as GPIO

# checking for the serial port connection
output = " "
ser = serial.Serial('/dev/ttyUSB0',38400 , 8, 'N', 1, timeout=1)

# uploading data to api
def Upload_Data(updata):
  url= 'http://smartiapi.azurewebsites.net/api/sensorcontroller/putsensordata'
  headers = {"Content-Type":"application/json",'Accept':'text/plain'}
  response=requests.post(url,data=updata,headers=headers)
  print(updata)
  print(response)

  output = ser.readline()
  print(output)


# Reading data from api and controlling motor actions
def code():
# defining pins
  GPIO.setmode(GPIO.BCM)
  GPIO.setwarnings(False)
  GPIO.setup(20,GPIO.OUT)

  main_api = 'http://smartiapi.azurewebsites.net/api/devicecontroller/getdevicestatus/1'
  url = main_api

  json_data = requests.get(url).json()
  print(json_data)

  device_status = json_data['device_data']['device_status']
  print()
  print('motor status: ' + device_status)

# checking for motor status in server
  if device_status == 'ON':
    GPIO.output(20, True)
    print("motor is on")
  else:
    GPIO.output(20, False)
    print("motor is off")

while True:
    code()
    Upload_Data(output)
    time.sleep(15)
