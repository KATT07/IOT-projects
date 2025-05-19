# Setup
You will need the following pre-configured
+ mqtt Broker
+ mqtt subscriber to send the messages node-red/homeassistant/etc

Goto Library Manager on Arduino IDE                                                                                                                                                                                                             
Add PubSubClient Library by Nick O'Leary

# Software
Replace XXX in the .ino to the correct details of ur mqtt broker and wifi credentials.                                                                                                                                                          
Also Make sure to use 2.4GHz Wifi as esp8266 cannot connect to 5Ghz wifi

# Connections
esp8266 -> Relay                                                                                                                                                                                                                            
D1 pin -> Relay 1 (In1)                                                                                                                                                                                                                         
D2 pin -> Relay 2 (In2)                                                                                                                                                                                                                         
Vin(5v) -> Vcc                                                                                                                                                                                                                            
Gnd(5v) -> Gnd                                                                                                                                                                                                                            

# General
MQTT messages:                                                                                                                                                                                                                            
on1: Relay 1 Turns on                                                                                                                                                                                                                           
on2: Relay 2 Turns on                                                                                                                                                                                                                           
off1: Relay 1 Turns off                                                                                                                                                                                                                         
off2: Relay 2 Turns off                                                                                                                                                                                                                         
