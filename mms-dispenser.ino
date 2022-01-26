#include <Servo.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <SoftwareWire.h>
SoftwareWire Wire(SDA,SCL);

#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include "secrets.h"

#include "zelda.h"
#include "gameofthrones.h"
#include "nokia.h"
#include "pacman.h"
#include "starwars.h"
#include "supermariobros.h"

Servo servo;
WiFiSSLClient client;
hd44780_I2Cexp lcd;

const int nof_users = 5;
String users[nof_users] = {"cedricelslander", "aldwin", "jgoubert", "robin_", "nathanvangierdegom"};
int stories[nof_users];

void setup() {
  Serial.begin(9600);
  while (!Serial) continue;
  
  lcd.begin(16,2);
  lcd.init();
}

void loop() {
  for(int i=0;i<nof_users;i++) {    
    int amount= get_story_points(i);
   
    if(amount > 0) {
      display_lcd(users[i], "STORY POINTS: " + String(amount));
      play(i);
      servo_dispense(amount);
    }
    delay(5000);
  }
}

void play(int user_index) {
  switch(user_index) {
    case 0:
      starwars();
      break;
    case 1:
      supermariobros();
      break;
    case 2:
      gameofthrones();
      break;
    case 3:
      nokia();
      break;
    case 4:
      pacman();
      break;
  }
}

void display_lcd(String message1, String message2) {
  lcd.clear();
  //lcd.setBacklight(0);
  //lcd.backlight();
  lcd.noBacklight();

  //lcd.home();
  lcd.setCursor(0, 0);
  lcd.print(message1);
  lcd.setCursor(0, 1);
  lcd.print(message2);
}

int disconnect_shortcut() {
  if (!client.connected()) {
    Serial.println("DISCONNECTING");
    client.stop();
  }
}

int connect_shortcut() {  
  connect_wifi();
  
  char server[] = "api.app.shortcut.com";
  if (!client.connect(server, 443)) {
    Serial.println(F("CONNECT FAILED"));
    return -1;
  }
  return 0;
}

int get_story_points(int user_index) {  
  while(!client.connected()) {
    if(connect_shortcut() == -1) {
      disconnect_shortcut();
      delay(1000);
    }
  }
  
  // Send HTTP request
  client.println(F("GET /api/v2/search/stories?token=" + SECRET_TOKEN + "&query=state:done%20owner:" + users[user_index] + "&page_size=1 HTTP/1.1"));
  client.println(F("Host: api.app.shortcut.com"));
  //client.println(F("Connection: close"));

  if (client.println() == 0) {
    Serial.println("SEND REQUEST ERR");
    return -1;
  }

  while(!client.available()) {
    delay(1000);
  }
  
  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("RESPONSE NOK"));
    Serial.println(status);
    return -1;
  }
  
  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("RESPONSE ERR"));
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
    stories[user_index] = i_id;
    return i_estimate;
  } else {
    return 0;
  }
}

void connect_wifi() {
  char ssid[] = SECRET_SSID;
  char pass[] = SECRET_PASS;
  
  int status = WL_IDLE_STATUS;

  while (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WIFI ERR");
    delay(1000);
  }

  while (status != WL_CONNECTED) {
    Serial.println("WIFI SSID:");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(1000);
  }

  Serial.println("WIFI CONNECTED");
  print_wifi_status();
}

void print_wifi_status() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID:");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP:");
  Serial.println(F(ip));
}

void servo_dispense(int amount) {
  int servoPin = 3;
  
  servo.attach(servoPin);
  servo.write(0);
  delay(amount * 1000);
  servo.detach();
}
