#include <Servo.h>
#include <SPI.h>
#include <WiFiNINA.h>

// LCD
#include <SoftwareWire.h>
SoftwareWire Wire(SDA, SCL);
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

// SONGS
#include "zelda.h"
#include "gameofthrones.h"
#include "nokia.h"
#include "pacman.h"
#include "starwars.h"
#include "supermariobros.h"

// CONFIG
#include "config.h"

Servo servo;
WiFiSSLClient client;
hd44780_I2Cexp lcd;

String users[] = {"cedricelslander", "aldwin", "jgoubert", "robin_", "nathanvangierdegom"};
void (*songs [])() = {starwars, gameofthrones, nokia, pacman, zelda, supermariobros};

int stories[sizeof(users)/sizeof(*users)];

void setup() {
  Serial.begin(9600);
  while (!Serial) continue;

  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  lcd.init();

  Serial.println("Starting up, found nof users:" + String(sizeof(users)/sizeof(*users)));
}

void loop() {
  for (int i = 0; i < sizeof(users)/sizeof(*users); i++) {
    int story_points = get_story_points(i);
    if (story_points > 0) {
      display_lcd(users[i], "STORY POINTS: " + String(story_points));
      play(i % sizeof(songs)/sizeof(*songs));
      servo_dispense(story_points);
    }
    lcd.backlight();
    delay(5000);
  }
}

void play(int index) {
  Serial.println("Play song with index:" + String(index));
  songs[index]();
}

void display_lcd(String message1, String message2) {
  Serial.println("Display message on lcd:" + message1 + " | " + message2);
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
  Serial.println("Disconnect from Shortcut");
  if (!client.connected()) {
    Serial.println("Disconnecting from Shortcut");
    client.stop();
  }
  Serial.println("Disconnected from Shortcut");
}

int connect_shortcut() {
  Serial.println("Connecting to Shortcut");
  connect_wifi();

  char server[] = "api.app.shortcut.com";
  if (!client.connect(server, 443)) {
    Serial.println("Connection failed");
    return -1;
  }

  Serial.println("Connected to Shortcut");
  
  return 0;
}

int get_story_points(int user_index) {
  Serial.println("Get story points for:" + users[user_index]);
  
  while (!client.connected()) {
    if (connect_shortcut() == -1) {
      disconnect_shortcut();
      delay(1000);
    }
  }

  // Send HTTP request  
  client.println("GET /api/v2/search/stories?token=" + String(SECRET_TOKEN) + "&query=state:done%20owner:" + users[user_index] + "&page_size=1 HTTP/1.1");
  client.println("Host: api.app.shortcut.com");
  //client.println("Connection: close");

  if (client.println() == 0) {
    Serial.println("Error occured while sending request");
    return -1;
  }

  while (!client.available()) {
    delay(1000);
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print("Response not ok: ");
    Serial.println(status);
    return -1;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println("Response error");
    return -1;
  }

  return parse_raw(user_index);
}

int parse_raw(int user_index) {
  Serial.println("Parse raw data");
  
  char buffer[100];
  char matcher_id[] = "\"id\":";
  char matcher_estimate[] = "\"estimate\":";
  String s_id;
  String s_estimate;
  int i_id = 0;
  int i_estimate = 0;

  while (client.available()) {
    char c = client.read();

    for (int i = 0; i < 100 - 1; i++) {
      buffer[i] = buffer[i + 1];
    }
    buffer[100 - 1] = c;

    if (strncmp(buffer, matcher_id, sizeof(matcher_id) - 1) == 0) {
      s_id = "";
      int i = 5;
      while (buffer[i] != ',') {
        s_id += buffer[i++];
      }
    }

    if (strncmp(buffer, matcher_estimate, sizeof(matcher_estimate) - 1) == 0) {
      s_estimate = "";
      int i = 11;
      while (buffer[i] != ',') {
        s_estimate += buffer[i++];
      }
    }
  }

  i_id = s_id.toInt();
  i_estimate = s_estimate.toInt();

  if (stories[user_index] != i_id) {
    Serial.println("Found new story with id:" + String(i_id) + " and estimate:" + String(i_estimate));
    stories[user_index] = i_id;
    return i_estimate;
  } else {
    Serial.println("No new story found");
    return 0;
  }
}

void connect_wifi() {
  Serial.println("Connecting to WIFI");
  
  char ssid[] = SECRET_SSID;
  char pass[] = SECRET_PASS;

  while (WiFi.status() == WL_NO_MODULE) {
    Serial.println("No wifi module found");
    delay(1000);
  }

  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    Serial.print("Trying to connect to WIFI with SSID:");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(1000);
  }

  Serial.println("WIFI connected");

  IPAddress ip = WiFi.localIP();
  Serial.print("Using IP:");
  Serial.println(ip);
}

void servo_dispense(int amount) {
  Serial.print("Dispensing:");
  Serial.println(amount);

  servo.attach(SERVO_PIN);
  servo.write(SERVO_SPEED); 
  delay(amount * SERVO_DELAY);
  servo.detach();
}
