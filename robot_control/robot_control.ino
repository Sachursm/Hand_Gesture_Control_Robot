#include <ESP8266WiFi.h>

const char* SSID = "YourWiFiSSID";
const char* PASS = "YourWiFiPassword";

WiFiServer server(4210);  // TCP port

// Motor pins
const int IN1 = D1; // GPIO5  - Left A
const int IN2 = D2; // GPIO4  - Left B
const int IN3 = D5; // GPIO14 - Right A
const int IN4 = D6; // GPIO12 - Right B

void setupPins() {
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  stopRobot();
}

void forward()  { Serial.print("forward");
                  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
                  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }

void backward() { Serial.print("backward");
                  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
                  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }

void left()     { Serial.print("left");
                  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
                  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }

void right()    { Serial.print("right");
                  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
                  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }

void stopRobot(){ Serial.print("stop");
                  digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);
                  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW); }

void setup() {
  Serial.begin(115200);
  setupPins();

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("Connected. IP: "); Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("TCP server started on port 4210");
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("Client connected");
  client.setTimeout(20);
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      Serial.printf("Cmd: %c\n", c);
      switch (c) {
        case 'F': forward();  break;
        case 'B': backward(); break;
        case 'L': left();     break;
        case 'R': right();    break;
        case 'S': stopRobot();break;
        default:  stopRobot();break;
      }
    }
    // optional: safety timeout / watchdog can go here
    yield();
  }
  stopRobot();
  Serial.println("Client disconnected");
}
