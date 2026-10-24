#include <WiFiS3.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// 1. ตั้งค่าการเชื่อมต่อและการกำหนดค่าระบบ (USER CONFIGURATION)
// ==========================================
const char* SECRET_SSID         = "wachi_";             // wifi name
const char* SECRET_PASS         = "12345678";           // wifi Password
const char* MQTT_BROKER         = "broker.hivemq.com";
const int   MQTT_PORT           = 1883;
const char* TABLE_BOARD_ID      = "A0";                 // ชื่อโต๊ะ 

// ตัวแปรตั้งค่าระยะทางและเวลาสำหรับการ คืนโต๊ะ (Logout)
const float DISTANCE_THRESHOLD  = 80.0;                 // ระยะทาง (cm) ถ้าน้อยกว่าหรือเท่ากับค่านี่ ถือว่ามีคนอยู่ (จะไม่นับเวลาคืนโต๊ะ)
const unsigned long AUTO_LOGOUT_TIMEOUT = 0.5 * 60 * 1000UL; // เวลาสำหรับคืนโต๊ะอัตโนมัติเมื่อไม่มีคนอยู่ (ตั้งไว้ 30 วินาที)

// 2. การกำหนดขาพอร์ต (PIN MAPPING)
// ==========================================
const int TRIG_1 = 2, ECHO_1 = 3;  // Ultrasonic 1
const int TRIG_2 = 4, ECHO_2 = 5;  // Ultrasonic 2
const int TMP36_PIN = A0;          // TMP36 Temp Sensor
const int LED_R = A1, LED_G = A2, LED_B = A3; // RGB LED

const byte ROW_PINS[4] = {6, 7, 8, 9};
const byte COL_PINS[4] = {10, 11, 12, 13};

LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Topics
String TOPIC_STATUS   = "smartdesk/" + String(TABLE_BOARD_ID) + "/status";
String TOPIC_TEMP     = "smartdesk/" + String(TABLE_BOARD_ID) + "/temp";
String TOPIC_PIN      = "smartdesk/" + String(TABLE_BOARD_ID) + "/pin";
String TOPIC_COMMAND  = "smartdesk/" + String(TABLE_BOARD_ID) + "/command";
String TOPIC_ONLINE   = "smartdesk/" + String(TABLE_BOARD_ID) + "/online";

// 3. ตัวแประบบและสถานะ (SYSTEM STATES)
// ==========================================
enum DeskState {
  STATE_OFF,
  STATE_VACANT,
  STATE_BOOKED,
  STATE_ENTER_PIN,
  STATE_WELCOME,
  STATE_MENU,
  STATE_SET_TIME,
  STATE_OCCUPIED,
  STATE_SHOW_TEMP,
  STATE_CONFIRM_OFF
};

enum LedColorMode {
  COLOR_UNKNOWN = -1,
  COLOR_OFF = 0,
  COLOR_BLUE = 1,
  COLOR_GREEN = 2,
  COLOR_RED = 3
};

DeskState currentState = STATE_VACANT;
DeskState previousState = STATE_VACANT;
DeskState previousStateBeforeOffConfirm = STATE_VACANT;

LedColorMode currentLedColor = COLOR_UNKNOWN;

uint32_t lcg_seed = 1234;
String generatedPin = "";
String inputBuffer = "";
bool isPinScreenActive = false;

// ตัวแปรสถานะการตรวจจับคน
bool isPersonPresent = false;

// ตัวแประบบนับเวลาหน้า Welcome
unsigned long welcomeStartTime = 0;

// ตัวแประบบนับถอยหลังและ Pause
bool isPaused = false;
unsigned long pauseStartTime = 0;
int lastMins = -1;
int lastSecs = -1;
bool lastPauseState = false;
bool lastTimeExpired = false; // แฟล็กเช็กการแสดงผลเมื่อหมดเวลา Office Syndrome

unsigned long lastSensorRead = 0;
unsigned long lastTempPublish = 0;
unsigned long lastLcdUpdate = 0;
unsigned long lastMovementTime = 0;
unsigned long officeSyndromeStartTime = 0;
unsigned long officeSyndromeDuration = 45 * 60 * 1000UL;
unsigned long lastMqttReconnect = 0;

float smoothedTemp = -999.0;

// ตัวแปรสำหรับสแกน Keypad แบบ Non-blocking
char lastKeyPressed = 0;
unsigned long keyPressTime = 0;
bool keyHandled = false;

// 4. ฟังก์ชันไฟ RGB LED (Common Anode Logic)
// ==========================================
void setRGBColorMode(LedColorMode mode, bool force = false) {
  if (!force && currentLedColor == mode) return;
  currentLedColor = mode;
  digitalWrite(LED_R, (mode == COLOR_RED)   ? LOW : HIGH);
  digitalWrite(LED_G, (mode == COLOR_GREEN) ? LOW : HIGH);
  digitalWrite(LED_B, (mode == COLOR_BLUE)  ? LOW : HIGH);
}

// 5. ฟังก์ชันสุ่มรหัส PIN (LCG Algorithm)
// ==========================================
String generateLCGPin() {
  uint32_t A = 1664525;
  uint32_t C = 1013904223;
  uint32_t m = 9999;
  lcg_seed = (A * lcg_seed + C) % m;
  
  char pinBuf[5];
  snprintf(pinBuf, sizeof(pinBuf), "%04lu", (unsigned long)lcg_seed);
  return String(pinBuf);
}

// 6. ฟังก์ชันอ่านค่าเซนเซอร์
// ==========================================
float readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW); delayMicroseconds(4);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000); // Timeout 30ms (~5 เมตร)
  if (duration == 0) return -1.0;
  return (duration * 0.0343) / 2.0;
}

float readTMP36() {
  long sumRaw = 0;
  for (int i = 0; i < 10; i++) {
    sumRaw += analogRead(TMP36_PIN);
    delayMicroseconds(50);
  }
  float avgRaw = (float)sumRaw / 10.0;
  float voltage = avgRaw * (5.0 / 1023.0);
  float rawTemp = ((voltage - 0.5) * 100.0) + 20.0; // ชดเชยค่า +20.0
  if (smoothedTemp == -999.0) smoothedTemp = rawTemp;
  smoothedTemp = (0.05 * rawTemp) + (0.95 * smoothedTemp);
  return smoothedTemp;
}

// 7. ฟังก์ชันเปิด/ปิด เครื่อง (Power ON/OFF)
// ==========================================
void resetToVacant() {
  currentState = STATE_VACANT;
  inputBuffer = "";
  isPinScreenActive = false;
  isPaused = false;
  lastMins = -1;
  lastSecs = -1;
  lastTimeExpired = false;
  setRGBColorMode(COLOR_OFF, true);
  
  if (mqttClient.connected()) {
    mqttClient.publish(TOPIC_STATUS.c_str(), "VACANT", true);
    mqttClient.publish(TOPIC_PIN.c_str(), "----", true);
    mqttClient.publish(TOPIC_ONLINE.c_str(), "ONLINE", true);
  }
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(" Please Booking ");
  lcd.setCursor(0, 1); lcd.print("  [*] Tern Off  ");
}

void turnOffMachine() {
  currentState = STATE_OFF;
  isPaused = false;
  setRGBColorMode(COLOR_OFF, true);
  lcd.clear();
  lcd.noBacklight();
  if (mqttClient.connected()) {
    mqttClient.publish(TOPIC_STATUS.c_str(), "OFF", true);
    mqttClient.publish(TOPIC_PIN.c_str(), "----", true);
    mqttClient.publish(TOPIC_ONLINE.c_str(), "OFFLINE", true);
  }
}

// 8. การอัปเดตหน้าจอ LCD ของสถานะต่างๆ
// ==========================================
void redrawCurrentStateLCD() {
  lcd.clear();
  if (currentState == STATE_VACANT) {
    lcd.setCursor(0, 0); lcd.print(" Please Booking ");
    lcd.setCursor(0, 1); lcd.print("  [*] Tern Off  ");
  } 
  else if (currentState == STATE_BOOKED || currentState == STATE_ENTER_PIN) {
    if (isPinScreenActive) {
      char pinLine[17];
      char padded[5] = "    ";
      for (int i = 0; i < inputBuffer.length() && i < 4; i++) padded[i] = inputBuffer[i];
      snprintf(pinLine, sizeof(pinLine), "PIN:%s  #|back", padded);
      lcd.setCursor(0, 0); lcd.print(pinLine);
      lcd.setCursor(0, 1); lcd.print("C|cancel  D|done");
    } else {
      lcd.setCursor(0, 0); lcd.print("  Please enter  ");
      lcd.setCursor(0, 1); lcd.print(" your  password ");
    }
  } 
  else if (currentState == STATE_MENU) {
    lcd.setCursor(0, 0); lcd.print("PressA: RoomTemp");
    lcd.setCursor(0, 1); lcd.print("PressB: Set Time");
  }
  else if (currentState == STATE_SET_TIME) {
    lcd.setCursor(0, 0); lcd.print("SetTime(m):     ");
    if (inputBuffer.length() > 0) {
      lcd.setCursor(12, 0); lcd.print(inputBuffer);
      lcd.setCursor(0, 1); lcd.print("C|cancel  D|done");
    } else {
      lcd.setCursor(0, 1); lcd.print("D|default B|back");
    }
  }
  else if (currentState == STATE_CONFIRM_OFF) {
    lcd.setCursor(0, 0); lcd.print("    Turn OFF    ");
    lcd.setCursor(0, 1); lcd.print("A|Yes       B|No");
  }
  else if (currentState == STATE_OCCUPIED) {
    lastMins = -1;
    lastSecs = -1;
    lastPauseState = !isPaused;
    lastTimeExpired = false;
  }
}

// 9. การรับสัญญาณและการจัดการ MQTT
// ==========================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (currentState == STATE_OFF) return;

  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == TOPIC_COMMAND) {
    if (message == "BOOK" && currentState == STATE_VACANT) {
      generatedPin = generateLCGPin();
      currentState = STATE_BOOKED;
      isPinScreenActive = false;
      inputBuffer = "";
      lastMovementTime = millis();
      
      mqttClient.publish(TOPIC_PIN.c_str(), generatedPin.c_str(), true);
      mqttClient.publish(TOPIC_STATUS.c_str(), "BOOKED", true);

      redrawCurrentStateLCD();
      setRGBColorMode(COLOR_BLUE);
    } 
    else if (message == "LOGOUT") {
      resetToVacant();
    }
  }
}

void maintainMqttConnection() {
  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastMqttReconnect > 5000) {
      lastMqttReconnect = now;
      
      String clientId = "SmartDesk_" + String(TABLE_BOARD_ID) + "_" + String(random(1000, 9999));
      if (mqttClient.connect(clientId.c_str(), NULL, NULL, TOPIC_ONLINE.c_str(), 1, true, "OFFLINE")) {
        mqttClient.subscribe(TOPIC_COMMAND.c_str());
        mqttClient.publish(TOPIC_ONLINE.c_str(), "ONLINE", true);

        const char* statusStr = "VACANT";
        if (currentState == STATE_OFF) statusStr = "OFF";
        else if (currentState == STATE_BOOKED) statusStr = "BOOKED";
        else if (currentState != STATE_VACANT) statusStr = "OCCUPIED";

        mqttClient.publish(TOPIC_STATUS.c_str(), statusStr, true);
        mqttClient.publish(TOPIC_TEMP.c_str(), String(readTMP36(), 1).c_str(), false);
      }
    }
  } else {
    mqttClient.loop();
  }
}

// 10. การสแกน Keypad แบบ Non-blocking (Single Press)
// ==========================================
char scanKeypad() {
  char keys[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
  };
  
  char detectedKey = 0;
  for (int r = 0; r < 4; r++) {
    digitalWrite(ROW_PINS[r], LOW);
    for (int c = 0; c < 4; c++) {
      if (digitalRead(COL_PINS[c]) == LOW) {
        detectedKey = keys[r][c];
      }
    }
    digitalWrite(ROW_PINS[r], HIGH);
    if (detectedKey != 0) break;
  }

  if (detectedKey != 0) {
    if (lastKeyPressed != detectedKey) {
      lastKeyPressed = detectedKey;
      keyPressTime = millis();
      keyHandled = false;
    } else if (!keyHandled && (millis() - keyPressTime >= 30)) {
      keyHandled = true;
      return detectedKey;
    }
  } else {
    lastKeyPressed = 0;
    keyHandled = false;
  }

  return 0;
}

void startOccupiedSession() {
  currentState = STATE_OCCUPIED;
  officeSyndromeStartTime = millis();
  isPaused = false;
  lastMins = -1;
  lastSecs = -1;
  lastPauseState = false;
  lastTimeExpired = false;
  lcd.clear();
}

// 11. SETUP & LOOP
// ==========================================
void setup() {
  Serial.begin(9600);

  Wire.setWireTimeout(3000, true);

  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  setRGBColorMode(COLOR_OFF, true);

  pinMode(TRIG_1, OUTPUT); pinMode(ECHO_1, INPUT);
  pinMode(TRIG_2, OUTPUT); pinMode(ECHO_2, INPUT);

  for (int i = 0; i < 4; i++) {
    pinMode(ROW_PINS[i], OUTPUT); digitalWrite(ROW_PINS[i], HIGH);
    pinMode(COL_PINS[i], INPUT_PULLUP);
  }

  lcd.init(); lcd.backlight(); lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Connecting WiFi");

  WiFi.begin(SECRET_SSID, SECRET_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); lcd.print(".");
  }

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  lcg_seed = millis() + analogRead(A0);
  resetToVacant();
}

void loop() {
  maintainMqttConnection();

  // ส่งข้อมูลอุณหภูมิ และคำว่า ONLINE เข้า MQTT ทุกๆ 5 วินาทีตราบใดที่ยังเชื่อมต่ออยู่
  if (millis() - lastTempPublish >= 5000) {
    lastTempPublish = millis();
    if (mqttClient.connected()) {
      float currentTemp = readTMP36();
      mqttClient.publish(TOPIC_TEMP.c_str(), String(currentTemp, 1).c_str(), false);
      mqttClient.publish(TOPIC_ONLINE.c_str(), "ONLINE", true);
    }
  }

  // แสดงหน้า Welcome 4 วินาที แบบ Non-blocking
  if (currentState == STATE_WELCOME) {
    if (millis() - welcomeStartTime >= 4000) {
      currentState = STATE_MENU;
      lastMovementTime = millis(); // รีเซ็ตเวลานับถอยหลังใหม่เมื่อเริ่มเข้าสู่หน้า MENU
      redrawCurrentStateLCD();
    }
  }

  char key = scanKeypad();

  // กดปุ่ม '*' เพียงครั้งเดียวเพื่อเปิด/ปิดเครื่อง
  if (key == '*') {
    if (currentState == STATE_OFF) {
      lcd.backlight();
      resetToVacant();
    } else if (currentState != STATE_CONFIRM_OFF) {
      previousStateBeforeOffConfirm = currentState;
      currentState = STATE_CONFIRM_OFF;
      redrawCurrentStateLCD();
    }
    return;
  }

  if (currentState == STATE_OFF) return;

  // 1. จัดการปุ่มกด (Keypad Logic)
  // ----------------------------------------------------
  if (key != 0) {
    // เงื่อนไขที่ 2: มีการกดปุ่มบน Keypad ให้รีเซ็ตเวลานับถอยหลังคืนโต๊ะทันที (จะไม่นับเวลาคืนโต๊ะ)
    lastMovementTime = millis();

    if (currentState == STATE_CONFIRM_OFF) {
      if (key == 'A') {
        turnOffMachine();
      } else if (key == 'B') {
        currentState = previousStateBeforeOffConfirm;
        redrawCurrentStateLCD();
      }
      return;
    }

    if (currentState == STATE_MENU) {
      if (key == 'A') {
        previousState = STATE_MENU;
        currentState = STATE_SHOW_TEMP;
        lcd.clear();
        return;
      } else if (key == 'B') {
        currentState = STATE_SET_TIME;
        inputBuffer = "";
        redrawCurrentStateLCD();
        return;
      }
    }

    if (currentState == STATE_SHOW_TEMP) {
      if (key == 'B') {
        currentState = previousState;
        redrawCurrentStateLCD();
      }
      return;
    }

    if (currentState == STATE_BOOKED || currentState == STATE_ENTER_PIN) {
      if (key >= '0' && key <= '9') {
        if (!isPinScreenActive) isPinScreenActive = true;
        if (inputBuffer.length() < 4) inputBuffer += key;
        redrawCurrentStateLCD();
      } 
      else if (key == '#') {
        if (inputBuffer.length() > 0) inputBuffer.remove(inputBuffer.length() - 1);
        redrawCurrentStateLCD();
      } 
      else if (key == 'C') {
        inputBuffer = "";
        isPinScreenActive = false;
        redrawCurrentStateLCD();
      } 
      else if (key == 'D') {
        if (inputBuffer == generatedPin) {
          lastMovementTime = millis(); 

          if (mqttClient.connected()) {
            mqttClient.publish(TOPIC_STATUS.c_str(), "OCCUPIED", true);
            mqttClient.publish(TOPIC_PIN.c_str(), "----", true);
          }

          currentState = STATE_WELCOME;
          welcomeStartTime = millis();
          lcd.clear();
          lcd.setCursor(0, 0); lcd.print("   WELCOME TO   ");
          lcd.setCursor(0, 1); lcd.print(" S.E.A.T. SENCE ");
        } else {
          lcd.clear();
          lcd.setCursor(0, 0); lcd.print("!Wrong Password!");
          delay(2000);
          inputBuffer = "";
          redrawCurrentStateLCD();
        }
      }
    } 
    else if (currentState == STATE_SET_TIME) {
      if (key >= '0' && key <= '9') {
        if (inputBuffer.length() < 3) inputBuffer += key;
        redrawCurrentStateLCD();
      } else if (key == 'C') {
        inputBuffer = "";
        redrawCurrentStateLCD();
      } else if (key == 'D') {
        if (inputBuffer.length() > 0) {
          officeSyndromeDuration = inputBuffer.toInt() * 60 * 1000UL;
        } else {
          officeSyndromeDuration = 45 * 60 * 1000UL;
        }
        startOccupiedSession();
      } else if (key == 'B') {
        if (inputBuffer.length() == 0) {
          currentState = STATE_MENU;
          redrawCurrentStateLCD();
        }
      }
    }
    else if (currentState == STATE_OCCUPIED) {
      unsigned long elapsed = isPaused ? (pauseStartTime - officeSyndromeStartTime) : (millis() - officeSyndromeStartTime);
      bool isExpired = (elapsed >= officeSyndromeDuration);

      if (key == 'A') {
        if (isExpired) {
          startOccupiedSession(); // เมื่อนับเวลาครบแล้ว กด A (again) จะเริ่มนับรอบใหม่ทันที
        } else {
          isPaused = !isPaused;   // ยังไม่หมดเวลา กด A สลับ Pause/Resume
          if (isPaused) {
            pauseStartTime = millis();
          } else {
            officeSyndromeStartTime += (millis() - pauseStartTime);
          }
          lastMins = -1;
          lastSecs = -1;
        }
      } 
      else if (key == 'B') {
        isPaused = false;
        setRGBColorMode(COLOR_OFF, true);
        currentState = STATE_SET_TIME; // กด B (back) กลับไปหน้าตั้งเวลา
        inputBuffer = "";
        redrawCurrentStateLCD();
      }
    }
  }

  // 2. การอัปเดตหน้าจอ LCD และไฟ RGB ตามปกติ
  // ----------------------------------------------------
  if (millis() - lastLcdUpdate >= 200) {
    lastLcdUpdate = millis();

    if (currentState == STATE_SHOW_TEMP) {
      char tempBuf[17];
      snprintf(tempBuf, sizeof(tempBuf), "RoomTemp: %.1fC ", readTMP36());
      lcd.setCursor(0, 0); lcd.print(tempBuf);
      lcd.setCursor(0, 1); lcd.print("     B|Back     ");
    }
    else if (currentState == STATE_OCCUPIED) {
      unsigned long elapsed;
      if (isPaused) {
        elapsed = pauseStartTime - officeSyndromeStartTime;
      } else {
        elapsed = millis() - officeSyndromeStartTime;
      }

      if (elapsed < officeSyndromeDuration) {
        lastTimeExpired = false;
        unsigned long remaining = (officeSyndromeDuration - elapsed) / 1000UL;
        int mins = remaining / 60;
        int secs = remaining % 60;

        if (mins != lastMins || secs != lastSecs || isPaused != lastPauseState) {
          lastMins = mins;
          lastSecs = secs;
          lastPauseState = isPaused;

          char timeBuf[17];
          snprintf(timeBuf, sizeof(timeBuf), "COUNT DOWN:%02d:%02d", mins, secs);
          lcd.setCursor(0, 0); lcd.print(timeBuf);

          if (isPaused) {
            lcd.setCursor(0, 1); lcd.print("A|Resume  B|Back");
          } else {
            lcd.setCursor(0, 1); lcd.print("A|Pause   B|Back");
          }
        }

        if (remaining > 30) {
          setRGBColorMode(COLOR_GREEN);
        } else {
          setRGBColorMode(COLOR_RED);
        }
      } else {
        // เมื่อนับเวลานั่งทำงานครบกำหนดแล้ว
        if (!lastTimeExpired) {
          lastTimeExpired = true;
          lcd.clear();
          lcd.setCursor(0, 0); lcd.print("  Move Around   "); // บรรทัดบน
          lcd.setCursor(0, 1); lcd.print("A|again   B|back"); // บรรทัดล่าง
        }

        // กะพริบไฟสีแดงเตือนให้ลุกเปลี่ยนอิริยาบถ
        if ((millis() / 1000) % 2 == 0) {
          setRGBColorMode(COLOR_RED);
        } else {
          setRGBColorMode(COLOR_OFF);
        }
      }
    }
    else if (currentState == STATE_BOOKED || currentState == STATE_ENTER_PIN) {
      setRGBColorMode(COLOR_BLUE);
    }
  }

  // 3. อ่านค่าเซนเซอร์ และตรวจจับเวลานับถอยหลังคืนโต๊ะ
  // ----------------------------------------------------
  if (millis() - lastSensorRead >= 1000) {
    lastSensorRead = millis();

    float d1 = readUltrasonic(TRIG_1, ECHO_1);
    delay(30); // หน่วงเวลาเล็กน้อยเพื่อป้องกันคลื่นกวนกันระหว่าง U1 และ U2
    float d2 = readUltrasonic(TRIG_2, ECHO_2);

    // เงื่อนไขที่ 1: ถ้าเซนเซอร์อ่านค่าได้ <= DISTANCE_THRESHOLD (80.0 cm) ถือว่ามีคนอยู่ -> รีเซ็ตเวลา ไม่คืนโต๊ะ
    bool currentlyPresent = (d1 > 0.0 && d1 <= DISTANCE_THRESHOLD) || (d2 > 0.0 && d2 <= DISTANCE_THRESHOLD);

    if (currentlyPresent) {
      if (!isPersonPresent) {
        isPersonPresent = true;
        if (currentState == STATE_BOOKED || currentState == STATE_ENTER_PIN) {
          setRGBColorMode(COLOR_BLUE, true);
        }
        redrawCurrentStateLCD();
      }
      lastMovementTime = millis(); // ถ้านั่งอยู่หน้าโต๊ะ (<= DISTANCE_THRESHOLD) ให้รีเซ็ตเวลานับถอยหลังเสมอ (จะไม่นับเวลาคืนโต๊ะ)
    } else {
      isPersonPresent = false;
    }

    // เช็กสถานะว่าโต๊ะถูกจองแล้วหรือกำลังทำงานอยู่ (ไม่นับตอน VACANT และ OFF)
    bool isDeskActive = (currentState != STATE_VACANT && currentState != STATE_OFF);

    // แปลงระยะทางเป็นข้อความเพื่อแสดงบน Serial Monitor
    char u1Str[12], u2Str[12];
    if (d1 < 0) sprintf(u1Str, "---.-cm"); else sprintf(u1Str, "%.1fcm", d1);
    if (d2 < 0) sprintf(u2Str, "---.-cm"); else sprintf(u2Str, "%.1fcm", d2);

    // เงื่อนไขการนับเวลาคืนโต๊ะ: โต๊ะถูกใช้งานอยู่ และ "ไม่มีคนอยู่" (ระยะทางทั้ง 2 ข้าง > DISTANCE_THRESHOLD)
    if (isDeskActive && !isPersonPresent) {
      unsigned long noMovementDuration = millis() - lastMovementTime;
      unsigned long remainingSec = (AUTO_LOGOUT_TIMEOUT > noMovementDuration) 
                                    ? (AUTO_LOGOUT_TIMEOUT - noMovementDuration) / 1000UL 
                                    : 0;

      int remainMins = remainingSec / 60;
      int remainSecs = remainingSec % 60;

      // แสดงระยะจริง พร้อมเวลานับถอยหลัง TimeOut บน Serial Monitor
      char serialBuf[120];
      snprintf(serialBuf, sizeof(serialBuf), 
               "U1 : %-8s TimeOut %02d:%02d U2 : %-8s", 
               u1Str, remainMins, remainSecs, u2Str);
      Serial.println(serialBuf);

      // เมื่อไม่มีคนอยู่นานครบเวลา (AUTO_LOGOUT_TIMEOUT) -> สั่งคืนโต๊ะทันที
      if (noMovementDuration >= AUTO_LOGOUT_TIMEOUT) {
        Serial.println(">>> Timeout! Sending LOGOUT and resetting to VACANT <<<");
        if (mqttClient.connected()) {
          mqttClient.publish(TOPIC_COMMAND.c_str(), "LOGOUT", false);
        }
        resetToVacant(); // คืนโต๊ะและตัดเข้าสู่หน้า VACANT ทันที
      }
    } 
    // สภาวะปกติ: แสดงระยะทางจริงของ U1 และ U2 บน Serial Monitor
    else {
      char serialBuf[120];
      snprintf(serialBuf, sizeof(serialBuf), 
               "U1 : %-8s                       U2 : %-8s", 
               u1Str, u2Str);
      Serial.println(serialBuf);
    }
  }
}