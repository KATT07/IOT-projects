#include <ESP8266WiFi.h> 
#include <PubSubClient.h> 
  
  
// WiFi 
const char *ssid = "XXX"; // Enter your WiFi name 
const char *password = "XXX";  // Enter WiFi password 
  
// MQTT Broker 
const char *mqtt_broker = "192.168.XXX.XXX"; 
const char *topic = "XXX"; 
const char *mqtt_username = "XXX"; 
const char *mqtt_password = "XXX"; 
const int mqtt_port = 1883; 
  
// Relay 1,2 
bool state1 = false; 
bool state2 = false; 
const int Relay1 = 5; 
const int Relay2 = 4; 
  
WiFiClient espClient; 
PubSubClient client(espClient); 
  
void setup() { 
     //Setup relay output mode 
     pinMode(Relay1, OUTPUT); 
     pinMode(Relay2,OUTPUT); 
     digitalWrite(Relay1, HIGH); 
     digitalWrite(Relay2,HIGH);   
  
     // Set software serial baud to 115200; 
     Serial.begin(115200); 
     delay(1000); // Delay for stability 
  
     // Connecting to a WiFi network 
     WiFi.begin(ssid, password); 
     while (WiFi.status() != WL_CONNECTED) { 
         delay(500); 
         Serial.println("Connecting to WiFi..."); 
     } 
     Serial.println("Connected to the WiFi network"); 
  
     // Connecting to an MQTT broker 
     client.setServer(mqtt_broker, mqtt_port); 
     client.setCallback(callback); 
     while (!client.connected()) { 
         String client_id = "esp8266-client-"; 
         client_id += String(WiFi.macAddress()); 
         Serial.printf("The client %s connects to the MQTT broker\n", client_id.c_str()); 
         if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) { 
             Serial.println("MQTT broker connected"); 
         } else { 
             Serial.print("Failed with state "); 
             Serial.print(client.state()); 
             delay(2000); 
         } 
     } 
  
     // Publish and subscribe 
     client.subscribe(topic); 
 } 
  
 void callback(char *topic, byte *payload, unsigned int length) { 
     Serial.print("Message arrived in topic: "); 
     Serial.println(topic); 
     Serial.print("Message: "); 
     String message; 
     for (int i = 0; i < length; i++) { 
         message += (char) payload[i];  // Convert *byte to string 
     } 
     Serial.println(message); 
     if ((message == "on1")&(state1==false)) { 
       Serial.println("OK Relay1 ON"); 
       digitalWrite(Relay1, LOW); 
       state1=true; 
  
     } 
     else if ((message == "off1")&(state1==true)) { 
       Serial.println("OK Relay1 OFF"); 
       digitalWrite(Relay1, HIGH);   
       state1=false; 
     } 
     else if ((message == "on2")&(state2==false)) { 
       Serial.println("OK Relay2 ON"); 
       digitalWrite(Relay2, LOW);   
       state2=true; 
     } 
     else if ((message == "off2")&(state2==true)) { 
       Serial.println("OK Relay2 OFF"); 
       digitalWrite(Relay2, HIGH);   
       state2=false; 
     } 
     else { 
       Serial.println("error invalid mqtt message"); 
     } 
     Serial.println(); 
     Serial.println("-----------------------"); 
 } 
  
 void loop() { 
     client.loop(); 
     delay(2000); // Delay for a short period in each loop iteration 
 }
