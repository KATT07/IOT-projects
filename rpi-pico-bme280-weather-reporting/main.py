import time
import network
from umqtt.simple import MQTTClient
from machine import Pin,I2C
import bme280

led = Pin("LED", Pin.OUT)

led.value(1)

i2c=I2C(0,sda=Pin(0), scl=Pin(1), freq=400000)

'''
#for station mode use this
ssid = 'Enter ur ssid'
password = 'Enter ur password'

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
while True:
    wlan.connect(ssid, password)
    if wlan.status()==3:
        break
    time.sleep(4)
status = wlan.ifconfig()

'''

#for soft ap mode use this
ssid='Enter ur ssid'
password='Enter ur password'
ap = network.WLAN(network.AP_IF)
ap.config(essid=ssid, password=password)
ap.active(True)
status = ap.ifconfig()
print(status)

led.value(0)

print("IP = "+status[0])

s = 3
while s > 0:
    s -= 1
    led.value(1)
    time.sleep(0.5)
    led.value(0)
    time.sleep(0.5)

def connectMQTT():
    client = MQTTClient(client_id=b"picow",server=b"192.168.4.50",port=0,user=b"pico",password=b"picopassword")
    client.connect()
    led.value(1)
    print("Connected to MQTTServer \n",client)
    return client

while True:
    try:
        client = connectMQTT()
        break
    except:
        print("Failed Retrying...")

while True:
    bme = bme280.BME280(i2c=i2c)          #BME280 object created
    print(bme.values)
    temp=bme.values[0]
    client.publish('temp',temp)
    humid=bme.values[2]
    client.publish('humid',humid)
    time.sleep(0.75)
    led.value(0)
    time.sleep(0.5)
    led.value(1)

#end of script


