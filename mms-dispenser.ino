#include <Servo.h>
#include <SPI.h>
#include <WiFiNINA.h>

#include "secrets.h" 

Servo servo;
WiFiSSLClient client;

String users[4] = {"cedricelslander", "aldwin", "jgoubert", "robin_"};
int stories[4];
  
void setup() {
  Serial.begin(9600);
  while (!Serial) continue;
}

void loop() {
  for(int i=0;i<4;i++) {    
    int amount= get_story_points(i);  
    if(amount > 0) {
      servo_dispense(amount);
    }
    delay(5000);
  }
}

int disconnect_shortcut() {
  if (!client.connected()) {  
    Serial.println(); 
    Serial.println("Disconnecting from server..."); 
    client.stop();
  }
}

int connect_shortcut() {
  Serial.println("Connecting to server...");
  
  connect_wifi();
  
  char server[] = "api.app.shortcut.com";
  if (!client.connect(server, 443)) {
    Serial.println(F("Connection failed"));
    return -1;
  }
  
  Serial.println("Connected to server");
  return 0;
}

int get_story_points(int user_index) {
  Serial.print("\nCheck for new completed story for:");
  Serial.println(users[user_index]);
  
  while(!client.connected()) {
    if(connect_shortcut() == -1) {
      disconnect_shortcut();
      delay(1000);
    }
  }
  
  // Send HTTP request
  client.println(F("GET /api/v2/search/stories?token=5d8c41c5-82ee-4f2c-8fe6-4d791f2e7cea&query=state:done%20owner:" + users[user_index] + "&page_size=1 HTTP/1.1"));
  client.println(F("Host: api.app.shortcut.com"));
  //client.println(F("Connection: close"));

  if (client.println() == 0) {
    Serial.println("\nFailed to send request");
    return -1;
  }
  Serial.println("\nRequest was sent");

  Serial.println("\nWaiting for available bytes to read...");
  while(!client.available()) {
    Serial.println("No available bytes to read yet");
    delay(1000);
  }
  
  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return -1;
  }
  
  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return -1;
  }

  return parse_raw(user_index);  
}

int parse_raw(int user_index) {
  char buffer[100];
  char matcher_id[] = "\"id\":";
  char matcher_estimate[] = "\"estimate\":";
  String s_id;
  String s_estimate;
  int i_id = 0;
  int i_estimate = 0;
  
  while (client.available()) {
    char c = client.read();
    
    for(int i=0;i<100-1;i++) {
      buffer[i] = buffer[i+1];
    }
    buffer[100-1] = c;
    
    if(strncmp(buffer, matcher_id, sizeof(matcher_id) - 1) == 0){
      s_id= "";
      int i=5;
      while (buffer[i] != ',') {
        s_id += buffer[i++];
      }
    }

    if(strncmp(buffer, matcher_estimate, sizeof(matcher_estimate) - 1) == 0){
      s_estimate= "";
      int i=11;
      while (buffer[i] != ',') {
        s_estimate += buffer[i++];
      }
    }    
  }

  i_id = s_id.toInt();
  i_estimate = s_estimate.toInt();

  if(stories[user_index] != i_id){
    Serial.println(F("Found a new story!"));

    Serial.println(F("latest story id:"));
    Serial.print(i_id);
    Serial.println(F("story points:"));
    Serial.print(i_estimate);
    
    stories[user_index] = i_id;
    return i_estimate;
  } else {
    Serial.println(F("No new story found"));
    return 0;
  }
}

void connect_wifi() {
  char ssid[] = SECRET_SSID;
  char pass[] = SECRET_PASS;
  
  int status = WL_IDLE_STATUS;

  while (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    delay(1000);
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(1000);
  }

  Serial.println("Connected to wifi");
  print_wifi_status();
}

void print_wifi_status() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void servo_dispense(int amount) {
  Serial.println(F("Dispensing..."));
  int servoPin = 3;
  
  servo.attach(servoPin);
  servo.write(75);
  delay(5000);
  servo.detach();
}
