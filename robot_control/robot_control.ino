#include <ESP8266WiFi.h>

const char* SSID = "GNXS-13D910";
const char* PASS = "12345678";

WiFiServer server(4210);  // TCP port

// Motor pins (L298N/L293D style)
const int IN1 = D1; // GPIO5  - Left A
const int IN2 = D2; // GPIO4  - Left B
const int IN3 = D5; // GPIO14 - Right A
const int IN4 = D6; // GPIO12 - Right B
const int ENA = D7; // GPIO13 - PWM OK
const int ENB = D8; // GPIO15 - PWM OK, BUT boot pin (see note)

const int DEFAULT_SPEED = 800; // 0..1023 on ESP8266

void stopRobot(){
  Serial.println("stop");
  digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);
}

void setupPins() {
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  stopRobot();
}

void forward()  { Serial.println("forward");
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void backward() { Serial.println("backward");
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void left()     { Serial.println("left");
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void right()    { Serial.println("right");
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void setup() {
  Serial.begin(115200);
  setupPins();

  // Set an initial speed
  analogWriteRange(1023);                // (default anyway)
  analogWrite(ENA, DEFAULT_SPEED);
  analogWrite(ENB, DEFAULT_SPEED);

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

  // Optional: command watchdog so motors stop if no recent command
  unsigned long lastCmdMs = millis();
  const unsigned long CMD_TIMEOUT_MS = 2000;

  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      Serial.printf("Cmd: %c\n", c);

      // Read speed from A0 if wired; otherwise use DEFAULT_SPEED
      int speed = analogRead(A0);               // 0..1023
      if (speed < 5) speed = DEFAULT_SPEED;     // crude fallback if A0 floating
      analogWrite(ENA, speed);
      analogWrite(ENB, speed);

      switch (c) {
        case 'F': forward();  break;
        case 'B': backward(); break;
        case 'L': left();     break;
        case 'R': right();    break;
        case 'S': stopRobot();break;
        default:  stopRobot();break;
      }
      lastCmdMs = millis();
    }

    // Safety: stop if no commands for a while
    if (millis() - lastCmdMs > CMD_TIMEOUT_MS) {
      stopRobot();
      lastCmdMs = millis(); // prevent spamming
    }

    yield();
  }
  stopRobot();
  Serial.println("Client disconnected");
}
